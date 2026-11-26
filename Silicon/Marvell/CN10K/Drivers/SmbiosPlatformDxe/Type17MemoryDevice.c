/** @file
  SMBIOS Type 17 (Memory Device) tables for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/SmcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

typedef struct _JEDEC_MF_ID {
  UINT8    VendorId;
  CHAR8    *ManufacturerString;
} JEDEC_MF_ID;

STATIC CONST JEDEC_MF_ID  Bank0Table[] = {
  { 0x01, "AMD"                },
  { 0x04, "Fujitsu"            },
  { 0x07, "Hitachi"            },
  { 0x89, "Intel"              },
  { 0x10, "NEC"                },
  { 0x97, "Texas Instrument"   },
  { 0x98, "Toshiba"            },
  { 0x1c, "Mitsubishi"         },
  { 0x1f, "Atmel"              },
  { 0x20, "STMicroelectronics" },
  { 0xa4, "IBM"                },
  { 0x2c, "Micron Technology"  },
  { 0xad, "SK Hynix"           },
  { 0xb0, "Sharp"              },
  { 0xb3, "IDT"                },
  { 0x3e, "Oracle"             },
  { 0xbf, "SST"                },
  { 0x40, "ProMos/Mosel"       },
  { 0xc1, "Infineon"           },
  { 0xc2, "Macronix"           },
  { 0x45, "SanDisk"            },
  { 0xce, "Samsung"            },
  { 0xda, "Winbond"            },
  { 0xe0, "LG Semi"            },
  { 0x62, "Sanyo"              },
  { 0xff, "Undefined"          }
};

STATIC CONST JEDEC_MF_ID  Bank1Table[] = {
  { 0x98, "Kingston"  },
  { 0xba, "PNY"       },
  { 0x4f, "Transcend" },
  { 0x7a, "Apacer"    },
  { 0xff, "Undefined" }
};

STATIC CONST JEDEC_MF_ID  Bank2Table[] = {
  { 0x9e, "Corsair"   },
  { 0xfe, "Elpida"    },
  { 0xff, "Undefined" }
};

STATIC CONST JEDEC_MF_ID  Bank3Table[] = {
  { 0x0b, "Nanya"     },
  { 0x94, "Mushkin"   },
  { 0x25, "Kingmax"   },
  { 0xff, "Undefined" }
};

STATIC CONST JEDEC_MF_ID  Bank4Table[] = {
  { 0xb0, "OCZ"       },
  { 0xcb, "A-DATA"    },
  { 0xcd, "G Skill"   },
  { 0xef, "Team"      },
  { 0xff, "Undefined" }
};

STATIC CONST JEDEC_MF_ID  Bank5Table[] = {
  { 0x02, "Patriot"   },
  { 0x9b, "Crucial"   },
  { 0x51, "Qimonda"   },
  { 0x57, "AENEON"    },
  { 0xf7, "Avant"     },
  { 0xff, "Undefined" }
};

STATIC CONST JEDEC_MF_ID  Bank6Table[] = {
  { 0x34, "Super Talent"  },
  { 0xd3, "Silicon Power" },
  { 0xff, "Undefined"     }
};

STATIC CONST JEDEC_MF_ID  Bank7Table[] = {
  { 0xff, "Undefined" }
};

STATIC JEDEC_MF_ID CONST * CONST  ManufacturerJedecIdBankTable[] = {
  Bank0Table,
  Bank1Table,
  Bank2Table,
  Bank3Table,
  Bank4Table,
  Bank5Table,
  Bank6Table,
  Bank7Table
};

STATIC SMBIOS_TABLE_TYPE17  mSmbiosType17Template = {
  { EFI_SMBIOS_TYPE_MEMORY_DEVICE, sizeof (SMBIOS_TABLE_TYPE17), 0 },
  1,                       // MemoryArrayHandle;
  0xFFFE,                  // MemoryErrorInformationHandle;
  0,                       // TotalWidth;
  0,                       // DataWidth;
  0,                       // Size;
  MemoryFormFactorUnknown, // FormFactor; Value from MEMORY_FORM_FACTOR.
  0x0,                     // DeviceSet;
  1,                       // DeviceLocator String
  2,                       // BankLocator String
  MemoryTypeUnknown,       // MemoryType; Value from MEMORY_DEVICE_TYPE.
  {     // TypeDetail;
    0,  // Reserved        :1;
    0,  // Other           :1;
    1,  // Unknown         :1;
    0,  // FastPaged       :1;
    0,  // StaticColumn    :1;
    0,  // PseudoStatic    :1;
    0,  // Rambus          :1;
    0,  // Synchronous     :1;
    0,  // Cmos            :1;
    0,  // Edo             :1;
    0,  // WindowDram      :1;
    0,  // CacheDram       :1;
    0,  // Nonvolatile     :1;
    0,  // Registered      :1;
    0,  // Unbuffered      :1;
    0,  // Reserved1       :1;
  },
  0,                    // Speed;
  3,                    // Manufacturer String
  4,                    // SerialNumber String
  5,                    // AssetTag String
  6,                    // PartNumber String
  0,                    // Attributes;
  0,                    // ExtendedSize;
  0,                    // ConfiguredMemoryClockSpeed;
  0,                    // UINT16 MinimumVoltage;
  0,                    // UINT16 MaximumVoltage;
  0,                    // UINT16 ConfiguredVoltage;
  MemoryTechnologyDram, // UINT8 MemoryTechnology; Value from MEMORY_DEVICE_TECHNOLOGY
  {  // MemoryOperatingModeCapability;
    {
      0, // Reserved                         :1;   ///< Set to 0.
      0, // Other                            :1;
      0, // Unknown                          :1;
      1, // VolatileMemory                   :1;
      0, // ByteAccessiblePersistentMemory   :1;
      0, // BlockAccessiblePersistentMemory  :1;
      0  // Reserved2                        :10;  ///< Set to 0.
    }
  },
  7,          // SMBIOS_TABLE_STRING FirmwareVersion;
  0,          // UINT16 ModuleManufacturerID;
  0,          // UINT16 ModuleProductID;
  0,          // UINT16 MemorySubsystemControllerManufacturerID;
  0,          // UINT16 MemorySubsystemControllerProductID;
  0,          // UINT64 NonVolatileSize;
  0,          // UINT64 VolatileSize;
  0,          // UINT64 CacheSize;
  0,          // UINT64 LogicalSize;
  0,          // UINT32 ExtendedSpeed;
  0,          // UINT32 ExtendedConfiguredMemorySpeed;
};

STATIC CHAR8  *mSmbiosType17Strings[] = {
  "DIMM SLOT",            // Device Locator
  "Bank A",               // Bank Locator
  "Unknown Manufacturer", // Manufacturer
  "Unknown Serial",       // Serial Number
  "Unknown Asset",        // Asset Tag
  "Unknown Part",         // Part Number
  "Not specified",        // Firmware version
  NULL
};

STATIC CHAR8  *mSmbiosType17DeviceLocators[] = {
  "DDR0",
  "DDR1",
  "DDR2",
};

STATIC CHAR8  *mSmbiosType17BankLocators[] = {
  "Bank0",
  "Bank1",
  "Bank2",
};

/**
  Install the SMBIOS Type 17 (Memory Device) records.

  Adds one record per populated or empty DSS. Each record is linked to the
  memory array from Context->PmaHandle and carries the size, speed, widths,
  rank, manufacturer, serial number, part number, voltage, and type detail
  decoded from the SPD data. The assigned handle is stored in
  Context->DimmHandles for the Type 20 records.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with the DIMM configuration and
                           memory array handle; receives DIMM handles.

  @retval  EFI_SUCCESS  All memory device records were installed.
  @retval  Others       A record could not be added.
**/
EFI_STATUS
InstallType17MemoryDevice (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  EFI_STATUS   Status;
  UINT8        Dss;
  DIMM_CFG     *DimmCfg;
  UINTN        Index;
  JEDEC_MF_ID CONST  *IdTablePointer;

  for (Dss = 0; Dss < FixedPcdGet8 (PcdDssPerNode); Dss++) {
    DimmCfg = &(Context->NodeConfig->DimmCfg[Dss]);

    if (DimmCfg->Status == DimmStatusNotPresent) {
      continue;
    }

    SMBIOS_SET_STRING (
      mSmbiosType17Strings,
      &mSmbiosType17Template,
      DeviceLocator,
      mSmbiosType17DeviceLocators[Dss]
      );

    SMBIOS_SET_STRING (
      mSmbiosType17Strings,
      &mSmbiosType17Template,
      BankLocator,
      mSmbiosType17BankLocators[Dss]
      );

    mSmbiosType17Template.MemoryArrayHandle = Context->PmaHandle;

    SetMem (&mSmbiosType17Template.TypeDetail, sizeof (mSmbiosType17Template.TypeDetail), 0);

    SMBIOS_SET_STRING (
      mSmbiosType17Strings,
      &mSmbiosType17Template,
      Manufacturer,
      "Unknown Manufacturer"
      );

    SMBIOS_SET_STRING (mSmbiosType17Strings, &mSmbiosType17Template, SerialNumber, "Unknown Serial");
    SMBIOS_SET_STRING (mSmbiosType17Strings, &mSmbiosType17Template, PartNumber, "Unknown Part");

    if (DimmCfg->Status == DimmStatusEmpty) {
      mSmbiosType17Template.Size                       = 0;
      mSmbiosType17Template.FormFactor                 = MemoryFormFactorUnknown;
      mSmbiosType17Template.Speed                      = 0;
      mSmbiosType17Template.ConfiguredMemoryClockSpeed = 0;
      mSmbiosType17Template.ExtendedSize               = 0;
      mSmbiosType17Template.VolatileSize               = 0;
      mSmbiosType17Template.LogicalSize                = 0;
      mSmbiosType17Template.Attributes                 = 0;
      mSmbiosType17Template.TotalWidth                 = 0;
      mSmbiosType17Template.DataWidth                  = 0;
      mSmbiosType17Template.ModuleManufacturerID       = DimmCfg->Mfg;

      mSmbiosType17Template.TypeDetail.Unknown = 1;
      mSmbiosType17Template.MemoryType         = MemoryTypeUnknown;
      mSmbiosType17Template.MinimumVoltage     = 0; // If the value is 0, the voltage is unknown
      mSmbiosType17Template.MaximumVoltage     = 0; // If the value is 0, the voltage is unknown
      mSmbiosType17Template.ConfiguredVoltage  = 0; // If the value is 0, the voltage is unknown
    } else if (DimmCfg->Status == DimmStatusAvailable) {
      mSmbiosType17Template.Size                       = (DimmCfg->SizeMb < 0x7FFF) ? (UINT16)DimmCfg->SizeMb : 0x7FFF;
      mSmbiosType17Template.FormFactor                 = MemoryFormFactorDimm;
      mSmbiosType17Template.Speed                      = (UINT16)DimmCfg->Speed;
      mSmbiosType17Template.ConfiguredMemoryClockSpeed = (UINT16)Context->DdrSpeed;
      mSmbiosType17Template.ExtendedSize               = (UINT32)DimmCfg->SizeMb;
      mSmbiosType17Template.VolatileSize               = (UINT64)DimmCfg->SizeMb * SIZE_1MB; // In bytes
      mSmbiosType17Template.LogicalSize                = (UINT64)DimmCfg->SizeMb * SIZE_1MB; // In bytes
      mSmbiosType17Template.Attributes                 = DimmCfg->Rank;
      mSmbiosType17Template.TotalWidth                 = DimmCfg->TotalWidth;
      mSmbiosType17Template.DataWidth                  = DimmCfg->DataWidth;
      mSmbiosType17Template.ModuleManufacturerID       = DimmCfg->Mfg;

      Index = DimmCfg->Mfg & 0x7F; // LSB indicates table number

      if (Index > 7) {
        Index = 7;
      }

      IdTablePointer = ManufacturerJedecIdBankTable[Index];

      // Search in Manufacturer table. MSB is manufacturer ID
      while ((IdTablePointer->VendorId != ((DimmCfg->Mfg >> 8) & 0xFF)) &&
             (IdTablePointer->VendorId != 0xff))
      {
        IdTablePointer++;
      }

      if (IdTablePointer->VendorId != 0xff) {
        SMBIOS_SET_STRING (
          mSmbiosType17Strings,
          &mSmbiosType17Template,
          Manufacturer,
          IdTablePointer->ManufacturerString
          );
      }

      if (DimmCfg->Serial[0]) {
        SMBIOS_SET_STRING (
          mSmbiosType17Strings,
          &mSmbiosType17Template,
          SerialNumber,
          DimmCfg->Serial
          );
      }

      if (DimmCfg->PartNo[0]) {
        SMBIOS_SET_STRING (
          mSmbiosType17Strings,
          &mSmbiosType17Template,
          PartNumber,
          DimmCfg->PartNo
          );
      }

      if (DimmCfg->Type == CN10K_SPD_MEMORY_TYPE_DDR4) {
        // DDR4
        mSmbiosType17Template.MemoryType = MemoryTypeDdr4;

        mSmbiosType17Template.MinimumVoltage    = 1200; // 1.1V
        mSmbiosType17Template.MaximumVoltage    = 1200; // 1.1V
        mSmbiosType17Template.ConfiguredVoltage = 1200; // 1.1V

        switch (DimmCfg->SubType) {
          // DDR4 SPD spec -> RDIMM Overlay Schema
          case 0x1:
          case 0x5:
          case 0x8:
            mSmbiosType17Template.TypeDetail.Registered = 1;
            break;
          // DDR4 SPD spec -> LRDIMM Overlay Schema
          case 0x4:
            mSmbiosType17Template.TypeDetail.LrDimm = 1;
            break;
          // DDR4 SPD spec -> UDIMM Overlay Schema
          case 0x2:
          case 0x3:
          case 0x6:
          case 0x9:
          case 0xC:
          case 0xD:
            mSmbiosType17Template.TypeDetail.Unbuffered = 1;
            break;
        }
      } else if (DimmCfg->Type == CN10K_SPD_MEMORY_TYPE_DDR5) {
        // DDR5
        mSmbiosType17Template.MemoryType        = MemoryTypeDdr5;
        mSmbiosType17Template.MinimumVoltage    = 1100; // 1.1V
        mSmbiosType17Template.MaximumVoltage    = 1100; // 1.1V
        mSmbiosType17Template.ConfiguredVoltage = 1100; // 1.1V

        switch (DimmCfg->SubType) {
          // JESD400-5 DDR5 SPD Contents 0.96 -> RDIMM Overlay Schema
          case 0x1:
            mSmbiosType17Template.TypeDetail.Registered = 1;
            break;
          // JESD400-5 DDR5 SPD Contents 0.96 -> LRDIMM Overlay Schema
          case 0x4:
            mSmbiosType17Template.TypeDetail.LrDimm = 1;
            break;
          // JESD400-5 DDR5 SPD Contents 0.96 -> UDIMM Overlay Schema
          case 0x2:
          case 0x3:
            mSmbiosType17Template.TypeDetail.Unbuffered = 1;
            break;
        }
      }
    }

    Status = SmbiosPlatformAddEntry (
               Smbios,
               (SMBIOS_STRUCTURE *)&mSmbiosType17Template,
               mSmbiosType17Strings,
               &Context->DimmHandles[Dss]
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

 #ifdef SMC_DIMM_HANDLE
    {
      UINTN  SmcStatus;

      SmcStatus = SmcSetDimmHandle (Dss, Context->DimmHandles[Dss]);
      if (SmcStatus != 0) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: SMC call failed for dimm handle with Status = 0x%x\n",
          __FUNCTION__,
          SmcStatus
          ));
      }
    }
 #endif
  }

  return EFI_SUCCESS;
}
