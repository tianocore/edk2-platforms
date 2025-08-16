/** @file
  Smbios type 4.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  Processor Information (Type 4).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
ProcessorInformationFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *SocketStr;
    CHAR8                       *ProcessorManufacturerStr;
    CHAR8                       *ProcessorVersionStr;
    CHAR8                       *SerialNumberStr;
    CHAR8                       *AssetTagStr;
    CHAR8                       *PartNumberStr;
    UINTN                       SocketStrLen;
    UINTN                       ProcessorManufacturerStrLen;
    UINTN                       ProcessorVersionStrLen;
    UINTN                       SerialNumberStrLen;
    UINTN                       AssetTagStrLen;
    UINTN                       PartNumberStrLen;         
    SMBIOS_TABLE_TYPE4          *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE4          *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;


    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType4ProcessorInformation);

    //
    // Get board socket string
    //
    SocketStr = PcdGetPtr (PcdSmbiosType4StringSocket);
    SocketStrLen = AsciiStrLen (SocketStr);
    ASSERT (SocketStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor manufacturer string
    //
    ProcessorManufacturerStr = PcdGetPtr (PcdSmbiosType4StringManufacturer);
    ProcessorManufacturerStrLen = AsciiStrLen (ProcessorManufacturerStr);
    ASSERT (ProcessorManufacturerStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor version string
    //
    ProcessorVersionStr = PcdGetPtr (PcdSmbiosType4StringVersion);
    ProcessorVersionStrLen = AsciiStrLen (ProcessorVersionStr);
    ASSERT (ProcessorVersionStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor serial number string
    //
    SerialNumberStr = PcdGetPtr (PcdSmbiosType4StringSerialNumber);
    SerialNumberStrLen = AsciiStrLen (SerialNumberStr);
    ASSERT (SerialNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor asset tag string
    //
    AssetTagStr = PcdGetPtr (PcdSmbiosType4StringAssetTag);
    AssetTagStrLen = AsciiStrLen (AssetTagStr);
    ASSERT (AssetTagStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board part number string
    //
    PartNumberStr = PcdGetPtr (PcdSmbiosType4StringPartNumber);
    PartNumberStrLen = AsciiStrLen (PartNumberStr);
    ASSERT (PartNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE4) + SocketStrLen + 1 + ProcessorManufacturerStrLen + 1 + ProcessorVersionStrLen + 1 +  SerialNumberStrLen + 1 + AssetTagStrLen + 1 + PartNumberStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE4));

    //
    // Fill in type 4 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_PROCESSOR_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE4);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SocketStr, SocketStrLen);
    StringOffset += SocketStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ProcessorManufacturerStr, ProcessorManufacturerStrLen);
    StringOffset += ProcessorManufacturerStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ProcessorVersionStr, ProcessorVersionStrLen);
    StringOffset += ProcessorVersionStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SerialNumberStr, SerialNumberStrLen);
    StringOffset += SerialNumberStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, AssetTagStr, AssetTagStrLen);
    StringOffset += AssetTagStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, PartNumberStr, PartNumberStrLen);
    StringOffset += PartNumberStrLen + 1;

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}
