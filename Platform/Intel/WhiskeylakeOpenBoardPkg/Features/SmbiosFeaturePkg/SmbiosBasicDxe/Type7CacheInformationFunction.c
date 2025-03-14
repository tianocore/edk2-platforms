/** @file
  Smbios type 7.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  CPU Cache (Type 7).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
CacheInformationFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *SocketDesignationStr;       
    UINTN                       SocketDesignationStrLen;             
    UINTN                       StringOffset;
    UINTN                       TableSize;
    SMBIOS_TABLE_TYPE7          *SmbiosRecord;
    SMBIOS_TABLE_TYPE7          *PcdSmbiosRecord;  
    EFI_SMBIOS_HANDLE           SmbiosHandle; 

    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType7CacheInformation);

    //
    // Get board socket designation string
    //
    SocketDesignationStr = PcdGetPtr (PcdSmbiosType7StringSocketDesignation);
    SocketDesignationStrLen = AsciiStrLen (SocketDesignationStr);
    ASSERT (SocketDesignationStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE7) + SocketDesignationStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE7));

    //
    // Fill in type 7 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_CACHE_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE7);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SocketDesignationStr, SocketDesignationStrLen);

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status;
}