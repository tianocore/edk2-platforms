/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 9 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE9, PlatformSystemSlot) = {
  {                                 // Table 1
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageAvailable,             // Current Usage
    SlotLengthLong,                 // Slot Length
    1,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    5,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 2
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth16X,            // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    2,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    0,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 3
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    3,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    4,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 4
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    4,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    9,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 5
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    5,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    7,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 6
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    6,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    7,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 7
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth8X,             // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    7,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    8,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 8
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth16X,            // Slot Data Bus Width
    SlotUsageUnavailable,           // Current Usage
    SlotLengthLong,                 // Slot Length
    8,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    1,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 9
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth4X,             // Slot Data Bus Width
    SlotUsageAvailable,             // Current Usage
    SlotLengthShort,                // Slot Length
    9,                              // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    5,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
  },
  {                                 // Table 10
    {                               // Header
      EFI_SMBIOS_TYPE_SYSTEM_SLOTS, // Type
      sizeof (SMBIOS_TABLE_TYPE9),  // Length
      SMBIOS_HANDLE_PI_RESERVED     // Handle
    },
    ADDITIONAL_STR_INDEX_1,         // Slot Designation
    SlotTypePciExpressGen4,         // Slot Type
    SlotDataBusWidth4X,             // Slot Data Bus Width
    SlotUsageAvailable,             // Current Usage
    SlotLengthShort,                // Slot Length
    10,                             // Slot ID
    { 0, 0, 1},                     // Slot Characteristics 1
    { 1},                           // Slot Characteristics 2
    5,                              // Segment Group Number
    0,                              // Bus Number
    0,                              // Device Function Number
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformSystemSlot) = {
  {                                                              // Table 1
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_1)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 2
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_2)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 3
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_3)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 4
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_4)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 5
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_5)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 6
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_6)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 7
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_7)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 8
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_8)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 9
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_9)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
  {                                                              // Table 10
    {                                                            // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_SYSTEM_SLOT_DESIGNATION_10)
    },
    ADDITIONAL_STR_INDEX_1                                       // Size of Tokens array
  },
};
