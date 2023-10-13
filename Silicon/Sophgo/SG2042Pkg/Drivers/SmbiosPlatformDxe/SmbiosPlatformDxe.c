/** @file
  This driver installs SMBIOS information for Sophgo SG2042EVB platforms

  Copyright (c) 2015-2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Smbios.h>

#include <IndustryStandard/SmBios.h>

//
// SMBIOS tables often reference each other using
// fixed constants, define a list of these constants
// for our hardcoded tables
//
enum SMBIOS_REFRENCE_HANDLES {
  SMBIOS_HANDLE_L1I = 0x1000,
  SMBIOS_HANDLE_L1D,
  SMBIOS_HANDLE_L2,
  SMBIOS_HANDLE_L3,
  SMBIOS_HANDLE_MOTHERBOARD,
  SMBIOS_HANDLE_CHASSIS,
  SMBIOS_HANDLE_CLUSTER,
  SMBIOS_HANDLE_MEMORY,
  SMBIOS_HANDLE_DIMM
};

//
// Type definition and contents of the default SMBIOS table.
// This table covers only the minimum structures required by
// the SMBIOS specification (section 6.2, version 3.0)
//

// BIOS information (section 7.1)
STATIC SMBIOS_TABLE_TYPE0 mSG2042EVBType0 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_BIOS_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE0),      // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  1,                                  // SMBIOS_TABLE_STRING       Vendor
  2,                                  // SMBIOS_TABLE_STRING       BiosVersion
  0xE800,                             // UINT16                    BiosSegment
  3,                                  // SMBIOS_TABLE_STRING       BiosReleaseDate
  0,                                  // UINT8                     BiosSize
  {                                   // BiosCharacteristics
    0,                                // Reserved                          :2
    0,                                // Unknown                           :1
    0,                                // BiosCharacteristicsNotSupported   :1
    0,                                // IsaIsSupported                    :1
    0,                                // McaIsSupported                    :1
    0,                                // EisaIsSupported                   :1
    1,                                // PciIsSupported                    :1
    0,                                // PcmciaIsSupported                 :1
    0,                                // PlugAndPlayIsSupported            :1
    0,                                // ApmIsSupported                    :1
    1,                                // BiosIsUpgradable                  :1
    0,                                // BiosShadowingAllowed              :1
    0,                                // VlVesaIsSupported                 :1
    0,                                // EscdSupportIsAvailable            :1
    1,                                // BootFromCdIsSupported             :1
    1,                                // SelectableBootIsSupported         :1
    0,                                // RomBiosIsSocketed                 :1
    0,                                // BootFromPcmciaIsSupported         :1
    0,                                // EDDSpecificationIsSupported       :1
    0,                                // JapaneseNecFloppyIsSupported      :1
    0,                                // JapaneseToshibaFloppyIsSupported  :1
    0,                                // Floppy525_360IsSupported          :1
    0,                                // Floppy525_12IsSupported           :1
    0,                                // Floppy35_720IsSupported           :1
    0,                                // Floppy35_288IsSupported           :1
    0,                                // PrintScreenIsSupported            :1
    0,                                // Keyboard8042IsSupported           :1
    0,                                // SerialIsSupported                 :1
    0,                                // PrinterIsSupported                :1
    0,                                // CgaMonoIsSupported                :1
    0,                                // NecPc98                           :1
    0                                 // ReservedForVendor                 :3
  },
  {
    0x3,                              // BIOSCharacteristicsExtensionBytes[0]
    0xC,                              // BIOSCharacteristicsExtensionBytes[1]
  },
  0xFF,                               // UINT8                     SystemBiosMajorRelease
  0xFF,                               // UINT8                     SystemBiosMinorRelease
  0xFF,                               // UINT8                     EmbeddedControllerFirmwareMajorRelease
  0xFF,                               // UINT8                     EmbeddedControllerFirmwareMinorRelease
};

STATIC CHAR8 CONST *mSG2042EVBType0Strings[] = {
  "EFI Development Kit II / Sophgo\0",            /* Vendor */
  "V1.0\0",                                       /* BiosVersion */
  __DATE__"\0",                                   /* BiosReleaseDate */
  NULL
};


