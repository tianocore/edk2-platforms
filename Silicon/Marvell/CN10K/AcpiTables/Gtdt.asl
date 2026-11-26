/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022-2026 Marvell
*
* Generic Timer Description Table (GTDT)
* Format: [ByteLength]  FieldName : HexFieldValue
*
**/

[0004]                          Signature : "GTDT"    [Generic Timer Description Table]
[0004]                       Table Length : 00000000  // Auto-updated when compiled by iasl
[0001]                           Revision : 03
[0001]                           Checksum : 00        // Auto-updated in runtime
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10K"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20140424  // Auto-updated when compiled by iasl

[0008]              Counter Block Address : 0000802000000000 // GTI_PF_BAR0
[0004]                           Reserved : 00000000

[0004]               Secure EL1 Interrupt : 0000001D // Secure Physical Timer, SBSA
[0004]          EL1 Flags (decoded below) : 00000004
                             Trigger Mode : 0 // Level triggered
                                 Polarity : 0 // Active high
                                Always On : 1 // Always-on Capability

[0004]           Non-Secure EL1 Interrupt : 0000001E // Non-Secure Physical Timer, SBSA
[0004]         NEL1 Flags (decoded below) : 00000004
                             Trigger Mode : 0 // Level triggered
                                 Polarity : 0 // Active high
                                Always On : 1 // Always-on Capability

[0004]            Virtual Timer Interrupt : 0000001B // Virtual Physical Timer event, SBSA
[0004]           VT Flags (decoded below) : 00000004
                             Trigger Mode : 0 // Level triggered
                                 Polarity : 0 // Active high
                                Always On : 1 // Always-on Capability

[0004]           Non-Secure EL2 Interrupt : 0000001A // Hypervisor Physical Timer, SBSA
[0004]         NEL2 Flags (decoded below) : 00000004
                             Trigger Mode : 0 // Level triggered
                                 Polarity : 0 // Active high
                                Always On : 1 // Always-on Capability
[0008]         Counter Read Block Address : 0000802000010000 // GTI_RD_CNTCV

[0004]               Platform Timer Count : 00000001 // One [Generic Watchdog Timer] below
[0004]              Platform Timer Offset : 00000068 // Offset of [Generic Watchdog Timer] below
[0004]             Virtual EL2 Timer GSIV : 0000001C // Hypervisor Virtual Timer event
[0004]            Virtual EL2 Timer Flags : 00000000 // For systems not implementing ARMv8.1 VHE

[0001]                      Subtable Type : 01 [Generic Watchdog Timer] // Arm Generic Watchdog
[0002]                             Length : 001C
[0001]                           Reserved : 00
[0008]              Refresh Frame Address : 00008020000B0000 // GTI_WR(1)_WRR
[0008]              Control Frame Address : 00008020000A0000 // GTI_WC(1)_WCS
[0004]                    Timer Interrupt : 00000028 // see ATF GTI_WATCHDOG_SPI_IRQ_BASE
[0004]        Timer Flags (decoded below) : 00000002
                             Trigger Mode : 0 // Level triggered
                                 Polarity : 0 // Active High
                                 Security : 0 // Non-secure
