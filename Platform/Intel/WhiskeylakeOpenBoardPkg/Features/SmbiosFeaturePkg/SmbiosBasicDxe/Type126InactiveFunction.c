/** @file
  Smbios type 126.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of
  Inactive (Type 126).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
InactiveFunction(
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
  )
{
  EFI_STATUS                          Status;
  SMBIOS_TABLE_TYPE126                *SmbiosRecord;
  SMBIOS_TABLE_TYPE126                *PcdSmbiosRecord;
  EFI_SMBIOS_HANDLE                   SmbiosHandle;
  UINTN                               TableSize;   
  
  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType126Inactive);

  //
  // Create table size based on string lengths
  //
  TableSize = sizeof (SMBIOS_TABLE_TYPE126) + 1 + 1;
  SmbiosRecord = AllocateZeroPool (TableSize);
  if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
  }  

  //
  // Fill in type 126 fields
  //
  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_INACTIVE;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE126);
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Full smbios record, call smbios protocol to add this record.
  //
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
  FreePool (SmbiosRecord);
  return Status;
}        