// System information (section 7.2)
STATIC SMBIOS_TABLE_TYPE1 mSG2042EVBType1 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_SYSTEM_INFORMATION,
    sizeof (SMBIOS_TABLE_TYPE1),
    SMBIOS_HANDLE_PI_RESERVED,
  },
  1,                                   // Manufacturer
  2,                                   // Product Name
  3,                                   // Version
  4,                                   // Serial
  { 0x9987FD42, 0x907E, 0x5446, { 0x1D,0x7D,0x7D,0xA0,0x10,0x9F,0x60,0xA1 }},    //UUID
  6,     //Wakeup type
  0,     //SKU
  0,     //Family
};

STATIC CHAR8 CONST *mSG2042EVBType1Strings[] = {
  "SOPHGO\0",                              /* Manufacturer */
  "Sophgo SG2042 EVB Board\0",             /* Product Name */
  "None\0",                                /* Version */
  "Not Set\0",                             /* Serial */
  NULL
};


// Baseboard (section 7.3)
STATIC SMBIOS_TABLE_TYPE2 mSG2042EVBType2 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE2),           // UINT8 Length
    SMBIOS_HANDLE_MOTHERBOARD,
  },
  1,                                       // Manufacturer
  2,                                       // Product Name
  3,                                       // Version
  4,                                       // Serial
  0,                                       // Asset tag
  {1},                                     // motherboard, not replaceable
  5,                                       // location of board
  SMBIOS_HANDLE_CHASSIS,
  BaseBoardTypeMotherBoard,
  1,
  {SMBIOS_HANDLE_CLUSTER},
};

STATIC CHAR8 CONST *mSG2042EVBType2Strings[] = {
  "SOPHGO\0",                             /* Manufacturer */
  "Sophgo SG2042 EVB Board\0",            /* Product Name */
  "None\0",                               /* Version */
  "Not Set\0",                            /* Serial */
  "Not Set\0",                            /* BaseBoardAssetTag */
  "Not Set\0",                            /* BaseBoardChassisLocation */
  NULL
};


// Enclosure
STATIC SMBIOS_TABLE_TYPE3 mSG2042EVBType3 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE3),      // UINT8 Length
    SMBIOS_HANDLE_CHASSIS,
  },
  1,                                  // Manufacturer
  2,                                  // enclosure type
  2,                                  // version
  3,                                  // serial
  0,                                  // asset tag
  ChassisStateUnknown,                // boot chassis state
  ChassisStateSafe,                   // power supply state
  ChassisStateSafe,                   // thermal state
  ChassisSecurityStatusNone,          // security state
  {0,0,0,0,},                         // OEM defined
  1,                                  // 1U height
  1,                                  // number of power cords
  0,                                  // no contained elements
};

STATIC CHAR8 CONST *mSG2042EVBType3Strings[] = {
  "SOPHGO\0",                             /* Manufacturer */
  "None\0",                               /* Version */
  "Not Set\0",                            /* Serial */
  "Not Set\0",                            /* AssetTag */
  NULL
};


// Processor
STATIC SMBIOS_TABLE_TYPE4 mSG2042EVBType4 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE4),           // UINT8 Length
    SMBIOS_HANDLE_CLUSTER,
  },
  1,                                       // socket type
  3,                                       // processor type CPU
  ProcessorFamilyIndicatorFamily2,         // processor family, acquire from field2
  2,                                       // manufactuer
  {{0,},{0.}},                             // processor id
  3,                                       // version
  {0,0,0,0,0,1},                           // voltage
  0,                                       // external clock
  2200,                                    // max speed
  0,                                       // current speed - requires update
  0x41,                                    // status
  ProcessorUpgradeOther,
  SMBIOS_HANDLE_L1I,                       // l1 cache handle
  SMBIOS_HANDLE_L2,                        // l2 cache handle
  SMBIOS_HANDLE_L3,                        // l3 cache handle
  0,                                       // serial not set
  0,                                       // asset not set
  4,                                       // part number
  64,                                      // core count in socket
  64,                                      // enabled core count in socket
  64,                                      // threads per socket
  0xEC,                                    // processor characteristics
  ProcessorFamilyRiscVRV64,                // RISC-V core
  0,                                       // CoreCount2;
  0,                                       // EnabledCoreCount2;
  0,                                       // ThreadCount2;
};

