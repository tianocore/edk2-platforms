/** @file
  Implements Mcfg.h
  This file describes the contents of the ACPI Memory Mapped Configuration
  Space Access Table (MCFG).  Some additional ACPI values are defined in Acpi10.h,
  Acpi20.h, and Acpi30.h.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MCFG_H_
#define MCFG_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID            'A','M','D',' ',' ',' '                       // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID      SIGNATURE_64('E','D','K','2',' ',' ',' ',' ') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION      0x00000002
#define EFI_ACPI_CREATOR_ID        SIGNATURE_32(' ',' ',' ',' ')
#define EFI_ACPI_CREATOR_REVISION  0x01000013

//
// MCFG Definitions
//

//
// Define the number of allocation structures so that we can build the table structure.
//
#define EFI_ACPI_ALLOCATION_STRUCTURE_COUNT  1

//
// MCFG structure
//

//
// Ensure proper structure formats
//
#pragma pack (1)

//
// MCFG Table structure
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER                                                              Header;
  UINT64                                                                                   Reserved;
 #if EFI_ACPI_ALLOCATION_STRUCTURE_COUNT > 0
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE    AllocationStructure[EFI_ACPI_ALLOCATION_STRUCTURE_COUNT];
 #endif
} EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_DESCRIPTION_TABLE;

#pragma pack ()

#endif
