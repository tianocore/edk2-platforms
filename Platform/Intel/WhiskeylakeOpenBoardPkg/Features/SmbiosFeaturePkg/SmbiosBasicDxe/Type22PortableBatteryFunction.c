/** @file
  Smbios type 22.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

/**
  This structure describes the attributes of the portable battery(s) for 
  the system. It also contains the static attributes for the particular 
  group. Each structure describes a single battery pack�s attributes. 
  (Type 22).

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
EFI_STATUS
EFIAPI
PortableBatterynFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
)
{
    EFI_STATUS                  Status;
    CHAR8                       *LocationStr;
	UINTN                       LocationStrLen;
    CHAR8                       *ManufacturerStr;
	UINTN                       ManufacturerStrLen;
    CHAR8                       *ManufactureDateStr;
	UINTN                       ManufactureDateStrLen;
    CHAR8                       *SerialNumberStr;
	UINTN                       SerialNumberStrLen;
    CHAR8                       *DeviceNameStr;
	UINTN                       DeviceNameStrLen;
    CHAR8                       *SBDSVersionNumberStr;
	UINTN                       SBDSVersionNumberStrLen;
    CHAR8                       *SBDSDeviceChemistryStr;
    UINTN                       SBDSDeviceChemistryStrLen;      
    SMBIOS_TABLE_TYPE22          *PcdSmbiosRecord;
    SMBIOS_TABLE_TYPE22          *SmbiosRecord;
    EFI_SMBIOS_HANDLE           SmbiosHandle;
    UINTN                       StringOffset;
    UINTN                       TableSize;


    PcdSmbiosRecord = PcdGetPtr (PcdSmbiosType22PortableBattery);

    //
    // Get board socket string
    //
	LocationStr = PcdGetPtr (PcdSmbiosType22StringLocation);
	LocationStrLen = AsciiStrLen (LocationStr);
    ASSERT (LocationStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor manufacturer string
    //
	ManufacturerStr = PcdGetPtr (PcdSmbiosType22StringManufacturer);
	ManufacturerStrLen = AsciiStrLen (ManufacturerStr);
    ASSERT (ManufacturerStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor version string
    //
	ManufactureDateStr = PcdGetPtr (PcdSmbiosType22StringManufactureDate);
	ManufactureDateStrLen = AsciiStrLen (ManufactureDateStr);
    ASSERT (ManufactureDateStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor serial number string
    //
    SerialNumberStr = PcdGetPtr (PcdSmbiosType22StringSerialNumber);
    SerialNumberStrLen = AsciiStrLen (SerialNumberStr);
    ASSERT (SerialNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board processor asset tag string
    //
    DeviceNameStr = PcdGetPtr (PcdSmbiosType22StringDeviceName);
    DeviceNameStrLen = AsciiStrLen (DeviceNameStr);
    ASSERT (DeviceNameStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Get board part number string
    //
    SBDSVersionNumberStr = PcdGetPtr (PcdSmbiosType22StringSBDSVersionNumber);
    SBDSVersionNumberStrLen = AsciiStrLen (SBDSVersionNumberStr);
    ASSERT (SBDSVersionNumberStrLen <= SMBIOS_STRING_MAX_LENGTH);
	//
    // Get board part number string
    //
	SBDSDeviceChemistryStr = PcdGetPtr (PcdSmbiosType22StringSBDSDeviceChemistryStr);
	SBDSDeviceChemistryStrLen = AsciiStrLen (SBDSDeviceChemistryStr);
    ASSERT (SBDSDeviceChemistryStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Create table size based on string lengths
    //
    TableSize = sizeof (SMBIOS_TABLE_TYPE22) + LocationStrLen + 1 + ManufacturerStrLen + 1 + ManufactureDateStrLen + 1 +  SerialNumberStrLen + 1 + DeviceNameStrLen + 1 + SBDSVersionNumberStrLen + 1 +SBDSDeviceChemistryStrLen + 1 + 1;
    SmbiosRecord = AllocateZeroPool (TableSize);
    if (SmbiosRecord == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (SmbiosRecord, PcdSmbiosRecord, sizeof(SMBIOS_TABLE_TYPE22));

    //
    // Fill in type 22 fields
    //
    SmbiosRecord->Hdr.Type = SMBIOS_TYPE_PORTABLE_BATTERY;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE22);
    SmbiosRecord->Hdr.Handle = 0;

    //
    // Add strings to bottom of data block
    //
    StringOffset = SmbiosRecord->Hdr.Length;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, LocationStr, LocationStrLen);
    StringOffset += LocationStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ManufacturerStr, ManufacturerStrLen);
    StringOffset += ManufacturerStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, ManufactureDateStr, ManufactureDateStrLen);
    StringOffset += ManufactureDateStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SerialNumberStr, SerialNumberStrLen);
    StringOffset += SerialNumberStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, DeviceNameStr, DeviceNameStrLen);
    StringOffset += DeviceNameStrLen + 1;
    CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SBDSVersionNumberStr, SBDSVersionNumberStrLen);
    StringOffset += SBDSVersionNumberStrLen + 1;
	CopyMem((UINT8*)SmbiosRecord + StringOffset, SBDSDeviceChemistryStr, SBDSDeviceChemistryStrLen);
	StringOffset += SBDSDeviceChemistryStrLen + 1;

    //
    // Full smbios record, call smbios protocol to add this record.
    //
    Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
    
    FreePool (SmbiosRecord);
    return Status; 
}