STATIC CHAR8 CONST *mSG2042EVBType4Strings[] = {
  "Socket type unknown\0",               /* Socket type placeholder */
  "SOPHGO\0",                            /* manufactuer */
  "SG2042\0",                            /* processor description */
  "Not Set\0",                           /* SerialNumber */
  NULL
};

// Cache
STATIC SMBIOS_TABLE_TYPE7 mSG2042EVBType7_l1i = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
    SMBIOS_HANDLE_L1I,
  },
  1,
  0x380,                              // L1 enabled, unknown WB
  48,                                 // 48k I-cache max
  48,                                 // 48k installed
  {0,1},                              // SRAM type
  {0,1},                              // SRAM type
  0,                                  // speed unknown
  CacheErrorParity,                   // parity checking
  CacheTypeInstruction,               // instruction cache
  CacheAssociativityOther,            // three way
  // SMBIOS 3.1.0 fields
  48,                                 //48k I-cache max
  48,                                 //48k installed
};

STATIC SMBIOS_TABLE_TYPE7 mSG2042EVBType7_l1d = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
    SMBIOS_HANDLE_L1D,
  },
  2,
  0x380,                               // L1 enabled, unknown WB
  32,                                  // 32k D-cache max
  32,                                  // 32k installed
  {0,1},                               // SRAM type
  {0,1},                               // SRAM type
  0,                                   // speed unknown
  CacheErrorSingleBit,                 // ECC checking
  CacheTypeData,                       // data cache
  CacheAssociativity2Way,              // two way
  // SMBIOS 3.1.0 fields
  32,                                  // 32k D-cache max
  32,                                  // 32k installed
};

STATIC SMBIOS_TABLE_TYPE7 mSG2042EVBType7_l2 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
    SMBIOS_HANDLE_L2,
  },
  3,
  0x181,                               // L2 enabled, WB
  512,                                 // 512k D-cache max
  512,                                 // 512k installed
  {0,1},                               // SRAM type
  {0,1},                               // SRAM type
  0,                                   // speed unknown
  CacheErrorSingleBit,                 // ECC checking
  CacheTypeUnified,                    // instruction cache
  CacheAssociativity16Way,             // 16 way associative
  // SMBIOS 3.1.0 fields
  512,                                 // 512k D-cache max
  512,                                 // 512k installed
};

STATIC SMBIOS_TABLE_TYPE7 mSG2042EVBType7_l3 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_CACHE_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE7),       // UINT8 Length
    SMBIOS_HANDLE_L3,
  },
  4,
  0x182,                               // L3 enabled, WB
  1024,                                // 1M cache max
  1024,                                // 1M installed
  {0,1},                               // SRAM type
  {0,1},                               // SRAM type
  0,                                   // speed unknown
  CacheErrorSingleBit,                 // ECC checking
  CacheTypeUnified,                    // instruction cache
  CacheAssociativity8Way,              // 8 way associative
  // SMBIOS 3.1.0 fields
  1024,                                // 1M cache max
  1024,                                // 1M installed
};

STATIC CONST CHAR8 *mSG2042EVBType7Strings[] = {
  "L1 Instruction\0",                 /* L1I  */
  "L1 Data\0",                        /* L1D  */
  "L2\0",                             /* L2   */
  "L3\0",                             /* L3   */
  NULL
};

// Slots
STATIC SMBIOS_TABLE_TYPE9 mSG2042EVBType9_0 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_INACTIVE,     // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE9),  // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  1,
  SlotTypePciExpressX16,
  SlotDataBusWidth16X,
  SlotUsageUnknown,
  SlotLengthLong,
  0,
  {1},                             // Unknown
  {1,0,1},                         // PME and SMBUS
  0,
  0,
  1,
};

STATIC SMBIOS_TABLE_TYPE9 mSG2042EVBType9_1 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_INACTIVE,     // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE9),  // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  2,
  SlotTypePciExpressGen2X8,
  SlotDataBusWidth8X,
  SlotUsageUnknown,
  SlotLengthLong,
  0,
  {1},
  {1,0,1},                         // PME and SMBUS
  0,
  0,
  2,
};

