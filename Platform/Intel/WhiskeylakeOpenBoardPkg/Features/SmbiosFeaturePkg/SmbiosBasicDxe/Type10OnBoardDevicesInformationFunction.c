/** @file
  Smbios type 10.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This structure is obsolete; the OnBoard Devices Extended 
  Information (Type 41) structure should be used instead. (Type 10).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
OnBoardDevicesInformationFunction(
	IN  EFI_SMBIOS_PROTOCOL*            Smbios
)
{
  EFI_STATUS                          Status;
  CHAR8                               *DescriptionStr;
  UINTN                               DescriptionStrLen;
  SMBIOS_TABLE_TYPE10                 *SmbiosRecord;
  SMBIOS_TABLE_TYPE10                 *PcdSmbiosRecord;
  EFI_SMBIOS_HANDLE                   SmbiosHandle;
  UINTN                               SourceSize;
  UINTN                               TotalSize;
  UINTN                               StringOffset;

  PcdSmbiosRecord = PcdGetPtr(PcdSmbiosType10OnBoardDevicesInformation);

  //
  // Get Designation String.
  //
  DescriptionStr = PcdGetPtr (PcdSmbiosType10StringDescriptionString);
  DescriptionStrLen = AsciiStrLen (DescriptionStr);
  ASSERT (DescriptionStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Allocate space for the record
  //
  SourceSize = PcdGetSize (PcdSmbiosType9SystemSlots);
  TotalSize = SourceSize + DescriptionStrLen + 1 + 1;
  SmbiosRecord = AllocateZeroPool(TotalSize);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the data to the table
  //
  CopyMem(SmbiosRecord, PcdSmbiosRecord, SourceSize);
  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE10);
  SmbiosRecord->Hdr.Handle = 0;
  StringOffset = SmbiosRecord->Hdr.Length;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, DescriptionStr, DescriptionStrLen);
  
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = AddSmbiosRecord(Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*)SmbiosRecord);
  
  FreePool(SmbiosRecord);
  return Status;
}