/** @file
  SMBIOS Type 20 (Memory Device Mapped Address) tables for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE20  mSmbiosType20Template = {
  { EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, sizeof (SMBIOS_TABLE_TYPE20), 0 },
  0xffffffff, // StartingAddress
  0xffffffff, // EndingAddress
  0,          // MemoryDeviceHandle
  0,          // MemoryArrayMappedAddressHandle,
  0xff,       // PartitionRowPosition;
  0xff,       // InterleavePosition;
  0xff,       // InterleavedDataDepth;
  0,          // ExtendedStartingAddress;
  0,          // ExtendedEndingAddress;
};

/**
  Install the SMBIOS Type 20 (Memory Device Mapped Address) records.

  Walks the populated DIMMs in order, assigns each one a contiguous address
  range starting at the RAM base, and links it to the corresponding Type 17
  device handle and the Type 19 mapped-address handle.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with RAM base, DIMM configuration,
                           DIMM handles, and mapped-address handle.

  @retval  EFI_SUCCESS  All records were installed.
  @retval  Others       A record could not be added.
**/
EFI_STATUS
InstallType20MemoryDeviceMappedAddress (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dss;
  UINT64      StartAddress;
  UINT64      SizeInBytes;

  StartAddress = Context->NodeConfig->RamStart;

  for (Dss = 0; Dss < FixedPcdGet8 (PcdDssPerNode); Dss++) {
    if (Context->NodeConfig->DimmCfg[Dss].Status != DimmStatusAvailable) {
      continue;
    }

    SizeInBytes = (UINT64)Context->NodeConfig->DimmCfg[Dss].SizeMb * SIZE_1MB;

    mSmbiosType20Template.MemoryDeviceHandle             = Context->DimmHandles[Dss];
    mSmbiosType20Template.MemoryArrayMappedAddressHandle = Context->MamaHandle;
    mSmbiosType20Template.ExtendedStartingAddress        = StartAddress;
    mSmbiosType20Template.ExtendedEndingAddress          = StartAddress + SizeInBytes - 1;

    Status = SmbiosPlatformAddEntry (
               Smbios,
               (SMBIOS_STRUCTURE *)&mSmbiosType20Template,
               NULL,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Prepare for next table 20: next start is at where previous area ends + 1
    StartAddress = mSmbiosType20Template.ExtendedEndingAddress + 1;
  }

  return EFI_SUCCESS;
}