STATIC SMBIOS_TABLE_TYPE9 mSG2042EVBType9_2 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_INACTIVE,     // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE9),  // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  3,
  SlotTypePciExpressGen2X8,
  SlotDataBusWidth8X,
  SlotUsageUnknown,
  SlotLengthLong,
  0,
  {1},
  {1,0,1},                        // PME and SMBUS
  0,
  0,
  3,
};

STATIC CHAR8 CONST *mSG2042EVBType9Strings[] = {
  "PCIE0 ExpressX16            \0",/* Slot0 */
  "PCIE1 ExpressGen2X8         \0",/* Slot1 */
  "PCIE1 ExpressGen2X8         \0",/* Slot2 */
  NULL
};

// Memory array
STATIC SMBIOS_TABLE_TYPE16 mSG2042EVBType16 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE16),          // UINT8 Length
    SMBIOS_HANDLE_MEMORY,
  },
  MemoryArrayLocationSystemBoard,          // on motherboard
  MemoryArrayUseSystemMemory,              // system RAM
  MemoryErrorCorrectionNone,               // ECC RAM
  0x1000000,                               // 16GB
  0xFFFE,                                  // No error information structure
  0x1,                                     // soldered memory
};

STATIC CHAR8 CONST *mSG2042EVBType16Strings[] = {
  NULL
};

// Memory device
STATIC SMBIOS_TABLE_TYPE17 mSG2042EVBType17 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_MEMORY_DEVICE, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE17),  // UINT8 Length
    SMBIOS_HANDLE_DIMM,
  },
  SMBIOS_HANDLE_MEMORY,            // array to which this module belongs
  0xFFFE,                          // no errors
  64,                              // single DIMM, no ECC is 64bits (for ecc this would be 72)
  32,                              // data width of this device (32-bits)
  0,                               // Memory size obtained dynamically
  MemoryFormFactorRowOfChips,      // Memory factor
  0,                               // Not part of a set
  1,                               // Location
  2,                               // Bank 0
  MemoryTypeDdr4,                  // DDR4
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // unbuffered
  0,                               // DRAM speed - requires update
  0,                               // varies between diffrent production runs
  0,                               // serial
  0,                               // asset tag
  0,                               // part number
  0,                               // rank
  0,                               // ExtendedSize; (since Size < 32GB-1)
  0,                               // ConfiguredMemoryClockSpeed - initialized at runtime
  0,                               // MinimumVoltage; (unknown)
  0,                               // MaximumVoltage; (unknown)
  0,                               // ConfiguredVoltage; (unknown)
  MemoryTechnologyDram,            // MemoryTechnology
  {{                               // MemoryOperatingModeCapability
    0,                             // Reserved                        :1;
    0,                             // Other                           :1;
    0,                             // Unknown                         :1;
    1,                             // VolatileMemory                  :1;
    0,                             // ByteAccessiblePersistentMemory  :1;
    0,                             // BlockAccessiblePersistentMemory :1;
    0                              // Reserved                        :10;
  }},
  0,                               // FirwareVersion
  0,                               // ModuleManufacturerID (unknown)
  0,                               // ModuleProductID (unknown)
  0,                               // MemorySubsystemControllerManufacturerID (unknown)
  0,                               // MemorySubsystemControllerProductID (unknown)
  0,                               // NonVolatileSize
  0,                               // VolatileSize - initialized at runtime
  0,                               // CacheSize
  0,                               // LogicalSize
  0,                               // ExtendedSpeed,
  0                                // ExtendedConfiguredMemorySpeed
};

STATIC CHAR8 CONST *mSG2042EVBType17Strings[] = {
  "DIMM SLOT\0",                      /* location */
  "BANK 0\0",                         /* bank description */
  NULL
};

//
// Memory array mapped address, this structure
// is overridden by SmbiosInstallMemoryStructure.
//
STATIC SMBIOS_TABLE_TYPE19 mSG2042EVBType19 = {
  {  // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE19),                // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  0xFFFFFFFF,                                    // invalid, look at extended addr field
  0xFFFFFFFF,
  SMBIOS_HANDLE_DIMM,                            // handle
  1,
  0x080000000,                                   // starting addr of first 2GB
  0x100000000,                                   // ending addr of first 2GB
};

// System boot infomSG2042EVBType4.
STATIC SMBIOS_TABLE_TYPE32 mSG2042EVBType32 = {
  { // SMBIOS_STRUCTURE Hdr
    EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE32),            // UINT8 Length
    SMBIOS_HANDLE_PI_RESERVED,
  },
  {0, 0, 0, 0, 0, 0},                        // reserved
  BootInformationStatusNoError,
};

