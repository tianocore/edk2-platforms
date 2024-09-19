/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/PlatformInfoHob.h>
#include <Guid/RootComplexInfoHob.h>
#include <IndustryStandard/Pci.h>
#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BoardPcieLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PciePhyLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>
#include <Library/TimerLib.h>

#include "PcieCore.h"

VOID
EnableDbiAccess (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex,
  BOOLEAN            EnableDbi
  );

BOOLEAN
EndpointCfgReady (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN UINT32             Timeout
  );

BOOLEAN
PcieLinkUpCheck (
  IN AC01_PCIE_CONTROLLER  *Pcie
  );

/**
  Return the next extended capability base address

  @param RootComplex      Pointer to AC01_ROOT_COMPLEX structure
  @param PcieIndex        PCIe controller index
  @param IsRootComplex    TRUE: Checking RootComplex configuration space
                          FALSE: Checking EP configuration space
  @param ExtCapabilityId
**/
PHYSICAL_ADDRESS
GetCapabilityBase (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN BOOLEAN            IsRootComplex,
  IN UINT16             ExtCapabilityId
  )
{
  BOOLEAN           IsExtCapability = FALSE;
  PHYSICAL_ADDRESS  CfgBase;
  PHYSICAL_ADDRESS  Ret = 0;
  PHYSICAL_ADDRESS  RootComplexCfgBase;
  UINT32            CapabilityId;
  UINT32            NextCapabilityPtr;
  UINT32            Val;
  UINT32            RestoreVal;

  RootComplexCfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);
  if (!IsRootComplex) {
    // Allow programming to config space
    EnableDbiAccess (RootComplex, PcieIndex, TRUE);

    Val        = MmioRead32 (RootComplexCfgBase + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG);
    RestoreVal = Val;
    Val        = SUB_BUS_SET (Val, DEFAULT_SUB_BUS);
    Val        = SEC_BUS_SET (Val, RootComplex->Pcie[PcieIndex].DevNum);
    Val        = PRIM_BUS_SET (Val, 0x0);
    MmioWrite32 (RootComplexCfgBase + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, Val);
    CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << BUS_SHIFT);

    if (!EndpointCfgReady (RootComplex, PcieIndex, EP_LINKUP_TIMEOUT)) {
      goto _CheckCapEnd;
    }
  } else {
    CfgBase = RootComplexCfgBase;
  }

  // Check if device provide capability
  Val = MmioRead32 (CfgBase + PCI_COMMAND_OFFSET);
  Val = GET_HIGH_16_BITS (Val); /* Status */
  if (!(Val & EFI_PCI_STATUS_CAPABILITY)) {
    goto _CheckCapEnd;
  }

  Val               = MmioRead32 (CfgBase + TYPE1_CAP_PTR_REG);
  NextCapabilityPtr = GET_LOW_8_BITS (Val);

  // Loop untill desired capability is found else return 0
  while (1) {
    if ((NextCapabilityPtr & WORD_ALIGN_MASK) != 0) {
      // Not alignment, just return
      Ret = 0;
      goto _CheckCapEnd;
    }

    Val = MmioRead32 (CfgBase + NextCapabilityPtr);
    if (NextCapabilityPtr < EXT_CAPABILITY_START_BASE) {
      CapabilityId = GET_LOW_8_BITS (Val);
    } else {
      CapabilityId = GET_LOW_16_BITS (Val);
    }

    if (CapabilityId == ExtCapabilityId) {
      Ret = (CfgBase + NextCapabilityPtr);
      goto _CheckCapEnd;
    }

    if (NextCapabilityPtr < EXT_CAPABILITY_START_BASE) {
      NextCapabilityPtr = GET_CAPABILITY_PTR (Val);
    } else {
      NextCapabilityPtr = GET_EXT_CAPABILITY_PTR (Val);
    }

    if ((NextCapabilityPtr == 0) && !IsExtCapability) {
      IsExtCapability   = TRUE;
      NextCapabilityPtr = EXT_CAPABILITY_START_BASE;
    }

    if ((NextCapabilityPtr == 0) && IsExtCapability) {
      Ret = 0;
      goto _CheckCapEnd;
    }
  }

_CheckCapEnd:
  if (!IsRootComplex) {
    MmioWrite32 (RootComplexCfgBase + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG, RestoreVal);

    // Disable programming to config space
    EnableDbiAccess (RootComplex, PcieIndex, FALSE);
  }

  return Ret;
}

/**
  Configure equalization settings for Gen3 and Gen4

  @param RootComplex     Pointer to AC01_ROOT_COMPLEX structure
  @param PcieIndex       PCIe controller index
**/
STATIC
VOID
ConfigureEqualization (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex
  )
{
  PHYSICAL_ADDRESS  CfgBase;
  PHYSICAL_ADDRESS  Gen3RelatedAddr;
  PHYSICAL_ADDRESS  Gen3EqControlAddr;
  UINT32            Val;

  CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);

  //
  // Gen3 and Gen4 EQ process use the same setting registers which are
  // GEN3_RELATED_OFF and GEN3_EQ_CONTROL_OFF. Both are shadow registers
  // and controlled by GEN3_RELATED_OFF[25:24].
  //
  Gen3RelatedAddr   = CfgBase + GEN3_RELATED_OFF;
  Gen3EqControlAddr = CfgBase + GEN3_EQ_CONTROL_OFF;

  //
  // Equalization setting for Gen3
  //
  Val = MmioRead32 (Gen3RelatedAddr);
  Val = RATE_SHADOW_SEL_SET (Val, GEN3_DATA_RATE);
  MmioWrite32 (Gen3RelatedAddr, Val);

  Val = EQ_PHASE_2_3_SET (Val, ENABLE_EQ_PHASE_2_3);
  Val = RXEQ_REGRDLESS_SET (Val, ASSERT_RXEQ);
  MmioWrite32 (Gen3RelatedAddr, Val);

  Val = MmioRead32 (Gen3EqControlAddr);
  Val = GEN3_EQ_FB_MODE (Val, FOM_METHOD);
  Val = GEN3_EQ_PRESET_VEC (Val, EQ_DEFAULT_PRESET_VECTOR);
  Val = GEN3_EQ_INIT_EVAL (Val, INCLUDE_INIT_FOM);
  MmioWrite32 (Gen3EqControlAddr, Val);

  //
  // Equalization setting for Gen4
  //
  Val = MmioRead32 (Gen3RelatedAddr);
  Val = RATE_SHADOW_SEL_SET (Val, GEN4_DATA_RATE);
  MmioWrite32 (Gen3RelatedAddr, Val);

  Val = EQ_PHASE_2_3_SET (Val, ENABLE_EQ_PHASE_2_3);
  Val = RXEQ_REGRDLESS_SET (Val, ASSERT_RXEQ);
  MmioWrite32 (Gen3RelatedAddr, Val);

  Val = MmioRead32 (Gen3EqControlAddr);
  Val = GEN3_EQ_FB_MODE (Val, FOM_METHOD);
  Val = GEN3_EQ_PRESET_VEC (Val, EQ_DEFAULT_PRESET_VECTOR);
  Val = GEN3_EQ_INIT_EVAL (Val, INCLUDE_INIT_FOM);
  MmioWrite32 (Gen3EqControlAddr, Val);
}

/**
  Configure presets for Gen3 equalization

  @param RootComplex     Pointer to AC01_ROOT_COMPLEX structure
  @param PcieIndex       PCIe controller index
**/
STATIC
VOID
ConfigurePresetGen3 (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex
  )
{
  PHYSICAL_ADDRESS  LaneEqControlAddr;
  PHYSICAL_ADDRESS  SpcieCapabilityBase;
  UINT32            Idx;
  UINT32            LinkWidth;
  UINT32            Val;

  // Get the Secondary PCI Express Extended capability base address
  SpcieCapabilityBase = GetCapabilityBase (RootComplex, PcieIndex, TRUE, SPCIE_CAPABILITY_ID);
  if (SpcieCapabilityBase == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "PCIE%d.%d: Cannot get SPCIE capability address\n",
      RootComplex->ID,
      PcieIndex
      ));
    return;
  }

  LinkWidth = RootComplex->Pcie[PcieIndex].MaxWidth;

  // Each register holds the Preset for 2 lanes
  for (Idx = 0; Idx < (LinkWidth / 2); Idx++) {
    LaneEqControlAddr = SpcieCapabilityBase + SPCIE_CAP_OFF_0C_REG + Idx * sizeof (UINT32);
    Val               = MmioRead32 (LaneEqControlAddr);
    Val               = DSP_TX_PRESET0_SET (Val, DEFAULT_GEN3_PRESET);
    Val               = DSP_TX_PRESET1_SET (Val, DEFAULT_GEN3_PRESET);
    MmioWrite32 (LaneEqControlAddr, Val);
  }
}

