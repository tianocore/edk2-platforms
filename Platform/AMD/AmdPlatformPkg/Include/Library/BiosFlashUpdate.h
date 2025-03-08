/** @file
  The header file for AMD BIOS Flash DXE Driver.
  And also for AMD BIOS Flash Shell Command.

  Copyright (C) 2023 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BIOS_FLASH_UPDATE_H_
#define BIOS_FLASH_UPDATE_H_

#include <Uefi.h>

#define EFI_AMD_FLASH_VARIABLE_NAME       L"AMDFLASH"
#define EFI_AMD_FLASH_EX_VARIABLE_NAME    L"AMDFLASH_EX"
#define EFI_AMD_FLASH_BIOS_FILE_NAME      L"\\EFI\\AMDFLASH\\BIOS.FD"
#define IOHC_NB_SMN_INDEX_2_BIOS          0x00B8
#define IOHC_NB_SMN_DATA_2_BIOS           0x00BC
#define FCH_SPI_SMN_BASE                  0x2DC4000
#define FCH_SPI_MMIO_REG60                0x60
#define AMD_BIOS_FLASH_OPTION_NV_STORAGE  0x0001
#define PRESERVED_REGION_INFO_COUNT       2

typedef union {
  struct {
    UINT16    OptionNumber;
    UINT16    Flags;
  } Option;
  UINT32    FlashOption;
} AMD_BIOS_FLASH_OPTION;

typedef struct {
  UINT64    Offset;
  UINT64    Size;
} PRESERVED_REGION_INFO_ENTRY;

/**
  To perform flash update according specific boot option.

  @retval   EFI_SUCCESS     BIOS flash update success.
  @retval   Others          To reference other functions.

**/
EFI_STATUS
EFIAPI
AmdBiosFlashCallBack (
  VOID
  );

#endif // BIOS_FLASH_UPDATE_H_
