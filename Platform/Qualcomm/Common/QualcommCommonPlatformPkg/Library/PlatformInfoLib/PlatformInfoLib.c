/** @file

  Interface implementation file for the PlatformInfo driver.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/SmemLib.h>
#include "PlatformInfoInternal.h"

/**
  Populates DrvCtx with the platform information read from Smem.

  @param[in,out]  DrvCtx  Driver context to populate.
  @param[in]      Smem    Pointer to the SMEM structure to read from.

  @retval EFI_SUCCESS          Population completed successfully.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate space for the KVP array.
**/
STATIC
EFI_STATUS
PlatformInfoPopulateCtxFromSmem (
  IN OUT PlatformInfoDrvCtx    *DrvCtx,
  IN     PlatformInfoSmemType  *Smem
  )
{
  UINT32                Size;
  PlatformInfoKvpsType  *Kvps;

  DrvCtx->PlatformInfo.PlatformType = Smem->PlatformType;
  DrvCtx->PlatformInfo.Version      = Smem->PlatformVersion;
  DrvCtx->PlatformInfo.Subtype      = Smem->PlatformSubtype;
  DrvCtx->PlatformInfo.Fusion       = Smem->Fusion;

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 17)) {
    DrvCtx->PlatformInfo.OemVariantId = Smem->OemVariantId;
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 18)) {
    if (Smem->NumKvps > 0) {
      Size          = sizeof (PlatformInfoKvpsType) * Smem->NumKvps;
      DrvCtx->Kvps = AllocatePages (EFI_SIZE_TO_PAGES (Size));
      if (DrvCtx->Kvps == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Kvps = (PlatformInfoKvpsType *)((UINT8 *)Smem + Smem->KvpsOffset);
      CopyMem (DrvCtx->Kvps, Kvps, Size);
      DrvCtx->NumKvps = Smem->NumKvps;
    }
  }

  return EFI_SUCCESS;
}

/**
  Initializes the PlatformInfo driver.

  @retval EFI_SUCCESS    Initialization completed successfully.
  @retval EFI_NOT_FOUND  Failed to retrieve platform information from SMEM.
**/
EFI_STATUS
EFIAPI
PlatformInfoInit (
  VOID
  )
{
  PlatformInfoDrvCtx    *DrvCtx;
  UINT32                Size;
  PlatformInfoSmemType  *Smem;
  EFI_STATUS            Status;

  DrvCtx = PlatformInfoGetDrvCtx ();

  if (DrvCtx->Initialized) {
    return EFI_SUCCESS;
  }

  Smem = SmemGetAddr (SmemHwSwBuildId, &Size);
  if (Smem == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PlatformInfoPopulateCtxFromSmem (DrvCtx, Smem);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DrvCtx->Initialized = TRUE;
  return EFI_SUCCESS;
}