/**
  Configure presets for Gen4 equalization

  @param RootComplex     Pointer to AC01_ROOT_COMPLEX structure
  @param PcieIndex       PCIe controller index
**/
STATIC
VOID
ConfigurePresetGen4 (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex
  )
{
  PHYSICAL_ADDRESS  LaneEqControlAddr;
  PHYSICAL_ADDRESS  Pl16gCapabilityBase;
  UINT32            Idx;
  UINT32            LinkWidth;
  UINT32            Val;
  UINT8             Preset;

  // Get the Physical Layer 16.0 GT/s Extended capability base address
  Pl16gCapabilityBase = GetCapabilityBase (RootComplex, PcieIndex, TRUE, PL16G_CAPABILITY_ID);
  if (Pl16gCapabilityBase == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "PCIE%d.%d: Cannot get PL16G capability address\n",
      RootComplex->ID,
      PcieIndex
      ));
    return;
  }

  if (RootComplex->PresetGen4[PcieIndex] == PRESET_INVALID) {
    Preset = DEFAULT_GEN4_PRESET;
  } else {
    Preset = RootComplex->PresetGen4[PcieIndex];
  }

  LinkWidth = RootComplex->Pcie[PcieIndex].MaxWidth;

  if (LinkWidth == CAP_MAX_LINK_WIDTH_X2) {
    LaneEqControlAddr = Pl16gCapabilityBase + PL16G_CAP_OFF_20H_REG;
    Val               = MmioRead32 (LaneEqControlAddr);
    Val               = DSP_16G_RXTX_PRESET0_SET (Val, Preset);
    Val               = DSP_16G_RXTX_PRESET1_SET (Val, Preset);
    MmioWrite32 (LaneEqControlAddr, Val);
  } else {
    // Each register holds the Preset for 4 lanes
    for (Idx = 0; Idx < (LinkWidth / 4); Idx++) {
      LaneEqControlAddr = Pl16gCapabilityBase + PL16G_CAP_OFF_20H_REG + Idx * sizeof (UINT32);
      Val               = MmioRead32 (LaneEqControlAddr);
      Val               = DSP_16G_RXTX_PRESET0_SET (Val, Preset);
      Val               = DSP_16G_RXTX_PRESET1_SET (Val, Preset);
      Val               = DSP_16G_RXTX_PRESET2_SET (Val, Preset);
      Val               = DSP_16G_RXTX_PRESET3_SET (Val, Preset);
      MmioWrite32 (LaneEqControlAddr, Val);
    }
  }
}

VOID
ProgramHostBridgeInfo (
  AC01_ROOT_COMPLEX  *RootComplex
  )
{
  EFI_STATUS        Status;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;

  // Program Root Complex Bifurcation
  if (RootComplex->Active) {
    if (RootComplex->Type == RootComplexTypeA) {
      TargetAddress = RootComplex->HostBridgeBase + AC01_HOST_BRIDGE_RCA_DEV_MAP_REG;
      Status        = MailboxMsgRegisterRead (RootComplex->Socket, TargetAddress, &Val);
      if (!RETURN_ERROR (Status)) {
        Val = RCA_DEV_MAP_SET (Val, RootComplex->DevMapLow);
        MailboxMsgRegisterWrite (RootComplex->Socket, TargetAddress, Val);
      }
    } else {
      TargetAddress = RootComplex->HostBridgeBase + AC01_HOST_BRIDGE_RCB_DEV_MAP_REG;
      Status        = MailboxMsgRegisterRead (RootComplex->Socket, TargetAddress, &Val);
      if (!RETURN_ERROR (Status)) {
        Val = RCB_DEV_MAP_LOW_SET (Val, RootComplex->DevMapLow);
        Val = RCB_DEV_MAP_HIGH_SET (Val, RootComplex->DevMapHigh);
        MailboxMsgRegisterWrite (RootComplex->Socket, TargetAddress, Val);
      }
    }
  }

  // Program Vendor ID and Device ID
  TargetAddress = RootComplex->HostBridgeBase + AC01_HOST_BRIDGE_VENDOR_DEVICE_ID_REG;
  Status        = MailboxMsgRegisterRead (RootComplex->Socket, TargetAddress, &Val);
  if (!RETURN_ERROR (Status)) {
    Val = VENDOR_ID_SET (Val, AMPERE_PCIE_VENDOR_ID);
    if (RootComplexTypeA == RootComplex->Type) {
      Val = DEVICE_ID_SET (Val, AC01_HOST_BRIDGE_DEVICE_ID_RCA);
    } else {
      Val = DEVICE_ID_SET (Val, AC01_HOST_BRIDGE_DEVICE_ID_RCB);
    }

    MailboxMsgRegisterWrite (RootComplex->Socket, TargetAddress, Val);
  }
}

VOID
ProgramRootPortInfo (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex
  )
{
  PHYSICAL_ADDRESS  CfgBase;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;

  CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);

  // Program Class Code
  TargetAddress = CfgBase + TYPE1_CLASS_CODE_REV_ID_REG;
  Val           = MmioRead32 (TargetAddress);
  Val           = REVISION_ID_SET (Val, DEFAULT_REVISION_ID);
  Val           = SUB_CLASS_CODE_SET (Val, DEFAULT_SUB_CLASS_CODE);
  Val           = BASE_CLASS_CODE_SET (Val, DEFAULT_BASE_CLASS_CODE);
  MmioWrite32 (TargetAddress, Val);

  // Program Vendor ID and Device ID
  TargetAddress = CfgBase + TYPE1_DEV_ID_VEND_ID_REG;
  Val           = MmioRead32 (TargetAddress);
  Val           = VENDOR_ID_SET (Val, AMPERE_PCIE_VENDOR_ID);
  if (RootComplexTypeA == RootComplex->Type) {
    Val = DEVICE_ID_SET (Val, AC01_PCIE_BRIDGE_DEVICE_ID_RCA + PcieIndex);
  } else {
    Val = DEVICE_ID_SET (Val, AC01_PCIE_BRIDGE_DEVICE_ID_RCB + PcieIndex);
  }

  MmioWrite32 (TargetAddress, Val);
}

VOID
ProgramLinkCapabilities (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex
  )
{
  PHYSICAL_ADDRESS  CfgBase;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;
  UINT8             MaxWidth;
  UINT8             MaxGen;

  CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);

  MaxWidth = RootComplex->Pcie[PcieIndex].MaxWidth;
  MaxGen   = RootComplex->Pcie[PcieIndex].MaxGen;

  TargetAddress = CfgBase + PORT_LINK_CTRL_OFF;
  Val           = MmioRead32 (TargetAddress);
  switch (MaxWidth) {
    case LINK_WIDTH_X2:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X2);
      break;

    case LINK_WIDTH_X4:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X4);
      break;

    case LINK_WIDTH_X8:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X8);
      break;

    case LINK_WIDTH_X16:
    default:
      Val = LINK_CAPABLE_SET (Val, LINK_CAPABLE_X16);
      break;
  }

  MmioWrite32 (TargetAddress, Val);

  TargetAddress = CfgBase + GEN2_CTRL_OFF;
  Val           = MmioRead32 (TargetAddress);
  switch (MaxWidth) {
    case LINK_WIDTH_X2:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X2);
      break;

    case LINK_WIDTH_X4:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X4);
      break;

    case LINK_WIDTH_X8:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X8);
      break;

    case LINK_WIDTH_X16:
    default:
      Val = NUM_OF_LANES_SET (Val, NUM_OF_LANES_X16);
      break;
  }

  MmioWrite32 (TargetAddress, Val);

  TargetAddress = CfgBase + PCIE_CAPABILITY_BASE + LINK_CAPABILITIES_REG;
  Val           = MmioRead32 (TargetAddress);
  switch (MaxWidth) {
    case LINK_WIDTH_X2:
      Val = CAP_MAX_LINK_WIDTH_SET (Val, CAP_MAX_LINK_WIDTH_X2);
      break;

    case LINK_WIDTH_X4:
      Val = CAP_MAX_LINK_WIDTH_SET (Val, CAP_MAX_LINK_WIDTH_X4);
      break;

    case LINK_WIDTH_X8:
      Val = CAP_MAX_LINK_WIDTH_SET (Val, CAP_MAX_LINK_WIDTH_X8);
      break;

    case LINK_WIDTH_X16:
    default:
      Val = CAP_MAX_LINK_WIDTH_SET (Val, CAP_MAX_LINK_WIDTH_X16);
      break;
  }

  switch (MaxGen) {
    case LINK_SPEED_GEN1:
      Val = CAP_MAX_LINK_SPEED_SET (Val, MAX_LINK_SPEED_25);
      break;

    case LINK_SPEED_GEN2:
      Val = CAP_MAX_LINK_SPEED_SET (Val, MAX_LINK_SPEED_50);
      break;

    case LINK_SPEED_GEN3:
      Val = CAP_MAX_LINK_SPEED_SET (Val, MAX_LINK_SPEED_80);
      break;

    default:
      Val = CAP_MAX_LINK_SPEED_SET (Val, MAX_LINK_SPEED_160);
      break;
  }

  // Enable ASPM Capability
  Val = CAP_ACTIVE_STATE_LINK_PM_SUPPORT_SET (Val, L0S_L1_SUPPORTED);
  MmioWrite32 (TargetAddress, Val);

  TargetAddress = CfgBase + PCIE_CAPABILITY_BASE + LINK_CONTROL2_LINK_STATUS2_REG;
  Val           = MmioRead32 (TargetAddress);
  switch (MaxGen) {
    case LINK_SPEED_GEN1:
      Val = CAP_TARGET_LINK_SPEED_SET (Val, MAX_LINK_SPEED_25);
      break;

    case LINK_SPEED_GEN2:
      Val = CAP_TARGET_LINK_SPEED_SET (Val, MAX_LINK_SPEED_50);
      break;

    case LINK_SPEED_GEN3:
      Val = CAP_TARGET_LINK_SPEED_SET (Val, MAX_LINK_SPEED_80);
      break;

    default:
      Val = CAP_TARGET_LINK_SPEED_SET (Val, MAX_LINK_SPEED_160);
      break;
  }

  MmioWrite32 (TargetAddress, Val);
}

