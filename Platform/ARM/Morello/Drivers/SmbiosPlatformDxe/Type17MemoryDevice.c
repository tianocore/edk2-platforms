/** @file
  SMBIOS Type 17 (Memory Device) table for ARM Morello System
  Development Platform

  This file installs SMBIOS Type 17 (Memory Device) table for ARM Morello
  System Development Platform. It includes the specification of each installed
  memory device such as size of each device, bank locator, memory device
  type, and other related information.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.18
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "MorelloPlatform.h"
#include "SmbiosPlatformDxe.h"

#define TYPE17_STRINGS                                  \
  "CHANNEL0\0"                                          \
  "CHANNEL1\0"                                          \
  "RDIMM0\0"                                            \
  "RDIMM1\0"                                            \
  "Micron\0"                                            \
  "MTA9ASF\0"                                           \
  "\0"

typedef enum {
  CHANNEL0 = 1,
  CHANNEL1,
  RDIMM0,
  RDIMM1,
  MICRON,
  PARTNUMBER,
} TYPE17_STRING_ELEMENTS;

/* SMBIOS Type17 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE17    Base;
  CHAR8                  Strings[sizeof (TYPE17_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE17;
#pragma pack()

/* Memory Device */
STATIC ARM_MORELLO_SMBIOS_TYPE17  mArmMorelloSmbiosType17[] = {
  {
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_MEMORY_DEVICE,  // Type 17
        sizeof (SMBIOS_TABLE_TYPE17),   // Length
        SMBIOS_HANDLE_MEMORY_DEVICE0000
      },
      SMBIOS_HANDLE_PHYSICAL_MEMORY,    // Physical memory array handle
      0xFFFE,                           // Memory error info handle
      0x48,                             // Total width
      0x40,                             // Data width
      0,                                // Size, Update dynamically
      MemoryFormFactorDimm,             // Form Factor
      0,                                // Device set, not part of a set
      RDIMM0,                           // Device locator
      CHANNEL0,                         // channel 0
      MemoryTypeDdr4,                   // Memory type
      {                                 // Type details
        0,                              // Reserved
        0,                              // Other
        0,                              // Unknown
        0,                              // Fast-paged
        0,                              // Static column
        0,                              // Pseudo-static
        0,                              // RAMBUS
        1,                              // Synchronous
        0,                              // CMOS
        0,                              // EDO
        0,                              // Window DRAM
        0,                              // Cache DRAM
        0,                              // Non-volatile
        1,                              // Registered (Buffered)
        0,                              // Unbuffered (Unregistered)
        0,                              // LRDIMM
      },
      2933,                 // Speed;
      MICRON,               // Manufacturer String
      0,                    // SerialNumber String
      0,                    // AssetTag String
      PARTNUMBER,           // PartNumber String
      0,                    // Attributes; (unknown rank)
      0,                    // ExtendedSize; (since Size < 32GB-1)
      2933,                 // ConfiguredMemoryClockSpeed;
      1200,                 // MinimumVoltage; (in millivolts)
      1200,                 // MaximumVoltage; (in millivolts)
      1200,                 // ConfiguredVoltage; (in millivolts)
      MemoryTechnologyDram, // MemoryTechnology
      {                     // MemoryOperatingModeCapability
        {
          0, // Reserved                        :1;
          0, // Other                           :1;
          0, // Unknown                         :1;
          1, // VolatileMemory                  :1;
          0, // ByteAccessiblePersistentMemory  :1;
          0, // BlockAccessiblePersistentMemory :1;
          0  // Reserved                        :10;
        }
      },
      0,                    // FirwareVersion
      0,                    // ModuleManufacturerID (unknown)
      0,                    // ModuleProductID (unknown)
      0,                    // MemorySubsystemControllerManufacturerID (unknown)
      0,                    // MemorySubsystemControllerProductID (unknown)
      0,                    // NonVolatileSize
      0,                    // VolatileSize
      0,                    // CacheSize
      0,                    // LogicalSize (since MemoryType is not MemoryTypeLogicalNonVolatileDevice)
      0,                    // ExtendedSpeed,
      0,                    // ExtendedConfiguredMemorySpeed
    },
    // Text strings (unformatted area)
    TYPE17_STRINGS
  },
  {
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_MEMORY_DEVICE,  // Type 17
        sizeof (SMBIOS_TABLE_TYPE17),   // Length
        SMBIOS_HANDLE_MEMORY_DEVICE0001
      },
      SMBIOS_HANDLE_PHYSICAL_MEMORY,    // Physical memory array handle
      0xFFFE,                           // Memory error info handle
      0x48,                             // Total width
      0x40,                             // Data width
      0,                                // Size, Update dynamically
      MemoryFormFactorDimm,             // Form Factor
      0,                                // Device set, not part of a set
      RDIMM1,                           // Device locator
      CHANNEL1,                         // channel 1
      MemoryTypeDdr4,                   // Memory type
      {                                 // Type details
        0,                              // Reserved
        0,                              // Other
        0,                              // Unknown
        0,                              // Fast-paged
        0,                              // Static column
        0,                              // Pseudo-static
        0,                              // RAMBUS
        1,                              // Synchronous
        0,                              // CMOS
        0,                              // EDO
        0,                              // Window DRAM
        0,                              // Cache DRAM
        0,                              // Non-volatile
        1,                              // Registered (Buffered)
        0,                              // Unbuffered (Unregistered)
        0,                              // LRDIMM
      },
      2933,                 // Speed;
      MICRON,               // Manufacturer String
      0,                    // SerialNumber String
      0,                    // AssetTag String
      PARTNUMBER,           // PartNumber String
      0,                    // Attributes; (unknown rank)
      0,                    // ExtendedSize; (since Size < 32GB-1)
      2933,                 // ConfiguredMemoryClockSpeed;
      1200,                 // MinimumVoltage; (in millivolts)
      1200,                 // MaximumVoltage; (in millivolts)
      1200,                 // ConfiguredVoltage; (in millivolts)
      MemoryTechnologyDram, // MemoryTechnology
      {                     // MemoryOperatingModeCapability
        {
          0, // Reserved                        :1;
          0, // Other                           :1;
          0, // Unknown                         :1;
          1, // VolatileMemory                  :1;
          0, // ByteAccessiblePersistentMemory  :1;
          0, // BlockAccessiblePersistentMemory :1;
          0  // Reserved                        :10;
        }
      },
      0,                    // FirwareVersion
      0,                    // ModuleManufacturerID (unknown)
      0,                    // ModuleProductID (unknown)
      0,                    // MemorySubsystemControllerManufacturerID (unknown)
      0,                    // MemorySubsystemControllerProductID (unknown)
      0,                    // NonVolatileSize
      0,                    // VolatileSize
      0,                    // CacheSize
      0,                    // LogicalSize (since MemoryType is not MemoryTypeLogicalNonVolatileDevice)
      0,                    // ExtendedSpeed,
      0,                    // ExtendedConfiguredMemorySpeed
    },
    // Text strings (unformatted area)
    TYPE17_STRINGS
  },
};

