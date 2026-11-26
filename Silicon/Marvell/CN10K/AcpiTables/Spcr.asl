/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022-2026 Marvell
*
* Serial Port Console Redirection table (SPCR)
* Format: [ByteLength]  FieldName : HexFieldValue
*
**/

[0004]                          Signature : "SPCR"    [Serial Port Console Redirection table]
[0004]                       Table Length : 00000000    // Auto-updated when compiled by iasl
[0001]                           Revision : 02
[0001]                           Checksum : 00          // Auto-updated in runtime
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10K"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20100528

[0001]                     Interface Type : 03          // Arm PL011 UART
[0003]                           Reserved : 000000

[0012]               Serial Port Register : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 20 // UART Data Registers are 32 bit registers
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 03 // Dword access
[0008]                            Address : 87E028000000 // UAA(0)_PF_BAR0

[0001]                     Interrupt Type : 08   // Bit[3]: ARMH GIC interrupt
[0001]                PCAT-compatible IRQ : 00   // Don't care for GIC interrupt
[0004]                          Interrupt : 00000020 // General purpose SPI messages
[0001]                          Baud Rate : 07   // 115200
[0001]                             Parity : 00   // No Parity
[0001]                          Stop Bits : 01   // 1 Stop bit
[0001]                       Flow Control : 00   // None flow control
[0001]                      Terminal Type : 00   // VT100
[0001]                           Reserved : 00   // Must be 0
[0002]                      PCI Device ID : A00F // PCC_PROD_E: 0xA0=GEN, PCC_DEV_IDL_E: 0x0F=UAA
[0002]                      PCI Vendor ID : 177D // PCC_VENDOR_E
[0001]                            PCI Bus : 01   // 0x1D8 -> PCC_DEV_CON_S bits 15-8 -> 0x1
[0001]                         PCI Device : 1B   // 0x1D8 -> PCC_DEV_CON_S bits 7-3  -> 0x1B
[0001]                       PCI Function : 00   // 0x1D8 -> PCC_DEV_CON_S bits 2-0  -> 0x0
[0004]                          PCI Flags : 00000001 // NOT suppress PNP enumeration or disable PM
[0001]                        PCI Segment : 00   // 0x1D8 -> PCC_DEV_CON_S bits 21-16 -> 0x0
[0004]                           Reserved : 00000000
[0004]                  Precise Baud Rate : 00000000
[0002]              NamespaceStringLength : 0000
[0002]              NamespaceStringOffset : 0000
[0000]                    NamespaceString : 
