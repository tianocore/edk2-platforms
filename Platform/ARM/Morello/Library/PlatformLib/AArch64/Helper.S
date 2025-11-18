/** @file

  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

.text
.align 3

//
// First platform specific function to be called in the PEI phase
//
// This function is actually the first function called by the PrePi
// or PrePeiCore modules. It allows to retrieve arguments passed to
// the UEFI firmware through the CPU registers.
//
ASM_FUNC(ArmPlatformPeiBootAction)
  adr  x10, gArgNtFwConfigDtPtr
  str  x0, [x10]
  adr  x10, gArgHwConfigDtPtr
  str  x1, [x10]
  ret
