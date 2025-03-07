/** @file
  Implements Madt.h
  This file describes the contents of the ACPI Multiple APIC Description
  Table (MADT).  Some additional ACPI values are defined in Acpi1_0.h and
  Acpi2_0.h.
  To make changes to the MADT, it is necessary to update the count for the
  APIC structure being updated, and to modify table found in Madt.c.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MADT_H_
#define MADT_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi.h>
// #include <AcpiHeaderDefaultValue.h>
//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID            'A','M','D',' ',' ',' '                       // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID      SIGNATURE_64('E','D','K','2',' ',' ',' ',' ') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION      0x00000002
#define EFI_ACPI_CREATOR_ID        SIGNATURE_32(' ',' ',' ',' ')
#define EFI_ACPI_CREATOR_REVISION  0x01000013

//
// Local APIC address
//
#define EFI_ACPI_LOCAL_APIC_ADDRESS  0xFEE00000
// #define EFI_IO_APIC_ADDRESS         0xFEC00000

//
// Multiple APIC Flags are defined in AcpiX.0.h
//
#define EFI_ACPI_5_0_MULTIPLE_APIC_FLAGS  (EFI_ACPI_5_0_PCAT_COMPAT)

//
// Define the number of each table type.
// This is where the table layout is modified.
//

#define EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT  16

#define EFI_ACPI_IO_APIC_COUNT  2

#define EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT      2
#define EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT  0

#define EFI_ACPI_LOCAL_APIC_NMI_COUNT  16

#define EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT  0
#define EFI_ACPI_IO_SAPIC_COUNT                     0
#define EFI_ACPI_PROCESSOR_LOCAL_SAPIC_COUNT        0
#define EFI_ACPI_PLATFORM_INTERRUPT_SOURCES_COUNT   0

//
// MADT structure
//

//
// Ensure proper structure formats
//
#pragma pack (1)
//
// ACPI 5.0 Table structure
//
typedef struct {
  EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER     Header;

 #if EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT > 0
  EFI_ACPI_5_0_PROCESSOR_LOCAL_APIC_STRUCTURE             LocalApic[EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT];
 #endif

 #if EFI_ACPI_IO_APIC_COUNT > 0
  EFI_ACPI_5_0_IO_APIC_STRUCTURE                          IoApic[EFI_ACPI_IO_APIC_COUNT];
 #endif

 #if EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT > 0
  EFI_ACPI_5_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE        Iso[EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT];
 #endif

 #if EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT > 0
  EFI_ACPI_5_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE    NmiSource[EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT];
 #endif

 #if EFI_ACPI_LOCAL_APIC_NMI_COUNT > 0
  EFI_ACPI_5_0_LOCAL_APIC_NMI_STRUCTURE                   LocalApicNmi[EFI_ACPI_LOCAL_APIC_NMI_COUNT];
 #endif

 #if EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT > 0
  EFI_ACPI_5_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE      LocalApicOverride[EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT];
 #endif

 #if EFI_ACPI_IO_SAPIC_COUNT > 0
  EFI_ACPI_5_0_IO_SAPIC_STRUCTURE                         IoSapic[EFI_ACPI_IO_SAPIC_COUNT];
 #endif

 #if EFI_ACPI_PROCESSOR_LOCAL_SAPIC_COUNT > 0
  EFI_ACPI_5_0_PROCESSOR_LOCAL_SAPIC_STRUCTURE            LocalSapic[EFI_ACPI_PROCESSOR_LOCAL_SAPIC_COUNT];
 #endif

 #if EFI_ACPI_PLATFORM_INTERRUPT_SOURCES_COUNT > 0
  EFI_ACPI_5_0_PLATFORM_INTERRUPT_SOURCES_STRUCTURE       PlatformInterruptSources[EFI_ACPI_PLATFORM_INTERRUPT_SOURCES_COUNT];
 #endif
} EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE;

#pragma pack ()

#endif
