/** @file
  SMBIOS Type 7 (Cache Information) tables for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L1DTemplate = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), 0 },
  1,                                        // SocketDesignation
  0x0180,                                   // CacheConfiguration
  { 0,                                 0 }, // MaximumCacheSize
  { 0,                                 0 }, // InstalledSize
  {    // Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  {    // Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  0x00,                           // CacheSpeed
  CacheErrorSingleBit,            // Single-bit ECC
  CacheTypeData,                  // SystemCacheType
  CacheAssociativity4Way          // 4-way Set-Associative
};

STATIC CHAR8  *mSmbiosType7L1DStrings[] = {
  "Internal L1D Cache",
  NULL
};

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L1ITemplate = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), 0 },
  1,                                        // SocketDesignation
  0x0180,                                   // CacheConfiguration
  { 0,                                 0 }, // MaximumCacheSize
  { 0,                                 0 }, // InstalledSize
  {    // Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  {    // Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  0x00,                           // CacheSpeed
  CacheErrorParity,               // ErrorCorrectionType = Parity
  CacheTypeInstruction,           // SystemCacheType
  CacheAssociativity4Way          // Associativity (4-way Set-Associative)
};

STATIC CHAR8  *mSmbiosType7L1IStrings[] = {
  "Internal L1I Cache",
  NULL
};

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L2Template = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), 0 },
  1,                                        // SocketDesignation
  0x0181,                                   // CacheConfiguration
  { 0,                                 0 }, // MaximumCacheSize
  { 0,                                 0 }, // InstalledSize
  {    // SupportedSRAMType = Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  {    // CurrentSRAMType = Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  0x00,                           // CacheSpeed
  CacheErrorSingleBit,            // ErrorCorrectionType = Single-bit ECC
  CacheTypeUnified,               // SystemCacheType
  CacheAssociativity8Way          // Associativity (8-way Set-Associative)
};

STATIC CHAR8  *mSmbiosType7L2Strings[] = {
  "Internal L2 Unified Cache",
  NULL
};

STATIC SMBIOS_TABLE_TYPE7  mSmbiosType7L3Template = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), 0 },
  1,                                        // SocketDesignation
  0x0182,                                   // CacheConfiguration
  { 0,                                 0 }, // MaximumCacheSize
  { 0,                                 0 }, // InstalledSize
  {    // SupportedSRAMType = Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  {    // CurrentSRAMType = Burst, Synchronous
    0, // Other         :1;
    0, // Unknown       :1;
    0, // NonBurst      :1;
    1, // Burst         :1;
    0, // PipelineBurst :1;
    1, // Synchronous   :1;
    0, // Asynchronous  :1;
    0  // Reserved      :9;
  },
  0x00,                           // CacheSpeed
  CacheErrorSingleBit,            // ErrorCorrectionType = Single-bit ECC
  CacheTypeUnified,               // SystemCacheType
  CacheAssociativity16Way         // Associativity (16-way Set-Associative)
};

STATIC CHAR8  *mSmbiosType7L3Strings[] = {
  "Last Level Cache",
  NULL
};

/**
  Install the SMBIOS Type 7 (Cache Information) records.

  For the supported CN10K variants, fills the L1 data, L1 instruction, L2, and
  L3 cache sizes. The L3 size is read from the device tree, falling back to
  the HRM value for the detected part. Each assigned handle is stored in the
  driver context for the Type 4 record.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with CPU type; receives cache
                           handles.

  @retval  EFI_SUCCESS  All cache records were installed.
  @retval  Others       A record could not be added.
**/
EFI_STATUS
InstallType7CacheInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;

  if (CpuTypeIsSupported (Context->NodeConfig->CPUType)) {
    // The 16-bit fields use 1 KiB units where possible, otherwise 64 KiB units.
    SmbiosSetCacheSize (&mSmbiosType7L1DTemplate, 64 * SIZE_1KB);   // L1D: HRM -> "L1 Data Cache"
    SmbiosSetCacheSize (&mSmbiosType7L1ITemplate, 64 * SIZE_1KB);   // L1I: HRM -> "L1 Instruction Cache"
    SmbiosSetCacheSize (&mSmbiosType7L2Template, 1024 * SIZE_1KB);  // L2D

    if (EFI_ERROR (GetLlcSizeOutOfDeviceTree (&Context->LlcSizeInBytes))) {
      // Fallback: when LLC (L3) size is not defined in DT, just use values according to HRM.
      if (Context->NodeConfig->CPUType == Cn10kaProductId) {
        Context->LlcSizeInBytes = 48 * SIZE_1MB;  // CN10KA
      } else if (Context->NodeConfig->CPUType == Cn10kbProductId) {
        Context->LlcSizeInBytes = 16 * SIZE_1MB;  // CN10KB
      } else {
        Context->LlcSizeInBytes = 36 * SIZE_1MB;  // CNF10KA and CNF10KB
      }
    }

    SmbiosSetCacheSize (&mSmbiosType7L3Template, Context->LlcSizeInBytes);
  }

  Status = SmbiosPlatformAddEntry (
             Smbios,
             (SMBIOS_STRUCTURE *)&mSmbiosType7L1DTemplate,
             mSmbiosType7L1DStrings,
             &Context->L1DHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SmbiosPlatformAddEntry (
             Smbios,
             (SMBIOS_STRUCTURE *)&mSmbiosType7L1ITemplate,
             mSmbiosType7L1IStrings,
             &Context->L1IHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SmbiosPlatformAddEntry (
             Smbios,
             (SMBIOS_STRUCTURE *)&mSmbiosType7L2Template,
             mSmbiosType7L2Strings,
             &Context->L2Handle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType7L3Template,
           mSmbiosType7L3Strings,
           &Context->L3Handle
           );
}
