/** @file
  Board initialization library for the Qualcomm Nord Generic.

  Implements the MinPlatformPkg BoardInitLib hooks for Nord. The hooks are
  currently minimal: the GENI UART console is brought up by SerialPortLib and
  the memory map (including the secure/firmware carveouts) comes from PEI
  ArmPlatformLib and the device tree, so the PEI-phase hooks only trace their
  invocation.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BoardInitLib.h>
#include <Library/DebugLib.h>

/**
  This board service detects the board type.

  @retval EFI_SUCCESS   The board initialization was successful.
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
  This board service initializes board-specific debug devices.

  @retval EFI_SUCCESS   The board initialization was successful.
**/
EFI_STATUS
EFIAPI
BoardDebugInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "BoardDebugInit()\n"));
  return EFI_SUCCESS;
}

/**
  A hook for board-specific initialization prior to memory initialization.

  @retval EFI_SUCCESS   The board initialization was successful.
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
