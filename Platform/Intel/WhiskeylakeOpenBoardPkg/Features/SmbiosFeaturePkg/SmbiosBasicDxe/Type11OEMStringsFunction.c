/** @file
  Smbios type 11.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  free-form strings defined by OEM (Type 11).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
OEMStringsFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    CHAR8                       *StringCountStr;
    UINTN                       StringCountStrLen;    
    EFI_STATUS                  Status;
    SMBIOS_TABLE_TYPE11         *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE11         *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;

    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType11OEMStrings);

    //
    // Get board string count string
    //
    StringCountStr = PcdGetPtr (PcdSmbiosType11StringCount);
    StringCountStrLen = AsciiStrLen (StringCountStr);
    ASSERT (StringCountStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE11) + StringCountStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE11));

    //
    // Fill in type 11 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_OEM_STRINGS;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE11);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, StringCountStr, StringCountStrLen);
    StringOffset += StringCountStrLen + 1;

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}