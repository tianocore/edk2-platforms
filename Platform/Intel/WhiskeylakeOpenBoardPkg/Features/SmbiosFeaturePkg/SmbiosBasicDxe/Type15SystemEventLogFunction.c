/** @file
  Smbios type 15.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  System Event Log (Type 15).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
SystemEventLogFunction(
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
  )
{
    EFI_STATUS                      Status;
    SMBIOS_TABLE_TYPE15             *SmbiosRecord;
    SMBIOS_TABLE_TYPE15             *PcdSmbiosRecord;
    EFI_SMBIOS_HANDLE               SmbiosHandle;
    UINTN                           TableSize;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType15SystemEventLog);

  //
  // Create table size based on string lengths
  //
  TableSize = sizeof (SMBIOS_TABLE_TYPE15) + 1 + 1;
  SmbiosRecord = AllocateZeroPool (TableSize);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in type 15 fields
  //
  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_SYSTEM_EVENT_LOG;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE15);
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Full smbios record, call smbios protocol to add this record.
  //
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
  FreePool (SmbiosRecord);
  return Status;
}