VOID
DisableCompletionTimeOut (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN BOOLEAN            IsMask
  )
{
  PHYSICAL_ADDRESS  CfgBase;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;

  CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);

  TargetAddress = CfgBase + AER_CAPABILITY_BASE + UNCORR_ERR_MASK_OFF;
  Val           = MmioRead32 (TargetAddress);
  Val           = CMPLT_TIMEOUT_ERR_MASK_SET (Val, IsMask ? 1 : 0);
  MmioWrite32 (TargetAddress, Val);
}

BOOLEAN
EnableItsMemory (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex
  )
{
  PHYSICAL_ADDRESS  CsrBase;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            TimeOut;
  UINT32            Val;

  CsrBase = RootComplex->Pcie[PcieIndex].CsrBase;

  // Clear memory shutdown
  TargetAddress = CsrBase + AC01_PCIE_CORE_RAM_SHUTDOWN_REG;
  Val           = MmioRead32 (TargetAddress);
  Val           = SD_SET (Val, 0);
  MmioWrite32 (TargetAddress, Val);

  // Poll till ITS Memory is ready
  TimeOut = MEMRDY_TIMEOUT;
  do {
    Val = MmioRead32 (CsrBase + AC01_PCIE_CORE_MEM_READY_REG);
    if (Val & MEMORY_READY) {
      return TRUE;
    }

    TimeOut--;
    MicroSecondDelay (1);
  } while (TimeOut > 0);

  return FALSE;
}

BOOLEAN
EnableAxiPipeClock (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex
  )
{
  PHYSICAL_ADDRESS  CsrBase;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            TimeOut;
  UINT32            Val;

  CsrBase = RootComplex->Pcie[PcieIndex].CsrBase;

  // Enable subsystem clock and release reset
  TargetAddress = CsrBase + AC01_PCIE_CORE_CLOCK_REG;
  Val           = MmioRead32 (TargetAddress);
  Val           = AXIPIPE_SET (Val, 1);
  MmioWrite32 (TargetAddress, Val);

  TargetAddress = CsrBase + AC01_PCIE_CORE_RESET_REG;
  Val           = MmioRead32 (TargetAddress);
  Val           = DWC_PCIE_SET (Val, 0);
  MmioWrite32 (TargetAddress, Val);

  //
  // Controller does not provide any indicator for reset released.
  // Must wait at least 1us as per EAS.
  //
  MicroSecondDelay (1);

  // Poll till PIPE clock is stable
  TimeOut = PIPE_CLOCK_TIMEOUT;
  do {
    Val = MmioRead32 (CsrBase + AC01_PCIE_CORE_LINK_STAT_REG);
    if (!(Val & PHY_STATUS_MASK)) {
      return TRUE;
    }

    TimeOut--;
    MicroSecondDelay (1);
  } while (TimeOut > 0);

  return FALSE;
}

VOID
SetLinkTimeout (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex,
  UINTN              Timeout
  )
{
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;

  TargetAddress = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT)
                  + AMBA_LINK_TIMEOUT_OFF;

  Val = MmioRead32 (TargetAddress);
  Val = LINK_TIMEOUT_PERIOD_DEFAULT_SET (Val, Timeout);
  MmioWrite32 (TargetAddress, Val);
}

VOID
StartLinkTraining (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex,
  BOOLEAN            StartLink
  )
{
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;

  TargetAddress = RootComplex->Pcie[PcieIndex].CsrBase + AC01_PCIE_CORE_LINK_CTRL_REG;

  Val = MmioRead32 (TargetAddress);
  Val = LTSSMENB_SET (Val, StartLink ? START_LINK_TRAINING : HOLD_LINK_TRAINING);
  MmioWrite32 (TargetAddress, Val);
}

VOID
EnableDbiAccess (
  AC01_ROOT_COMPLEX  *RootComplex,
  UINT32             PcieIndex,
  BOOLEAN            EnableDbi
  )
{
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            Val;

  TargetAddress = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT)
                  + MISC_CONTROL_1_OFF;

  Val = MmioRead32 (TargetAddress);
  Val = DBI_RO_WR_EN_SET (Val, EnableDbi ? ENABLE_WR : DISABLE_WR);
  MmioWrite32 (TargetAddress, Val);
}

VOID
Ac01PcieUpdateMaxWidth (
  IN AC01_ROOT_COMPLEX  *RootComplex
  )
{
  if (RootComplex->Type == RootComplexTypeA) {
    switch (RootComplex->DevMapLow) {
      case DevMapMode1:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X16;
        break;

      case DevMapMode2:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X8;
        RootComplex->Pcie[PcieController2].MaxWidth = CAP_MAX_LINK_WIDTH_X8;
        break;

      case DevMapMode3:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X8;
        RootComplex->Pcie[PcieController2].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController3].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        break;

      case DevMapMode4:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController1].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController2].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController3].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        break;

      default:
        ASSERT (FALSE);
    }
  } else {
    switch (RootComplex->DevMapLow) {
      case DevMapMode1:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X8;
        break;

      case DevMapMode2:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController2].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        break;

      case DevMapMode3:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController2].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController3].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        break;

      case DevMapMode4:
        RootComplex->Pcie[PcieController0].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController1].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController2].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController3].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        break;

      default:
        ASSERT (FALSE);
    }

    switch (RootComplex->DevMapHigh) {
      case DevMapMode1:
        RootComplex->Pcie[PcieController4].MaxWidth = CAP_MAX_LINK_WIDTH_X8;
        break;

      case DevMapMode2:
        RootComplex->Pcie[PcieController4].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController6].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        break;

      case DevMapMode3:
        RootComplex->Pcie[PcieController4].MaxWidth = CAP_MAX_LINK_WIDTH_X4;
        RootComplex->Pcie[PcieController6].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController7].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        break;

      case DevMapMode4:
        RootComplex->Pcie[PcieController4].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController5].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController6].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        RootComplex->Pcie[PcieController7].MaxWidth = CAP_MAX_LINK_WIDTH_X2;
        break;

      default:
        ASSERT (FALSE);
    }
  }
}

