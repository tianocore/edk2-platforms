/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef BOARD_SPI_BUS_INSTANCE_H_
#define BOARD_SPI_BUS_INSTANCE_H_

#include <PiDxe.h>
#include <Protocol/SpiIo.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiConfiguration.h>

#define SPI_IO_SIGNATURE SIGNATURE_32 ('s', 'i', 'o', 's')

typedef struct {
  UINTN Signature;
  EFI_HANDLE Handle;
  EFI_SPI_IO_PROTOCOL Protocol;
  EFI_SPI_BUS_TRANSACTION BusTransaction;
  EFI_SPI_CONFIGURATION_PROTOCOL *SpiConfig;
  EFI_SPI_HC_PROTOCOL *SpiHc;
} SPI_IO_INSTANCE;

#define SPI_IO_FROM_THIS(a) \
  CR (a, SPI_IO_INSTANCE, Protocol, \
      SPI_IO_SIGNATURE)

#endif // BOARD_SPI_BUS_INSTANCE_H_
