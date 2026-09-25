/** @file

  SPI FVB for RDE Response header file.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Protocol/SpiNorFlash.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <AmdRdeDirectory.h>
#include <AmdPspDirectory.h>

#define ROM_ADDRESS_MASK  0x00ffffff  // 16MB ROM area
#define BLOCK_SIZE        (FixedPcdGet32 (PcdFlashNvStorageBlockSize))

UINT32
GetSpiFlashOffset (
  UINT32  FlashSize
  );

EFI_STATUS
InitFvbRdeResponse (
  );

extern EFI_SPI_NOR_FLASH_PROTOCOL          *mSpiNorFlashProtocol;
extern EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mSpiFvbProtocol;
extern EFI_PHYSICAL_ADDRESS                mNvStorageBase;
extern UINT64                              mNvStorageSize;
extern EFI_LBA                             mNvStorageLbaOffset;
extern UINT32                              mSpiFlashOffset;

extern EFI_SPI_NOR_FLASH_PROTOCOL          *mSpiNorFlashProtocol;
extern EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mSpiFvbProtocol;
