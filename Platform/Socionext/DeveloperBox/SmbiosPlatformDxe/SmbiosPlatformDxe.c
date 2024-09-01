/** @file

  This driver installs SMBIOS information for Socionext SynQuacer platforms

  Copyright (c) 2015, ARM Limited. All rights reserved.
  Copyright (c) 2018, Linaro, Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/SdramSpdDdr4.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>

STATIC EFI_SMBIOS_PROTOCOL       *mSmbios;

#define SPD4_MEM_BUS_WIDTH_8BIT    (0x00)
#define SPD4_MEM_BUS_WIDTH_16BIT   (BIT0)
#define SPD4_MEM_BUS_WIDTH_32BIT   (BIT1)
#define SPD4_MEM_BUS_WIDTH_64BIT   (BIT0 | BIT1)

#define SPD4_MEM_DEV_WIDTH_4BIT    (0x00)
#define SPD4_MEM_DEV_WIDTH_8BIT    (BIT0)
#define SPD4_MEM_DEV_WIDTH_16BIT   (BIT1)
#define SPD4_MEM_DEV_WIDTH_32BIT   (BIT0 | BIT1)

#define SPD4_DDR4_SDRAM_TYPE          (0x0C)

#define SPD4_MEM_MODULE_TYPE_RDIMM    (0x01)
#define SPD4_MEM_MODULE_TYPE_UDIMM    (0x02)
#define SPD4_MEM_MODULE_TYPE_SODIMM   (0x03)

#define TYPE17_DEVICE_LOCATOR_LEN     (8 + 1)
#define TYPE17_BANK_LOCATOR_LEN       (20 + 1)
#define TYPE17_MANUFACTURER_NAME_LEN  (30 + 1)
#define TYPE17_SERIAL_NUMBER_LEN      (16 + 1)
#define TYPE17_ASSETTAG_LEN           (16 + 1)
#define TYPE17_MODULE_PART_NUMBER_LEN (20 + 1)
#define TYPE17_FIRMWARE_VERSION_LEN   (8 + 1)

#define TYPE17_STRINGS_MAX_LEN        (TYPE17_DEVICE_LOCATOR_LEN + \
                                       TYPE17_BANK_LOCATOR_LEN + \
                                       TYPE17_MANUFACTURER_NAME_LEN + \
                                       TYPE17_SERIAL_NUMBER_LEN + \
                                       TYPE17_ASSETTAG_LEN + \
                                       TYPE17_MODULE_PART_NUMBER_LEN + \
                                       TYPE17_FIRMWARE_VERSION_LEN + \
                                       1/* null SMBIOS_TABLE_STRING terminator */ )

//
// Type definition and contents of the default SMBIOS table.
// This table covers only the minimum structures required by
// the SMBIOS specification (section 6.2, version 3.0)
//
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE0  Base;
  CHAR8               Strings[];
} ARM_TYPE0;

typedef struct {
  SMBIOS_TABLE_TYPE1  Base;
  CHAR8               Strings[];
} ARM_TYPE1;

typedef struct {
  SMBIOS_TABLE_TYPE2  Base;
  CHAR8               Strings[];
} ARM_TYPE2;

typedef struct {
  SMBIOS_TABLE_TYPE3  Base;
  CHAR8               Strings[];
} ARM_TYPE3;

typedef struct {
  SMBIOS_TABLE_TYPE4  Base;
  CHAR8               Strings[];
} ARM_TYPE4;

typedef struct {
  SMBIOS_TABLE_TYPE7  Base;
  CHAR8               Strings[];
} ARM_TYPE7;

typedef struct {
  SMBIOS_TABLE_TYPE9  Base;
  CHAR8               Strings[];
} ARM_TYPE9;

typedef struct {
  SMBIOS_TABLE_TYPE16 Base;
  CHAR8               Strings[];
} ARM_TYPE16;

typedef struct {
  SMBIOS_TABLE_TYPE17 Base;
  CHAR8               Strings[TYPE17_STRINGS_MAX_LEN];
} ARM_TYPE17;

typedef struct {
  SMBIOS_TABLE_TYPE19 Base;
  CHAR8               Strings[];
} ARM_TYPE19;

typedef struct {
  SMBIOS_TABLE_TYPE32 Base;
  CHAR8               Strings[];
} ARM_TYPE32;
#pragma pack()

