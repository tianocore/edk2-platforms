/** @file
  Implements AMD FlashUpdateSmm.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLASH_UPDATE_SMM_H_
#define FLASH_UPDATE_SMM_H_

#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmMemLib.h>
#include <Library/PcdLib.h>

#include <Protocol/SpiFlashUpdate.h>
#include <Protocol/SpiCommon.h>

#include <Uefi/UefiBaseType.h>

#include "FlashUpdateCommon.h"
#include "PcRtc.h"

/**
  Read data from flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[out] Buffer                      Buffer contain the read data.

  @retval EFI_SUCCESS                     Read successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceFlashFdRead (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes,
  OUT VOID   *Buffer
  );

/**
  Erase flash region according to input in a block size.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte, a block size in flash device.

  @retval EFI_SUCCESS                     Erase successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceFlashFdErase (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes
  );

/**
  Write data to flash device.

  Write Buffer(FlashAddress|NumBytes) to flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[in]  Buffer                      Buffer contain the write data.

  @retval EFI_SUCCESS                     Write successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceFlashFdWrite (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes,
  IN  UINT8  *Buffer
  );

/**
  Get flash device size and flash block size.

  @param[out] FlashSize                   Pointer to the size of flash device.
  @param[out] BlockSize                   Pointer to the size of block in flash device.

  @retval EFI_SUCCESS                     Get successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.

**/
EFI_STATUS
EFIAPI
FlashUpdateServiceGetFlashSizeBlockSize (
  OUT  UINTN  *FlashSize,
  OUT  UINTN  *BlockSize
  );

/**
  Set AMD Capsule SMM Flag hook

  @param[out] Reserved                    Not used; Must be 0.

  @retval EFI_SUCCESS                     Set successfully.

**/

typedef EFI_STATUS (*AMD_CAPSULE_SMM_HOOK) (
  IN       UINT32  Reserved
  );

typedef struct _AMD_CAPSULE_SMM_HOOK_PROTOCOL {
  AMD_CAPSULE_SMM_HOOK    Hook;
} AMD_CAPSULE_SMM_HOOK_PROTOCOL;

extern EFI_GUID  gAmdSetCapsuleS3FlagGuid;
extern EFI_GUID  gAmdCapsuleSmmHookProtocolGuid;

#endif // _FLASH_UPDATE_SMM_H_
