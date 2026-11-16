/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022-2026 Marvell
*
* APIC ACPI Table
* Format: [ByteLength]  FieldName : HexFieldValue
*
* Compile with iasl version 20230628
**/

[0004]                          Signature : "APIC"    [Multiple APIC Description Table (MADT)]
[0004]                       Table Length : 00000000 // Auto-updated when compiled by iasl
[0001]                           Revision : 05
[0001]                           Checksum : 00       // Auto-updated in runtime
[0006]                             Oem ID : "MRVL"
[0008]                       Oem Table ID : "CN10K"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20201113 // Auto-updated when compiled by iasl

[0004]                 Local Apic Address : 00000000
[0004]              Flags (decoded below) : 00000000
                      PC-AT Compatibility : 0

// Similar GIC structure is created for each core by AcpiPlatformDxe. This is just template.
[0001]                      Subtable Type : 0B [Generic Interrupt Controller]
[0001]                             Length : 50
[0002]                           Reserved : 0000
[0004]               CPU Interface Number : 00000000
[0004]                      Processor UID : 00000000 // Updated by AcpiPlatformDxe
[0004]              Flags (decoded below) : 00000005
                        Processor Enabled : 1
       Performance Interrupt Trigger Mode : 0
       Virtual GIC Interrupt Trigger Mode : 0
[0004]           Parking Protocol Version : 00000000
[0004]              Performance Interrupt : 00000017 // Core PMU overflow interrupt, SBSA
[0008]                     Parked Address : 0000000000000000
[0008]                       Base Address : 0000000000000000
[0008]           Virtual GIC Base Address : 0000000000000000
[0008]        Hypervisor GIC Base Address : 0000000000000000
[0004]              Virtual GIC Interrupt : 00000019 // Virtual CPU Interface Maintenance, SBSA
[0008]         Redistributor Base Address : 0000000000000000 // Ignored as GICR is present
[0008]                          ARM MPIDR : 0000000000000000 // Updated by AcpiPlatformDxe
[0001]                   Efficiency Class : 00
[0001]                           Reserved : 00
[0002]             SPE Overflow Interrupt : 0000
[0002]                     TRBE Interrupt : 0000

// ------- Arm CoreLink GIC-700 Generic Interrupt Controller r2p0 -------

// One, and only one, GIC distributor structure must be present in the MADT for an ARM based system
[0001]                      Subtable Type : 0C [Generic Interrupt Distributor]
[0001]                             Length : 18
[0002]                           Reserved : 0000
[0004]              Local GIC Hardware ID : 00000000
[0008]                       Base Address : 0000801000000000 // GIC_PF_BAR0
[0004]                     Interrupt Base : 00000000
[0001]                            Version : 04 // Compliant with Arm GIC v4.1
[0003]                           Reserved : 000000
//GICR
[0001]                      Subtable Type : 0E [Generic Interrupt Redistributor]
[0001]                             Length : 10
[0002]                           Reserved : 0000
[0008]                       Base Address : 0000801000080000 // GICR(0..23)_CTLR
[0004]                             Length : 00600000         // 24 * 0x40000
//GITS
[0001]                      Subtable Type : 0F [Generic Interrupt Translator]
[0001]                             Length : 14
[0002]                           Reserved : 0000
[0004]                     Translation ID : 00000000
[0008]                       Base Address : 0000801000040000 // GITS0_CTLR
[0004]                           Reserved : 00000000
