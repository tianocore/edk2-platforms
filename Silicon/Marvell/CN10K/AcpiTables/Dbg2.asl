/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022-2026 Marvell
*
* Debug Port table type 2 (DBG2)
* Format: [ByteLength]  FieldName : HexFieldValue
*
* According to:
* https://docs.microsoft.com/en-us/windows-hardware/drivers/bringup/acpi-debug-port-table
*
**/

[0004]                          Signature : "DBG2"    [Debug Port table type 2]
[0004]                       Table Length : 00000000 // Auto-updated when compiled by iasl
[0001]                           Revision : 00       // This value is 0.
[0001]                           Checksum : 00       // Auto-updated in runtime
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10K"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20131115 // Auto-updated when compiled by iasl

[0004]                        Info Offset : 00000000 // Auto-updated when compiled by iasl
[0004]                         Info Count : 00000002 // Number of Debug Device structures below

// First Debug Device Information structure
[0001]                           Revision : 00   // Must be 0
[0002]                             Length : 0000 // Auto-updated when compiled by iasl
[0001]                     Register Count : 01   // Number of Generic Address Structures below
[0002]                    Namepath Length : 0000 // Auto-updated when compiled by iasl
[0002]                    Namepath Offset : 0000 // Auto-updated when compiled by iasl
[0002]                    OEM Data Length : 0000 // OEM data not present
[0002]                    OEM Data Offset : 0000 // OEM data not present
[0002]                          Port Type : 8000 // Serial
[0002]                       Port Subtype : 0003 // Arm PL011 UART
[0002]                           Reserved : 0000 // Reserved, must be 0.
[0002]                Base Address Offset : 0000 // Auto-updated when compiled by iasl
[0002]                Address Size Offset : 0000 // Auto-updated when compiled by iasl

[0012]              Base Address Register : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory] // The Address Space ID must be 0
[0001]                          Bit Width : 64 // UAA(0)_PF_BAR0 is 64 bit register
[0001]                         Bit Offset : 00 // Register Bit Offset field must be 0
[0001]               Encoded Access Width : 04 [QWord Access:64]
[0008]                            Address : 87E028000000 // UAA(0)_PF_BAR0

[0004]                       Address Size : 00000008 // 64 bits = 8 bytes

[000B]                           Namepath : "\_SB_.COM0"

// Second Debug Device Information structure
[0001]                           Revision : 00   // Must be 0
[0002]                             Length : 0000 // Auto-updated when compiled by iasl
[0001]                     Register Count : 01   // Number of Generic Address Structures below
[0002]                    Namepath Length : 0000 // Auto-updated when compiled by iasl
[0002]                    Namepath Offset : 0000 // Auto-updated when compiled by iasl
[0002]                    OEM Data Length : 0000 // OEM data not present
[0002]                    OEM Data Offset : 0000 // OEM data not present
[0002]                          Port Type : 8000 // Serial
[0002]                       Port Subtype : 0003 // Arm PL011 UART
[0002]                           Reserved : 0000 // Reserved, must be 0.
[0002]                Base Address Offset : 0000 // Auto-updated when compiled by iasl
[0002]                Address Size Offset : 0000 // Auto-updated when compiled by iasl

[0012]              Base Address Register : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory] // The Address Space ID must be 0
[0001]                          Bit Width : 64 // UAA(0)_PF_BAR0 is 64 bit register
[0001]                         Bit Offset : 00 // Register Bit Offset field must be 0
[0001]               Encoded Access Width : 04 [QWord Access:64]
[0008]                            Address : 87E029000000 // UAA(1)_PF_BAR0

[0004]                       Address Size : 00000008 // 64 bits = 8 bytes

[000B]                           Namepath : "\_SB_.COM1"
