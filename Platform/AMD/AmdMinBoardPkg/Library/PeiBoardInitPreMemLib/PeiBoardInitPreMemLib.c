/** @file
  BoardInitLib library implementation for pre-mem PEI phase.

Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BoardInitLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include "PeiMemoryInit.h"

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gAmdMemoryInfoHobPpiGuid,
  EndofAmdMemoryInfoHobPpiGuidCallBack
};

/**
  Get Pcie base address from MSR and set PcdPciExpressBaseSize

  @retval EFI_SUCCESS   PcdPciExpressBaseSize value set successfully.
**/
EFI_STATUS
EFIAPI
SetPcieBaseSize (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      PcieBaseSize;
  UINT8       BusRange;

  Status = EFI_SUCCESS;

  // Gather the value of PcdPciExpressBaseSize from MSR
  BusRange     =  RShiftU64 (AsmReadMsr64 (0xC0010058), 2) & 0xF;
  PcieBaseSize =  MultU64x64 (LShiftU64 (1, BusRange), SIZE_1MB);
  PcdSet64S (PcdPciExpressBaseSize, (UINT64)PcieBaseSize);
  return Status;
}

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
  EFI_STATUS  Status;

  Status = PeiServicesNotifyPpi (&mNotifyList);
  ASSERT_EFI_ERROR (Status);
  Status = SetPcieBaseSize ();
  ASSERT_EFI_ERROR (Status);
  return (Status);
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
