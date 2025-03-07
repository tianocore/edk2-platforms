/** @file
  Implements SpiInfo.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_INFO_H_
#define SPI_INFO_H_

#include <Uefi/UefiBaseType.h>
#include <Protocol/Spi.h>

#ifdef FCH_SPI_EXEC_OPCODE
  #undef FCH_SPI_EXEC_OPCODE
#define FCH_SPI_EXEC_OPCODE  BIT7
#endif

#define FCH_SPI_MMIO_REG45  0x45             // OpCode Access
#define FCH_SPI_MMIO_REG47  0x47             // Execute Access
#define FCH_SPI_MMIO_REG4C  0x4C             // SPI Status

#endif
