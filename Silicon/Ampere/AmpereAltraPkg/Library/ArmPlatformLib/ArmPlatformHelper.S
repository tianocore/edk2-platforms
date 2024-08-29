/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
 * Derived from edk2/ArmPlatformPkg/Library/ArmPlatformLibNull/AArch64/ArmPlatformHelper.S
 **/

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

ASM_FUNC(ArmPlatformPeiBootAction)
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
