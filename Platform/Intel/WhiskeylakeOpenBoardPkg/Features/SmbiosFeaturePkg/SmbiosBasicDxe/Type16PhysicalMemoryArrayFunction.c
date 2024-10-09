/** @file
  Smbios type 16.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  Physical MemoryArray (Type 16).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
PhysicalMemoryArrayFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
  EFI_STATUS                  Status;
  SMBIOS_TABLE_TYPE16         *PcdSmbiosRecord;
  SMBIOS_TABLE_TYPE16         *SmbiosRecord;
  EFI_SMBIOS_HANDLE           SmbiosHandle;
  UINTN                       TableSize;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType16PhysicalMemoryArray);
  
  //
  // Create table size based on string lengths
  //
  TableSize = sizeof (SMBIOS_TABLE_TYPE16) + 1 + 1;
  SmbiosRecord = AllocateZeroPool (TableSize);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in type 16 fields
  //
  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE16);
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Full smbios record, call smbios protocol to add this record.
  //
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
  FreePool (SmbiosRecord);
  return Status;
}