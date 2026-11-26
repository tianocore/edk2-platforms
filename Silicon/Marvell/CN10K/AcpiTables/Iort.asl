/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022-2026 Marvell
*
* IO Remapping Table (IORT)
* Format: [ByteLength]  FieldName : HexFieldValue
* According to ARM DEN0049E_IO_Remapping_Table.pdf
* Compile with iasl version 20230628
**/

[0004]                          Signature : "IORT"    [IO Remapping Table]
[0004]                       Table Length : 00000000  // Auto-updated when compiled by iasl
[0001]                           Revision : 03
[0001]                           Checksum : 00        // Auto-updated in runtime
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10K"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20210105  // Auto-updated when compiled by iasl

[0004]                         Node Count : 00000000  // Auto-updated when compiled by iasl
[0004]                        Node Offset : 00000000  // Auto-updated when compiled by iasl
[0004]                           Reserved : 00000000
[0004]                   Optional Padding : 49 4F 52 54 // IORT

[0001]                               Type : 00         // ITS Group
[0002]                             Length : 0018
[0001]                           Revision : 00         // 0 according to spec.
[0004]                         Identifier : 00000010   // Any random but unique value
[0004]                      Mapping Count : 00000000   // 0 entries with Input base below
[0004]                     Mapping Offset : 00000000   // Offset to first Input base

[0004]                           ItsCount : 00000001
[0004]                        Identifiers : 00000000

[0001]                               Type : 04         // SMMUv3
[0002]                             Length : 006C
[0001]                           Revision : 03         // 3 according to spec.
[0004]                         Identifier : 00000014   // Any random but unique value
[0004]                      Mapping Count : 0000000A   // 10 entries with Input base below
[0004]                     Mapping Offset : 00000044   // Offset to first Input base

[0008]                       Base Address : 0000830000000000 // SMMU(0)_PF_BAR0
[0004]              Flags (decoded below) : 00000001
                          COHACC Override : 1
                            HTTU Override : 0
                   Proximity Domain Valid : 0
                           DeviceID Valid : 0
[0004]                           Reserved : 00000000
[0008]                      VATOS Address : 0000000000000000
[0004]                              Model : 00000000
[0004]                         Event GSIV : 00000000
[0004]                           PRI GSIV : 00000000
[0004]                          GERR GSIV : 00000000
[0004]                          Sync GSIV : 00000000
[0004]                   Proximity Domain : 00000000
[0004]            Device ID Mapping Index : 00000009

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00000000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00010000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00010000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00020000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00020000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00030000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00030000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00040000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00040000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00050000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00050000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00060000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00060000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00070000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00070000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00080000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00080000
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 00000038
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000001
                           Single Mapping : 1
//PCI0
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0040
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000020   // Any random but unique value
[0004]                      Mapping Count : 00000001   // 1 entry with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000000
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00000000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCI1
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0040
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000021   // Any random but unique value
[0004]                      Mapping Count : 00000001   // 1 entry with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000001
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00010000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCI2
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0040
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000022   // Any random but unique value
[0004]                      Mapping Count : 00000001   // 1 entry with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000002
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00020000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCIERC(0)
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0054
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000023   // Any random but unique value
[0004]                      Mapping Count : 00000002   // 2 entries with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000003
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00030000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 00010080
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCIERC(1)
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0054
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000024   // Any random but unique value
[0004]                      Mapping Count : 00000002   // 2 entries with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000004
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00040000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 00010088
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCIERC(2)
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0054
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000025   // Any random but unique value
[0004]                      Mapping Count : 00000002   // 2 entries with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000005
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00050000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 00010090
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCIERC(3)
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0054
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000026   // Any random but unique value
[0004]                      Mapping Count : 00000002   // 2 entries with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000006
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00060000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 00010098
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCIERC(4)
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0054
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000027   // Any random but unique value
[0004]                      Mapping Count : 00000002   // 2 entries with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000007
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00070000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 000100A0
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0
//PCIERC(5)
[0001]                               Type : 02         // Root complex
[0002]                             Length : 0054
[0001]                           Revision : 02         // 2 according to spec.
[0004]                         Identifier : 00000028   // Any random but unique value
[0004]                      Mapping Count : 00000002   // 2 entries with Input base below
[0004]                     Mapping Offset : 00000024   // Offset to first Input base

[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001 // SBSA requires root to support IO coherency
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 01
                                Coherency : 1
                         Device Attribute : 0
[0004]                      ATS Attribute : 00000000   // No PRI, nor ATS support
[0004]                 PCI Segment Number : 00000008
[0001]                  Memory Size Limit : 30 // 48-bit physical address (PA)
[0002]                 PASID Capabilities : 0000
[0001]                           Reserved : 00

[0004]                         Input base : 00000000
[0004]                           ID Count : 0000FFFF
[0004]                        Output Base : 00080000
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 000100A8
[0004]                   Output Reference : 00000034
[0004]              Flags (decoded below) : 00000000
                           Single Mapping : 0

// Named component node for MMC controller
[0001]                               Type : 01
[0002]                             Length : 0080
[0001]                           Revision : 04
[0004]                         Identifier : 00000030
[0004]                      Mapping Count : 00000001
[0004]                     Mapping Offset : 0000002C

[0004]                         Node Flags : 00000000
[0008]                  Memory Properties : [IORT Memory Access Properties]
[0004]                    Cache Coherency : 00000001
[0001]              Hints (decoded below) : 00
                                Transient : 0
                           Write Allocate : 0
                            Read Allocate : 0
                                 Override : 0
[0002]                           Reserved : 0000
[0001]       Memory Flags (decoded below) : 00
                                Coherency : 1
                         Device Attribute : 0
[0001]                  Memory Size Limit : 30
[0015]                        Device Name : "\_SB_.EMMC"

[0004]                         Input base : 00000000
[0004]                           ID Count : 00000000
[0004]                        Output Base : 00000058  //Stream id from DTS
[0004]                   Output Reference : 0000004C
[0004]              Flags (decoded below) : 00000001
                           Single Mapping : 1

