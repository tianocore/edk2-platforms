//Based on ArmPlatformPkg/Library/ArmPlatformLibNull/AArch64/ArmPlatformHelper.S
//
//  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
//  Copyright (c) 2016, Marvell. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED
//

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

ASM_FUNC(ArmPlatformPeiBootAction)
  mov   x29, xzr
  ret
