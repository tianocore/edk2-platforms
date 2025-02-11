/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/JedecJep106Lib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include "SmbiosPlatformDxe.h"

#define NULL_TERMINATED_ID                         0xFF

#define ASCII_SPACE_CHARACTER_CODE                 0x20
#define ASCII_TILDE_CHARACTER_CODE                 0x7E

#define SPD_PARITY_BIT_MASK                        0x80
#define SPD_MEMORY_TYPE_OFFSET                     0x02
#define SPD_CONTINUATION_CHARACTER                 0x7F

#define DDR4_SPD_MANUFACTURER_MEMORY_TYPE          0x0C
#define DDR4_SPD_MANUFACTURER_ID_BANK_OFFSET       320
#define DDR4_SPD_MANUFACTURER_ID_CODE_OFFSET       321
#define DDR4_SPD_MANUFACTURER_PART_NUMBER_OFFSET   329
#define DDR4_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET 325

#define MAX_DIMMS  16

#define PRINTABLE_CHARACTER(Character) \
  (Character >= ASCII_SPACE_CHARACTER_CODE) && (Character <= ASCII_TILDE_CHARACTER_CODE) ? \
  Character : ASCII_SPACE_CHARACTER_CODE

typedef enum {
  DEVICE_LOCATOR_TOKEN_INDEX = 0,
  BANK_LOCATOR_TOKEN_INDEX,
  MANUFACTURER_TOKEN_INDEX,
  SERIAL_NUMBER_TOKEN_INDEX,
  ASSET_TAG_TOKEN_INDEX,
  PART_NUMBER_TOKEN_INDEX
} MEMORY_DEVICE_TOKEN_INDEX;

#pragma pack(1)
typedef struct {
  UINT8  VendorId;
  CHAR16 *ManufacturerString;
} JEDEC_MF_ID;
#pragma pack()

VOID
UpdateManufacturer (
  IN UINT8  *SpdData,
  IN UINT16 ManufacturerToken
  )
{
  UINTN       Index;
  UINT8       VendorId;
  UINT8       MemType;
  CONST CHAR8 *ManufacturerString;
  CHAR16      *UnicodeManufacturerString;
  UINTN        Length;

  MemType = SpdData[SPD_MEMORY_TYPE_OFFSET];
  switch (MemType) {
  case DDR4_SPD_MANUFACTURER_MEMORY_TYPE:
    Index = SpdData[DDR4_SPD_MANUFACTURER_ID_BANK_OFFSET] & (~SPD_PARITY_BIT_MASK); // Remove parity bit
    VendorId = SpdData[DDR4_SPD_MANUFACTURER_ID_CODE_OFFSET];
    break;

  default: // Not supported
    DEBUG ((DEBUG_ERROR, "Unsupported/unknown DDR memory type encountered: %d\n", MemType));
    return;
  }

  ManufacturerString = Jep106GetManufacturerName (VendorId, Index);
  if (ManufacturerString == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to get JEDEC JEP107 manufacturer from VendorID %d, Index %d\n", VendorId, Index));
    return;
  }

  Length = AsciiStrSize (ManufacturerString);
  UnicodeManufacturerString = AllocateZeroPool (Length * sizeof (CHAR16));
  if (UnicodeManufacturerString == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to allocate memory for DDR manufacturer string.\n"));
    return;
  }

  AsciiStrToUnicodeStrS (ManufacturerString, UnicodeManufacturerString, Length);
  HiiSetString (mSmbiosPlatformDxeHiiHandle, ManufacturerToken, UnicodeManufacturerString, NULL);
  FreePool (UnicodeManufacturerString);
}

VOID
UpdateSerialNumber (
  IN UINT8  *SpdData,
  IN UINT16 SerialNumberToken
  )
{
  UINT8  MemType;
  UINTN  Offset;
  CHAR16 SerialNumberStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  MemType = SpdData[SPD_MEMORY_TYPE_OFFSET];
  switch (MemType) {
  case DDR4_SPD_MANUFACTURER_MEMORY_TYPE:
    Offset = DDR4_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET;
    break;

  default: // Not supported
    DEBUG ((DEBUG_ERROR, "Unsupported/unknown DDR memory type encountered: %d\n", MemType));
    return;
  }

  UnicodeSPrint (
    SerialNumberStr,
    sizeof (SerialNumberStr),
    L"%02X%02X%02X%02X",
    SpdData[Offset],
    SpdData[Offset + 1],
    SpdData[Offset + 2],
    SpdData[Offset + 3]
    );
  HiiSetString (mSmbiosPlatformDxeHiiHandle, SerialNumberToken, SerialNumberStr, NULL);
}

