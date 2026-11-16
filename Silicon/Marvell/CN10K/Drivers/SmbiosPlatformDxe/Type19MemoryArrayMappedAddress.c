/** @file
  SMBIOS Type 19 (Memory Array Mapped Address) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE19  mSmbiosType19Template = {
  { EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, sizeof (SMBIOS_TABLE_TYPE19), 0 },
  0xffffffff, // StartingAddress;
  0xffffffff, // EndingAddress;
  0,          // MemoryArrayHandle;
  1,          // PartitionWidth;
  0,          // ExtendedStartingAddress;
  0,          // ExtendedEndingAddress;
};

/**
  Install the SMBIOS Type 19 (Memory Array Mapped Address) record.

  Maps the whole memory array from the RAM start address through
  RamStart + RamSize - 1, links it to Context->PmaHandle, and stores the
  assigned handle in Context->MamaHandle for the Type 20 records.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with the RAM range and memory array
                           handle; receives the mapped-address handle.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType19MemoryArrayMappedAddress (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  mSmbiosType19Template.MemoryArrayHandle       = Context->PmaHandle;
  mSmbiosType19Template.ExtendedStartingAddress = Context->NodeConfig->RamStart;
  mSmbiosType19Template.ExtendedEndingAddress   =
    Context->NodeConfig->RamStart + Context->NodeConfig->RamSize - 1;

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType19Template,
           NULL,
           &Context->MamaHandle
           );
}
