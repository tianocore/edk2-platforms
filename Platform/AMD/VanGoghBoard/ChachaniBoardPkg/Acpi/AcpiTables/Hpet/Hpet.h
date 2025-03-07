/** @file
  Implements Hpet.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HPET_H_
#define HPET_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID            'A','M','D',' ',' ',' '                       // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID      SIGNATURE_64('E','D','K','2',' ',' ',' ',' ') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION      0x00000002
#define EFI_ACPI_CREATOR_ID        SIGNATURE_32(' ',' ',' ',' ')
#define EFI_ACPI_CREATOR_REVISION  0x01000013

//
// HPET structure
//
#define EFI_ACPI_5_0_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION  0x00

#define EFI_ACPI_5_0_HPET_EVENT_TIMER_BLOCK_ID  0x10228201
// [31:16] 0x1022 - PCI Vendor ID of 1st Timer Block
//    [15] 0x01 - Legacy Replacement IRQ Routing Capable
//    [14] 0x00 - Reserved
//    [13] 0x00 - COUNT_SIZE_CAP counter size
// [12:08] 0x02 - Number of Comparators in 1st Timer Block
// [07:00] 0x01 - Hardware Rev ID
#define EFI_ACPI_5_0_HPET_BASE_ADDRESS_SPACE_ID             0x00
#define EFI_ACPI_5_0_HPET_BASE_ADDRESS_REGISTER_BIT_WIDTH   0x00
#define EFI_ACPI_5_0_HPET_BASE_ADDRESS_REGISTER_BIT_OFFSET  0x00
#define EFI_ACPI_5_0_HPET_BASE_ADDRESS_LOWER_32BIT          0xFED00000
#define EFI_ACPI_5_0_HPET_NUMBER                            0x00
#define EFI_ACPI_5_0_HPET_MIN_CLOCK_TICK                    0x0080
#define EFI_ACPI_5_0_HPET_PAGE_PROTECTION_AND_ATTRIBUTE     0x00

//
// Ensure proper structure formats
//
#pragma pack (1)
//
// ACPI 5.0 Table structure
//
typedef struct {
  // ACPI Common header
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  // HPET
  UINT32                         EventTimerBlockID;           // Offset 0x24
  UINT8                          BaseAddress_SpaceID;
  UINT8                          BaseAddress_RegisterBitWidth;
  UINT8                          BaseAddress_RegisterBitOffset;
  UINT8                          Reserved0[1];
  UINT32                         BaseAddressLower32bit;       // Offset 0x28
  UINT32                         Reserved1[1];
  UINT8                          HpetNumber;                    // Offset 0x34
  UINT16                         MinClockTick;                  // Offset 0x35
  UINT8                          PageProtectionAndOemAttribute; // Offset 0x37
} EFI_ACPI_5_0_HIGH_PRECISION_EVENT_TIMER_TABLE;

#pragma pack ()

#endif
