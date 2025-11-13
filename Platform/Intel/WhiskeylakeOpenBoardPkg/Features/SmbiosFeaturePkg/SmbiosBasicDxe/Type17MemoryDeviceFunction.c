/** @file
  Smbios type 17.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This function makes boot time changes to the contents of the
  MemoryDeviceFunction (Type 17).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
MemoryDeviceFunction(
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
  )
{
  EFI_STATUS                          Status;
  CHAR8                               *DeviceLocatorStr;
  UINTN                               DeviceLocatorStrLen;
  CHAR8                               *BankLocatorStr;
  UINTN                               BankLocatorStrLen;
  CHAR8                               *ManufacturerStr;
  UINTN                               ManufacturerStrLen;
  CHAR8                               *SerialNumberStr;
  UINTN                               SerialNumberStrLen;
  CHAR8                               *AssetTagStr;
  UINTN                               AssetTagStrLen;
  CHAR8                               *PartNumberStr;
  UINTN                               PartNumberStrLen;
  CHAR8                               *FirmwareVersionStr;
  UINTN                               FirmwareVersionStrLen;
  SMBIOS_TABLE_TYPE17                 *SmbiosRecord;
  SMBIOS_TABLE_TYPE17                 *PcdSmbiosRecord;
  EFI_SMBIOS_HANDLE                   SmbiosHandle;
  UINTN                               SourceSize;
  UINTN                               TotalSize;
  UINTN                               StringOffset;

  PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType17MemoryDevice);

  //
  // Get Device Locator String.
  //
  DeviceLocatorStr = PcdGetPtr (PcdSmbiosType17StringDeviceLocator);
  DeviceLocatorStrLen = AsciiStrLen (DeviceLocatorStr);
  ASSERT (DeviceLocatorStrLen <= SMBIOS_STRING_MAX_LENGTH);
  //
  // Get Bank Locator String.
  //
  BankLocatorStr = PcdGetPtr (PcdSmbiosType17StringBankLocator);
  BankLocatorStrLen = AsciiStrLen (BankLocatorStr);
  ASSERT (BankLocatorStrLen <= SMBIOS_STRING_MAX_LENGTH);
  //
  // Get Manufacturer String.
  //
  ManufacturerStr = PcdGetPtr (PcdSmbiosType17StringManufacturer);
  ManufacturerStrLen = AsciiStrLen (ManufacturerStr);
  ASSERT (ManufacturerStrLen <= SMBIOS_STRING_MAX_LENGTH);
  //
  // Get Serial Number String.
  //
  SerialNumberStr = PcdGetPtr (PcdSmbiosType17StringSerialNumber);
  SerialNumberStrLen = AsciiStrLen (SerialNumberStr);
  ASSERT (SerialNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);
  //
  // Get Asset Tag String.
  //
  AssetTagStr = PcdGetPtr (PcdSmbiosType17StringAssetTag);
  AssetTagStrLen = AsciiStrLen (AssetTagStr);
  ASSERT (AssetTagStrLen <= SMBIOS_STRING_MAX_LENGTH);
  //
  // Get Part Number String.
  //
  PartNumberStr = PcdGetPtr (PcdSmbiosType17StringPartNumber);
  PartNumberStrLen = AsciiStrLen (PartNumberStr);
  ASSERT (PartNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);
  //
  // Get Firmware Version String.
  //
  FirmwareVersionStr = PcdGetPtr (PcdSmbiosType17StringFirmwareVersion);
  FirmwareVersionStrLen = AsciiStrLen (FirmwareVersionStr);
  ASSERT (FirmwareVersionStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Allocate space for the record
  //
  SourceSize = PcdGetSize (PcdSmbiosType17MemoryDevice);
  TotalSize = SourceSize + DeviceLocatorStrLen + 1 + BankLocatorStrLen + 1 + ManufacturerStrLen + 1 + SerialNumberStrLen + 1;
  TotalSize += AssetTagStrLen + 1 + PartNumberStrLen + 1 + FirmwareVersionStrLen + 1 + 1;
  SmbiosRecord = AllocateZeroPool(TotalSize);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the data to the table
  //
  CopyMem (SmbiosRecord, PcdSmbiosRecord, SourceSize);

  SmbiosRecord->Hdr.Type = SMBIOS_TYPE_MEMORY_DEVICE;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE17);
  SmbiosRecord->Hdr.Handle = 0;

  StringOffset = SmbiosRecord->Hdr.Length;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, DeviceLocatorStr, DeviceLocatorStrLen);
  StringOffset += DeviceLocatorStrLen + 1;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, BankLocatorStr, BankLocatorStrLen);
  StringOffset += BankLocatorStrLen + 1;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ManufacturerStr, ManufacturerStrLen);
  StringOffset += ManufacturerStrLen + 1;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SerialNumberStr, SerialNumberStrLen);
  StringOffset += SerialNumberStrLen + 1;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, AssetTagStr, AssetTagStrLen);
  StringOffset += AssetTagStrLen + 1;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, PartNumberStr, PartNumberStrLen);
  StringOffset += PartNumberStrLen + 1;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, FirmwareVersionStr, FirmwareVersionStrLen);
  StringOffset += FirmwareVersionStrLen + 1;
  
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool (SmbiosRecord);
  return Status;
}