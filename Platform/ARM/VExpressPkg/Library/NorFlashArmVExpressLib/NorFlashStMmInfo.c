/** @file

 Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Base.h>
#include <Library/NorFlashPlatformLib.h>
#include <ArmPlatform.h>

NOR_FLASH_DESCRIPTION mNorFlashDevices[] = {
#ifdef ENABLE_FIRMWARE_UPDATE
  { // FWU
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE,
    SIZE_64MB,
    512,
  },
#endif
#ifdef ENABLE_UEFI_SECURE_VARIABLE
  {
    ARM_VE_SMB_NOR1_BASE,
    ARM_VE_SMB_NOR1_BASE,
    SIZE_256KB * 255,
    SIZE_256KB,
  },
  { // UEFI Variable Services non-volatile storage
    ARM_VE_SMB_NOR1_BASE,
    ARM_VE_SMB_NOR1_BASE + SIZE_256KB * 255,
    SIZE_64KB * 4,
    SIZE_64KB,
  },
#endif
};

UINT32 mNorFlashCount = ARRAY_SIZE (mNorFlashDevices);
