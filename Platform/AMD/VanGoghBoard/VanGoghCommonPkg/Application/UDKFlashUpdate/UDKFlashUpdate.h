/** @file
  Implements UDKFlashUpdate.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UDK_FLASH_UPDATE_H_
#define UDK_FLASH_UPDATE_H_

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/Shell.h>
#include <Protocol/Spi.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/SpiCommon.h>

#include "SpiFlashDevice.h"

#define SPI_OPCODE_JEDEC_ID_INDEX           0
#define SPI_OPCODE_WRITE_S_INDEX            1
#define SPI_OPCODE_WRITE_INDEX              2
#define SPI_OPCODE_READ_INDEX               3
#define SPI_OPCODE_ERASE_INDEX              4
#define SPI_OPCODE_READ_S_INDEX             5
#define SPI_OPCODE_CHIP_ERASE_INDEX         6
#define SPI_OPCODE_READ_SFDP_INDEX          7
#define SPI_COMMAND_RPMC_OP1_INDEX          8
#define SPI_COMMAND_RPMC_OP2_INDEX          9
#define SPI_COMMAND_Enter_4Byte_Addr_INDEX  10
#define SPI_COMMAND_Exit_4Byte_Addr_INDEX   11

extern SPI_INIT_TABLE        mSpiInitTable[];
extern EFI_RUNTIME_SERVICES  *gRT;

#endif
