/** @file
  Implements CapsuleHookLib.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CAPSULE_HOOK_LIB_H_
#define CAPSULE_HOOK_LIB_H_

/**
  Detect Capsule file from ESP partition and update capsule.

  @retval EFI_SUCCESS.              Opertion is successful.
  @retval EFI_OUT_OF_RESOURCES      No enough buffer to allocate.
  @retval EFI_ERROR                 Internal error when update Capsule.

**/
EFI_STATUS
EFIAPI
CapsuleUpdateViaFileHook (
  VOID
  );

/**
  Detect Capsule file from ESP partition and update capsule.

  @retval EFI_SUCCESS.              Opertion is successful.
  @retval EFI_OUT_OF_RESOURCES      No enough buffer to allocate.
  @retval EFI_ERROR                 Internal error when update Capsule.

**/
EFI_STATUS
EFIAPI
CapsuleUpdateViaFileLib (
  VOID
  );

#endif
