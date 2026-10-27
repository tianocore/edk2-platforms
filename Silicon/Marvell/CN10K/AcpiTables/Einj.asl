/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2026 Marvell
*
* Error Injection Table (EINJ)
*
**/

[0004]                          Signature : "EINJ"    [Error Injection Table]
[0004]                       Table Length : 00000150
[0001]                           Revision : 01
[0001]                           Checksum : 00
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10KA"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20170303

[0004]            Injection Header Length : 00000030
[0001]                              Flags : 00
[0003]                           Reserved : 000000
[0004]              Injection Entry Count : 00000009

/* Entry [0]: BEGIN_OPERATION - NOOP */
[0001]                             Action : 00 [Begin Operation]
[0001]                        Instruction : 04 [NOOP]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00
[0001]                          Bit Width : 00
[0001]                         Bit Offset : 0
[0001]               Encoded Access Width : 00
[0008]                            Address : 0

[0008]                              Value : 0
[0008]                               Mask : 0

/* Entry [1]: GET_ERROR_TYPE - Read supported error types from shared memory */
[0001]                             Action : 03 [Get Error Type]
[0001]                        Instruction : 00 [Read Register]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 000000000033C0000

[0008]                              Value : 0000000000000000
[0008]                               Mask : FFFFFFFFFFFFFFFF

/* Entry [2]: EINJV2_GET_ERROR_TYPE - Read V2 error types from shared memory */
[0001]                             Action : 11
[0001]                        Instruction : 00 [Read Register]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 000000000033C0008

[0008]                              Value : 0000000000000000
[0008]                               Mask : FFFFFFFFFFFFFFFF

/* Entry [3]: END_OPERATION - Write register value */
[0001]                             Action : 04 [End Operation]
[0001]                        Instruction : 03 [Write Register Value]
[0001]              Flags (decoded below) : 01
                   Preserve Register Bits : 1
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 0
[0001]               Encoded Access Width : 04 [QWord Access:64]
[0008]                            Address : 000087E1C0008008

[0008]                              Value : 8000000000000000
[0008]                               Mask : 8000000000000000

/* Entry [4]: EXECUTE_OPERATION - Write register value */
[0001]                             Action : 05 [Execute Operation]
[0001]                        Instruction : 03 [Write Register Value]
[0001]              Flags (decoded below) : 01
                   Preserve Register Bits : 1
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 0
[0001]               Encoded Access Width : 04 [QWord Access:64]
[0008]                            Address : 000087E1C0008008

[0008]                              Value : 8000000000000000
[0008]                               Mask : 8000000000000000

/* Entry [5]: CHECK_BUSY_STATUS - Read busy flag from shared memory */
[0001]                             Action : 06 [Check Busy Status]
[0001]                        Instruction : 00 [Read Register]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 000000000033C0010

[0008]                              Value : 0000000000000001
[0008]                               Mask : FFFFFFFFFFFFFFFF

/* Entry [6]: GET_COMMAND_STATUS - Read command status from shared memory */
[0001]                             Action : 07 [Get Command Status]
[0001]                        Instruction : 00 [Read Register]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 000000000033C0018

[0008]                              Value : 0000000000000001
[0008]                               Mask : FFFFFFFFFFFFFFFF

/* Entry [7]: GET_TRIGGER_TABLE - Read trigger table address from shared memory */
[0001]                             Action : 01 [Get Trigger Table]
[0001]                        Instruction : 00 [Read Register]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04
[0008]                            Address : 000000000033C0020

[0008]                              Value : 0000000000000000
[0008]                               Mask : FFFFFFFFFFFFFFFF

/* Entry [8]: SET_ERROR_TYPE_WITH_ADDRESS (ACPI 5) */
[0001]                             Action : 08 [Set Error Type With Address]
[0001]                        Instruction : 02 [Write Register]
[0001]              Flags (decoded below) : 00
                   Preserve Register Bits : 0
[0001]                           Reserved : 00

[0012]                    Register Region : [Generic Address Structure]
[0001]                           Space ID : 00 [SystemMemory]
[0001]                          Bit Width : 40
[0001]                         Bit Offset : 00
[0001]               Encoded Access Width : 04 [QWord Access:64]
[0008]                            Address : 000000000033C0028

[0008]                              Value : 0000000000000000
[0008]                               Mask : 0000000000000000
