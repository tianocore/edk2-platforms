/** @file
  SMBIOS Specific driver for Type32.

  Copyright (C) 2017 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (C) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosCommon.h"

/**
  This function makes boot time changes to the contents of the
  BootInformation (Type 32).

  @param[in]  Smbios                 The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
BootInfoStatusFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE32  *PcdSmbiosRecord;
  SMBIOS_TABLE_TYPE32  *SmbiosRecord;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType32SystemBootInformation);

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE32) + 1 + 1);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof (SMBIOS_TABLE_TYPE32));

  SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE32);
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = AddCommonSmbiosRecord (
             Smbios,
             &SmbiosHandle,
             (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
             );

  FreePool (SmbiosRecord);
  return Status;
}