VOID
UpdatePartNumber (
  IN UINT8  *SpdData,
  IN UINT16 PartNumberToken
  )
{
  UINT8  MemType;
  UINTN  Offset;
  CHAR16 PartNumberStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  MemType = SpdData[SPD_MEMORY_TYPE_OFFSET];
  switch (MemType) {
  case DDR4_SPD_MANUFACTURER_MEMORY_TYPE:
    Offset = DDR4_SPD_MANUFACTURER_PART_NUMBER_OFFSET;
    break;

  default: // Not supported
    DEBUG ((DEBUG_ERROR, "Unsupported/unknown DDR memory type encountered: %d\n", MemType));
    return;
  }

  UnicodeSPrint (
    PartNumberStr,
    sizeof (PartNumberStr),
    L"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
    PRINTABLE_CHARACTER (SpdData[Offset]),
    PRINTABLE_CHARACTER (SpdData[Offset + 1]),
    PRINTABLE_CHARACTER (SpdData[Offset + 2]),
    PRINTABLE_CHARACTER (SpdData[Offset + 3]),
    PRINTABLE_CHARACTER (SpdData[Offset + 4]),
    PRINTABLE_CHARACTER (SpdData[Offset + 5]),
    PRINTABLE_CHARACTER (SpdData[Offset + 6]),
    PRINTABLE_CHARACTER (SpdData[Offset + 7]),
    PRINTABLE_CHARACTER (SpdData[Offset + 8]),
    PRINTABLE_CHARACTER (SpdData[Offset + 9]),
    PRINTABLE_CHARACTER (SpdData[Offset + 10]),
    PRINTABLE_CHARACTER (SpdData[Offset + 11]),
    PRINTABLE_CHARACTER (SpdData[Offset + 12]),
    PRINTABLE_CHARACTER (SpdData[Offset + 13]),
    PRINTABLE_CHARACTER (SpdData[Offset + 14]),
    PRINTABLE_CHARACTER (SpdData[Offset + 15]),
    PRINTABLE_CHARACTER (SpdData[Offset + 16]),
    PRINTABLE_CHARACTER (SpdData[Offset + 17])
    );
  HiiSetString (mSmbiosPlatformDxeHiiHandle, PartNumberToken, PartNumberStr, NULL);
}

