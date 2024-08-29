/** @file
*
*  Copyright (c) 2018, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

.text
.align 3

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)
GCC_ASM_IMPORT(NtFwConfigDtBlob)

//
// First platform specific function to be called in the PEI phase
//
// The trusted firmware passed the hw config DT blob in x1 register.
// Keep a copy of it in a global variable.
//VOID
//ArmPlatformPeiBootAction (
//  VOID
//  );
ASM_PFX(ArmPlatformPeiBootAction):
  adr  x10, NtFwConfigDtBlob
  str  x0, [x10]
  ret