enum {
  SMBIOS_HANDLE_A53_L1I = 0x1000,
  SMBIOS_HANDLE_A53_L1D,
  SMBIOS_HANDLE_A53_L2,
  SMBIOS_HANDLE_A53_L3,
  SMBIOS_HANDLE_MOTHERBOARD,
  SMBIOS_HANDLE_CHASSIS,
  SMBIOS_HANDLE_A53_CLUSTER,
  SMBIOS_HANDLE_MEMORY,
};

struct JEP106_MANUFACTURER_TABLE {
  UINT16 ManufacturerId;
  CHAR8  ManufacturerName[TYPE17_MANUFACTURER_NAME_LEN];
};

STATIC CONST struct JEP106_MANUFACTURER_TABLE Manufacturer[] = {
  {0x0010, "NEC\0"},
  {0x002C, "Micron Technology\0"},
  {0x003D, "Tektronix\0"},
  {0x0097, "Texas Instruments\0"},
  {0x00AD, "SK Hynix\0"},
  {0x00B3, "IDT\0"},
  {0x00C1, "Infineon\0"},
  {0x00CE, "Samsung\0"},
  {0x00DA, "Winbond Electronic\0"},
  {0x014F, "Transcend Information\0"},
  {0x0194, "Smart Modular\0"},
  {0x0198, "Kingston\0"},
  {0x02C8, "Agilent Technologies\0"},
  {0x02FE, "Elpida\0"},
  {0x030B, "Nanya Technology\0"},
  {0x0443, "Ramaxel Technology\0"},
  {0x04B3, "Inphi Corporation\0"},
  {0x04C8, "Powerchip Semiconductor\0"},
  {0x0551, "Qimonda\0"},
  {0x0557, "AENEON\0"},
  {0x059B, "Crucial Technology\0"},
  {0xFFFF, "Unknown\0"}
};

enum SPD4_SDRAM_CAPACITY {
  SPD4_SDRAM_CAPACITY_256MBIT = 0,
  SPD4_SDRAM_CAPACITY_512MBIT,
  SPD4_SDRAM_CAPACITY_1GBIT,
  SPD4_SDRAM_CAPACITY_2GBIT,
  SPD4_SDRAM_CAPACITY_4GBIT,
  SPD4_SDRAM_CAPACITY_8GBIT,
  SPD4_SDRAM_CAPACITY_16GBIT,
  SPD4_SDRAM_CAPACITY_32GBIT,
  SPD4_SDRAM_CAPACITY_12GBIT,
  SPD4_SDRAM_CAPACITY_24GBIT,
  SPD4_SDRAM_CAPACITY_INVALID = 0xFF,
};

struct SPD4_SDRAM_CAPACITY_TABLE {
  enum SPD4_SDRAM_CAPACITY        Capacity;
  UINT16                          SizeMbit;
};

STATIC CONST struct SPD4_SDRAM_CAPACITY_TABLE CapacityTable[]  = {
  {SPD4_SDRAM_CAPACITY_256MBIT,     256        },
  {SPD4_SDRAM_CAPACITY_512MBIT,     512        },
  {SPD4_SDRAM_CAPACITY_1GBIT,       (1 * 1024) },
  {SPD4_SDRAM_CAPACITY_2GBIT,       (2 * 1024) },
  {SPD4_SDRAM_CAPACITY_4GBIT,       (4 * 1024) },
  {SPD4_SDRAM_CAPACITY_8GBIT,       (8 * 1024) },
  {SPD4_SDRAM_CAPACITY_16GBIT,      (16 * 1024)},
  {SPD4_SDRAM_CAPACITY_32GBIT,      (32 * 1024)},
  {SPD4_SDRAM_CAPACITY_12GBIT,      (12 * 1024)},
  {SPD4_SDRAM_CAPACITY_24GBIT,      (24 * 1024)},
  {SPD4_SDRAM_CAPACITY_INVALID,     0          },
};