VOID
Ac01PcieUpdateActive (
  IN AC01_ROOT_COMPLEX  *RootComplex
  )
{
  switch (RootComplex->DevMapLow) {
    case DevMapMode1:
      RootComplex->Pcie[PcieController0].Active = TRUE;
      RootComplex->Pcie[PcieController1].Active = FALSE;
      RootComplex->Pcie[PcieController2].Active = FALSE;
      RootComplex->Pcie[PcieController3].Active = FALSE;
      break;

    case DevMapMode2:
      RootComplex->Pcie[PcieController0].Active = TRUE;
      RootComplex->Pcie[PcieController1].Active = FALSE;
      RootComplex->Pcie[PcieController2].Active = TRUE;
      RootComplex->Pcie[PcieController3].Active = FALSE;
      break;

    case DevMapMode3:
      RootComplex->Pcie[PcieController0].Active = TRUE;
      RootComplex->Pcie[PcieController1].Active = FALSE;
      RootComplex->Pcie[PcieController2].Active = TRUE;
      RootComplex->Pcie[PcieController3].Active = TRUE;
      break;

    case DevMapMode4:
      RootComplex->Pcie[PcieController0].Active = TRUE;
      RootComplex->Pcie[PcieController1].Active = TRUE;
      RootComplex->Pcie[PcieController2].Active = TRUE;
      RootComplex->Pcie[PcieController3].Active = TRUE;
      break;

    default:
      ASSERT (FALSE);
  }

  if (RootComplex->Type == RootComplexTypeB) {
    switch (RootComplex->DevMapHigh) {
      case DevMapMode1:
        RootComplex->Pcie[PcieController4].Active = TRUE;
        RootComplex->Pcie[PcieController5].Active = FALSE;
        RootComplex->Pcie[PcieController6].Active = FALSE;
        RootComplex->Pcie[PcieController7].Active = FALSE;
        break;

      case DevMapMode2:
        RootComplex->Pcie[PcieController4].Active = TRUE;
        RootComplex->Pcie[PcieController5].Active = FALSE;
        RootComplex->Pcie[PcieController6].Active = TRUE;
        RootComplex->Pcie[PcieController7].Active = FALSE;
        break;

      case DevMapMode3:
        RootComplex->Pcie[PcieController4].Active = TRUE;
        RootComplex->Pcie[PcieController5].Active = FALSE;
        RootComplex->Pcie[PcieController6].Active = TRUE;
        RootComplex->Pcie[PcieController7].Active = TRUE;
        break;

      case DevMapMode4:
        RootComplex->Pcie[PcieController4].Active = TRUE;
        RootComplex->Pcie[PcieController5].Active = TRUE;
        RootComplex->Pcie[PcieController6].Active = TRUE;
        RootComplex->Pcie[PcieController7].Active = TRUE;
        break;

      default:
        ASSERT (FALSE);
    }
  }
}

