/** @file

  Marvell CN10K xSPI DXE driver definitions.

  Copyright (c) 2025-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Protocol/Spi.h>
#include <Protocol/SpiFlash.h>

//
// Controller ownership lock register, relative to the controller base.
//
#define SPI_LOCK_OFFSET  0x208C

//
// Values written to the ownership lock register.
//
#define SPI_NOT_CLAIMED  0x00
#define SPI_UEFI_OWN     0x03

//
// Static configuration of the CN10K xSPI controller instance.
//
typedef struct {
  // Base address of the controller register block.
  UINTN     MpiRegBase;
  // Base address of the direct-access (SDMA) window.
  UINTN     SpiDirectBase;
  // Chip select routed to the NOR flash device.
  UINT32    ChipSelect;
  // Requested serial clock frequency, in Hz.
  UINT32    ClkFrequency;
  // Index of the MPI bus controller used by the flash.
  UINT32    BusControllerNum;
  // Size of the flash device, in bytes.
  UINT32    FlashSize;
} CN10K_SPI_CONFIG;
