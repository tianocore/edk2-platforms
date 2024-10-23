/** @file

 Copyright (c) 2011-2014, ARM Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <ArmPlatform.h>

/*
 * This is normal world's view of the nor flash area.
 *
 * +--------------------------------+ 0x0c000000
 * |    Variable Storage (256KB)    | (ARM_VE_SMB_NOR0_BASE + ARM_VE_SMB_NOR0_SZ)
 * | (!ENABLE_UEFI_SECURE_VARAIBLE) |
 * +--------------------------------+ 0x0bfC0000
 * |    Reserved (2MB - 256KB)      |
 * +--------------------------------+ 0x0be00000
 * |                                |
 * |                                |
 * |      BootMonfs  (62MB)         |
 * |         or reserved            |
 * |                                |
 * |                                |
 * +--------------------------------+ 0x08000000
 *                                    (ARM_VE_SMB_NOR0_BASE)
 */
NOR_FLASH_DESCRIPTION  mNorFlashDevices[] = {
  {
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE,
    /// Reserve last 2MB for secure storage.
    SIZE_256KB * 248,
    SIZE_256KB,
  },
 #ifndef ENABLE_UEFI_SECURE_VARIABLE
  ///
  /// If variable service is provided in normal world,
  /// Use last 256KB for variable storage.
  ///
  {
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE + SIZE_256KB * 255,
    SIZE_64KB * 4,
    SIZE_64KB,
  },
 #endif
};

UINT32  mNorFlashCount = ARRAY_SIZE (mNorFlashDevices);
