/** @file
  Smbios type 42.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  ManagementControllerHostInterface (Type 42).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
ManagementControllerHostInterfaceFunction(
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
  )
{
  EFI_STATUS                          Status;
  SMBIOS_TABLE_TYPE42                 *SmbiosRecord;
  SMBIOS_TABLE_TYPE42                 *PcdSmbiosRecord;
  EFI_SMBIOS_HANDLE                   SmbiosHandle;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType42ManagementControllerHostInterface);

  SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE42) + 1 + 1);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE42));

  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE42);
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool (SmbiosRecord);
  return Status;
}