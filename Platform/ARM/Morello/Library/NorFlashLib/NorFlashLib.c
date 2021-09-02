/** @file
  NOR flash lib for Morello

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <MorelloPlatform.h>
#include <PiDxe.h>

#define FW_ENV_REGION_BASE  FixedPcdGet32 (PcdFlashNvStorageVariableBase)
#define FW_ENV_REGION_SIZE  (FixedPcdGet32 (PcdFlashNvStorageVariableSize) +           \
                                      FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) + \
                                      FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize))

STATIC NOR_FLASH_DESCRIPTION  mNorFlashDevices[] = {
  {
    // Environment variable region
    MORELLO_AP_QSPI_AHB_BASE,                           // device base
    FW_ENV_REGION_BASE,                                 // region base
    FW_ENV_REGION_SIZE,                                 // region size
    SIZE_4KB,                                           // block size
  },
};

/**
  Get NOR flash region info

  @param[out]    NorFlashDevices    NOR flash regions info.
  @param[out]    Count              number of flash instance.

  @retval        EFI_SUCCESS        Success.
**/
EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION  **NorFlashDevices,
  OUT UINT32                 *Count
  )
{
  if ((NorFlashDevices == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *NorFlashDevices = mNorFlashDevices;
  *Count           = ARRAY_SIZE (mNorFlashDevices);
  return EFI_SUCCESS;
}
