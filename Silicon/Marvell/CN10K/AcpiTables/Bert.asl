/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2025-2026 Marvell
*
* Boot Error Record Table (BERT)
*
**/

[0004]                          Signature : "BERT"    [Boot Error Record Table]
[0004]                       Table Length : 48  // Auto-updated when compiled by iasl
[0001]                           Revision : 01
[0001]                           Checksum : 00        // Auto-updated in runtime
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10KA"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "MRVL"
[0004]              Asl Compiler Revision : 20100528  // Auto-updated when compiled by iasl

[0004]           Boot Error Region Length : 00000000         // Updated by CN10KProcessBert()
[0008]          Boot Error Region Address : 0000000000000000 // Updated by CN10KProcessBert()
