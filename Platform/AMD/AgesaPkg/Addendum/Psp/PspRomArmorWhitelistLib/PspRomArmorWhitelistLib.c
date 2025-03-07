/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/DebugLib.h>
#include <Library/AmdPspRomArmorLib.h>
#include <Library/PlatformPspRomArmorWhitelistLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

/*
  Return allocated and filled AMD PSP ROM Armor White list Table

  @param[in]  PlatformSpiWhitelist   Pointer to white list table

  @return    EFI_SUCCESS
  @return    EFI_OUT_OF_RESOURCES      Buffer to return could not be allocated
 */
EFI_STATUS
EFIAPI
GetPspRomArmorWhitelist (
  IN       SPI_WHITE_LIST  **PlatformSpiWhitelist
  )
{
  return EFI_SUCCESS;
}