// BIOS information (section 7.1)
STATIC CONST ARM_TYPE0 mArmDefaultType0 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_BIOS_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE0),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,                                            // Vendor
    2,                                            // BiosVersion
    0xE800,                                       // BiosSegment
    3,                                            // BiosReleaseDate
    (FixedPcdGet32 (PcdFdSize) - 1) / SIZE_64KB,  // BiosSize
    {
      0, 0, 0, 0, 0, 0,
      1,                                          // PCI supported
      0,
      1,                                          // PNP supported
      0,
      1,                                          // BIOS upgradable
      0, 0, 0,
      1,                                          // Boot from CD
      1,                                          // Selectable boot
    },
    { 0x3, 0xC, },                                // BIOSCharacteristicsExtensionBytes[2]

    FixedPcdGet32 (PcdFirmwareRevision) >> 16,    // SystemBiosMajorRelease
    FixedPcdGet32 (PcdFirmwareRevision) & 0xff,   // SystemBiosMinorRelease
    0xFF,                                         // EmbeddedControllerFirmwareMajorRelease
    0xFF                                          // EmbeddedControllerFirmwareMinorRelease
  }, {
    FIRMWARE_VENDOR " \0"
    "build #" BUILD_NUMBER "\0"
    __DATE__ "\0"
  }
};

// System information (section 7.2)
STATIC CONST ARM_TYPE1 mArmDefaultType1 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_INFORMATION,
      sizeof(SMBIOS_TABLE_TYPE1),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,     // Manufacturer
    2,     // Product Name
    0,     // Version
    0,     // Serial
    { 0xbf4ec78a, 0x431d, 0x4eb6, { 0xbb, 0xc9, 0x0c, 0x06, 0x19, 0x05, 0xca, 0x13 }},
    6,     // Wakeup type
    0,     // SKU
    0,     // Family
  }, {
    "Socionext\0"
    "SynQuacer E-series DeveloperBox\0"
  }
};

// Enclosure
STATIC CONST ARM_TYPE3 mArmDefaultType3 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE,
      sizeof (SMBIOS_TABLE_TYPE3),
      SMBIOS_HANDLE_CHASSIS,
    },
    1,   // Manufacturer
    4,   // Enclosure type (low profile desktop)
    2,   // Version
    0,   // Serial
    0,   // Asset tag
    ChassisStateUnknown,          // boot chassis state
    ChassisStateSafe,             // power supply state
    ChassisStateSafe,             // thermal state
    ChassisSecurityStatusNone,    // security state
    { 0, 0, 0, 0 },               // OEM defined
    1,                            // 1U height
    1,                            // number of power cords
    0,                            // no contained elements
  }, {
    "InWin\0"
    "BK623\0"
  }
};

STATIC CONST ARM_TYPE4 mArmDefaultType4 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE4),
      SMBIOS_HANDLE_A53_CLUSTER,
    },
    0,                                // socket type
    3,                                // processor type CPU
    ProcessorFamilyIndicatorFamily2,  // processor family, acquire from field2
    1,                                // manufacturer
    {},                               // processor id
    2,                                // version
    { 0, 0, 0, 0, 0, 1 },             // voltage
    0,                                // external clock
    1000,                             // max speed
    1000,                             // current speed
    0x41,                             // status
    ProcessorUpgradeNone,
    SMBIOS_HANDLE_A53_L1D,            // l1 cache handle
    SMBIOS_HANDLE_A53_L2,             // l2 cache handle
    SMBIOS_HANDLE_A53_L3,             // l3 cache handle
    0,                                // serial not set
    0,                                // asset not set
    3,                                // part number
    24,                               // core count in socket
    24,                               // enabled core count in socket
    24,                               // threads per socket
    0xEC,                             // processor characteristics
    ProcessorFamilyARM,               // ARM core
  }, {
    "ARM Ltd.\0"
    "Cortex-A53\0"
    "0xd03\0"
  }
};

STATIC CONST ARM_TYPE7 mArmDefaultType7_l1i = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE7),
      SMBIOS_HANDLE_A53_L1I,
    },
    1,
    0x380,                      // L1 enabled
    32,                         // 32k i cache max
    32,                         // 32k installed
    { 0, 1 },                   // SRAM type
    { 0, 1 },                   // SRAM type
    0,                          // unknown speed
    CacheErrorParity,
    CacheTypeInstruction,
    CacheAssociativity2Way,
  }, {
    "L1 Instruction\0"
  }
};

STATIC CONST ARM_TYPE7 mArmDefaultType7_l1d = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE7),
      SMBIOS_HANDLE_A53_L1D,
    },
    1,
    0x180,                      // L1 enabled, WB
    32,                         // 32k d cache max
    32,                         // 32k installed
    { 0, 1 },                   // SRAM type
    { 0, 1 },                   // SRAM type
    0,                          // unknown speed
    CacheErrorSingleBit,
    CacheTypeData,
    CacheAssociativity4Way,
  }, {
    "L1 Data\0"
  }
};

