/** @file
  Smbios type 30.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  Out-of-Band Remote Access (Type 30).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
OutOfBoundRemoteAccessFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *ManufacturerNameStr;
    UINTN                       ManufacturerNameStrLen;
    SMBIOS_TABLE_TYPE30         *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE30         *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;

    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType30OutOfBoundRemoteAccess);

    //
    // Get Manufacturer Name
    //
    ManufacturerNameStr = PcdGetPtr (PcdSmbiosType30StringManufacturerName);
    ManufacturerNameStrLen = AsciiStrLen (ManufacturerNameStr);
    ASSERT (ManufacturerNameStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE30) + ManufacturerNameStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE30));

    //
    // Fill in type 30 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE30);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ManufacturerNameStr, ManufacturerNameStrLen);
    StringOffset += ManufacturerNameStrLen + 1;

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}