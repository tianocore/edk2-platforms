/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

//
// First platform specific function to be called in the PEI phase
//
// This function is actually the first function called by the PrePi
// or PrePeiCore modules. It allows to retrieve arguments passed to
// the UEFI firmware through the CPU registers.
//
ASM_FUNC(ArmPlatformPeiBootAction)
  bx   lr
