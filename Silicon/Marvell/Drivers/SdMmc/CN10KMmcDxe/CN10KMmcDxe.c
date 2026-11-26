/** @file
  CN10K eMMC platform glue.

  Registers the memory-mapped Cadence SD6 SDHCI controller as a
  non-discoverable SDHCI device, and provides the EDKII_SD_MMC_OVERRIDE
  implementation for the Cadence specific capability and clock handling.

  SPDX-License-Identifier: BSD-2-Clause-Patent
  Copyright (C) 2024-2026 Marvell
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/CadenceSd6PhyLib.h>
#include <Protocol/FdtClient.h>
#include <Protocol/NonDiscoverableDevice.h>
#include <Protocol/SdMmcOverride.h>

//
// Cadence SD6 host register set (wrapper) base.  The SDHCI register block is
// located 0x200 bytes above it.
//
#define CN10K_EMMC_WRAPPER_BASE  (FixedPcdGet64 (PcdEmmcWrapperBase64))
#define CN10K_EMMC_SDHCI_OFFSET  0x200
#define CN10K_EMMC_SDHCI_BASE    (CN10K_EMMC_WRAPPER_BASE + CN10K_EMMC_SDHCI_OFFSET)
#define CN10K_EMMC_SDHCI_SIZE    0x1000

//
// Fixed SD6 PHY reference clock.
//
#define CN10K_EMMC_MAX_CLK  (FixedPcdGet32 (PcdEmmcBaseClockHz))

//
// SD Host Controller Clock Control register offset (SDHCI block).
//
#define CN10K_SDHCI_CLOCK_CTRL  0x2C

//
// SD Host Controller Version register (read for diagnostics only).
//
#define CN10K_SDHCI_CTRL_VER  0xFE

STATIC CADENCE_SD6_PHY_CONFIG  mPhyConfig;
STATIC BOOLEAN                mBusWidth8    = FALSE;
STATIC BOOLEAN                mNonRemovable = FALSE;

/**
  Read a 32-bit device tree property and convert it to CPU byte order.

  @param[in]   Fdt    The FDT client protocol instance.
  @param[in]   Node   Handle of the node to read from.
  @param[in]   Name   Property name.
  @param[out]  Value  Converted property value. Only updated on success.

  @retval  EFI_SUCCESS  The property was found and converted.
  @retval  Others       The property could not be read.
**/
STATIC
EFI_STATUS
GetFdtProp32 (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *Fdt,
  IN  FDT_HANDLE                Node,
  IN  CONST CHAR8               *Name,
  OUT UINT32                    *Value
  )
{
  EFI_STATUS    Status;
  CONST UINT32  *Prop;

  Status = Fdt->GetNodeProperty (Fdt, Node, Name, (CONST VOID **)&Prop, NULL);
  if (!EFI_ERROR (Status)) {
    *Value = FdtToCpu32 (*Prop);
  }

  return Status;
}

/**
  Read the Cadence PHY configuration from the device tree.

  Locates the MRVL FDT client, finds the eMMC node at
  /soc@0/sdhci@<wrapper base>, and reads the optional PHY tuning properties.
  Missing properties leave the corresponding defaults in place. The bus width
  and card detect properties are cached in mBusWidth8 and mNonRemovable.

  @retval  EFI_SUCCESS  The configuration was read.
  @retval  Others       The FDT client or the eMMC node was not found.
**/
STATIC
EFI_STATUS
ParseFdtConfig (
  VOID
  )
{
  EFI_STATUS                Status;
  MRVL_FDT_CLIENT_PROTOCOL  *Fdt;
  FDT_HANDLE                Node;
  UINT32                    Value;
  CONST VOID                *Prop;
  CHAR8                     FdtPath[64];

  Status = gBS->LocateProtocol (&gMrvlFdtClientProtocolGuid, NULL, (VOID **)&Fdt);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: FDT client protocol not found: %r\n", __func__, Status));
    return Status;
  }

  AsciiSPrint (FdtPath, sizeof (FdtPath), "/soc@0/sdhci@%llx", (UINT64)CN10K_EMMC_WRAPPER_BASE);
  Status = Fdt->GetNode (Fdt, FdtPath, &Node);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: eMMC FDT node not found: %r\n", __func__, Status));
    return Status;
  }

  ZeroMem (&mPhyConfig, sizeof (mPhyConfig));

  mPhyConfig.WrapperBase = (UINTN)CN10K_EMMC_WRAPPER_BASE;
  mPhyConfig.BaseClockHz = CN10K_EMMC_MAX_CLK;

  if (!EFI_ERROR (GetFdtProp32 (Fdt, Node, "cdns,delay_element", &Value))) {
    mPhyConfig.DelayElement         = Value;
    mPhyConfig.OverrideDelayElement = TRUE;
  }

  if (!EFI_ERROR (GetFdtProp32 (Fdt, Node, "cdns,iocell_input_delay", &Value))) {
    mPhyConfig.IocellInputDelay = Value;
  }

  if (!EFI_ERROR (GetFdtProp32 (Fdt, Node, "cdns,iocell_output_delay", &Value))) {
    mPhyConfig.IocellOutputDelay = Value;
  }

  mPhyConfig.HostDrive = 2;
  if (!EFI_ERROR (GetFdtProp32 (Fdt, Node, "cdns,host_drive", &Value))) {
    mPhyConfig.HostDrive = Value;
  }

  mPhyConfig.HostSlew = 3;
  if (!EFI_ERROR (GetFdtProp32 (Fdt, Node, "cdns,host_slew", &Value))) {
    mPhyConfig.HostSlew = Value;
  }

  mBusWidth8 = FALSE;
  if (!EFI_ERROR (GetFdtProp32 (Fdt, Node, "bus-width", &Value))) {
    mBusWidth8 = (BOOLEAN)(Value >= 8);
  }

  mNonRemovable = !EFI_ERROR (Fdt->GetNodeProperty (Fdt, Node, "non-removable", &Prop, NULL));

  return EFI_SUCCESS;
}

