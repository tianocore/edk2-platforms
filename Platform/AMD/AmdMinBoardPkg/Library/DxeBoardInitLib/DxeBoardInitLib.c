/** @file
  BoardInitLib library implementation for DXE phase.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BoardInitLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include "DxeBoardInitLibInternal.h"

EFI_HANDLE        mImageHandle;
EFI_SYSTEM_TABLE  *mSystemTable;

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

  @retval EFI_BOOT_MODE The boot mode.
  @retval EFI_NOT_READY The board has not been detected yet.
**/
EFI_BOOT_MODE
EFIAPI
BoardBootModeDetect (
  VOID
  )
{
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
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  Status = ReserveLegacyVgaIoSpace ();
  DEBUG ((DEBUG_INFO, "ReserveLegacyVgaIoSpace...%r.\n", Status));

  Status = ReservePcieExtendedConfigSpace (mImageHandle, mSystemTable);
  DEBUG ((DEBUG_INFO, "ReservePcieExtendedConfigSpace...%r.\n", Status));

  return Status;
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
  return EFI_SUCCESS;
}

/**
  The constructor function caches the PCI Express Base Address and creates a
  Set Virtual Address Map event to convert physical address to virtual addresses.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor completed successfully.
  @retval Other value   The constructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
DxeBoardInitLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mImageHandle = ImageHandle;
  mSystemTable = SystemTable;
  return EFI_SUCCESS;
}