/**
  This function adds SMBIOS Table (Type 17) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformMemoryDevice) {
  UINTN               HandleCount;
  UINTN               DimmIndex;
  UINTN               ChannelIndex;
  UINTN               MemorySize;
  UINT16              *HandleArray;
  CHAR16              UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  EFI_STATUS          Status;
  SMBIOS_HANDLE       MemoryArrayHandle;
  PLATFORM_DIMM       *Dimm;
  STR_TOKEN_INFO      *InputStrToken;
  PLATFORM_DIMM_LIST  *DimmList;
  PLATFORM_DRAM_INFO  *DramInfo;
  SMBIOS_TABLE_TYPE17 *InputData;
  SMBIOS_TABLE_TYPE17 *Type17Record;

  HandleCount   = 0;
  HandleArray   = NULL;

  GetDimmList (&DimmList);
  if (DimmList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get Dimm List\n",
      __FUNCTION__,
      __LINE__
      ));
    return EFI_NOT_FOUND;
  }

  GetDramInfo (&DramInfo);
  if (DramInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get DRAM Information\n",
      __FUNCTION__,
      __LINE__
      ));
    return EFI_NOT_FOUND;
  }

  SmbiosPlatformDxeGetLinkTypeHandle (
    EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,
    &HandleArray,
    &HandleCount
    );
  if (HandleArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "HandleCount: %d\n", HandleCount));

  if (HandleCount < 1) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get Memory Array Handle\n",
      __FUNCTION__,
      __LINE__
      ));
    FreePool (HandleArray);
    return EFI_NOT_FOUND;
  }

  InputData = (SMBIOS_TABLE_TYPE17 *)RecordData;
  InputStrToken = (STR_TOKEN_INFO *)StrToken;
  MemoryArrayHandle = HandleArray[0];

  DimmIndex = 0;

  for (ChannelIndex = 0; ChannelIndex < (MAX_DIMMS / 2); ChannelIndex++) {
    //
    // Prepare additional strings for SMBIOS Table.
    //
    Dimm = &DimmList->Dimm[DimmIndex];
    if (Dimm->NodeId != 0) {
      continue;
    }

    Status = SmbiosPlatformDxeSaveHiiDefaultString (InputStrToken);
    if (EFI_ERROR (Status)) {
      FreePool (HandleArray);
      return Status;
    }
    if (Dimm->Info.DimmStatus == DIMM_INSTALLED_OPERATIONAL) {
      UpdateManufacturer (Dimm->SpdData.Data, InputStrToken->TokenArray[MANUFACTURER_TOKEN_INDEX]);
      UpdateSerialNumber (Dimm->SpdData.Data, InputStrToken->TokenArray[SERIAL_NUMBER_TOKEN_INDEX]);
      UpdatePartNumber (Dimm->SpdData.Data, InputStrToken->TokenArray[PART_NUMBER_TOKEN_INDEX]);
    }
    UnicodeSPrint (UnicodeStr, sizeof (UnicodeStr), L"DIMM %c1", 'A' + ChannelIndex);
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[DEVICE_LOCATOR_TOKEN_INDEX], UnicodeStr, NULL);
    UnicodeSPrint (UnicodeStr, sizeof (UnicodeStr), L"Bank %d", ChannelIndex);
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[BANK_LOCATOR_TOKEN_INDEX], UnicodeStr, NULL);
    UnicodeSPrint (UnicodeStr, sizeof (UnicodeStr), L"Asset Tag Not Set");
    HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[ASSET_TAG_TOKEN_INDEX], UnicodeStr, NULL);

    //
    // Create Table and fill up information.
    //
    SmbiosPlatformDxeCreateTable (
      (VOID *)&Type17Record,
      (VOID *)&InputData,
      sizeof (SMBIOS_TABLE_TYPE17),
      InputStrToken
      );
    if (Type17Record == NULL) {
      FreePool (HandleArray);
      return EFI_OUT_OF_RESOURCES;
    }

      if (Dimm->Info.DimmStatus != DIMM_NOT_INSTALLED) {
        DEBUG ((DEBUG_INFO, "DIMM Info: \n"));
	DEBUG ((DEBUG_INFO, "\tStatus (1=Installed-Operational, 2=Installed-NonOperational, 3=Installed-Failed): %d\n", Dimm->Info.DimmStatus));
        DEBUG ((DEBUG_INFO, "\tPart Number: %a\n", Dimm->Info.PartNumber));
        DEBUG ((DEBUG_INFO, "\tDimmSize: %llu\n", Dimm->Info.DimmSize));
        DEBUG ((DEBUG_INFO, "\tDimmMfcId: %d\n", Dimm->Info.DimmMfcId));
        DEBUG ((DEBUG_INFO, "\tDimmNrRank: %d\n", Dimm->Info.DimmNrRank));
        DEBUG ((DEBUG_INFO, "\tDimmType: %d\n", Dimm->Info.DimmType));
        DEBUG ((DEBUG_INFO, "\tDimmDevType: %d\n", Dimm->Info.DimmDevType));
      }

      if (Dimm->Info.DimmStatus == DIMM_INSTALLED_OPERATIONAL) {
        MemorySize = Dimm->Info.DimmSize * 1024;

        if (MemorySize >= 0x7FFF) {
          Type17Record->Size = 0x7FFF;
          Type17Record->ExtendedSize = MemorySize;
        } else {
          Type17Record->Size = (UINT16)MemorySize;
          Type17Record->ExtendedSize = 0;
        }

        Type17Record->MemoryType                 = MemoryTypeDdr4;
        Type17Record->Speed                      = (UINT16)DramInfo->MaxSpeed;
        Type17Record->ConfiguredMemoryClockSpeed = (UINT16)DramInfo->MaxSpeed;
        Type17Record->Attributes                 = Dimm->Info.DimmNrRank & 0x0F;
        Type17Record->ConfiguredVoltage          = 1200;
        Type17Record->MinimumVoltage             = 1140;
        Type17Record->MaximumVoltage             = 1260;
        Type17Record->DeviceSet                  = 0; // None

        if (Dimm->Info.DimmType == UDIMM || Dimm->Info.DimmType == SODIMM) {
          Type17Record->TypeDetail.Unbuffered = 1; // BIT 14: unregistered
        } else if (Dimm->Info.DimmType == RDIMM
                  || Dimm->Info.DimmType == LRDIMM
                  || Dimm->Info.DimmType == RSODIMM)
        {
          Type17Record->TypeDetail.Registered = 1; // BIT 13: registered
        }
        /* FIXME: Determine if need to set technology to NVDIMM-* when supported */
        Type17Record->MemoryTechnology = MemoryTechnologyDram;
      }
      // Update Type 16 handle
      Type17Record->MemoryArrayHandle = MemoryArrayHandle;

      //
      // Add Table record and free pool.
      //
      Status = SmbiosPlatformDxeAddRecord ((UINT8 *)Type17Record, NULL);
      if (EFI_ERROR (Status)) {
        FreePool (HandleArray);
        FreePool (Type17Record);
        return Status;
      }

      FreePool (Type17Record);
      Status = SmbiosPlatformDxeRestoreHiiDefaultString (InputStrToken);
      if (EFI_ERROR (Status)) {
        FreePool (HandleArray);
        return Status;
      }

      DimmIndex += 2;
    }

  FreePool (HandleArray);

  return Status;
}