EFI_STATUS
Ac01PcieCorrectBifurcation (
  IN     AC01_ROOT_COMPLEX             *RootComplex,
  IN     PCI_REG_PCIE_LINK_CAPABILITY  *LinkCap,
  IN     UINTN                         LinkCapLength,
  IN OUT DEV_MAP_MODE                  *Bifur
  )
{
  UINTN       Count;
  UINTN       Idx;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if ((RootComplex == NULL) || (LinkCap == NULL) || (Bifur == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (LinkCapLength != 4) {
    // Only process 4 controller at a same time
    return EFI_INVALID_PARAMETER;
  }

  if (LinkCap[PcieController1].Uint32 != 0) {
    // Bifurcation should be X/X/X/X
    *Bifur = BIFURCATION_XXXX;
    return Status;
  }

  Count = 0;
  for (Idx = 0; Idx < LinkCapLength; Idx++) {
    if (LinkCap[Idx].Uint32 != 0) {
      Count++;
    }
  }

  switch (Count) {
    case 3:
      // Bifurcation should be X/0/X/X
      *Bifur = BIFURCATION_X0XX;
      break;

    case 2:
      if (LinkCap[PcieController0].Uint32 != 0) {
        if (LinkCap[PcieController2].Uint32) {
          *Bifur = BIFURCATION_X0X0;
        } else {
          *Bifur = BIFURCATION_X0XX;
        }
      } else {
        *Bifur = BIFURCATION_XXXX;
      }

      break;

    case 1:
      if (LinkCap[PcieController0].Uint32 != 0) {
        *Bifur = BIFURCATION_X000;
      } else if (LinkCap[PcieController2].Uint32 != 0) {
        *Bifur = BIFURCATION_X0X0;
      } else {
        // In the lane reverse case, we choose best width
        switch (LinkCap[PcieController3].Bits.MaxLinkWidth) {
          /* MAX_SPEED [9:4] */
          case CAP_MAX_LINK_WIDTH_X1:
          case CAP_MAX_LINK_WIDTH_X2:
            *Bifur = BIFURCATION_XXXX;
            break;

          case CAP_MAX_LINK_WIDTH_X4:
            if (RootComplex->Type == RootComplexTypeA) {
              *Bifur = BIFURCATION_XXXX;
            } else {
              *Bifur = BIFURCATION_X0X0;
            }

            break;

          case CAP_MAX_LINK_WIDTH_X8:
            if (RootComplex->Type == RootComplexTypeA) {
              *Bifur = BIFURCATION_X0X0;
            } else {
              *Bifur = BIFURCATION_X000;
            }

            break;

          default:
            *Bifur = BIFURCATION_X000;
            break;
        }
      }

      break;

    default:
      Status = EFI_NOT_AVAILABLE_YET;
      break;
  }

  return Status;
}

/**
  Setup and initialize the AC01 PCIe Root Complex and underneath PCIe controllers

  @param RootComplex           Pointer to Root Complex structure
  @param ReInit                Re-init status
  @param ReInitPcieIndex       PCIe controller index

  @retval RETURN_SUCCESS       The Root Complex has been initialized successfully.
  @retval RETURN_DEVICE_ERROR  PHY, Memory or PIPE is not ready.
**/
RETURN_STATUS
Ac01PcieCoreSetupRC (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN BOOLEAN            ReInit,
  IN UINT8              ReInitPcieIndex
  )
{
  PHYSICAL_ADDRESS              CfgBase;
  PHYSICAL_ADDRESS              CsrBase;
  PHYSICAL_ADDRESS              TargetAddress;
  RETURN_STATUS                 Status;
  UINT32                        Val;
  UINT8                         PcieIndex;
  BOOLEAN                       AutoLaneBifurcationEnabled = FALSE;
  PCI_REG_PCIE_LINK_CAPABILITY  LinkCap[MaxPcieController];
  AC01_PCIE_CONTROLLER          *Pcie;
  DEV_MAP_MODE                  DevMapMode;

  DEBUG ((DEBUG_INFO, "Initializing Socket%d RootComplex%d\n", RootComplex->Socket, RootComplex->ID));

AutoLaneBifurcationRetry:
  if (!ReInit) {
    if (AutoLaneBifurcationEnabled) {
      //
      // We are here after the first round
      //
      // As per 2.7.2. AC Specifications of PCIe card specification this TPVPERL time and
      // should be minimum 100ms. So this is minimum time we need add and found during test.
      //
      MicroSecondDelay (100000);
      SetMem ((VOID *)LinkCap, sizeof (LinkCap), 0);
      for (PcieIndex = 0; PcieIndex < RootComplex->MaxPcieController; PcieIndex++) {
        Pcie = &RootComplex->Pcie[PcieIndex];
        if (!Pcie->Active || !PcieLinkUpCheck (Pcie)) {
          continue;
        }

        DEBUG ((DEBUG_INFO, "RootComplex->ID:%d Port:%d link up\n", RootComplex->ID, PcieIndex));
        TargetAddress = GetCapabilityBase (RootComplex, PcieIndex, FALSE, EFI_PCI_CAPABILITY_ID_PCIEXP);
        if (TargetAddress == 0) {
          continue;
        }

        LinkCap[PcieIndex].Uint32 = MmioRead32 (TargetAddress + LINK_CAPABILITIES_REG);
      }

      Status = Ac01PcieCorrectBifurcation (RootComplex, LinkCap, MaxPcieControllerOfRootComplexA, &DevMapMode);
      if (!EFI_ERROR (Status)) {
        RootComplex->DevMapLow = DevMapMode;
        DEBUG ((
          DEBUG_INFO,
          "RootComplex->ID:%d Auto Bifurcation done, DevMapMode:%d\n",
          RootComplex->ID,
          RootComplex->DevMapLow
          ));
      } else {
        RootComplex->DevMapLow = DevMapMode1;
        DEBUG ((
          DEBUG_INFO,
          "RootComplex->ID:%d Auto Bifurcation failed, revert to DevMapMode1\n",
          RootComplex->ID
          ));
      }

      AutoLaneBifurcationEnabled = FALSE;

      if (DevMapMode == DevMapMode4) {
        // Return directly as the RootComplex already initialized in this mode
        return EFI_SUCCESS;
      }

      //
      // Update the RootComplex data with new DevMapMode
      //
      Ac01PcieUpdateActive (RootComplex);
      Ac01PcieUpdateMaxWidth (RootComplex);
    } else {
      if (RootComplex->DevMapLow == DevMapModeAuto) {
        // Set lowest bifurcation mode
        RootComplex->DevMapLow = DevMapMode4;

        AutoLaneBifurcationEnabled = TRUE;
        DEBUG ((
          DEBUG_INFO,
          "RootComplex->ID:%d Auto Bifurcation enabled\n",
          RootComplex->ID
          ));
      }
    }

    ProgramHostBridgeInfo (RootComplex);

    // Fix for UEFI hang due to timing change with bifurcation
    // register moved very close to PHY initialization.
    MicroSecondDelay (100000);

    Status = PciePhyInit (RootComplex->SerdesBase);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to initialize the PCIe PHY\n", __func__));
      return RETURN_DEVICE_ERROR;
    }
  }

  // Setup each controller
  for (PcieIndex = 0; PcieIndex < RootComplex->MaxPcieController; PcieIndex++) {
    if (ReInit) {
      PcieIndex = ReInitPcieIndex;
    }

    if (!RootComplex->Pcie[PcieIndex].Active) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "Initializing Controller %d\n", PcieIndex));

    CsrBase = RootComplex->Pcie[PcieIndex].CsrBase;
    CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);

    // Put Controller into reset if not in reset already
    TargetAddress = CsrBase + AC01_PCIE_CORE_RESET_REG;
    Val           = MmioRead32 (TargetAddress);
    if (!(Val & RESET_MASK)) {
      Val = DWC_PCIE_SET (Val, ASSERT_RESET);
      MmioWrite32 (TargetAddress, Val);

      // Delay 50ms to ensure controller finish its reset
      MicroSecondDelay (50000);
    }

    if (!EnableItsMemory (RootComplex, PcieIndex)) {
      DEBUG ((DEBUG_ERROR, "- Pcie[%d] - ITS Memory is not ready\n", PcieIndex));
      return RETURN_DEVICE_ERROR;
    }

    // Hold link training
    StartLinkTraining (RootComplex, PcieIndex, FALSE);

    // Clear BUSCTRL.CfgUrMask to set CRS (Configuration Request Retry Status) to 0xFFFF.FFFF
    // rather than 0xFFFF.0001 as per PCIe specification requirement. Otherwise, this causes
    // device drivers respond incorrectly on timeout due to long device operations.
    TargetAddress = CsrBase + AC01_PCIE_CORE_BUS_CONTROL_REG;
    Val           = MmioRead32 (TargetAddress);
    Val          &= ~BUS_CTL_CFG_UR_MASK;
    MmioWrite32 (TargetAddress, Val);

    if (!EnableAxiPipeClock (RootComplex, PcieIndex)) {
      DEBUG ((DEBUG_ERROR, "- Pcie[%d] - PIPE clock is not stable\n", PcieIndex));
      return RETURN_DEVICE_ERROR;
    }

    // Start PERST pulse
    BoardPcieAssertPerst (RootComplex, PcieIndex, TRUE);

    // Allow programming to config space
    EnableDbiAccess (RootComplex, PcieIndex, TRUE);

    // Program the power limit
    TargetAddress = CfgBase + PCIE_CAPABILITY_BASE + SLOT_CAPABILITIES_REG;
    Val           = MmioRead32 (TargetAddress);
    // In order to detect the NVMe after OS boots successfully but
    // that NVMe's not present previously. Hot Plug Slot Capable
    // will help PCI Linux driver to initialize its slot iomem resource
    // which is used for detecting the disk when it's inserted.
    Val = SLOT_HPC_SET (Val, 1);
    Val = SLOT_CAP_SLOT_POWER_LIMIT_VALUE_SET (Val, SLOT_POWER_LIMIT_75W);
    MmioWrite32 (TargetAddress, Val);

    // Program DTI for ATS support
    TargetAddress = CfgBase + DTIM_CTRL0_OFF;
    Val           = MmioRead32 (TargetAddress);
    Val           = DTIM_CTRL0_ROOT_PORT_ID_SET (Val, 0);
    MmioWrite32 (TargetAddress, Val);

    //
    // Program number of lanes used
    // - Reprogram LINK_CAPABLE of PORT_LINK_CTRL_OFF
    // - Reprogram NUM_OF_LANES of GEN2_CTRL_OFF
    // - Reprogram CAP_MAX_LINK_WIDTH of LINK_CAPABILITIES_REG
    //
    ProgramLinkCapabilities (RootComplex, PcieIndex);

    // Set Zero byte request handling
    TargetAddress = CfgBase + FILTER_MASK_2_OFF;
    Val           = MmioRead32 (TargetAddress);
    Val           = CX_FLT_MASK_VENMSG0_DROP_SET (Val, 0);
    Val           = CX_FLT_MASK_VENMSG1_DROP_SET (Val, 0);
    Val           = CX_FLT_MASK_DABORT_4UCPL_SET (Val, 0);
    MmioWrite32 (TargetAddress, Val);

    TargetAddress = CfgBase + AMBA_ORDERING_CTRL_OFF;
    Val           = MmioRead32 (TargetAddress);
    Val           = AX_MSTR_ZEROLREAD_FW_SET (Val, 0);
    MmioWrite32 (TargetAddress, Val);

    //
    // Set Completion with CRS handling for CFG Request
    // Set Completion with CA/UR handling non-CFG Request
    //
    TargetAddress = CfgBase + AMBA_ERROR_RESPONSE_DEFAULT_OFF;
    Val           = MmioRead32 (TargetAddress);
    // 0x2: OKAY with FFFF_0001 and FFFF_FFFF
    Val = AMBA_ERROR_RESPONSE_CRS_SET (Val, 0x2);
    MmioWrite32 (TargetAddress, Val);

    // Set Legacy PCIE interrupt map to INTA
    TargetAddress = CfgBase + BRIDGE_CTRL_INT_PIN_INT_LINE_REG;
    Val           = MmioRead32 (TargetAddress);
    Val           = INT_PIN_SET (Val, IRQ_INT_A);
    MmioWrite32 (TargetAddress, Val);

    TargetAddress = CsrBase + AC01_PCIE_CORE_IRQ_SEL_REG;
    Val           = MmioRead32 (TargetAddress);
    Val           = INTPIN_SET (Val, IRQ_INT_A);
    MmioWrite32 (TargetAddress, Val);

    if (RootComplex->Pcie[PcieIndex].MaxGen >= LINK_SPEED_GEN2) {
      ConfigureEqualization (RootComplex, PcieIndex);
      if (RootComplex->Pcie[PcieIndex].MaxGen >= LINK_SPEED_GEN3) {
        ConfigurePresetGen3 (RootComplex, PcieIndex);
        if (RootComplex->Pcie[PcieIndex].MaxGen >= LINK_SPEED_GEN4) {
          ConfigurePresetGen4 (RootComplex, PcieIndex);
        }
      }
    }

    //
    // As AMBA_LINK_TIMEOUT_OFF spec, it impacts OS HP removal delay.
    // The greater value the longer delay it is. Per discussion,
    // set it 2 from beginning of RP initialization.
    //
    SetLinkTimeout (RootComplex, PcieIndex, 2);

    DisableCompletionTimeOut (RootComplex, PcieIndex, TRUE);

    ProgramRootPortInfo (RootComplex, PcieIndex);

    // Enable common clock for downstream
    TargetAddress = CfgBase + PCIE_CAPABILITY_BASE + LINK_CONTROL_LINK_STATUS_REG;
    Val           = MmioRead32 (TargetAddress);
    Val           = CAP_SLOT_CLK_CONFIG_SET (Val, 1);
    Val           = CAP_COMMON_CLK_SET (Val, 1);
    MmioWrite32 (TargetAddress, Val);

    // Match aux_clk to system
    TargetAddress = CfgBase + AUX_CLK_FREQ_OFF;
    Val           = MmioRead32 (TargetAddress);
    Val           = AUX_CLK_FREQ_SET (Val, AUX_CLK_500MHZ);
    MmioWrite32 (TargetAddress, Val);

    // Assert PERST low to reset endpoint
    BoardPcieAssertPerst (RootComplex, PcieIndex, FALSE);

    // Complete the PERST pulse
    BoardPcieAssertPerst (RootComplex, PcieIndex, TRUE);

    // Start link training
    StartLinkTraining (RootComplex, PcieIndex, TRUE);

    // Lock programming of config space
    EnableDbiAccess (RootComplex, PcieIndex, FALSE);

    if (ReInit) {
      return RETURN_SUCCESS;
    }
  }

  if (AutoLaneBifurcationEnabled) {
    goto AutoLaneBifurcationRetry;
  }

  return RETURN_SUCCESS;
}

