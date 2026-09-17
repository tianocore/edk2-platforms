/** @file
  Smbios type 40.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  Additional Information (Type 40).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
AdditionalInformationFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
  EFI_STATUS                  Status;  
  CHAR8                       *EntryStr;
  UINTN                       EntryStrLen; 
  SMBIOS_TABLE_TYPE40         *PcdSmbiosRecord;
  SMBIOS_TABLE_TYPE40         *SmbiosRecord;
  EFI_SMBIOS_HANDLE           SmbiosHandle;
  UINTN                       SourceSize;
  UINTN                       TotalSize;
  UINTN                       StringOffset;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType40AdditionalInformation);
  
  //
  // Get Slot Designation String.
  //
  EntryStr = PcdGetPtr (PcdSmbiosType40StringEntryString);
  EntryStrLen = AsciiStrLen (EntryStr);
  ASSERT (EntryStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Allocate space for the record
  //
  SourceSize = PcdGetSize (PcdSmbiosType40AdditionalInformation);
  TotalSize = SourceSize + EntryStrLen + 1 + 1;
  SmbiosRecord = AllocateZeroPool(TotalSize);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (SmbiosRecord, PcdSmbiosRecord, SourceSize);

  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_ADDITIONAL_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE40);
  SmbiosRecord->Hdr.Handle = 0;

  StringOffset = SmbiosRecord->Hdr.Length;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, EntryStr, EntryStrLen);

  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool (SmbiosRecord);
  return Status;
} 