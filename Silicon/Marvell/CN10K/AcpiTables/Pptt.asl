/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022-2026 Marvell
*
* Template for [PPTT] ACPI Table (static data table)
* Format: [ByteLength]  FieldName : HexFieldValue
*
**/
[0004]                          Signature : "PPTT"    [Processor Properties Topology Table]
[0004]                       Table Length : 00000000  // Auto-updated when compiled by iasl
[0001]                           Revision : 03
[0001]                           Checksum : 86
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10K"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20170303


/* [0x24 36] Level 3 cache TAD0 & TAD1 */
[0001]                      Subtable Type : 01 [Cache Type]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 000000FF
                               Size valid : 1
                     Number of Sets valid : 1
                      Associativity valid : 1
                    Allocation Type valid : 1
                         Cache Type valid : 1
                       Write Policy valid : 1
                          Line Size valid : 1
                           Cache ID valid : 1
[0004]                Next Level of Cache : 00000000
[0004]                               Size : 03000000 // 48MB for CN10KA
[0004]                     Number of Sets : 00003000 // 48Mb/Line Size/Associativity = 12288
[0001]                      Associativity : 20
[0001]                         Attributes : 0A
                          Allocation Type : 2     // Read and Write allocate
                               Cache Type : 2     // Unified cache
                             Write Policy : 0     // Write back
[0002]                          Line Size : 00080 // 128-byte cache line size
[0004]                          Cache ID  : 1     // Cache Identifier

/* [0x40 64] Level 2 cache */
[0001]                      Subtable Type : 01 [Cache Type]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 000000FF
                               Size valid : 1
                     Number of Sets valid : 1
                      Associativity valid : 1
                    Allocation Type valid : 1
                         Cache Type valid : 1
                       Write Policy valid : 1
                          Line Size valid : 1
                           Cache ID valid : 1
[0004]                Next Level of Cache : 00000000
[0004]                               Size : 00100000 // 1MB
[0004]                     Number of Sets : 00000800 // 1MB//Line Size/Associativity = 2048
[0001]                      Associativity : 08       // eight-way set-associative
[0001]                         Attributes : 0A
                          Allocation Type : 2     // Read and Write allocate
                               Cache Type : 2     // Unified cache
                             Write Policy : 0     // Write back
[0002]                          Line Size : 00040 // 64-byte block size
[0004]                          Cache ID  : 2     // Cache Identifier

/* [0x5C 92] Level 1 ICache */
[0001]                      Subtable Type : 01 [Cache Type]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 000000FF
                               Size valid : 1
                     Number of Sets valid : 1
                      Associativity valid : 1
                    Allocation Type valid : 1
                         Cache Type valid : 1
                       Write Policy valid : 1
                          Line Size valid : 1
                           Cache ID valid : 1
[0004]                Next Level of Cache : 00000040
[0004]                               Size : 00010000 // 64K
[0004]                     Number of Sets : 00000100 // 64K//Line Size/Associativity = 256
[0001]                      Associativity : 04       // four-way skewed set-associative
[0001]                         Attributes : 04
                          Allocation Type : 0     // Read allocate
                               Cache Type : 1     // Instruction cache
                             Write Policy : 0     // Write back
[0002]                          Line Size : 00040 // 64-byte block size
[0004]                          Cache ID  : 3     // Cache Identifier

/* [0x78 120] Level 1 DCache */
[0001]                      Subtable Type : 01 [Cache Type]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 000000FF
                               Size valid : 1
                     Number of Sets valid : 1
                      Associativity valid : 1
                    Allocation Type valid : 1
                         Cache Type valid : 1
                       Write Policy valid : 1
                          Line Size valid : 1
                           Cache ID valid : 1
[0004]                Next Level of Cache : 00000040
[0004]                               Size : 00010000 // 64K
[0004]                     Number of Sets : 00000100 // 64K/Line Size/Associativity = 256
[0001]                      Associativity : 04       // four-way set-associative
[0001]                         Attributes : 02
                          Allocation Type : 2     // Read and Write allocate
                               Cache Type : 0     // Data cache
                             Write Policy : 0     // Write back
[0002]                          Line Size : 00040 // 64-byte cache lines
[0004]                          Cache ID  : 4     // Cache Identifier

/* [0x94 148] Package */
[0001]                      Subtable Type : 00 [Cluster Hierarchy Node]
[0001]                             Length : 18
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 00000011
                         Physical package : 1
                  ACPI Processor ID valid : 0
                    Processor is a thread : 0
                           Node is a leaf : 0
                 Identical Implementation : 1
[0004]                             Parent : 00000000
[0004]                  ACPI Processor ID : 00000000
[0004]            Private Resource Number : 00000001
[0004]                   Private Resource : 00000024

/* [0xAC 172] Processor 0 */
[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000000
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

/* [0xc8 200] Processor 1 */
[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000001
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

/* [0xe4 228] Processor 2 */
[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000002
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

/* [0x100 256] Processor 3 */
[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000003
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

/* [0x11C 284] Processor 4 */
[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000004
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

/* [0x138 312] Processor 5 */
[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000005
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000006
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000007
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000008
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000009
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 0000000A
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 0000000B
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 0000000C
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 0000000D
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 0000000E
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 0000000F
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000010
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000011
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000012
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000013
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000014
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000015
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000016
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078

[0001]                      Subtable Type : 00 [Processor Hierarchy Node]
[0001]                             Length : 1C
[0002]                           Reserved : 0000
[0004]              Flags (decoded below) : 0000000A
                         Physical package : 0
                  ACPI Processor ID valid : 1
                    Processor is a thread : 0
                           Node is a leaf : 1
                 Identical Implementation : 0
[0004]                             Parent : 00000094
[0004]                  ACPI Processor ID : 00000017
[0004]            Private Resource Number : 00000002
[0004]                   Private Resource : 0000005C
[0004]                   Private Resource : 00000078
