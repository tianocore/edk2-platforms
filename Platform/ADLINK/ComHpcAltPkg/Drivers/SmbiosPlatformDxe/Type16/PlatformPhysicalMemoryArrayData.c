/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 16 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE16, PlatformPhysicalMemoryArray) = {
  {                                          // Table 1
    {                                        // Header
      EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, // Type
      sizeof (SMBIOS_TABLE_TYPE16),          // Length
      SMBIOS_HANDLE_PI_RESERVED,             // Handle
    },
    MemoryArrayLocationSystemBoard,          // Location
    MemoryArrayUseSystemMemory,              // Use
    MemoryErrorCorrectionMultiBitEcc,        // Memory Error Correction
    0x80000000,                              // Maximum Capacity
    0xFFFE,                                  // Memory Error Information Handle
    0x10,                                    // Number Of Memory Device
    0x40000000000ULL                         // Extended Maximum Capacity
  },
  {                                          // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformPhysicalMemoryArray) = {
  {                                         // Table 1
    {                                       // Tokens array
      NULL_TERMINATED_TOKEN
    },
    0                                       // Size of Tokens array
  }
};
