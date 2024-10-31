/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// SMBIOS Type 7 Table is already initialized and installed in
// ArmPkg/SmbiosMiscDxe for Cache Level 1 and 2. Need to add one
// Type 7 Table for System Level Cache (SLC).
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE7, PlatformCache) = {
  {                                         // Table 1
    {                                       // Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,    // Type
      sizeof (SMBIOS_TABLE_TYPE7),          // Length
      SMBIOS_HANDLE_PI_RESERVED             // Handle
    },
    ADDITIONAL_STR_INDEX_1,                 // Socket Designation
    0x182,                                  // Write Back, Enabled, Internal, Not Socketed, Cache Level 3
    0x8010,                                 // Maximum Cache Size: 1M
    0x8010,                                 // Installed Size: 1M
    { 0, 0, 0, 0, 0, 1},                    // Supported SRAM Type: Synchronous
    { 0, 0, 0, 0, 0, 1},                    // Current SRAM Type: Synchronous
    0,                                      // Cache Speed
    CacheErrorSingleBit,                    // Error Correction Type
    CacheTypeUnified,                       // System Cache Type
    CacheAssociativity16Way                 // Associativity
  },
  {                                         // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformCache) = {
  {                                         // Table 1
    {                                       // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_CACHE)
    },
    ADDITIONAL_STR_INDEX_1                  // Size of Tokens array
  }
};
