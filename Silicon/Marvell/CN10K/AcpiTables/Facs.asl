/** @file
 *
 *  Firmware ACPI Control Structure (FACS)
 *
 *  Copyright (c) 2021-2026 Marvell
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *  https://spdx.org/licenses
 *
 **/

[0004]                          Signature : "FACS"
[0004]                             Length : 00000040
[0004]                 Hardware Signature : 00000000
[0004]          32 Firmware Waking Vector : 00000000
[0004]                        Global Lock : 00000000
[0004]              Flags (decoded below) : 00000000
                   S4BIOS Support Present : 0
               64-bit Wake Supported (V2) : 1
[0008]          64 Firmware Waking Vector : 0000000000000000
[0001]                            Version : 02
[0003]                           Reserved : 000000
[0004]          OspmFlags (decoded below) : 00000000
            64-bit Wake Env Required (V2) : 1