STATIC CHAR8 CONST *mSG2042EVBType32Strings[] = {
  NULL
};

STATIC CONST VOID *DefaultCommonTables[][2] =
{
  { &mSG2042EVBType0,         mSG2042EVBType0Strings  },
  { &mSG2042EVBType1,         mSG2042EVBType1Strings  },
  { &mSG2042EVBType2,         mSG2042EVBType2Strings  },
  { &mSG2042EVBType3,         mSG2042EVBType3Strings  },
  { &mSG2042EVBType4,         mSG2042EVBType4Strings  },
  { &mSG2042EVBType7_l1i,     mSG2042EVBType7Strings  },
  { &mSG2042EVBType7_l1d,     mSG2042EVBType7Strings  },
  { &mSG2042EVBType7_l2,      mSG2042EVBType7Strings  },
  { &mSG2042EVBType7_l3,      mSG2042EVBType7Strings  },
  { &mSG2042EVBType9_0,       mSG2042EVBType9Strings  },
  { &mSG2042EVBType9_1,       mSG2042EVBType9Strings  },
  { &mSG2042EVBType9_2,       mSG2042EVBType9Strings  },
  { &mSG2042EVBType16,        mSG2042EVBType16Strings },
  { &mSG2042EVBType17,        mSG2042EVBType17Strings },
  { &mSG2042EVBType32,        mSG2042EVBType32Strings },
  { NULL,                     NULL                    },
};

/**

  Create SMBIOS record.

  Converts a fixed SMBIOS structure and an array of pointers to strings into
  an SMBIOS record where the strings are cat'ed on the end of the fixed record
  and terminated via a double NULL and add to SMBIOS table.

  SMBIOS_TABLE_TYPE32 gSmbiosType12 = {
    { EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS, sizeof (SMBIOS_TABLE_TYPE12), 0 },
    1 // StringCount
  };

  CHAR8 *gSmbiosType12Strings[] = {
    "Not Found",
    NULL
  };

  ...

  LogSmbiosData (
    (EFI_SMBIOS_TABLE_HEADER*)&gSmbiosType12,
    gSmbiosType12Strings
    );

  @param  Smbios      SMBIOS protocol
  @param  Template    Fixed SMBIOS structure, required.
  @param  StringArray Array of strings to convert to an SMBIOS string pack.
                      NULL is OK.
**/
STATIC
EFI_STATUS
EFIAPI
LogSmbiosData (
  IN  EFI_SMBIOS_PROTOCOL     *Smbios,
  IN  EFI_SMBIOS_TABLE_HEADER *Template,
  IN  CONST CHAR8 * CONST     *StringArray
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  UINTN                     Index;
  UINTN                     StringSize;
  UINTN                     Size;
  CHAR8                     *Str;


  // Calculate the size of the fixed record and optional string pack
  Size = Template->Length;
  if (StringArray == NULL) {
    // At least a double null is required
    Size += 1;
  } else {
    for (Index = 0; StringArray[Index] != NULL; Index++) {
      StringSize = AsciiStrSize (StringArray[Index]);
      Size += StringSize;
    }
    if (StringArray[0] == NULL) {
      // At least a double null is required
      Size += 1;
    }

    // Don't forget the terminating double null
    Size += 1;
  }

  // Copy over Template
  Record = (EFI_SMBIOS_TABLE_HEADER *)AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (Record, Template, Template->Length);

  // Append string pack
  Str = (CHAR8*)((UINTN)Record + Record->Length);
  for (Index = 0; StringArray[Index] != NULL; Index++) {
    StringSize = AsciiStrSize (StringArray[Index]);
    CopyMem (Str, StringArray[Index], StringSize);
    Str += StringSize;
  }
  *Str = 0;

  Status = Smbios->Add (Smbios,
                     NULL,
                     &Record->Handle,
                     Record);
  ASSERT_EFI_ERROR (Status);

  FreePool (Record);

  return Status;
}

