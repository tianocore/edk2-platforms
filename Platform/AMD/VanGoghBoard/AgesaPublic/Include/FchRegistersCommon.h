/** @file
  Implements FchRegistersCommon.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#define R_FCH_ACPI_PM1_STATUS  0x00
#define R_FCH_ACPI_PM1_ENABLE  0x02
#define R_FCH_ACPI_PM_CONTROL  0x04
#define ACPI_MMIO_BASE         0xFED80000ul
#define SMI_BASE               0x200          // DWORD
#define PMIO_BASE              0x300          // DWORD
#define FCH_SMI_REG80          0x80           // SmiStatus0
#define FCH_SMI_REG84          0x84           // SmiStatus1
#define FCH_SMI_REG88          0x88           // SmiStatus2
#define FCH_SMI_REG8C          0x8C           // SmiStatus3
#define FCH_SMI_REG90          0x90           // SmiStatus4
#define FCH_SMI_REG98          0x98           // SmiTrig
#define FCH_SMI_REGA0          0xA0
#define FCH_SMI_REGB0          0xB0
#define FCH_SMI_REGC4          0xC4
#define FCH_PMIOA_REG60        0x60           // AcpiPm1EvtBlk