/**
  Read the SDHCI clock divider programmed by the generic driver and
  reprogram the Cadence PHY for that frequency.

  The SDHCI divider is split between the upper 8 bits of the Clock Control
  register and the two frequency-select bits, so both fields are combined
  before the PHY is updated.
**/
STATIC
VOID
UpdateCadenceClock (
  VOID
  )
{
  UINT16  ClockCtrl;
  UINT32  Divider;

  ClockCtrl = MmioRead16 ((UINTN)CN10K_EMMC_SDHCI_BASE + CN10K_SDHCI_CLOCK_CTRL);
  Divider   = ((ClockCtrl >> 8) & 0xFF) | ((ClockCtrl & 0xC0) << 2);
  if (Divider == 0) {
    Divider = 1;
  }

  CadenceSd6PhySetClock (Divider);
}

/**
  Adjust the SDHCI slot capabilities for the Cadence controller.

  The controller is clocked from a fixed source, so the reported base clock
  and clock multiplier are replaced with the platform values. The capability
  word is then clamped to what the controller supports here: eMMC high speed,
  3.3 V signalling, optional 8-bit bus and non-removable slot, PIO only, and
  no tuning or modes above high-speed SDR.

  @param[in]      ControllerHandle      The controller handle.
  @param[in]      Slot                  The slot number.
  @param[in,out]  SdMmcHcSlotCapability  Pointer to the 64-bit capability word.
  @param[in,out]  BaseClkFreq           Receives the base clock in MHz.

  @retval  EFI_SUCCESS            The capabilities were updated.
  @retval  EFI_INVALID_PARAMETER  A pointer argument is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSdMmcCapability (
  IN     EFI_HANDLE  ControllerHandle,
  IN     UINT8       Slot,
  IN OUT VOID        *SdMmcHcSlotCapability,
  IN OUT UINT32      *BaseClkFreq
  )
{
  UINT64  Capability;
  UINT32  BaseClock;

  if ((SdMmcHcSlotCapability == NULL) || (BaseClkFreq == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The controller is clocked from a fixed 200 MHz source.  Reported base
  // clock from the capability register is not reliable.
  //
  BaseClock = CN10K_EMMC_MAX_CLK;

  Capability  = *(UINT64 *)SdMmcHcSlotCapability;
  Capability &= ~(UINT64)(0xFFULL << 8);   // BaseClkFreq
  Capability &= ~(UINT64)(0xFFULL << 48);  // ClkMultiplier
  Capability |= (UINT64)((BaseClock / 1000000) & 0xFF) << 8;

  //
  // 512-byte max block length and eMMC High Speed support.
  //
  Capability &= ~(UINT64)(0x3ULL << 16); // MaxBlkLen = 512 bytes
  Capability |= (UINT64)(1ULL << 21);    // HighSpeed

  //
  // The Cadence wrapper is configured for 3.3V signalling.
  //
  Capability &= ~(UINT64)((1ULL << 25) | (1ULL << 26)); // Voltage30, Voltage18
  Capability |= (UINT64)(1ULL << 24);                   // Voltage33

  if (mBusWidth8) {
    Capability |= (UINT64)(1ULL << 18); // BusWidth8
  }

  if (mNonRemovable) {
    Capability &= ~(UINT64)(0x3ULL << 30); // SlotType
    Capability |= (UINT64)(1ULL << 30);    // EmbeddedSlot
  }

  //
  // Force PIO.  The Marvell reference driver uses non-DMA transfers on this
  // platform.
  //
  Capability &= ~(UINT64)((1ULL << 19) | (1ULL << 22) | (1ULL << 59)); // Adma2, Sdma, Adma3

  //
  // No tuning, no high speed modes beyond eMMC High Speed SDR.
  //
  Capability &= ~(UINT64)(
                          (1ULL << 32) |   // Sdr50
                          (1ULL << 33) |   // Sdr104
                          (1ULL << 34) |   // Ddr50
                          (1ULL << 35) |   // UhsII
                          (1ULL << 45) |   // TuningSDR50
                          (0x3ULL << 46) | // RetuningMod
                          (0xFULL << 40) | // TimerCount
                          (1ULL << 63)     // Hs400
                          );

  //
  // Keep everything on a 32-bit system bus and synchronous interrupts.
  //
  Capability &= ~(UINT64)(
                          (1ULL << 23) | // SuspRes
                          (1ULL << 27) | // SysBus64V4
                          (1ULL << 28) | // SysBus64V3
                          (1ULL << 29)   // AsyncInt
                          );

  *(UINT64 *)SdMmcHcSlotCapability = Capability;

  *BaseClkFreq = BaseClock / 1000000;
  return EFI_SUCCESS;
}

/**
  Update the Cadence PHY when the generic driver changes the bus clock.

  After the host is initialized and after a clock frequency switch, the
  divider written by the generic SDHCI driver is read back and the PHY is
  reprogrammed to match it.

  @param[in]      ControllerHandle  The controller handle.
  @param[in]      Slot              The slot number.
  @param[in]      PhaseType         The SD/MMC override phase being entered.
  @param[in,out]  PhaseData         Phase-specific data. Unused.

  @retval  EFI_SUCCESS  Always returned.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSdMmcNotifyPhase (
  IN     EFI_HANDLE               ControllerHandle,
  IN     UINT8                    Slot,
  IN     EDKII_SD_MMC_PHASE_TYPE  PhaseType,
  IN OUT VOID                     *PhaseData
  )
{
  switch (PhaseType) {
    case EdkiiSdMmcInitHostPost:
    case EdkiiSdMmcSwitchClockFreqPost:
      UpdateCadenceClock ();
      break;

    default:
      break;
  }

  return EFI_SUCCESS;
}

STATIC EDKII_SD_MMC_OVERRIDE  mSdMmcOverride = {
  EDKII_SD_MMC_OVERRIDE_PROTOCOL_VERSION,
  CN10KSdMmcCapability,
  CN10KSdMmcNotifyPhase
};

/**
  Non-discoverable device initialization callback.

  Called by NonDiscoverablePciDeviceDxe just before the SDHCI controller is
  enabled, i.e. before SdMmcPciHcDxe touches any register.

  @param[in]  This  The non-discoverable device instance.

  @retval  EFI_SUCCESS  The PHY was initialized.
  @retval  Others       CadenceSd6PhyInit() failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KEmmcPhyInitCallback (
  IN NON_DISCOVERABLE_DEVICE  *This
  )
{
  EFI_STATUS  Status;

  Status = CadenceSd6PhyInit (&mPhyConfig);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cadence SD6 PHY init failed: %r\n", __func__, Status));
  }

  return Status;
}

/**
  The entry point of the CN10K eMMC platform glue driver.

  @param[in]  ImageHandle  The firmware allocated handle for the image.
  @param[in]  SystemTable  A pointer to the EFI system table.

  @retval EFI_SUCCESS      The driver was initialized.
  @retval Others           The driver failed to initialize.
**/
EFI_STATUS
EFIAPI
InitializeCN10KMmcDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  OverrideHandle;

  Status = ParseFdtConfig ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: SDHCI version register = 0x%04x\n",
    __func__,
    MmioRead16 ((UINTN)CN10K_EMMC_SDHCI_BASE + CN10K_SDHCI_CTRL_VER)
    ));

  //
  // Install the override first so the generic driver finds it when it binds.
  //
  OverrideHandle = NULL;
  Status         = gBS->InstallProtocolInterface (
                          &OverrideHandle,
                          &gEdkiiSdMmcOverrideProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          &mSdMmcOverride
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to install override protocol: %r\n", __func__, Status));
    return Status;
  }

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeSdhci,
             NonDiscoverableDeviceDmaTypeCoherent,
             CN10KEmmcPhyInitCallback,
             NULL,
             1,
             (UINTN)CN10K_EMMC_SDHCI_BASE,
             (UINTN)CN10K_EMMC_SDHCI_SIZE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to register SDHCI device: %r\n", __func__, Status));
    gBS->UninstallProtocolInterface (
           OverrideHandle,
           &gEdkiiSdMmcOverrideProtocolGuid,
           &mSdMmcOverride
           );
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: CN10K eMMC SDHCI device registered at 0x%lx\n",
    __func__,
    (UINT64)CN10K_EMMC_SDHCI_BASE
    ));

  return EFI_SUCCESS;
}