STATIC CONST ARM_TYPE7 mArmDefaultType7_l2 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE7),
      SMBIOS_HANDLE_A53_L2,
    },
    1,
    0x181,                      // L2 enabled, WB
    256,                        // 256 KB cache max
    256,                        // 256 KB installed
    { 0, 1 },                   // SRAM type
    { 0, 1 },                   // SRAM type
    0,                          // unknown speed
    CacheErrorSingleBit,
    CacheTypeUnified,
    CacheAssociativity16Way,
  }, {
    "L2\0"
  }
};

STATIC CONST ARM_TYPE7 mArmDefaultType7_l3 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE7),
      SMBIOS_HANDLE_A53_L3,
    },
    1,
    0x182,                      // L3 enabled, WB
    4096,                       // 4M cache max
    4096,                       // 4M installed
    { 0, 1 },                   // SRAM type
    { 0, 1 },                   // SRAM type
    0,                          // unknown speed
    CacheErrorSingleBit,
    CacheTypeUnified,
    CacheAssociativity16Way,
  }, {
    "L3\0"
  }
};

// Slots
STATIC CONST ARM_TYPE9 mArmDefaultType9_0 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS,
      sizeof (SMBIOS_TABLE_TYPE9),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,
    SlotTypePciExpressGen2X16,
    SlotDataBusWidth4X,
    SlotUsageUnknown,
    SlotLengthLong,
    0,
    { 1 },
    {},
    1,
    0,
    0,
  }, {
    "J-PCIEX16\0"
  }
};

STATIC CONST ARM_TYPE9 mArmDefaultType9_1 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS,
      sizeof (SMBIOS_TABLE_TYPE9),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,
    SlotTypePciExpressGen2X1,
    SlotDataBusWidth1X,
    SlotUsageUnknown,
    SlotLengthShort,
    0,
    { 1 },
    {},
    0x0,
    0x3,
    0x0,
  }, {
    "J-PCIE1\0"
  }
};

STATIC CONST ARM_TYPE9 mArmDefaultType9_2 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS,
      sizeof (SMBIOS_TABLE_TYPE9),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    1,
    SlotTypePciExpressGen2X1,
    SlotDataBusWidth1X,
    SlotUsageUnknown,
    SlotLengthShort,
    0,
    { 1 },
    {},
    0x0,
    0x5,
    0x0,
  }, {
    "J-PCIE2\0"
  }
};

// Memory array
STATIC CONST ARM_TYPE16 mArmDefaultType16 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,
      sizeof (SMBIOS_TABLE_TYPE16),
      SMBIOS_HANDLE_MEMORY,
    },
    MemoryArrayLocationSystemBoard,     // on motherboard
    MemoryArrayUseSystemMemory,         // system RAM
    MemoryErrorCorrectionNone,
    0x4000000,                          // max 64 GB
    0xFFFE,                             // No error information structure
    4,                                  // 4 DIMMs
  }, {
    "\0"
  }
};

