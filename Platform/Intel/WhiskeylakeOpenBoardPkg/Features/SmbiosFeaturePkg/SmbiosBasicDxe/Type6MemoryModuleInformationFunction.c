/** @file
  Smbios type 6.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  One Memory Module Information structure is included for each memory-module 
  socket in the system. The structure describes the speed, type, size, and 
  error status of each memory module. (Type 6).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
MemoryModuleInformationFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *SocketDesignationStr;
    UINTN                       SocketDesignationStrLen;       
    SMBIOS_TABLE_TYPE6          *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE6          *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;


    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType6MemoryModuleInformation);

    //
    // Get board socket designation string
    //
	SocketDesignationStr = PcdGetPtr (PcdSmbiosType6StringSocketDesignation);
	SocketDesignationStrLen = AsciiStrLen (SocketDesignationStr);
    ASSERT (SocketDesignationStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE6) + SocketDesignationStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE6));

    //
    // Fill in type 6 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_MEMORY_MODULE_INFORMATON;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE6);
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
