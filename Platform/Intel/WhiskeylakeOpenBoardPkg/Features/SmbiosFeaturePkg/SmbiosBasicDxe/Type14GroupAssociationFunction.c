/** @file
  Smbios type 14.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  The group associations structure is provided for the OEM�s who want to specify
  the arrangement or hierarchy of certain components within the system. (Type 14).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
GroupAssociationsFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *GroupNameStr;
    UINTN                       GroupNameStrLen;       
    SMBIOS_TABLE_TYPE14          *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE14          *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;


    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType14GroupAssociations);

    //
    // Get group name string
    //
	GroupNameStr = PcdGetPtr (PcdSmbiosType14StringGroupName);
	GroupNameStrLen = AsciiStrLen (GroupNameStr);
    ASSERT (GroupNameStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE14) + GroupNameStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE14));

    //
    // Fill in type 14 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_GROUP_ASSOCIATIONS;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE14);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, GroupNameStr, GroupNameStrLen);
    

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}
