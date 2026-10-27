/** @file
  SMBIOS Type 16 (Physical Memory Array) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE16  mSmbiosType16Template = {
  { EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, sizeof (SMBIOS_TABLE_TYPE16), SMBIOS_HANDLE_PI_RESERVED },
  MemoryArrayLocationSystemBoard,    // Location; Value from MEMORY_ARRAY_LOCATION.
  MemoryArrayUseSystemMemory,        // Use; Value from MEMORY_ARRAY_USE.
  MemoryErrorCorrectionSingleBitEcc, // MemoryErrorCorrection; Value from MEMORY_ERROR_CORRECTION
  0x80000000,                        // MaximumCapacity;
  0xFFFE,                            // MemoryErrorInformationHandle;
  CN10K_SMBIOS_MAX_DIMMS,            // NumberOfMemoryDevices; one per DSS
  0,                                 // ExtendedMaximumCapacity;
};

/**
  Install the SMBIOS Type 16 (Physical Memory Array) record.

  Encodes the total RAM size in MaximumCapacity when it fits below 2 TB, or in
  ExtendedMaximumCapacity otherwise, and stores the assigned handle in
  Context->PmaHandle for the Type 17 and Type 19 records.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with the RAM size; receives the
                           memory array handle.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType16PhysicalMemoryArray (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  // Values 2 TB (8000 0000h) or greater must be represented
  // in the Extended Maximum Capacity field.
  if (Context->NodeConfig->RamSize / 1024 < 0x80000000) {
    // RamSize in bytes, recalculate to KB
    mSmbiosType16Template.MaximumCapacity = (UINT32)(Context->NodeConfig->RamSize / 1024);
  } else {
    mSmbiosType16Template.ExtendedMaximumCapacity = Context->NodeConfig->RamSize; // In bytes!
  }

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType16Template,
           NULL,
           &Context->PmaHandle
           );
}
