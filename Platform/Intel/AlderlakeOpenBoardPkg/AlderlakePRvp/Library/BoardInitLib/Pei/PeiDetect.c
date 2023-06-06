/** @file

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <PlatformBoardType.h>
#include <PlatformBoardId.h>
#include <PlatformBoardConfig.h>
#include <Pins/GpioPinsVer2Lp.h>
#include <Library/BoardConfigLib.h>

BOOLEAN
IsAdlP (
  VOID
  )
{
  UINT16          BoardId;
   BoardId = PcdGet16 (PcdBoardId);
  if (BoardId == 0) {
    DEBUG ((DEBUG_INFO, "Let's get Board information first ...\n"));
    GetBoardConfig ();
    BoardId = PcdGet16 (PcdBoardId);
  }
  switch (BoardId) {
    case BoardIdAdlPDdr5Rvp:
        DEBUG ((DEBUG_INFO, "AlderLake P Board detected\n"));

      // set sku type to ADL P
      PcdSet8S (PcdSkuType, AdlPSkuType);
      return TRUE;
      break;
    default:
      return FALSE;
  }
}

EFI_STATUS
EFIAPI
AdlPBoardDetect (
  VOID
  )
{
  UINTN          SkuId;
  SkuId      = 0;

  if (LibPcdGetSku () != 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "AlderLakeP Board Detection Callback\n"));

  if (IsAdlP ()) {
    SkuId = (UINTN) (PcdGet16 (PcdBoardBomId) << 16) | (PcdGet16 (PcdBoardRev) << 8) | (PcdGet16 (PcdBoardId));
    LibPcdSetSku (SkuId);
    DEBUG ((DEBUG_INFO, "SKU_ID: 0x%x\n", LibPcdGetSku()));
  }
  return EFI_SUCCESS;
}
