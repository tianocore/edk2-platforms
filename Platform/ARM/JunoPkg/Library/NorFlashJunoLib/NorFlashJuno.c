/** @file

 Copyright (c) 2011-2024, Arm Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <ArmPlatform.h>

extern NOR_FLASH_DESCRIPTION mNorFlashDevices[];
extern UINT32                mNorFlashCount;

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  // Everything seems ok so far, so now we need to disable the platform-specific
  // flash write protection for Versatile Express
  if ((MmioRead32 (ARM_VE_SYS_FLASH) & 0x1) == 0) {
    // Writing to NOR FLASH is disabled, so enable it
    MmioWrite32 (ARM_VE_SYS_FLASH, 1);
    DEBUG((DEBUG_BLKIO, "NorFlashPlatformInitialization: informational - Had to enable HSYS_FLASH flag.\n" ));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION   **NorFlashDevices,
  OUT UINT32                  *Count
  )
{
  if ((NorFlashDevices == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *NorFlashDevices = mNorFlashDevices;
  *Count = mNorFlashCount;

  return EFI_SUCCESS;
}
