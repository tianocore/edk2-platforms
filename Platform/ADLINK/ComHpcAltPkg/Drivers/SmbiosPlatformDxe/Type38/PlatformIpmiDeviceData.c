/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 38 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE38, PlatformIpmiDevice) = {
  {                                            // Table 1
    {                                          // Header
      EFI_SMBIOS_TYPE_IPMI_DEVICE_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE38),            // Length
      SMBIOS_HANDLE_PI_RESERVED                // Handle
    },
    IPMIDeviceInfoInterfaceTypeSSIF,           // Interface Type
    0x20,                                      // IPMI Specification Revision
    0x20,                                      // I2C Slave Address
    0xFF,                                      // NV Storage Device Address
    0x20,                                      // Base Address
  },
  {                                            // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformIpmiDevice) = {
  {                          // Table 1
    {                        // Tokens array
      NULL_TERMINATED_TOKEN
    },
    0                        // Size of Tokens array
  }
};