BOOLEAN
PcieLinkUpCheck (
  IN AC01_PCIE_CONTROLLER  *Pcie
  )
{
  PHYSICAL_ADDRESS  CsrBase;
  UINT32            BlockEvent;
  UINT32            LinkStat;

  CsrBase = Pcie->CsrBase;

  // Check if card present
  // smlh_ltssm_state[13:8] = 0
  // phy_status[2] = 0
  // smlh_link_up[1] = 0
  // rdlh_link_up[0] = 0
  LinkStat = MmioRead32 (CsrBase + AC01_PCIE_CORE_LINK_STAT_REG);
  LinkStat = LinkStat & (SMLH_LTSSM_STATE_MASK | PHY_STATUS_MASK_BIT |
                         SMLH_LINK_UP_MASK_BIT | RDLH_LINK_UP_MASK_BIT);
  if (LinkStat == 0) {
    return FALSE;
  }

  BlockEvent = MmioRead32 (CsrBase + AC01_PCIE_CORE_BLOCK_EVENT_STAT_REG);
  LinkStat   = MmioRead32 (CsrBase + AC01_PCIE_CORE_LINK_STAT_REG);

  if (  ((BlockEvent & LINKUP_MASK) != 0)
     && (SMLH_LTSSM_STATE_GET (LinkStat) == LTSSM_STATE_L0))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Callback function when the Host Bridge enumeration end.

  @param RootComplex          Pointer to the Root Complex structure
**/
VOID
Ac01PcieCoreEndEnumeration (
  IN AC01_ROOT_COMPLEX  *RootComplex
  )
{
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            PcieIndex;
  UINT32            Val;

  if ((RootComplex == NULL) || !RootComplex->Active) {
    return;
  }

  // Clear uncorrectable error during enumuration phase. Mainly completion timeout.
  for (PcieIndex = 0; PcieIndex < RootComplex->MaxPcieController; PcieIndex++) {
    if (!RootComplex->Pcie[PcieIndex].Active) {
      continue;
    }

    if (!PcieLinkUpCheck (&RootComplex->Pcie[PcieIndex])) {
      // If link down/disabled after enumeration, disable completed time out
      DisableCompletionTimeOut (RootComplex, PcieIndex, TRUE);
    }

    // Clear all errors
    TargetAddress = RootComplex->MmcfgBase + ((PcieIndex + 1) << DEV_SHIFT) \
                    + AER_CAPABILITY_BASE + UNCORR_ERR_STATUS_OFF;
    Val = MmioRead32 (TargetAddress);
    if (Val != 0) {
      // Clear error by writting
      MmioWrite32 (TargetAddress, Val);
    }
  }
}

/**
  Comparing current link status with the max capabilities of the link

  @param RootComplex   Pointer to AC01_ROOT_COMPLEX structure
  @param PcieIndex     PCIe controller index
  @param EpMaxWidth    EP max link width
  @param EpMaxGen      EP max link speed

  @retval              -1: Link status do not match with link max capabilities
                        1: Link capabilites are invalid
                        0: Link status are correct
**/
INT32
Ac01PcieCoreLinkCheck (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN UINT8              EpMaxWidth,
  IN UINT8              EpMaxGen
  )
{
  PHYSICAL_ADDRESS  CsrBase, CfgBase;
  UINT32            Val, LinkStat;
  UINT32            MaxWidth, MaxGen;

  CsrBase = RootComplex->Pcie[PcieIndex].CsrBase;
  CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);

  Val = MmioRead32 (CfgBase + PCIE_CAPABILITY_BASE + LINK_CAPABILITIES_REG);
  if ((CAP_MAX_LINK_WIDTH_GET (Val) == 0) ||
      (CAP_MAX_LINK_SPEED_GET (Val) == 0))
  {
    DEBUG ((DEBUG_INFO, "\tPCIE%d.%d: Wrong RootComplex capabilities\n", RootComplex->ID, PcieIndex));
    return LINK_CHECK_WRONG_PARAMETER;
  }

  if ((EpMaxWidth == 0) || (EpMaxGen == 0)) {
    DEBUG ((DEBUG_INFO, "\tPCIE%d.%d: Wrong EP capabilities\n", RootComplex->ID, PcieIndex));
    return LINK_CHECK_FAILED;
  }

  // Compare RootComplex and EP capabilities
  if (CAP_MAX_LINK_WIDTH_GET (Val) > EpMaxWidth) {
    MaxWidth = EpMaxWidth;
  } else {
    MaxWidth = CAP_MAX_LINK_WIDTH_GET (Val);
  }

  // Compare RootComplex and EP capabilities
  if (CAP_MAX_LINK_SPEED_GET (Val) > EpMaxGen) {
    MaxGen = EpMaxGen;
  } else {
    MaxGen = CAP_MAX_LINK_SPEED_GET (Val);
  }

  LinkStat = MmioRead32 (CsrBase + AC01_PCIE_CORE_LINK_STAT_REG);
  Val      = MmioRead32 (CfgBase + PCIE_CAPABILITY_BASE + LINK_CONTROL_LINK_STATUS_REG);
  DEBUG ((
    DEBUG_INFO,
    "PCIE%d.%d: Link MaxWidth %d MaxGen %d, AC01_PCIE_CORE_LINK_STAT_REG 0x%x",
    RootComplex->ID,
    PcieIndex,
    MaxWidth,
    MaxGen,
    LinkStat
    ));

  // Checking all conditions of the link
  // If one of them is not sastified, return link up fail
  if ((CAP_NEGO_LINK_WIDTH_GET (Val) != MaxWidth) ||
      (CAP_LINK_SPEED_GET (Val) != MaxGen) ||
      (RDLH_SMLH_LINKUP_STATUS_GET (LinkStat) != (SMLH_LINK_UP_MASK_BIT | RDLH_LINK_UP_MASK_BIT)))
  {
    DEBUG ((DEBUG_INFO, "\tLinkCheck FAILED\n"));
    return LINK_CHECK_FAILED;
  }

  DEBUG ((DEBUG_INFO, "\tLinkCheck SUCCESS\n"));
  return LINK_CHECK_SUCCESS;
}

INT32
PFACounterRead (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN UINT64             RasDesCapabilityBase
  )
{
  INT32   Ret = LINK_CHECK_SUCCESS;
  UINT32  Val;
  UINT8   ErrCode, ErrGrpNum;

  UINT32  ErrCtrlCfg[] = {
    0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x00A, // Per Lane
    0x105, 0x106, 0x107, 0x108, 0x109, 0x10A,
    0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207,
    0x300, 0x301, 0x302, 0x303, 0x304, 0x305,
    0x400, 0x401, // Per Lane
    0x500, 0x501, 0x502, 0x503, 0x504, 0x505, 0x506, 0x507, 0x508, 0x509, 0x50A, 0x50B, 0x50C, 0x50D
  };

  for (ErrCode = 0; ErrCode < ARRAY_SIZE (ErrCtrlCfg); ErrCode++) {
    ErrGrpNum = GET_HIGH_8_BITS (ErrCtrlCfg[ErrCode]);
    // Skipping per lane group
    // Checking common lane group because AER error are included in common group only
    if ((ErrGrpNum != 0) && (ErrGrpNum != 4)) {
      Val = MmioRead32 (RasDesCapabilityBase + EVENT_COUNTER_CONTROL_REG);
      if (RootComplex->Type == RootComplexTypeA) {
        // RootComplexTypeA - 4 PCIe controller per port, 1 controller in charge of 4 lanes
        Val = ECCR_LANE_SEL_SET (Val, PcieIndex * 4);
      } else {
        // RootComplexTypeB - 8 PCIe controller per port, 1 controller in charge of 2 lanes
        Val = ECCR_LANE_SEL_SET (Val, PcieIndex * 2);
      }

      Val = ECCR_GROUP_EVENT_SEL_SET (Val, ErrCtrlCfg[ErrCode]);
      MmioWrite32 (RasDesCapabilityBase + EVENT_COUNTER_CONTROL_REG, Val);

      // After setting Counter Control reg
      // This delay just to make sure Counter Data reg is update with new value
      MicroSecondDelay (1);
      Val = MmioRead32 (RasDesCapabilityBase + EVENT_COUNTER_DATA_REG);
      if (Val != 0) {
        Ret = LINK_CHECK_FAILED;
        DEBUG ((
          DEBUG_ERROR,
          "\tSocket%d RootComplex%d RP%d \t: %d \tGROUP:%d-EVENT:%d\n",
          RootComplex->Socket,
          RootComplex->ID,
          PcieIndex,
          Val,
          ErrGrpNum,
          GET_LOW_8_BITS (ErrCtrlCfg[ErrCode])
          ));
      }
    }
  }

  return Ret;
}

