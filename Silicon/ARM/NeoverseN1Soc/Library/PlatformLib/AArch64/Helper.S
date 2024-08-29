/** @file
*
*  Copyright (c) 2019 - 2024, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

.text
.align 3

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)

//
// First platform specific function to be called in the PEI phase
//
// This function is actually the first function called by the PrePi
// or PrePeiCore modules. It allows to retrieve arguments passed to
// the UEFI firmware through the CPU registers.
//
ASM_PFX(ArmPlatformPeiBootAction):
  adr  x10, gArgNtFwConfigDtPtr
  str  x0, [x10]
  ret
