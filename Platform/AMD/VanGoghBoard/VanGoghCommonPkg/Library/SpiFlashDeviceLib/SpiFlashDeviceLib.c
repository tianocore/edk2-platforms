/** @file
  Implements SpiFlashDeviceLib.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/SpiFlashDeviceLib.h>

SPI_INIT_TABLE  mSpiInitTable[] = {
  { // W25Q256JW/W74M25JW
    SF_VENDOR_ID_WINBOND,
    SF_DEVICE_ID0_W25Q256JW,
    SF_DEVICE_ID1_W25Q256JW,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      { EnumSpiOpcodeReadNoAddr,SPI_COMMAND_JEDEC_ID,          EnumSpiOperationJedecId            },
      { EnumSpiOpcodeWriteNoAddr,SPI_COMMAND_WRITE_S,           EnumSpiOperationWriteStatus        },
      { EnumSpiOpcodeWrite,    SPI_COMMAND_WRITE,             EnumSpiOperationProgramData_1_Byte },
      { EnumSpiOpcodeRead,     SPI_COMMAND_READ,              EnumSpiOperationReadData           },
      { EnumSpiOpcodeWrite,    SPI_COMMAND_ERASE,             EnumSpiOperationErase_4K_Byte      },
      { EnumSpiOpcodeReadNoAddr,SPI_COMMAND_READ_S,            EnumSpiOperationReadStatus         },
      { EnumSpiOpcodeWriteNoAddr,SPI_COMMAND_CHIP_ERASE,        EnumSpiOperationFullChipErase      },
      { EnumSpiOpcodeRead,     SPI_COMMAND_READ_SFDP,         EnumSpiOperationReadData           },
      { EnumSpiOpcodeWriteNoAddr,SPI_COMMAND_RPMC_OP1,          EnumSpiOperationOther              },
      { EnumSpiOpcodeReadNoAddr,SPI_COMMAND_RPMC_OP2,          EnumSpiOperationReadData           },
      { EnumSpiOpcodeReadNoAddr,SPI_COMMAND_Enter_4Byte_Addr,  EnumSpiOperationOther              },
      { EnumSpiOpcodeReadNoAddr,SPI_COMMAND_Exit_4Byte_Addr,   EnumSpiOperationOther              }
    },
    0,
    0x2000000   // BIOS image size in flash
  }
};

//
// The total number of support flash part
//
UINT8  mNumSpiFlashMax = sizeof (mSpiInitTable) / sizeof (mSpiInitTable[0]);