/**
   Installs a memory descriptor (type19) for the given address range

   @param  Smbios               SMBIOS protocol.
   @param  StartingAddress      Start address of the memory chunk.
   @param  RegionLength         Memory chunk size.

**/
EFI_STATUS
SmbiosInstallMemoryStructure (
  IN EFI_SMBIOS_PROTOCOL       *Smbios,
  IN UINT64                    StartingAddress,
  IN UINT64                    RegionLength
  )
{
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  SMBIOS_TABLE_TYPE19       MemoryDescriptor;
  EFI_STATUS                Status;

  Status = EFI_SUCCESS;

  CopyMem (&MemoryDescriptor,
    &mSG2042EVBType19,
    sizeof (SMBIOS_TABLE_TYPE19));

  MemoryDescriptor.ExtendedStartingAddress = StartingAddress;
  MemoryDescriptor.ExtendedEndingAddress = StartingAddress + RegionLength;
  SmbiosHandle = MemoryDescriptor.Hdr.Handle;

  Status = Smbios->Add (Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER*) &MemoryDescriptor);

  return Status;
}

/**
   Install a whole table worth of structructures

   @param  Smbios               SMBIOS protocol.
   @param  DefaultTables        A pointer to the default SMBIOS table structure.

**/
EFI_STATUS
SmbiosInstallStructures (
   IN EFI_SMBIOS_PROTOCOL   *Smbios,
   IN CONST VOID            *DefaultTables[][2]
   )
{
    EFI_STATUS                Status;
    INTN TableEntry;

    Status = EFI_SUCCESS;

    for (TableEntry = 0; DefaultTables[TableEntry][0] != NULL; TableEntry++) {
      // Omit disabled tables
      if (((EFI_SMBIOS_TABLE_HEADER *)DefaultTables[TableEntry][0])->Type ==
          EFI_SMBIOS_TYPE_INACTIVE) {
        continue;
      }

      Status = LogSmbiosData (Smbios,
                 ((EFI_SMBIOS_TABLE_HEADER *)DefaultTables[TableEntry][0]),
                 DefaultTables[TableEntry][1]);
      if (EFI_ERROR (Status))
        break;
    }

    return Status;
}

/**
   Update memory information basing on the HOB list.

   @param  Smbios               SMBIOS protocol

**/
STATIC
EFI_STATUS
SmbiosMemoryInstall (
  IN EFI_SMBIOS_PROTOCOL       *Smbios
  )
{
  EFI_PEI_HOB_POINTERS    Hob;
  UINT64 MemorySize;
  EFI_STATUS Status;

  MemorySize = 0;

  //
  // Get the HOB list for processing
  //
  Hob.Raw = GetHobList ();

  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
          MemorySize += (UINT64)(Hob.ResourceDescriptor->ResourceLength);

          Status = SmbiosInstallMemoryStructure (Smbios,
                     Hob.ResourceDescriptor->PhysicalStart,
                     Hob.ResourceDescriptor->ResourceLength);
          if (EFI_ERROR(Status)) {
            return Status;
          }
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Update TYPE17 memory size fields
  //
  mSG2042EVBType17.Size = (UINT16)(MemorySize >> 20);
  mSG2042EVBType17.VolatileSize = MemorySize;

  return EFI_SUCCESS;
}

/**
   Install all structures from the DefaultTables structure

   @param  Smbios               SMBIOS protocol

**/
EFI_STATUS
SmbiosInstallAllStructures (
   IN EFI_SMBIOS_PROTOCOL       *Smbios
   )
{
  EFI_STATUS    Status;

  //
  // Generate memory descriptors.
  //
  Status = SmbiosMemoryInstall (Smbios);
  ASSERT_EFI_ERROR (Status);

  //
  // Install all tables.
  //
  Status = SmbiosInstallStructures (Smbios, DefaultCommonTables);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
   Installs SMBIOS information for SG2042EVB platforms

   @param ImageHandle     Module's image handle
   @param SystemTable     Pointer of EFI_SYSTEM_TABLE

   @retval EFI_SUCCESS    Smbios data successfully installed
   @retval Other          Smbios data was not installed

**/
EFI_STATUS
EFIAPI
SmbiosPlatformDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;

  //
  // Find the SMBIOS protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SmbiosInstallAllStructures (Smbios);

  return Status;
}
