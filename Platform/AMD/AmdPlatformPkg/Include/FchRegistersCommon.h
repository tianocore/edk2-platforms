/** @file

  Copyright (C) 2008-2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FCH_REGISTER_COMMON_H_
#define FCH_REGISTER_COMMON_H_

// Misc
#define R_FCH_ACPI_PM1_STATUS  0x00
#define R_FCH_ACPI_PM1_ENABLE  0x02
#define R_FCH_ACPI_PM_CONTROL  0x04

#define FCH_LPC_BUS   0
#define FCH_LPC_DEV   20
#define FCH_LPC_FUNC  3

#define ACPI_MMIO_BASE  0xFED80000ul
#define SMI_BASE        0x200           // DWORD
#define IOMUX_BASE      0xD00           // BYTE
#define MISC_BASE       0xE00
#define PMIO_BASE       0x300           // DWORD

//
//  FCH LPC Device  0x780E
//  Device 20 (0x14) Func 3
//
#define FCH_LPC_REG48  0x48             // IO/Mem Port Decode Enable Register 5- RW
#define FCH_LPC_REG74  0x74             // Alternative Wide IO Range Enable- W/R
#define FCH_LPC_REG7C  0x7C             // TPM (trusted plant form module) reg- W/R
#define FCH_LPC_REGA0  0x0A0            // SPI base address
#define FCH_LPC_REGB8  0x0B8

//
//  FCH MMIO Base (SMI)
//    offset : 0x200
//
#define FCH_SMI_REG80  0x80                   // SmiStatus0
#define FCH_SMI_REG84  0x84                   // SmiStatus1
#define FCH_SMI_REG88  0x88                   // SmiStatus2
#define FCH_SMI_REG8C  0x8C                   // SmiStatus3
#define FCH_SMI_REG90  0x90                   // SmiStatus4
#define FCH_SMI_REG98  0x98                   // SmiTrig
#define FCH_SMI_REGA0  0xA0
#define FCH_SMI_REGB0  0xB0
#define FCH_SMI_REGC4  0xC4

//
//  FCH MMIO Base (PMIO)
//    offset : 0x300
//
#define FCH_PMIOA_REG60  0x60                 // AcpiPm1EvtBlk

//
//
#define FCH_MISC_REG80  0x80
// FCH SPI
//

#define FCH_SPI_BASE_ADDRESS  0xFEC10000

#define FCH_SPI_MMIO_REG00                0x00
#define FCH_SPI_FIFO_PTR_CRL              0x00100000l //
#define FCH_SPI_BUSY                      0x80000000l //
#define FCH_SPI_MMIO_REG1D                0x1D        //
#define FCH_SPI_MMIO_REG20                0x20
#define FCH_SPI_MMIO_REG22                0x22        //
#define FCH_SPI_MMIO_REG30                0x30        //
#define FCH_SPI_R2VAL24                   0x00000001l //
#define FCH_SPI_R2VAL25                   0x00000002l //
#define FCH_SPI_R2MSK24                   0x00000004l //
#define FCH_SPI_R2MSK25                   0x00000008l //
#define FCH_SPI_MMIO_REG45_CMDCODE        0x45        //
#define FCH_SPI_MMIO_REG47_CMDTRIGGER     0x47        //
#define FCH_SPI_MMIO_REG48_TX_BYTE_COUNT  0x48        //
#define FCH_SPI_MMIO_REG4B_RX_BYTE_COUNT  0x4B        //
#define FCH_SPI_MMIO_REG4C_SPISTATUS      0x4C        //
#define FCH_SPI_MMIO_REG5C_ADDR32_CTRL3   0x5C        //
#define FCH_SPI_SPIROM_PAGE_MASK          0xFF        //
#define FCH_SPI_MMIO_REG80_FIFO           0x80        //

#endif /* FCH_REGISTER_COMMON_H_ */