// Memory array mapped address, this structure
// is overridden by InstallMemoryStructure
STATIC CONST ARM_TYPE19 mArmDefaultType19 = {
  {
    {  // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS,
      sizeof (SMBIOS_TABLE_TYPE19),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    0xFFFFFFFF,             // invalid, look at extended addr field
    0xFFFFFFFF,
    SMBIOS_HANDLE_MEMORY,   // handle
    1,
    0x0,
    0x0,
  }, {
    "\0"
  }
};

// System boot info
STATIC CONST ARM_TYPE32 mArmDefaultType32 = {
  {
    { // SMBIOS_STRUCTURE Hdr
      EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION,
      sizeof (SMBIOS_TABLE_TYPE32),
      SMBIOS_HANDLE_PI_RESERVED,
    },
    {},
    BootInformationStatusNoError,
  }, {
    "\0"
  }
};

STATIC SMBIOS_STRUCTURE * CONST FixedTables[] = {
  (SMBIOS_STRUCTURE *)&mArmDefaultType0.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType1.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType3.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType7_l1i.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType7_l1d.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType7_l2.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType7_l3.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType4.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType9_0.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType9_1.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType9_2.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType16.Base.Hdr,
  (SMBIOS_STRUCTURE *)&mArmDefaultType32.Base.Hdr,
};

STATIC
UINT16
GetPrimaryBusWidth (
  IN UINT8           SpdPrimaryBusWidth
  )
{
  UINT16                    PrimaryBusWidth;

  switch (SpdPrimaryBusWidth) {
  case SPD4_MEM_BUS_WIDTH_8BIT:
    PrimaryBusWidth = 8;
    break;
  case SPD4_MEM_BUS_WIDTH_16BIT:
    PrimaryBusWidth = 16;
    break;
  case SPD4_MEM_BUS_WIDTH_32BIT:
    PrimaryBusWidth = 32;
    break;
  case SPD4_MEM_BUS_WIDTH_64BIT:
    PrimaryBusWidth = 64;
    break;
  default:
    PrimaryBusWidth = 0xFFFF;
    ASSERT(FALSE);
    break;
  }

  return PrimaryBusWidth;
}

STATIC
UINT16
GetSdramDeviceWidth (
  IN UINT8            SpdSdramDeviceWidth
  )
{
  UINT16                    SdramDeviceWidth;

  switch (SpdSdramDeviceWidth) {
  case SPD4_MEM_DEV_WIDTH_4BIT:
    SdramDeviceWidth = 4;
    break;
  case SPD4_MEM_DEV_WIDTH_8BIT:
    SdramDeviceWidth = 8;
    break;
  case SPD4_MEM_DEV_WIDTH_16BIT:
    SdramDeviceWidth = 16;
    break;
  case SPD4_MEM_DEV_WIDTH_32BIT:
    SdramDeviceWidth = 32;
    break;
  default:
    SdramDeviceWidth = 0;
    ASSERT(FALSE);
    break;
  }

  return SdramDeviceWidth;
}

STATIC
UINT16
CalculateModuleDramCapacityMB (
 IN SPD4_BASE_SECTION            *Spd4Base
 )
{
  UINT32                    SdramCapacityMbit;
  UINT16                    PrimaryBusWidth;
  UINT8                     SdramDeviceWidth;
  UINT8                     SdramDeviceWidthShiftNum;
  UINT8                     RankCount;
  UINT16                    DramSize;
  UINT32                    Index;

  SdramCapacityMbit = 0;

  for (Index = 0; CapacityTable[Index].Capacity != SPD4_SDRAM_CAPACITY_INVALID; Index++) {
    if (Spd4Base->SdramDensityAndBanks.Bits.Density == CapacityTable[Index].Capacity) {
      SdramCapacityMbit = CapacityTable[Index].SizeMbit;
      break;
    }
  }

  PrimaryBusWidth = GetPrimaryBusWidth (Spd4Base->ModuleMemoryBusWidth.Bits.PrimaryBusWidth);
  SdramDeviceWidth = GetSdramDeviceWidth (Spd4Base->ModuleOrganization.Bits.SdramDeviceWidth);
  RankCount = Spd4Base->ModuleOrganization.Bits.RankCount + 1;

  if ((SdramCapacityMbit == 0) || (PrimaryBusWidth == 0xFFFF) ||
      (SdramDeviceWidth == 0) || RankCount == 0) {
    DEBUG ((DEBUG_ERROR, "Calculate DRAM size failed. Cap:%d, BusWidth:%d, "
                         "DevWidth:%d, Rank:%d\n", SdramCapacityMbit,
                         PrimaryBusWidth, SdramDeviceWidth, RankCount));
    return 0;
  }

  //
  //Total[MB] = SDRAM Capacity[Mb] / 8 * Primary Bus Width /
  //            SDRAM Width * Logical Ranks per DIMM
  //
  switch (SdramDeviceWidth) {
  case 4:
    SdramDeviceWidthShiftNum = 2;
    break;
  case 8:
    SdramDeviceWidthShiftNum = 3;
    break;
  case 16:
    SdramDeviceWidthShiftNum = 4;
    break;
  case 32:
    SdramDeviceWidthShiftNum = 5;
    break;
  default:
    SdramDeviceWidthShiftNum = 0;
    ASSERT(FALSE);
    break;
  };
  DramSize = (((SdramCapacityMbit >> 3) * PrimaryBusWidth) >> SdramDeviceWidthShiftNum) * RankCount;

  return DramSize;
}

STATIC
VOID
GetManufacturerName (
  IN UINT16           SpdManufacturerId,
  OUT CHAR8           *ManufacturerStr
  )
{
  UINT16                    ManufacturerId;
  UINT32                    Index;
  RETURN_STATUS             RetStatus;

  ManufacturerId = SwapBytes16 (SpdManufacturerId);
  ManufacturerId &= 0x7FFF; // ignore odd parity bit

  for (Index = 0; Manufacturer[Index].ManufacturerId != 0xFFFF; Index++) {
    if (ManufacturerId == Manufacturer[Index].ManufacturerId) {
      RetStatus = AsciiStrCpyS (ManufacturerStr, TYPE17_MANUFACTURER_NAME_LEN,
                                Manufacturer[Index].ManufacturerName);
      ASSERT_RETURN_ERROR (RetStatus);
      return;
    }
  }

  RetStatus = AsciiStrCpyS (ManufacturerStr, TYPE17_MANUFACTURER_NAME_LEN,
                            Manufacturer[Index].ManufacturerName);
  ASSERT_RETURN_ERROR (RetStatus);
}

STATIC
BOOLEAN
IsValidSPD (
  IN SPD4_BASE_SECTION            *Spd4Base
  )
{
  //
  // Developerbox only supports DDR4.
  // If the device type is not DDR4 SDRAM, the SPD is invalid
  //
  if (Spd4Base->DramDeviceType.Bits.Type == SPD4_DDR4_SDRAM_TYPE) {
    return TRUE;
  } else {
    return FALSE;
  }
}

STATIC
EFI_STATUS
InstallMemoryDeviceStructure (
  VOID
  )
{
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  ARM_TYPE17                *Descriptor;
  SPD_DDR4                  *Spd;
  UINT8                     Slot;
  CHAR8                     *StringPtr;

  CHAR8                     DeviceLocatorStr[TYPE17_DEVICE_LOCATOR_LEN];
  CHAR8                     BankLocatorStr[TYPE17_BANK_LOCATOR_LEN];
  CHAR8                     ManufacturerStr[TYPE17_MANUFACTURER_NAME_LEN];
  CHAR8                     SerialNumberStr[TYPE17_SERIAL_NUMBER_LEN];
  CHAR8                     PartNumberStr[TYPE17_MODULE_PART_NUMBER_LEN];
  CHAR8                     FirmwareVersionStr[TYPE17_FIRMWARE_VERSION_LEN];

  Descriptor = AllocateZeroPool(sizeof (ARM_TYPE17));
  if (Descriptor == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Spd = (SPD_DDR4 *) FixedPcdGet32 (PcdStoredSpdDDR4Address);

  for (Slot = 0; Slot < 4; Slot++, Spd++) {
    SetMem (Descriptor, sizeof (ARM_TYPE17), 0);
    // fill fixed parameters
    Descriptor->Base.Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
    Descriptor->Base.Hdr.Length = sizeof (SMBIOS_TABLE_TYPE17);
    Descriptor->Base.Hdr.Handle = SMBIOS_HANDLE_PI_RESERVED;
    Descriptor->Base.MemoryArrayHandle = SMBIOS_HANDLE_MEMORY;
    Descriptor->Base.MemoryErrorInformationHandle = 0xFFFE;
    Descriptor->Base.ExtendedSize = 0;
    Descriptor->Base.ExtendedSpeed = 0;
    Descriptor->Base.ExtendedConfiguredMemorySpeed = 0;
    Descriptor->Base.MemoryOperatingModeCapability.Bits.VolatileMemory = 1;
    Descriptor->Base.DeviceSet = 1;
    Descriptor->Base.MemoryType = MemoryTypeDdr4;
    Descriptor->Base.DeviceLocator = 1;
    Descriptor->Base.BankLocator = 2;

    AsciiSPrint (DeviceLocatorStr, sizeof(DeviceLocatorStr), "DIMM %d\0", (Slot + 1));
    AsciiSPrint (BankLocatorStr, sizeof(BankLocatorStr), "CHANNEL %d SLOT %d\0", (Slot / 2), (Slot % 2));

    StringPtr = Descriptor->Strings;
    AsciiStrnCpyS (StringPtr, TYPE17_DEVICE_LOCATOR_LEN, DeviceLocatorStr,
                   TYPE17_DEVICE_LOCATOR_LEN - 1);
    StringPtr += AsciiStrSize (StringPtr);
    AsciiStrnCpyS (StringPtr, TYPE17_BANK_LOCATOR_LEN, BankLocatorStr,
                   TYPE17_BANK_LOCATOR_LEN - 1);
    StringPtr += AsciiStrSize (StringPtr);

    if (IsValidSPD ((SPD4_BASE_SECTION *)Spd)) {
      Descriptor->Base.Manufacturer = 3;
      Descriptor->Base.SerialNumber = 4;
      Descriptor->Base.AssetTag = 5;
      Descriptor->Base.PartNumber = 6;
      Descriptor->Base.FirmwareVersion = 7;

      Descriptor->Base.TotalWidth = Descriptor->Base.DataWidth =
        GetPrimaryBusWidth (Spd->Base.ModuleMemoryBusWidth.Bits.PrimaryBusWidth);
      if (Spd->Base.ModuleMemoryBusWidth.Bits.BusWidthExtension) {
        if (Descriptor->Base.TotalWidth != 0xFFFF) {
          Descriptor->Base.TotalWidth += 8;
        }
      }

      Descriptor->Base.Size = CalculateModuleDramCapacityMB ((SPD4_BASE_SECTION *)Spd);

      switch (Spd->Base.ModuleType.Bits.ModuleType) {
      case SPD4_MEM_MODULE_TYPE_RDIMM:
        Descriptor->Base.FormFactor = 0x09;
        Descriptor->Base.TypeDetail.Registered = 1;
        Descriptor->Base.TypeDetail.Unbuffered = 0;
        break;
      case SPD4_MEM_MODULE_TYPE_UDIMM:
        Descriptor->Base.FormFactor = 0x09;
        Descriptor->Base.TypeDetail.Registered = 0;
        Descriptor->Base.TypeDetail.Unbuffered = 1;
        break;
      case SPD4_MEM_MODULE_TYPE_SODIMM:
        Descriptor->Base.FormFactor = 0x0D;
        Descriptor->Base.TypeDetail.Registered = 0;
        Descriptor->Base.TypeDetail.Unbuffered = 1;
        break;
      default:
        Descriptor->Base.FormFactor = 0x01;
        Descriptor->Base.TypeDetail.Registered = 0;
        Descriptor->Base.TypeDetail.Unbuffered = 0;
        break;
      }

      Descriptor->Base.Attributes = Spd->Base.ModuleOrganization.Bits.RankCount + 1;
      Descriptor->Base.Speed = 2133;
      Descriptor->Base.ConfiguredMemoryClockSpeed = 2133;
      Descriptor->Base.MinimumVoltage = 1200;
      Descriptor->Base.MaximumVoltage = 1200;
      Descriptor->Base.ConfiguredVoltage = 1200;
      Descriptor->Base.MemoryTechnology = MemoryTechnologyDram;
      Descriptor->Base.ModuleManufacturerID = Spd->ManufactureInfo.ModuleId.IdCode.Data;

      GetManufacturerName (Spd->ManufactureInfo.ModuleId.IdCode.Data, ManufacturerStr);
      AsciiSPrint (SerialNumberStr, TYPE17_SERIAL_NUMBER_LEN, "0x%08X\0",
                   SwapBytes32(Spd->ManufactureInfo.ModuleId.SerialNumber.Data));
      //
      // Module part number is not null terminated string in SPD DDR4,
      // unused digits are coded as ASCII blanks(0x20).
      //
      ZeroMem(PartNumberStr, TYPE17_MODULE_PART_NUMBER_LEN);
      CopyMem (PartNumberStr,
               (CHAR8 *)Spd->ManufactureInfo.ModulePartNumber.ModulePartNumber,
               TYPE17_MODULE_PART_NUMBER_LEN - 1);

      AsciiSPrint (FirmwareVersionStr, sizeof(FirmwareVersionStr), "%d\0", FixedPcdGet32 (PcdFirmwareRevision));

      AsciiStrnCpyS (StringPtr, TYPE17_MANUFACTURER_NAME_LEN, ManufacturerStr,
                     TYPE17_MANUFACTURER_NAME_LEN - 1);
      StringPtr += AsciiStrSize (StringPtr);
      AsciiStrnCpyS (StringPtr, TYPE17_SERIAL_NUMBER_LEN, SerialNumberStr,
                     TYPE17_SERIAL_NUMBER_LEN - 1);
      StringPtr += AsciiStrSize (StringPtr);
      AsciiStrnCpyS (StringPtr, TYPE17_ASSETTAG_LEN, "-\0",
                     TYPE17_ASSETTAG_LEN - 1);
      StringPtr += AsciiStrSize (StringPtr);
      AsciiStrnCpyS (StringPtr, TYPE17_MODULE_PART_NUMBER_LEN, PartNumberStr,
                     TYPE17_MODULE_PART_NUMBER_LEN - 1);
      StringPtr += AsciiStrSize (StringPtr);
      AsciiStrnCpyS (StringPtr, TYPE17_FIRMWARE_VERSION_LEN, FirmwareVersionStr,
                     TYPE17_FIRMWARE_VERSION_LEN - 1);
    } else {
      Descriptor->Base.TotalWidth = 0xFFFF;
      Descriptor->Base.DataWidth = 0xFFFF;
      Descriptor->Base.Size = 0;
      Descriptor->Base.FormFactor = 0x09;
      Descriptor->Base.DeviceSet = 1;
      Descriptor->Base.Attributes = 0;
      Descriptor->Base.Speed = 0;
      Descriptor->Base.ConfiguredMemoryClockSpeed = 0;
      Descriptor->Base.MinimumVoltage = 0;
      Descriptor->Base.MaximumVoltage = 0;
      Descriptor->Base.ConfiguredVoltage = 0;
    }

    SmbiosHandle = Descriptor->Base.Hdr.Handle;
    mSmbios->Add (mSmbios, NULL, &SmbiosHandle, &Descriptor->Base.Hdr);
  }

  FreePool (Descriptor);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InstallMemoryStructure (
  IN UINT64                    StartingAddress,
  IN UINT64                    RegionLength
  )
{
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  CHAR8                     Buffer[sizeof (mArmDefaultType19)];
  ARM_TYPE19                *Descriptor;

  CopyMem (Buffer, &mArmDefaultType19, sizeof (Buffer));

  Descriptor = (ARM_TYPE19 *)Buffer;
  Descriptor->Base.ExtendedStartingAddress = StartingAddress;
  Descriptor->Base.ExtendedEndingAddress = StartingAddress + RegionLength;
  SmbiosHandle = Descriptor->Base.Hdr.Handle;

  return mSmbios->Add (mSmbios, NULL, &SmbiosHandle, &Descriptor->Base.Hdr);
}


STATIC
VOID
InstallAllStructures (
   VOID
   )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  UINTN                     Idx;
  EFI_PEI_HOB_POINTERS      Hob;

  for (Idx = 0; Idx < ARRAY_SIZE (FixedTables); Idx++) {
    SmbiosHandle = FixedTables[Idx]->Handle;
    Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, FixedTables[Idx]);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_WARN, "%a: failed to add SMBIOS type %u table - %r\n",
        __func__, FixedTables[Idx]->Type, Status));
      break;
    }
  }

  //
  // SPD_DDR4 data is stored in Non Secure SRAM by SCP-firmware.
  // Install Type17 record by analyzing SPD_DDR4 information.
  //
  Status = InstallMemoryDeviceStructure();
  if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_WARN, "%a: failed to add SMBIOS type 17 table - %r\n",
        __func__, Status));
  }

  for (Hob.Raw = GetHobList ();
       !END_OF_HOB_LIST (Hob);
       Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR &&
        Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      Status = InstallMemoryStructure (Hob.ResourceDescriptor->PhysicalStart,
                                       Hob.ResourceDescriptor->ResourceLength);
      if (EFI_ERROR(Status)) {
        DEBUG ((DEBUG_WARN, "%a: failed to add SMBIOS type 19 table - %r\n",
          __func__, Status));
        break;
      }
    }
  }
}

/**
   Installs SMBIOS information for SynQuacer platform

   @param ImageHandle     Module's image handle
   @param SystemTable     Pointer of EFI_SYSTEM_TABLE

   @retval EFI_SUCCESS    Smbios data successfully installed
   @retval Other          Smbios data was not installed

**/
EFI_STATUS
EFIAPI
SmbiosPlatformDxeEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                Status;

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL,
                  (VOID **)&mSmbios);
  ASSERT_EFI_ERROR (Status);

  InstallAllStructures ();

  return EFI_SUCCESS;
}
