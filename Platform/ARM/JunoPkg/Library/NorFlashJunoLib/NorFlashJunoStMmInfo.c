/** @file

 Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Base.h>
#include <Library/NorFlashPlatformLib.h>
#include <ArmPlatform.h>

/*
 * This is secure world's view of the nor flash area.
 *
 * +--------------------------------+ 0x0c000000
 * |    Variable Storage (256KB)    | (ARM_VE_SMB_NOR0_BASE + ARM_VE_SMB_NOR0_SZ)
 * |  (ENABLE_UEFI_SECURE_VARAIBLE) |
 * +--------------------------------+ 0x0bfC0000
 * |     TPM Nv Storage (256KB)     |
 * |          (ENABLE_TPM)          |
 * +--------------------------------+ 0x0bf80000
 * |                                |
 * |                                |
 * |     Reserved  (63MB - 512KB)   |
 * |                                |
 * |                                |
 * +--------------------------------+ 0x08000000
 *                                    (ARM_VE_SMB_NOR0_BASE)
 */
NOR_FLASH_DESCRIPTION  mNorFlashDevices[] = {
  { /// fTpm Nv area. This should be over PcdTpmNvMemoryBaseSize + 0x200 (metadata block).
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE + SIZE_256KB * 254,
    SIZE_256KB,
    SIZE_64KB,
  },
 #ifdef ENABLE_UEFI_SECURE_VARIABLE
  { /// UEFI Variable Services non-volatile storage
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE + SIZE_256KB * 255,
    SIZE_64KB * 4,
    SIZE_64KB,
  },
 #endif
};

UINT32  mNorFlashCount = ARRAY_SIZE (mNorFlashDevices);
