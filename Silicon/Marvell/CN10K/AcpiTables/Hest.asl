/*
 *
 * Template for [HEST] ACPI Table
 * Format: [ByteLength]  FieldName : HexFieldValue
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 * Copyright (c) 2026, Marvell. All rights reserved.
 */

[0004]                          Signature : "HEST"    [Hardware Error Source Table]
[0004]                       Table Length : 00000740
[0001]                           Revision : 01
[0001]                           Checksum : 97
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10KA"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20170303
[0004]                 Error Source Count : 0000001D


[0002]                      Subtable Type : 0009 [MDC]
[0002]                          Source Id : 0000
[0002]                  Related Source Id : 0000
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000000
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000000
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000100


[0002]                      Subtable Type : 0009 [DSS]
[0002]                          Source Id : 0001
[0002]                  Related Source Id : 0001
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000008
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000001
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000100


[0002]                      Subtable Type : 0009 [TAD]
[0002]                          Source Id : 0002
[0002]                  Related Source Id : 0002
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000010
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000002
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000100

[0002]                      Subtable Type : 0009 [GIC]
[0002]                          Source Id : 001B
[0002]                  Related Source Id : 001B
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000018
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000001B
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000100

[0002]                      Subtable Type : 0009 [EA]
[0002]                          Source Id : 001C
[0002]                  Related Source Id : 001C
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000020
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000001C
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-0]
[0002]                          Source Id : 0003
[0002]                  Related Source Id : 0003
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000028
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000003
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-1]
[0002]                          Source Id : 0004
[0002]                  Related Source Id : 0004
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000030
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000004
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-2]
[0002]                          Source Id : 0005
[0002]                  Related Source Id : 0005
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000038
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000005
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-3]
[0002]                          Source Id : 0006
[0002]                  Related Source Id : 0006
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000040
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000006
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-4]
[0002]                          Source Id : 0007
[0002]                  Related Source Id : 0007
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000048
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000007
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-5]
[0002]                          Source Id : 0008
[0002]                  Related Source Id : 0008
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000050
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000008
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-6]
[0002]                          Source Id : 0009
[0002]                  Related Source Id : 0009
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000058
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000009
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-7]
[0002]                          Source Id : 000A
[0002]                  Related Source Id : 000A
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000060
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000000A
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-8]
[0002]                          Source Id : 000B
[0002]                  Related Source Id : 000B
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000068
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000000B
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-9]
[0002]                          Source Id : 000C
[0002]                  Related Source Id : 000C
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000070
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000000C
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-A]
[0002]                          Source Id : 000D
[0002]                  Related Source Id : 000D
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000078
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000000D
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-B]
[0002]                          Source Id : 000E
[0002]                  Related Source Id : 000E
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000080
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000000E
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-C]
[0002]                          Source Id : 000F
[0002]                  Related Source Id : 000F
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000088
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000000F
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-D]
[0002]                          Source Id : 0010
[0002]                  Related Source Id : 0010
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000090
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000010
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-E]
[0002]                          Source Id : 0011
[0002]                  Related Source Id : 0011
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 0000000000000098
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000011
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-F]
[0002]                          Source Id : 0012
[0002]                  Related Source Id : 0012
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000A0
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000012
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-10]
[0002]                          Source Id : 0013
[0002]                  Related Source Id : 0013
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000A8
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000013
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400

[0002]                      Subtable Type : 0009 [CORE-11]
[0002]                          Source Id : 0014
[0002]                  Related Source Id : 0014
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000B0
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000014
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-12]
[0002]                          Source Id : 0015
[0002]                  Related Source Id : 0015
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000B8
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000015
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-13]
[0002]                          Source Id : 0016
[0002]                  Related Source Id : 0016
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000C0
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000016
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-14]
[0002]                          Source Id : 0017
[0002]                  Related Source Id : 0017
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000C8
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000017
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-15]
[0002]                          Source Id : 0018
[0002]                  Related Source Id : 0018
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000D0
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000018
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-16]
[0002]                          Source Id : 0019
[0002]                  Related Source Id : 0019
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000D8
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 40000019
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400


[0002]                      Subtable Type : 0009 [CORE-17]
[0002]                          Source Id : 001A
[0002]                  Related Source Id : 001A
[0001]                           Reserved : 00
[0001]                            Enabled : 01
[0004]             Records To Preallocate : 00000001
[0004]            Max Sections Per Record : 00000001
[0004]                Max Raw Data Length : 00000000
[0001]                           Space ID : 00
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 00000000000000E0
[0001]                        Notify Type : 0B
[0001]                      Notify Length : 1C
[0002]         Configuration Write Enable : 0000
[0004]                       PollInterval : 00000000
[0004]                             Vector : 4000001A
[0004]            Polling Threshold Value : 00000001
[0004]           Polling Threshold Window : 00000001
[0004]              Error Threshold Value : 00000001
[0004]             Error Threshold Window : 00000001
[0004]          Error Status Block Length : 00000400