/**
  Handle Predictive Failure Analysis command

  @param RootComplex           Pointer to Root Complex structure
  @param PcieIndex             PCIe controller index
  @param PFAMode               The PFA mode

  @retval                      The link status
**/
INT32
Ac01PFACommand (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN UINT8              PFAMode
  )
{
  PHYSICAL_ADDRESS  RasDesCapabilityBase;
  PHYSICAL_ADDRESS  TargetAddress;
  INT32             Ret = LINK_CHECK_SUCCESS;
  UINT32            Val;

  // Allow programming to config space
  EnableDbiAccess (RootComplex, PcieIndex, TRUE);

  // Get the RAS D.E.S. capability base address
  RasDesCapabilityBase = GetCapabilityBase (RootComplex, PcieIndex, TRUE, RAS_DES_CAPABILITY_ID);
  if (RasDesCapabilityBase == 0) {
    DEBUG ((DEBUG_INFO, "PCIE%d.%d: Cannot get RAS DES capability address\n", RootComplex->ID, PcieIndex));
    return LINK_CHECK_WRONG_PARAMETER;
  }

  TargetAddress = RasDesCapabilityBase + EVENT_COUNTER_CONTROL_REG;

  switch (PFAMode) {
    case PFA_MODE_ENABLE:
      Val = MmioRead32 (TargetAddress);
      Val = ECCR_EVENT_COUNTER_ENABLE_SET (Val, EVENT_COUNTER_ENABLE_ALL_ON);
      Val = ECCR_EVENT_COUNTER_CLEAR_SET (Val, EVENT_COUNTER_CLEAR_NO_CHANGE);
      MmioWrite32 (TargetAddress, Val);
      break;

    case PFA_MODE_CLEAR:
      Val = MmioRead32 (TargetAddress);
      Val = ECCR_EVENT_COUNTER_ENABLE_SET (Val, EVENT_COUNTER_ENABLE_NO_CHANGE);
      Val = ECCR_EVENT_COUNTER_CLEAR_SET (Val, EVENT_COUNTER_CLEAR_ALL_CLEAR);
      MmioWrite32 (TargetAddress, Val);
      break;

    case PFA_MODE_READ:
      Ret = PFACounterRead (RootComplex, PcieIndex, RasDesCapabilityBase);
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid PFA mode\n"));
  }

  // Disable programming to config space
  EnableDbiAccess (RootComplex, PcieIndex, FALSE);

  return Ret;
}

BOOLEAN
EndpointCfgReady (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex,
  IN UINT32             TimeOut
  )
{
  PHYSICAL_ADDRESS  CfgBase;
  UINT32            Val;

  CfgBase = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << BUS_SHIFT);

  // Loop read CfgBase value until got valid value or
  // reach to Timeout (or more depend on card)
  do {
    Val = MmioRead32 (CfgBase);
    if ((Val != 0xFFFF0001) && (Val != 0xFFFFFFFF)) {
      return TRUE;
    }

    TimeOut -= LINK_WAIT_INTERVAL_US;
    MicroSecondDelay (LINK_WAIT_INTERVAL_US);
  } while (TimeOut > 0);

  return FALSE;
}

/**
   Get link capabilities link width and speed of endpoint

   @param RootComplex[in]  Pointer to AC01_ROOT_COMPLEX structure
   @param PcieIndex[in]    PCIe controller index
   @param EpMaxWidth[out]  EP max link width
   @param EpMaxGen[out]    EP max link speed
**/
VOID
Ac01PcieCoreGetEndpointInfo (
  IN  AC01_ROOT_COMPLEX  *RootComplex,
  IN  UINT8              PcieIndex,
  OUT UINT8              *EpMaxWidth,
  OUT UINT8              *EpMaxGen
  )
{
  PHYSICAL_ADDRESS  CfgBase;
  PHYSICAL_ADDRESS  EpCfgAddr;
  PHYSICAL_ADDRESS  PcieCapBase;
  PHYSICAL_ADDRESS  SecLatTimerAddr;
  PHYSICAL_ADDRESS  TargetAddress;
  UINT32            RestoreVal;
  UINT32            Val;

  CfgBase         = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << DEV_SHIFT);
  SecLatTimerAddr = CfgBase + SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG;

  *EpMaxWidth = 0;
  *EpMaxGen   = 0;

  // Allow programming to config space
  EnableDbiAccess (RootComplex, PcieIndex, TRUE);

  Val        = MmioRead32 (SecLatTimerAddr);
  RestoreVal = Val;
  Val        = SUB_BUS_SET (Val, DEFAULT_SUB_BUS);
  Val        = SEC_BUS_SET (Val, RootComplex->Pcie[PcieIndex].DevNum);
  Val        = PRIM_BUS_SET (Val, DEFAULT_PRIM_BUS);
  MmioWrite32 (SecLatTimerAddr, Val);
  EpCfgAddr = RootComplex->MmcfgBase + (RootComplex->Pcie[PcieIndex].DevNum << BUS_SHIFT);

  if (!EndpointCfgReady (RootComplex, PcieIndex, EP_LINKUP_EXTRA_TIMEOUT)) {
    goto Exit;
  }

  Val = MmioRead32 (EpCfgAddr);
  // Check whether EP config space is accessible or not
  if (Val == 0xFFFFFFFF) {
    *EpMaxWidth = 0;   // Invalid Width
    *EpMaxGen   = 0;   // Invalid Speed
    DEBUG ((DEBUG_ERROR, "PCIE%d.%d Cannot access EP config space!\n", RootComplex->ID, PcieIndex));
  } else if (Val == 0xFFFF0001) {
    *EpMaxWidth = 0;   // Invalid Width
    *EpMaxGen   = 0;   // Invalid Speed
    DEBUG ((DEBUG_ERROR, "PCIE%d.%d EP config space still not ready to access, need poll more time!!!\n", RootComplex->ID, PcieIndex));
  } else {
    PcieCapBase = GetCapabilityBase (RootComplex, PcieIndex, FALSE, PCIE_CAPABILITY_ID);
    if (PcieCapBase == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "PCIE%d.%d Cannot get PCIe capability base address!\n",
        RootComplex->ID,
        PcieIndex
        ));
    } else {
      Val         = MmioRead32 (PcieCapBase + LINK_CAPABILITIES_REG);
      *EpMaxWidth = CAP_MAX_LINK_WIDTH_GET (Val);
      *EpMaxGen   = CAP_MAX_LINK_SPEED_GET (Val);
      DEBUG ((
        DEBUG_INFO,
        "PCIE%d.%d EP MaxWidth %d EP MaxGen %d \n",
        RootComplex->ID,
        PcieIndex,
        *EpMaxWidth,
        *EpMaxGen
        ));

      // From EP, enabling common clock for upstream
      TargetAddress = PcieCapBase + LINK_CONTROL_LINK_STATUS_REG;
      Val           = MmioRead32 (TargetAddress);
      Val           = CAP_SLOT_CLK_CONFIG_SET (Val, 1);
      Val           = CAP_COMMON_CLK_SET (Val, 1);
      MmioWrite32 (TargetAddress, Val);
    }
  }

Exit:
  // Restore value in order to not affect enumeration process
  MmioWrite32 (SecLatTimerAddr, RestoreVal);

  // Disable programming to config space
  EnableDbiAccess (RootComplex, PcieIndex, FALSE);
}

VOID
PollLinkUp (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex
  )
{
  UINT32  TimeOut;

  // Poll until link up
  // This checking for linkup status and
  // give LTSSM state the time to transit from DECTECT state to L0 state
  // Total delay are 100ms, smaller number of delay cannot always make sure
  // the state transition is completed
  TimeOut = LTSSM_TRANSITION_TIMEOUT;
  do {
    if (PcieLinkUpCheck (&RootComplex->Pcie[PcieIndex])) {
      DEBUG ((
        DEBUG_INFO,
        "\tPCIE%d.%d LinkStat is correct after soft reset, transition time: %d\n",
        RootComplex->ID,
        PcieIndex,
        TimeOut
        ));
      RootComplex->Pcie[PcieIndex].LinkUp = TRUE;
      break;
    }

    MicroSecondDelay (100);
    TimeOut -= 100;
  } while (TimeOut > 0);

  if (TimeOut <= 0) {
    DEBUG ((DEBUG_ERROR, "\tPCIE%d.%d LinkStat TIMEOUT after re-init\n", RootComplex->ID, PcieIndex));
  } else {
    DEBUG ((DEBUG_INFO, "PCIE%d.%d Link re-initialization passed!\n", RootComplex->ID, PcieIndex));
  }
}

