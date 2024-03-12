/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __SPI_FLASH_DXE_H__
#define __SPI_FLASH_DXE_H__

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/Spi.h>
#include <Protocol/SpiFlash.h>

#define SPI_FLASH_SIGNATURE      SIGNATURE_32 ('F', 'S', 'P', 'I')

#define STATUS_REG_POLL_WIP_MSK  (1 << 0)

typedef struct {
  SPI_FLASH_PROTOCOL    SpiFlashProtocol;
  UINTN                 Signature;
  EFI_HANDLE            Handle;
} SPI_FLASH_INSTANCE;

#endif //__SPI_FLASH_DXE_H__
