/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 11 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE11, PlatformOemString) = {
  {                                 // Table 1
    {                               // Header
      EFI_SMBIOS_TYPE_OEM_STRINGS,  // Type
      sizeof (SMBIOS_TABLE_TYPE11), // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1          // String Count
  },
  {                                 // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformOemString) = {
  {                                               // Table 1
    {                                             // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_STRING_1)
    },
    ADDITIONAL_STR_INDEX_1                        // Size of Tokens array
  }
};
