/** @file
  SMBIOS Type 4 (Processor Information) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE4  mSmbiosType4Template = {
  { EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE4), 0 },
  1,                               // Socket String
  CentralProcessor,                // ProcessorType; Value from PROCESSOR_TYPE_DATA.
  ProcessorFamilyIndicatorFamily2, // ProcessorFamily; Value from PROCESSOR_FAMILY_DATA.
  2,                               // ProcessorManufacture String;
  // On AArch64 PROCESSOR_ID_DATA holds the Arm64 SoC ID fields.
  { 0,                                     0,                           0, 0}, // ProcessorId; SmbiosGetProcessorId()
  3,                                                                         // ProcessorVersion String;
  {     // Voltage;
    0,  // ProcessorVoltageCapability5V        :1;
    0,  // ProcessorVoltageCapability3_3V      :1;
    0,  // ProcessorVoltageCapability2_9V      :1;
    0,  // ProcessorVoltageCapabilityReserved  :1; ///< Bit 3, must be zero.
    0,  // ProcessorVoltageReserved            :3; ///< Bits 4-6, must be zero.
    0   // ProcessorVoltageIndicateLegacy      :1;
  },
  0,                      // ExternalClock;
  2500,                   // MaxSpeed; // 2500MHz
  0,                      // CurrentSpeed;
  0x41,                   // Status;
  ProcessorUpgradeNone,   // ProcessorUpgrade; Value from PROCESSOR_UPGRADE.
  0,                      // L1CacheHandle;
  0,                      // L2CacheHandle;
  0,                      // L3CacheHandle;
  4,                      // SerialNumber;
  5,                      // AssetTag;
  6,                      // PartNumber;
  0,                      // CoreCount;
  0,                      // EnabledCoreCount;
  0,                      // ThreadCount;
  0x00DC,                 // 64-bit, Multi-Core, Execute Protection, Enh Virtual, Power/Perf Ctrl
  ProcessorFamilyARMv9,   // ProcessorFamily2;
};

STATIC CHAR8  *mSmbiosType4Strings[] = {
  "DPU",
  "Marvell",
  "0.0",
  "Not Specified",
  " ",
  "Not Specified",
  NULL
};

/**
  Install the SMBIOS Type 4 (Processor Information) record.

  Sets the current speed from the configured maximum core clock, fills the
  version, part number, and serial number strings, and populates the core and
  thread counts for the supported CN10K variants. The L1/L2/L3 cache handles
  are taken from the cache records installed earlier.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with CPU data and cache handles.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType4ProcessorInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  UINT8  ProcessorVoltage;

  // VDD_CORE initial = 0.75V = ~0.8V -> 88h: legacy encoding, 0.8V
  ProcessorVoltage = 0x88;

  // CORECLOCK-FREQ-MAX or default 2500Mhz
  mSmbiosType4Template.CurrentSpeed = (Context->NodeConfig->CPUSpeed) ? (UINT16)Context->NodeConfig->CPUSpeed : 2500;
  CopyMem (&mSmbiosType4Template.Voltage, &ProcessorVoltage, sizeof (mSmbiosType4Template.Voltage));

  SMBIOS_SET_STRING (
    mSmbiosType4Strings,
    &mSmbiosType4Template,
    ProcessorVersion,
    Context->NodeConfig->CPUVersion
    );

  if (Context->NodeConfig->CPUPartNumber[0]) {
    SMBIOS_SET_STRING (
      mSmbiosType4Strings,
      &mSmbiosType4Template,
      PartNumber,
      Context->NodeConfig->CPUPartNumber
      );
  }

  if (Context->NodeConfig->CPUSerial[0]) {
    SMBIOS_SET_STRING (
      mSmbiosType4Strings,
      &mSmbiosType4Template,
      SerialNumber,
      Context->NodeConfig->CPUSerial
      );
  }

  if (CpuTypeIsSupported (Context->NodeConfig->CPUType)) {
    // For core counts of 256 or greater,
    // the Core Count field is set to FFh and the Core Count 2 field is set to number of cores.
    mSmbiosType4Template.CoreCount  = (PcdGet32 (PcdCoresPerNode) >= 256) ? 0xFF : PcdGet32 (PcdCoresPerNode);
    mSmbiosType4Template.CoreCount2 = PcdGet32 (PcdCoresPerNode);

    // Assume there is no way to disable cores (e.g. via some configuration).
    // In such case enabled cores = all available cores.
    mSmbiosType4Template.EnabledCoreCount  = mSmbiosType4Template.CoreCount;
    mSmbiosType4Template.EnabledCoreCount2 = mSmbiosType4Template.CoreCount2;

    // Single thread cores
    mSmbiosType4Template.ThreadCount  = mSmbiosType4Template.CoreCount;
    mSmbiosType4Template.ThreadCount2 = mSmbiosType4Template.ThreadCount;
  }

  mSmbiosType4Template.L1CacheHandle = Context->L1DHandle;
  mSmbiosType4Template.L2CacheHandle = Context->L2Handle;
  mSmbiosType4Template.L3CacheHandle = Context->L3Handle;

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType4Template,
           mSmbiosType4Strings,
           NULL
           );
}
