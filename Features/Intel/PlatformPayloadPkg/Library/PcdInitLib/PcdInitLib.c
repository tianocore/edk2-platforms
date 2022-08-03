/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/NvVariableInfoGuid.h>
#include <Library/HobLib.h>


/**
  Initialize the variable store

  @retval     EFI_SUCCESS if initialize the store success.

**/
EFI_STATUS
PcdInitConstructor (
  VOID
  )
{
  EFI_STATUS         Status;
  UINT32             NvStorageBase;
  UINT32             NvStorageSize;
  UINT32             NvVariableSize;
  UINT32             FtwWorkingSize;
  UINT32             FtwSpareSize;
  EFI_HOB_GUID_TYPE  *GuidHob;
  NV_VARIABLE_INFO   *NvVariableInfo;

  //
  // Find SPI flash variable hob
  //
  GuidHob = GetFirstGuidHob (&gNvVariableInfoGuid);
  if (GuidHob == NULL) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  NvVariableInfo = (NV_VARIABLE_INFO *)GET_GUID_HOB_DATA (GuidHob);

  //
  // Get variable region base and size.
  //
  NvStorageSize = NvVariableInfo->VariableStoreSize;
  NvStorageBase = NvVariableInfo->VariableStoreBase;

  //
  // NvStorageBase needs to be 4KB aligned, NvStorageSize needs to be 8KB * n
  //
  if (((NvStorageBase & (SIZE_4KB - 1)) != 0) || ((NvStorageSize & (SIZE_8KB - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  FtwSpareSize   = NvStorageSize / 2;
  FtwWorkingSize = 0x2000;
  NvVariableSize = NvStorageSize / 2 - FtwWorkingSize;
  DEBUG ((DEBUG_INFO, "NvStorageBase:0x%x, NvStorageSize:0x%x\n", NvStorageBase, NvStorageSize));

  if (NvVariableSize >= 0x80000000) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PcdSet32S (PcdFlashNvStorageVariableSize, NvVariableSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageVariableBase, NvStorageBase);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageVariableBase64, NvStorageBase);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingSize, FtwWorkingSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingBase, NvStorageBase + NvVariableSize);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdFlashNvStorageFtwSpareSize, FtwSpareSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageFtwSpareBase, NvStorageBase + FtwSpareSize);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

