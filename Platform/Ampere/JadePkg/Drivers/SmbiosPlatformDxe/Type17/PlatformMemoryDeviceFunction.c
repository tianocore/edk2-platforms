/** @file

  Copyright (c) 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include "SmbiosPlatformDxe.h"

#define NULL_TERMINATED_ID  0xFF

#define ASCII_SPACE_CHARACTER_CODE  0x20
#define ASCII_TILDE_CHARACTER_CODE  0x7E

#define SPD_PARITY_BIT_MASK         0x80
#define SPD_MEMORY_TYPE_OFFSET      0x02
#define SPD_CONTINUATION_CHARACTER  0x7F

#define DDR2_SPD_MANUFACTURER_MEMORY_TYPE           0x08
#define DDR2_SPD_MANUFACTURER_ID_CODE_LENGTH        8
#define DDR2_SPD_MANUFACTURER_ID_CODE_OFFSET        64
#define DDR2_SPD_MANUFACTURER_PART_NUMBER_OFFSET    73
#define DDR2_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET  95

#define DDR3_SPD_MANUFACTURER_MEMORY_TYPE           0x0B
#define DDR3_SPD_MANUFACTURER_ID_BANK_OFFSET        117
#define DDR3_SPD_MANUFACTURER_ID_CODE_OFFSET        118
#define DDR3_SPD_MANUFACTURER_PART_NUMBER_OFFSET    128
#define DDR3_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET  122

#define DDR4_SPD_MANUFACTURER_MEMORY_TYPE           0x0C
#define DDR4_SPD_MANUFACTURER_ID_BANK_OFFSET        320
#define DDR4_SPD_MANUFACTURER_ID_CODE_OFFSET        321
#define DDR4_SPD_MANUFACTURER_PART_NUMBER_OFFSET    329
#define DDR4_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET  325

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
  UINT8     VendorId;
  CHAR16    *ManufacturerString;
} JEDEC_MF_ID;
#pragma pack()

JEDEC_MF_ID  Bank0Table[] = {
  { 0x01,               L"AMD\0"                },
  { 0x04,               L"Fujitsu\0"            },
  { 0x07,               L"Hitachi\0"            },
  { 0x89,               L"Intel\0"              },
  { 0x10,               L"NEC\0"                },
  { 0x97,               L"Texas Instrument\0"   },
  { 0x98,               L"Toshiba\0"            },
  { 0x1C,               L"Mitsubishi\0"         },
  { 0x1F,               L"Atmel\0"              },
  { 0x20,               L"STMicroelectronics\0" },
  { 0xA4,               L"IBM\0"                },
  { 0x2C,               L"Micron Technology\0"  },
  { 0xAD,               L"SK Hynix\0"           },
  { 0xB0,               L"Sharp\0"              },
  { 0xB3,               L"IDT\0"                },
  { 0x3E,               L"Oracle\0"             },
  { 0xBF,               L"SST\0"                },
  { 0x40,               L"ProMos/Mosel\0"       },
  { 0xC1,               L"Infineon\0"           },
  { 0xC2,               L"Macronix\0"           },
  { 0x45,               L"SanDisk\0"            },
  { 0xCE,               L"Samsung\0"            },
  { 0xDA,               L"Winbond\0"            },
  { 0xE0,               L"LG Semi\0"            },
  { 0x62,               L"Sanyo\0"              },
  { NULL_TERMINATED_ID, L"Undefined\0"          }
};

JEDEC_MF_ID  Bank1Table[] = {
  { 0x98,               L"Kingston\0"  },
  { 0xBA,               L"PNY\0"       },
  { 0x4F,               L"Transcend\0" },
  { 0x7A,               L"Apacer\0"    },
  { NULL_TERMINATED_ID, L"Undefined\0" }
};

JEDEC_MF_ID  Bank2Table[] = {
  { 0x9E,               L"Corsair\0"   },
  { 0xFE,               L"Elpida\0"    },
  { NULL_TERMINATED_ID, L"Undefined\0" }
};

JEDEC_MF_ID  Bank3Table[] = {
  { 0x0B,               L"Nanya\0"     },
  { 0x94,               L"Mushkin\0"   },
  { 0x25,               L"Kingmax\0"   },
  { NULL_TERMINATED_ID, L"Undefined\0" }
};

JEDEC_MF_ID  Bank4Table[] = {
  { 0xB0,               L"OCZ\0"       },
  { 0xCB,               L"A-DATA\0"    },
  { 0xCD,               L"G Skill\0"   },
  { 0xEF,               L"Team\0"      },
  { NULL_TERMINATED_ID, L"Undefined\0" }
};

JEDEC_MF_ID  Bank5Table[] = {
  { 0x02,               L"Patriot\0"   },
  { 0x9B,               L"Crucial\0"   },
  { 0x51,               L"Qimonda\0"   },
  { 0x57,               L"AENEON\0"    },
  { 0xF7,               L"Avant\0"     },
  { NULL_TERMINATED_ID, L"Undefined\0" }
};

JEDEC_MF_ID  Bank6Table[] = {
  { 0x34,               L"Super Talent\0"  },
  { 0xD3,               L"Silicon Power\0" },
  { NULL_TERMINATED_ID, L"Undefined\0"     }
};

JEDEC_MF_ID  Bank7Table[] = {
  { NULL_TERMINATED_ID, L"Undefined\0" }
};

JEDEC_MF_ID  *ManufacturerJedecIdBankTable[] = {
  Bank0Table,
  Bank1Table,
  Bank2Table,
  Bank3Table,
  Bank4Table,
  Bank5Table,
  Bank6Table,
  Bank7Table
};

VOID
UpdateManufacturer (
  IN UINT8   *SpdData,
  IN UINT16  ManufacturerToken
  )
{
  UINTN        Index;
  UINT8        VendorId;
  UINT8        MemType;
  UINT8        NumberOfJedecIdBankTables;
  JEDEC_MF_ID  *IdTblPtr = NULL;

  MemType = SpdData[SPD_MEMORY_TYPE_OFFSET];
  switch (MemType) {
    case DDR2_SPD_MANUFACTURER_MEMORY_TYPE:
      for (Index = 0; Index < DDR2_SPD_MANUFACTURER_ID_CODE_LENGTH; Index++) {
        VendorId = SpdData[DDR2_SPD_MANUFACTURER_ID_CODE_OFFSET + Index];
        if (VendorId != SPD_CONTINUATION_CHARACTER) {
          break;
        }
      }

      break;

    case DDR3_SPD_MANUFACTURER_MEMORY_TYPE:
      Index    = SpdData[DDR3_SPD_MANUFACTURER_ID_BANK_OFFSET] & (~SPD_PARITY_BIT_MASK); // Remove parity bit
      VendorId = SpdData[DDR4_SPD_MANUFACTURER_ID_CODE_OFFSET];
      break;

    case DDR4_SPD_MANUFACTURER_MEMORY_TYPE:
      Index    = SpdData[DDR4_SPD_MANUFACTURER_ID_BANK_OFFSET] & (~SPD_PARITY_BIT_MASK); // Remove parity bit
      VendorId = SpdData[DDR4_SPD_MANUFACTURER_ID_CODE_OFFSET];
      break;

    default: // Not supported
      return;
  }

  NumberOfJedecIdBankTables = ARRAY_SIZE (ManufacturerJedecIdBankTable) - 1; // Exclude NULL-terminated table
  if (Index > NumberOfJedecIdBankTables) {
    Index = NumberOfJedecIdBankTables;
  }

  IdTblPtr = ManufacturerJedecIdBankTable[Index];

  // Search in Manufacturer table and update vendor name accordingly in HII Database
  while (IdTblPtr->VendorId != NULL_TERMINATED_ID) {
    if (IdTblPtr->VendorId == VendorId) {
      HiiSetString (mSmbiosPlatformDxeHiiHandle, ManufacturerToken, IdTblPtr->ManufacturerString, NULL);
      break;
    }

    IdTblPtr++;
  }
}

VOID
UpdateSerialNumber (
  IN UINT8   *SpdData,
  IN UINT16  SerialNumberToken
  )
{
  UINT8   MemType;
  UINTN   Offset;
  CHAR16  SerialNumberStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  MemType = SpdData[SPD_MEMORY_TYPE_OFFSET];
  switch (MemType) {
    case DDR2_SPD_MANUFACTURER_MEMORY_TYPE:
      Offset = DDR2_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET;
      break;

    case DDR3_SPD_MANUFACTURER_MEMORY_TYPE:
      Offset = DDR3_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET;
      break;

    case DDR4_SPD_MANUFACTURER_MEMORY_TYPE:
      Offset = DDR4_SPD_MANUFACTURER_SERIAL_NUMBER_OFFSET;
      break;

    default: // Not supported
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
  IN UINT8   *SpdData,
  IN UINT16  PartNumberToken
  )
{
  UINT8   MemType;
  UINTN   Offset;
  CHAR16  PartNumberStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];

  MemType = SpdData[SPD_MEMORY_TYPE_OFFSET];
  switch (MemType) {
    case DDR2_SPD_MANUFACTURER_MEMORY_TYPE:
      Offset = DDR2_SPD_MANUFACTURER_PART_NUMBER_OFFSET;
      break;

    case DDR3_SPD_MANUFACTURER_MEMORY_TYPE:
      Offset = DDR3_SPD_MANUFACTURER_PART_NUMBER_OFFSET;
      break;

    case DDR4_SPD_MANUFACTURER_MEMORY_TYPE:
      Offset = DDR4_SPD_MANUFACTURER_PART_NUMBER_OFFSET;
      break;

    default: // Not supported
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
  UINT8                Index;
  UINT8                SlotIndex;
  UINTN                HandleCount;
  UINTN                MemorySize;
  UINT16               *HandleArray;
  CHAR16               UnicodeStr[SMBIOS_UNICODE_STRING_MAX_LENGTH];
  EFI_STATUS           Status;
  SMBIOS_HANDLE        MemoryArrayHandle;
  PLATFORM_DIMM        *Dimm;
  STR_TOKEN_INFO       *InputStrToken;
  PLATFORM_DIMM_LIST   *DimmList;
  PLATFORM_DRAM_INFO   *DramInfo;
  SMBIOS_TABLE_TYPE17  *InputData;
  SMBIOS_TABLE_TYPE17  *Type17Record;

  HandleCount = 0;
  HandleArray = NULL;

  GetDimmList (&DimmList);
  if (DimmList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get Dimm List\n",
      __func__,
      __LINE__
      ));
    return EFI_NOT_FOUND;
  }

  GetDramInfo (&DramInfo);
  if (DramInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get DRAM Information\n",
      __func__,
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

  if (HandleCount != GetNumberOfSupportedSockets ()) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Failed to get Memory Array Handle\n",
      __func__,
      __LINE__
      ));
    FreePool (HandleArray);
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < GetNumberOfSupportedSockets (); Index++) {
    InputData         = (SMBIOS_TABLE_TYPE17 *)RecordData;
    InputStrToken     = (STR_TOKEN_INFO *)StrToken;
    MemoryArrayHandle = HandleArray[Index];

    while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
      for (SlotIndex = 0; SlotIndex < DimmList->BoardDimmSlots; SlotIndex++) {
        //
        // Prepare additional strings for SMBIOS Table.
        //
        Dimm = &DimmList->Dimm[SlotIndex];
        if (Dimm->NodeId != Index) {
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

        UnicodeSPrint (UnicodeStr, sizeof (UnicodeStr), L"Socket %d DIMM %d", Index, SlotIndex);
        HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[DEVICE_LOCATOR_TOKEN_INDEX], UnicodeStr, NULL);
        UnicodeSPrint (UnicodeStr, sizeof (UnicodeStr), L"Bank %d", SlotIndex);
        HiiSetString (mSmbiosPlatformDxeHiiHandle, InputStrToken->TokenArray[BANK_LOCATOR_TOKEN_INDEX], UnicodeStr, NULL);
        UnicodeSPrint (UnicodeStr, sizeof (UnicodeStr), L"Array %d Asset Tag %d", Index, SlotIndex);
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

        if (Dimm->Info.DimmStatus == DIMM_INSTALLED_OPERATIONAL) {
          MemorySize = Dimm->Info.DimmSize * 1024;
          if (MemorySize >= 0x7FFF) {
            Type17Record->Size         = 0x7FFF;
            Type17Record->ExtendedSize = MemorySize;
          } else {
            Type17Record->Size         = (UINT16)MemorySize;
            Type17Record->ExtendedSize = 0;
          }

          Type17Record->MemoryType                 = 0x1A; // DDR4
          Type17Record->Speed                      = (UINT16)DramInfo->MaxSpeed;
          Type17Record->ConfiguredMemoryClockSpeed = (UINT16)DramInfo->MaxSpeed;
          Type17Record->Attributes                 = Dimm->Info.DimmNrRank & 0x0F;
          Type17Record->ConfiguredVoltage          = 1200;
          Type17Record->MinimumVoltage             = 1140;
          Type17Record->MaximumVoltage             = 1260;
          Type17Record->DeviceSet                  = 0; // None

          if ((Dimm->Info.DimmType == UDIMM) || (Dimm->Info.DimmType == SODIMM)) {
            Type17Record->TypeDetail.Unbuffered = 1; // BIT 14: unregistered
          } else if (  (Dimm->Info.DimmType == RDIMM)
                    || (Dimm->Info.DimmType == LRDIMM)
                    || (Dimm->Info.DimmType == RSODIMM))
          {
            Type17Record->TypeDetail.Registered = 1; // BIT 13: registered
          }

          /* FIXME: Determine if need to set technology to NVDIMM-* when supported */
          Type17Record->MemoryTechnology = 0x3; // DRAM
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
      }

      InputData++;
      InputStrToken++;
    }
  }

  FreePool (HandleArray);

  return Status;
}
