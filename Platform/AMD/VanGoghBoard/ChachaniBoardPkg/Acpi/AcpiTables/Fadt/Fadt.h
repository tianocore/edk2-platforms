/** @file
  Implements Fadt.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FADT_H_
#define FADT_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi50.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID            'A','M','D',' ',' ',' '                       // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID      SIGNATURE_64('E','D','K','2',' ',' ',' ',' ') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION      0x00000002
#define EFI_ACPI_CREATOR_ID        SIGNATURE_32(' ',' ',' ',' ')
#define EFI_ACPI_CREATOR_REVISION  0x01000013

//
// FADT Definitions
//
#define SCI_INT_VECTOR   0x0009
#define SMI_CMD_IO_PORT  0x000000B0 // SMI Port 0xB0
#define ACPI_ENABLE      0x0A0
#define ACPI_DISABLE     0x0A1

#define PM1a_EVT_BLK  0x00000400
#define PM1b_EVT_BLK  0x00000000
#define PM1a_CNT_BLK  0x00000404
#define PM1b_CNT_BLK  0x00000000
#define PM2_CNT_BLK   0x00000800
#define PM_TMR_BLK    0x00000408
#define GPE0_BLK      0x00000420
#define GPE1_BLK      0x00000000
#define PM1_EVT_LEN   0x04
#define PM1_CNT_LEN   0x02
#define PM2_CNT_LEN   0x01
#define PM_TM_LEN     0x04
#define GPE0_BLK_LEN  0x08
#define GPE1_BLK_LEN  0x00
#define GPE1_BASE     0x00

#define RESERVED        0x00
#define P_LVL2_LAT      0x0064
#define P_LVL3_LAT      0x03e9
#define FLUSH_SIZE      0x0000
#define FLUSH_STRIDE    0x0000
#define DUTY_OFFSET     0x01
#define DUTY_WIDTH      0x03
#define DAY_ALRM        0x0D
#define MON_ALRM        0x00
#define CENTURY         0x00
#define IAPC_BOOT_ARCH  EFI_ACPI_2_0_LEGACY_DEVICES
// #define FLAG            (EFI_ACPI_1_0_WBINVD | EFI_ACPI_1_0_PROC_C1 | EFI_ACPI_1_0_SLP_BUTTON | EFI_ACPI_1_0_RTC_S4)
#define FLAG   0x0000C5AD
#define FLAG2  (EFI_ACPI_2_0_WBINVD | EFI_ACPI_2_0_PROC_C1 | EFI_ACPI_2_0_PWR_BUTTON | EFI_ACPI_2_0_SLP_BUTTON | EFI_ACPI_2_0_RTC_S4 | EFI_ACPI_2_0_RESET_REG_SUP | EFI_ACPI_3_0_USE_PLATFORM_CLOCK)

#endif
