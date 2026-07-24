/** @file
  Board initialization library

  Copyright (c) 2022 Theo Jehl All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
 
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BoardInitLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

/**
  This board service detects the board type.

  @retval EFI_SUCCESS   The board was detected successfully.
  @retval EFI_NOT_FOUND The board could not be detected.
**/
EFI_STATUS
EFIAPI
BoardDetect (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardDetect()\n"));
  return EFI_SUCCESS;
}

/**
  This board service initializes board-specific debug devices.

  @retval EFI_SUCCESS   Board-specific debug initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardDebugInit (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  This board service detects the boot mode.

  @return  BOOT_WITH_FULL_CONFIGURATION always.
**/
EFI_BOOT_MODE
EFIAPI
BoardBootModeDetect (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardBootModeDetect()\n"));
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  A hook for board-specific initialization prior to memory initialization.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitBeforeMemoryInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitBeforeMemoryInit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization after memory initialization.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitAfterMemoryInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitAfterMemoryInit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization prior to disabling temporary RAM.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitBeforeTempRamExit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitBeforeTempRamExit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization after disabling temporary RAM.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitAfterTempRamExit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitAfterTempRamExit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization prior to silicon initialization.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitBeforeSiliconInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitBeforeSiliconInit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization after silicon initialization.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitAfterSiliconInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitAfterSiliconInit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization after PCI enumeration.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitAfterPciEnumeration (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitAfterPciEnumeration()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific functionality for the ReadyToBoot event.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitReadyToBoot (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitReadyToBoot()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific functionality for the ExitBootServices event.

  @retval EFI_SUCCESS   The board initialization was successful.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_STATUS
EFIAPI
BoardInitEndOfFirmware (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardInitEndOfFirmware()\n"));
  return EFI_SUCCESS;
}
