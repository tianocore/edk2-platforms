/** @file
  Implements SpiFlashDevice.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_FLASH_DEVICE_LIB_H_
#define SPI_FLASH_DEVICE_LIB_H_

#include <Protocol/Spi.h>

//
// Provides mSpiInitTable and the total number of flash part in mSpiInitTable for other modules.
//
extern SPI_INIT_TABLE  mSpiInitTable[];
extern UINT8           mNumSpiFlashMax;

//
// Flash Device commands
//
// If a supported device uses a command different from the list below, a device specific command
// will be defined just below it's JEDEC id section.
//
#define SPI_COMMAND_WRITE             0x02
#define SPI_COMMAND_WRITE_AAI         0xAD
#define SPI_COMMAND_READ              0x03
#define SPI_COMMAND_ERASE             0x20
#define SPI_COMMAND_WRITE_DISABLE     0x04
#define SPI_COMMAND_READ_S            0x05
#define SPI_COMMAND_WRITE_ENABLE      0x06
#define SPI_COMMAND_READ_ID           0xAB
#define SPI_COMMAND_JEDEC_ID          0x9F
#define SPI_COMMAND_WRITE_S_EN        0x50
#define SPI_COMMAND_WRITE_S           0x01
#define SPI_COMMAND_CHIP_ERASE        0xC7
#define SPI_COMMAND_BLOCK_ERASE       0xD8
#define SPI_COMMAND_READ_SFDP         0x5A
#define SPI_COMMAND_RPMC_OP1          0x9B
#define SPI_COMMAND_RPMC_OP2          0x96
#define SPI_COMMAND_Enter_4Byte_Addr  0xB7
#define SPI_COMMAND_Exit_4Byte_Addr   0xE9

//
// Winbond 256Mbit parts
//
#define SF_VENDOR_ID_WINBOND     0xEF
#define SF_DEVICE_ID1_W25Q256JW  0x19          // Capacity 256Mbit
#define SF_DEVICE_ID0_W25Q256JW  0x60

//
// index for prefix opcodes
//
#define SPI_WREN_INDEX  0                     // Prefix Opcode 0: SPI_COMMAND_WRITE_ENABLE
#define SPI_EWSR_INDEX  1                     // Prefix Opcode 1: SPI_COMMAND_WRITE_S_EN
#define BIOS_CTRL       0xDC

#endif
