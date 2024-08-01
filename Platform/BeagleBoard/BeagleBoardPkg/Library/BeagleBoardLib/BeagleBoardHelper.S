#
#  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

#include <AsmMacroIoLib.h>
#include <AutoGen.h>

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)

ASM_PFX(ArmPlatformPeiBootAction):
  bx    lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
