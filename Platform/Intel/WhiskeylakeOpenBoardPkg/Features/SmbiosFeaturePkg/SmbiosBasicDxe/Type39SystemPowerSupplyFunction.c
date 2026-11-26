/** @file
  Smbios type 39.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  System Power Supply (Type 39).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
SystemPowerSupplyFunction(
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                          Status;
    CHAR8                               *LocationStr;
    CHAR8                               *DeviceNameStr;
    CHAR8                               *ManufacturerStr;
    CHAR8                               *SerialNumberStr;
    CHAR8                               *AssetTagNumberStr;
    CHAR8                               *ModelPartNumberStr;
    CHAR8                               *RevisionLevelStr;
    UINTN                               LocationStrLen;
    UINTN                               DeviceNameStrLen;
    UINTN                               ManufacturerStrLen;
    UINTN                               SerialNumberStrLen;
    UINTN                               AssetTagNumberStrLen;
    UINTN                               ModelPartNumberStrLen;
    UINTN                               RevisionLevelStrLen;
    SMBIOS_TABLE_TYPE39                 *SmbiosRecord;
    SMBIOS_TABLE_TYPE39                 *PcdSmbiosRecord;
    EFI_SMBIOS_HANDLE                   SmbiosHandle;
    UINTN                               TableSize;
    UINTN                               StringOffset;

    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType39SystemPowerSupply);

    //
    // Get board location string.
    //
    LocationStr = PcdGetPtr (PcdSmbiosType39StringLocation);
    LocationStrLen = AsciiStrLen (LocationStr);
    ASSERT (LocationStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board device name.
    //
    DeviceNameStr = PcdGetPtr (PcdSmbiosType39StringDeviceName);
    DeviceNameStrLen = AsciiStrLen (DeviceNameStr);
    ASSERT (DeviceNameStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board Manufacturer.
    //
    ManufacturerStr = PcdGetPtr (PcdSmbiosType39Manufacturer);
    ManufacturerStrLen = AsciiStrLen (ManufacturerStr);
    ASSERT (ManufacturerStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board Serial Number.
    //
    SerialNumberStr = PcdGetPtr (PcdSmbiosType39SerialNumber);
    SerialNumberStrLen = AsciiStrLen (SerialNumberStr);
    ASSERT (SerialNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board Asset tag.
    //
    AssetTagNumberStr = PcdGetPtr (PcdSmbiosType39AssetTagNumber);
    AssetTagNumberStrLen = AsciiStrLen (AssetTagNumberStr);
    ASSERT (AssetTagNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board Model part number.
    //
    ModelPartNumberStr = PcdGetPtr (PcdSmbiosType39ModelPartNumber);
    ModelPartNumberStrLen = AsciiStrLen (ModelPartNumberStr);
    ASSERT (ModelPartNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board Revision level.
    //
    RevisionLevelStr = PcdGetPtr (PcdSmbiosType39RevisionLevel);
    RevisionLevelStrLen = AsciiStrLen (RevisionLevelStr);
    ASSERT (RevisionLevelStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE4) + LocationStrLen + 1 + DeviceNameStrLen + 1 + ManufacturerStrLen + 1 + SerialNumberStrLen + 1 + AssetTagNumberStrLen + 1 + ModelPartNumberStrLen + 1 + RevisionLevelStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE39));

    //
    // Fill in type 39 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_SYSTEM_POWER_SUPPLY;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE39);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, LocationStr, LocationStrLen);
    StringOffset += LocationStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, DeviceNameStr, DeviceNameStrLen);
    StringOffset += DeviceNameStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ManufacturerStr, ManufacturerStrLen);
    StringOffset += ManufacturerStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SerialNumberStr, SerialNumberStrLen);
    StringOffset += SerialNumberStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, AssetTagNumberStr, AssetTagNumberStrLen);
    StringOffset += AssetTagNumberStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ModelPartNumberStr, ModelPartNumberStrLen);
    StringOffset += ModelPartNumberStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, RevisionLevelStr, RevisionLevelStrLen);
    StringOffset += RevisionLevelStrLen + 1;

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status;
}