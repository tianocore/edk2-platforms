/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

//
// Define data for SMBIOS Type 8 Table.
//
SMBIOS_PLATFORM_DXE_TABLE_DATA (SMBIOS_TABLE_TYPE8, PlatformPortConnector) = {
  {                                               // Table 1
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeDB15Female,                  // External Connector Type
    PortTypeVideoPort                             // Port Type
  },
  {                                               // Table 2
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeUsb,                         // External Connector Type
    PortTypeUsb                                   // Port Type
  },
  {                                               // Table 3
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeUsb,                         // External Connector Type
    PortTypeUsb                                   // Port Type
  },
  {                                               // Table 4
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeUsb,                         // External Connector Type
    PortTypeUsb                                   // Port Type
  },
  {                                               // Table 5
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeOther,                       // External Connector Type
    PortTypeUsb                                   // Port Type
  },
  {                                               // Table 6
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeOther,                       // External Connector Type
    PortTypeUsb                                   // Port Type
  },
  {                                               // Table 7
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeRJ45,                        // External Connector Type
    PortTypeNetworkPort                           // Port Type
  },
  {                                               // Table 8
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    0,                                            // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_1,                       // External Reference Designator
    PortConnectorTypeRJ45,                        // External Connector Type
    PortTypeNetworkPort                           // Port Type
  },
  {                                               // Table 9
    {                                             // Header
      EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, // Type
      sizeof (SMBIOS_TABLE_TYPE8),                // Length
      SMBIOS_HANDLE_PI_RESERVED                   // Handle
    },
    ADDITIONAL_STR_INDEX_1,                       // Internal Reference Designator
    PortConnectorTypeNone,                        // Internal Connector Type
    ADDITIONAL_STR_INDEX_2,                       // External Reference Designator
    PortConnectorTypeDB9Male,                     // External Connector Type
    PortTypeSerial16550ACompatible                // Port Type
  },
  {                                               // Null-terminated table
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
SMBIOS_PLATFORM_DXE_STRING_TOKEN_DATA (PlatformPortConnector) = {
  {                                                                                  // Table 1
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_1)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 2
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_2)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 3
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_3)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 4
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_4)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 5
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_5)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 6
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_6)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 7
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_7)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 8
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_8)
    },
    ADDITIONAL_STR_INDEX_1                                                           // Size of Tokens array
  },
  {                                                                                  // Table 9
    {                                                                                // Tokens array
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_INTERNAL_REFERENCE_DESIGNATOR_9),
      STRING_TOKEN (STR_PLATFORM_DXE_PORT_CONNECTOR_EXTERNAL_REFERENCE_DESIGNATOR_9)
    },
    ADDITIONAL_STR_INDEX_2                                                           // Size of Tokens array
  }
};
