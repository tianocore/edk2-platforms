/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 19 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE19, PlatformMemoryArrayMappedAddress) = {
  {                                                 // Table 1
    {                                               // Header
      EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS,  // Type
      sizeof (SMBIOS_TABLE_TYPE19),                 // Length
      SMBIOS_HANDLE_PI_RESERVED                     // Handle
    },
    0xFFFFFFFF,                                     // Starting Address
    0xFFFFFFFF,                                     // Ending Address
    0xFFFF,                                         // Memory Array Handle
    1,                                              // Partition Width
    0x0,                                            // Extended Starting Address
    0x0                                             // Extended Ending Address
  },
  {                                                 // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformMemoryArrayMappedAddress) = {
  {                          // Table 1
    {                        // Tokens array
      NULL_TERMINATED_TOKEN
    },
    0                        // Size of Tokens array
  }
};
