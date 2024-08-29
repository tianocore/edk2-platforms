#/**
#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.<BR>
#  Copyright (c) 2014 - 2016, AMD Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#**/
#/**
#  Derived from:
#   ArmPlatformPkg/ArmVExpressPkg/Library/ArmVExpressLibRTSM/RTSMHelper.S
#
#**/
#include <AsmMacroLib.h>
#include <Library/ArmLib.h>

//VOID
//ArmPlatformPeiBootAction (
//  VOID
//  );
ASM_FUNC(ArmPlatformPeiBootAction)
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