/**
  Check active PCIe controllers of RootComplex, retrain or soft reset if needed

  @param RootComplex[in]  Pointer to AC01_ROOT_COMPLEX structure
  @param PcieIndex[in]    PCIe controller index

  @retval                 -1: Link recovery had failed
                          1: Link width and speed are not correct
                          0: Link recovery succeed
**/
INT32
Ac01PcieCoreQoSLinkCheckRecovery (
  IN AC01_ROOT_COMPLEX  *RootComplex,
  IN UINT8              PcieIndex
  )
{
  INT32  LinkStatusCheck, RasdesChecking;
  INT32  NumberOfReset = MAX_REINIT;
  UINT8  EpMaxWidth, EpMaxGen;

  // PCIe controller is not active or Link is not up
  // Nothing to be done
  if ((!RootComplex->Pcie[PcieIndex].Active) || (!RootComplex->Pcie[PcieIndex].LinkUp)) {
    return LINK_CHECK_WRONG_PARAMETER;
  }

  do {
    if (RootComplex->Pcie[PcieIndex].LinkUp) {
      // Enable all of RASDES register to detect any training error
      Ac01PFACommand (RootComplex, PcieIndex, PFA_MODE_ENABLE);

      // Accessing Endpoint and checking current link capabilities
      Ac01PcieCoreGetEndpointInfo (RootComplex, PcieIndex, &EpMaxWidth, &EpMaxGen);
      LinkStatusCheck = Ac01PcieCoreLinkCheck (RootComplex, PcieIndex, EpMaxWidth, EpMaxGen);

      // Delay to allow the link to perform internal operation and generate
      // any error status update. This allows detection of any error observed
      // during initial link training. Possible evaluation time can be
      // between 100ms to 200ms.
      MicroSecondDelay (100000);

      // Check for error
      RasdesChecking = Ac01PFACommand (RootComplex, PcieIndex, PFA_MODE_READ);

      // Clear error counter
      Ac01PFACommand (RootComplex, PcieIndex, PFA_MODE_CLEAR);

      // If link check functions return passed, then breaking out
      // else go to soft reset
      if ((LinkStatusCheck != LINK_CHECK_FAILED) &&
          (RasdesChecking != LINK_CHECK_FAILED) &&
          PcieLinkUpCheck (&RootComplex->Pcie[PcieIndex]))
      {
        return LINK_CHECK_SUCCESS;
      }

      RootComplex->Pcie[PcieIndex].LinkUp = FALSE;
    }

    // Trigger controller soft reset
    DEBUG ((DEBUG_INFO, "PCIE%d.%d Start link re-initialization..\n", RootComplex->ID, PcieIndex));
    Ac01PcieCoreSetupRC (RootComplex, TRUE, PcieIndex);

    PollLinkUp (RootComplex, PcieIndex);

    NumberOfReset--;
  } while (NumberOfReset > 0);

  return LINK_CHECK_SUCCESS;
}

BOOLEAN
Ac01PcieCoreCheckCardPresent (
  IN AC01_PCIE_CONTROLLER  *PcieController
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  UINT32                ControlValue;

  ControlValue = 0;

  TargetAddress = PcieController->CsrBase;

  ControlValue = MmioRead32 (TargetAddress + AC01_PCIE_CORE_LINK_CTRL_REG);

  if (0 == LTSSMENB_GET (ControlValue)) {
    //
    // LTSSMENB is clear to 0x00 by Hardware -> link partner is connected.
    //
    return TRUE;
  }

  return FALSE;
}

VOID
Ac01PcieCoreUpdateLink (
  IN  AC01_ROOT_COMPLEX  *RootComplex,
  OUT BOOLEAN            *IsNextRoundNeeded,
  OUT INT8               *FailedPciePtr,
  OUT INT8               *FailedPcieCount
  )
{
  AC01_PCIE_CONTROLLER  *Pcie;
  UINT8                 PcieIndex;
  UINT32                Index;

  *IsNextRoundNeeded = FALSE;
  *FailedPcieCount   = 0;
  for (Index = 0; Index < MaxPcieControllerOfRootComplexB; Index++) {
    FailedPciePtr[Index] = -1;
  }

  if (!RootComplex->Active) {
    return;
  }

  // Loop for all controllers
  for (PcieIndex = 0; PcieIndex < RootComplex->MaxPcieController; PcieIndex++) {
    Pcie = &RootComplex->Pcie[PcieIndex];

    if (Pcie->Active && !Pcie->LinkUp) {
      if (PcieLinkUpCheck (Pcie)) {
        Pcie->LinkUp = TRUE;

        // Doing link checking and recovery if needed
        Ac01PcieCoreQoSLinkCheckRecovery (RootComplex, PcieIndex);

        // Un-mask Completion Timeout
        DisableCompletionTimeOut (RootComplex, PcieIndex, FALSE);
      } else {
        FailedPciePtr[*FailedPcieCount] = PcieIndex;
        *FailedPcieCount               += 1;

        if (Ac01PcieCoreCheckCardPresent (Pcie)) {
          *IsNextRoundNeeded = TRUE;
          DEBUG ((DEBUG_INFO, "PCIE%d.%d Link retry\n", RootComplex->ID, PcieIndex));
        }
      }
    }
  }
}

/**
  Verify the link status and retry to initialize the Root Complex if there's any issue.

  @param RootComplexList      Pointer to the Root Complex list
**/
VOID
Ac01PcieCorePostSetupRC (
  IN AC01_ROOT_COMPLEX  *RootComplexList
  )
{
  UINT8    RCIndex, Idx;
  BOOLEAN  IsNextRoundNeeded, NextRoundNeeded;
  UINT64   PrevTick, CurrTick, ElapsedCycle;
  UINT64   TimerTicks64;
  UINT8    ReInit;
  INT8     FailedPciePtr[MaxPcieControllerOfRootComplexB];
  INT8     FailedPcieCount;

  ReInit = 0;

_link_polling:
  NextRoundNeeded = FALSE;
  //
  // It is not guaranteed the timer service is ready prior to PCI Dxe.
  // Calculate system ticks for link training.
  //
  TimerTicks64 = ArmGenericTimerGetTimerFreq (); /* 1 Second */
  PrevTick     = ArmGenericTimerGetSystemCount ();
  ElapsedCycle = 0;

  do {
    CurrTick = ArmGenericTimerGetSystemCount ();
    if (CurrTick < PrevTick) {
      ElapsedCycle += MAX_UINT64 - PrevTick;
      PrevTick      = 0;
    }

    ElapsedCycle += (CurrTick - PrevTick);
    PrevTick      = CurrTick;
  } while (ElapsedCycle < TimerTicks64);

  for (RCIndex = 0; RCIndex < AC01_PCIE_MAX_ROOT_COMPLEX; RCIndex++) {
    Ac01PcieCoreUpdateLink (&RootComplexList[RCIndex], &IsNextRoundNeeded, FailedPciePtr, &FailedPcieCount);
    if (IsNextRoundNeeded) {
      NextRoundNeeded = TRUE;
    }
  }

  if (NextRoundNeeded && (ReInit < MAX_REINIT)) {
    //
    // Timer is up. Give another chance to re-program controller
    //
    ReInit++;
    for (RCIndex = 0; RCIndex < AC01_PCIE_MAX_ROOT_COMPLEX; RCIndex++) {
      Ac01PcieCoreUpdateLink (&RootComplexList[RCIndex], &IsNextRoundNeeded, FailedPciePtr, &FailedPcieCount);
      if (IsNextRoundNeeded) {
        for (Idx = 0; Idx < FailedPcieCount; Idx++) {
          if (FailedPciePtr[Idx] == -1) {
            continue;
          }

          //
          // Some controller still observes link-down. Re-init controller
          //
          Ac01PcieCoreSetupRC (&RootComplexList[RCIndex], TRUE, FailedPciePtr[Idx]);
        }
      }
    }

    goto _link_polling;
  }
}
