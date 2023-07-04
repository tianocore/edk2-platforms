/** @file
  Smbios type 26.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  One Memory Module Information structure is included for each memory-module 
  socket in the system. The structure describes the speed, type, size, and 
  error status of each memory module. (Type 26).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
VoltageProbeFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *DescriptionStr;
    UINTN                       DescriptionStrLen;       
    SMBIOS_TABLE_TYPE26          *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE26          *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;


    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType26VoltageProbe);

    //
    // Get board socket designation string
    //
	DescriptionStr = PcdGetPtr (PcdSmbiosType26StringDescription);
	DescriptionStrLen = AsciiStrLen (DescriptionStr);
    ASSERT (DescriptionStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE26) + DescriptionStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE26));

    //
    // Fill in type 26 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_VOLTAGE_PROBE;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE26);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, DescriptionStr, DescriptionStrLen);
    

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}
