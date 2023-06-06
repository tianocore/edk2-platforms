/** @file
  DXE Multi-Board Initilialization Library

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

@par Specification Reference:
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MultiBoardInitSupportLib.h>
#include <PlatformBoardId.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
AdlPBoardInitAfterPciEnumeration (
  VOID
  );

EFI_STATUS
EFIAPI
AdlPBoardInitReadyToBoot (
  VOID
  );

EFI_STATUS
EFIAPI
AdlPBoardInitEndOfFirmware (
  VOID
  );

BOARD_NOTIFICATION_INIT_FUNC mAdlPBoardDxeInitFunc = {
  AdlPBoardInitAfterPciEnumeration,
  AdlPBoardInitReadyToBoot,
  AdlPBoardInitEndOfFirmware
};

/**
  The constructor determines which board init functions should be registered.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeAdlPMultiBoardInitLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT8  SkuType;
  SkuType = PcdGet8 (PcdSkuType);

  if (SkuType==AdlPSkuType) {
    DEBUG ((DEBUG_INFO, "SKU_ID: 0x%x\n", LibPcdGetSku()));
    return RegisterBoardNotificationInit (&mAdlPBoardDxeInitFunc);
  }
  return EFI_SUCCESS;
}
