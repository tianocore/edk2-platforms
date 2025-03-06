/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 17 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE17, PlatformMemoryDevice) = {
  {                                   // Table 1
    {                                 // Hdr
      EFI_SMBIOS_TYPE_MEMORY_DEVICE,  // Type
      sizeof (SMBIOS_TABLE_TYPE17),   // Length
      SMBIOS_HANDLE_PI_RESERVED       // Handle
    },
    0xFFFF,                           // Memory Array Handle
    0xFFFE,                           // Memory Error Information Handle
    72,                               // Total Width
    64,                               // Data Width
    0,                                // Size
    0x09,                             // Form Factor
    1,                                // Device Set
    ADDITIONAL_STR_INDEX_1,           // Device Locator
    ADDITIONAL_STR_INDEX_2,           // Bank Locator
    MemoryTypeDdr4,                   // Memory Type
    {},                               // Type Detail
    0,                                // Speed
    ADDITIONAL_STR_INDEX_3,           // Manufacturer
    ADDITIONAL_STR_INDEX_4,           // Serial
    ADDITIONAL_STR_INDEX_5,           // Asset Tag
    ADDITIONAL_STR_INDEX_6,           // Part Number
    0,                                // Attributes
  },
  {                                   // Null-terminated table
    {
      NULL_TERMINATED_TYPE,
      0,
      0
    },
  }
};

//
// Define string Tokens for additional strings.
//
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformMemoryDevice) = {
  {                                                                 // Table 1
    {                                                               // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_DEVICE_LOCATOR),
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_BANK_LOCATOR),
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_MANUFACTURER),
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_SERIAL_NUMBER),
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_ASSET_TAG),
      STRING_TOKEN (STR_PLATFORM_DXE_MEMORY_DEVICE_PART_NUMBER)
    },
    ADDITIONAL_STR_INDEX_6                                          // Size of Tokens array
  }
};
