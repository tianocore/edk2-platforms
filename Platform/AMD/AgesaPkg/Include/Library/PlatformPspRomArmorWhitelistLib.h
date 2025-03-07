/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_PSP_ROM_ARMOR_WHILE_LIST_
#define PLATFORM_PSP_ROM_ARMOR_WHILE_LIST_

#include <Library/AmdPspRomArmorLib.h>

/*
 *  Return allocated and filled AMD PSP ROM Armor White list Table
 *
 *
 * @param[in]  PlatformSpiWhitelist   Pointer to white list table
 *
 * @return    EFI_SUCCESS
 * @return    EFI_OUT_OF_RESOURCES      Buffer to return couldn't be allocated
 */
EFI_STATUS
EFIAPI
GetPspRomArmorWhitelist (
  IN       SPI_WHITE_LIST  **PlatformSpiWhitelist
  );

#endif // PLATFORM_PSP_ROM_ARMOR_WHILE_LIST_
