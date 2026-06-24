/** @file
  Smbios type 13.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  BiosLanguageFunction (Type 13).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
BiosLanguageFunction(
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
  )
{
  EFI_STATUS                          Status;
  CHAR8                               *CurrentLanguagesStr;
  UINTN                               CurrentLanguagesStrLen;
  SMBIOS_TABLE_TYPE9                  *SmbiosRecord;
  SMBIOS_TABLE_TYPE9                  *PcdSmbiosRecord;
  EFI_SMBIOS_HANDLE                   SmbiosHandle;
  UINTN                               SourceSize;
  UINTN                               TotalSize;
  UINTN                               StringOffset;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType13BiosLanguageInformation);

  //
  // Get Current Languages String.
  //
  CurrentLanguagesStr = PcdGetPtr (PcdSmbiosType13StringCurrentLanguages);
  CurrentLanguagesStrLen = AsciiStrLen (CurrentLanguagesStr);
  ASSERT (CurrentLanguagesStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Allocate space for the record
  //
  SourceSize = PcdGetSize (PcdSmbiosType13BiosLanguageInformation);
  TotalSize = SourceSize + CurrentLanguagesStrLen + 1 + 1;
  SmbiosRecord = AllocateZeroPool(TotalSize);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the data to the table
  //
  CopyMem (SmbiosRecord, PcdSmbiosRecord, SourceSize);

  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE13);
  SmbiosRecord->Hdr.Handle = 0;

  StringOffset = SmbiosRecord->Hdr.Length;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, CurrentLanguagesStr, CurrentLanguagesStrLen);
  
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool (SmbiosRecord);
  return Status;
}