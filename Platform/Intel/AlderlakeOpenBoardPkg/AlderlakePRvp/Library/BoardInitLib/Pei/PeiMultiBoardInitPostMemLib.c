/** @file
  PEI Multi-Board Initialization in Post-Memory PEI Library

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BoardInitLib.h>
#include <Library/MultiBoardInitSupportLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

#include <PlatformBoardId.h>

EFI_STATUS
EFIAPI
AdlPBoardInitBeforeSiliconInit (
  VOID
  );

EFI_STATUS
EFIAPI
AdlPBoardInitAfterSiliconInit (
  VOID
  );

BOARD_POST_MEM_INIT_FUNC  mAdlPBoardInitFunc = {
  AdlPBoardInitBeforeSiliconInit,
  AdlPBoardInitAfterSiliconInit,
};

EFI_STATUS
EFIAPI
PeiAdlPMultiBoardInitLibConstructor (
  VOID
  )
{
  UINT8     SkuType;
  SkuType = PcdGet8 (PcdSkuType);

  if (SkuType==AdlPSkuType) {
    DEBUG ((DEBUG_INFO, "SKU_ID: 0x%x\n", LibPcdGetSku()));
    return RegisterBoardPostMemInit (&mAdlPBoardInitFunc);
  }
  return EFI_SUCCESS;
}