/** Update the memory size fields in SMBIOS Memory Device (Type 17) table.

  @param  [in]  Type17Table  Pointer to the Type 17 table.
  @param  [in]  MemorySize   Memory size available on the platform.
                             - If no memory device is installed, this value
                               is 0.
                             - If size is unknown, this value is MAX_UINT64.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER Invalid Type 17 Table pointer.
**/
STATIC
UINTN
UpdateMemorySize (
  IN  SMBIOS_TABLE_TYPE17  *Type17Table,
  IN  UINT64               MemorySize
  )
{
  if (Type17Table == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  /* Ref: SMBIOS Specification, Version 3.4.0, Document Identifier: DSP0134,
     Table 75 – Memory Device (Type 17) structure, description for Size field.
     If the value is 0, no memory device is installed in the socket; if
     the size is unknown, the field value is FFFFh.
  */
  if (MemorySize == 0) {
    Type17Table->Size         = 0;
    Type17Table->ExtendedSize = 0;
    return EFI_SUCCESS;
  }

  if (MemorySize == MAX_UINT64) {
    Type17Table->Size         = MAX_UINT16;
    Type17Table->ExtendedSize = 0;
    return EFI_SUCCESS;
  }

  /* Ref: SMBIOS Specification, Version 3.4.0, Document Identifier: DSP0134,
     Table 75 – Memory Device (Type 17) structure, description for Size field.
     If the size is 32 GB-1 MB or greater, the field value is 7FFFh and the
     actual size is stored in the Extended Size field.
  */
  if (MemorySize < (SIZE_32GB - SIZE_1MB)) {
    /* Ref: SMBIOS Specification, Version 3.4.0, Document Identifier: DSP0134,
       section 7.18.5 Memory Device — Extended Size
       For compatibility with older SMBIOS parsers, memory devices
       smaller than (32 GB - 1 MB) should be represented using their
       size in the Size field, leaving the Extended Size field set to 0.
    */
    Type17Table->ExtendedSize = 0;

    /* Ref: SMBIOS Specification, Version 3.4.0, Document Identifier: DSP0134,
       Table 75 – Memory Device (Type 17) structure, description for Size field.
       The granularity in which the value is specified depends on the setting
       of the most-significant bit (bit 15). If the bit is 0, the value is
       specified in megabyte units; if the bit is 1, the value is specified
       in kilobyte units.
       For example, the value 8100h identifies a 256 KB memory device
       and 0100h identifies a 256 MB memory device.
    */
    if (MemorySize < SIZE_1MB) {
      Type17Table->Size  = MemorySize >> 10;
      Type17Table->Size |= BIT15;
    } else {
      Type17Table->Size = MemorySize >> 20;
    }

    return EFI_SUCCESS;
  }

  /* Ref: SMBIOS Specification, Version 3.4.0, Document Identifier: DSP0134,
      section 7.18.5 Memory Device — Extended Size
      The Extended Size field is intended to represent memory devices
      larger than 32,767 MB (32 GB - 1 MB), which cannot be described
      using the Size field. This field is only meaningful if the value
      in the Size field is 7FFFh.
  */
  Type17Table->Size = 0x7FFF;

  /* Ref: SMBIOS Specification, Version 3.4.0, Document Identifier: DSP0134,
     section 7.18.5 Memory Device — Extended Size
     Bit 31 is reserved for future use and must be set to 0.
     Bits 30:0 represent the size of the memory device in megabytes.
     EXAMPLE: 0000_8000h indicates a 32 GB memory device (32,768 MB),
              0002_0000h represents a 128 GB memory device (131,072 MB), and
              0000_7FFFh represents a 32,767 MB (32 GB – 1 MB) device.
  */
  Type17Table->ExtendedSize = (MemorySize >> 20) & (~BIT31);
  return EFI_SUCCESS;
}

/**
  Install SMBIOS memory device Table.

  Install the SMBIOS memory device (type 17) table for Arm Morello
  System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType17MemoryDevice (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  UINT64             DramBlock2Size = 0;
  UINT64             TotalMemory    = 0;
  UINT8              Idx;

  Status = MorelloGetDramBlock2Size (&DramBlock2Size);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get Dram Block 2 Size.\n"
      ));
    return Status;
  }

  /* The total system memory = dram1 + dram2; */
  TotalMemory = PcdGet64 (PcdSystemMemorySize) + DramBlock2Size;

  /* Update system memory information */
  Status = UpdateMemorySize (
             &mArmMorelloSmbiosType17[0].Base,
             TotalMemory/2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to update DRAM-1 size.\n"
      ));
  }

  Status = UpdateMemorySize (
             &mArmMorelloSmbiosType17[1].Base,
             TotalMemory/2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to update DRAM-2 size\n"
      ));
  }

  /* Install valid entries */
  for (Idx = 0; Idx < 2; Idx++) {
    SmbiosHandle =
      ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType17[Idx])->Handle;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType17[Idx]
                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "SMBIOS: Failed to install Type17 SMBIOS table.\n"
        ));
      break;
    }
  }

  return Status;
}
