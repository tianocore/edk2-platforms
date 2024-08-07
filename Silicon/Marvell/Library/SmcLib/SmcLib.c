/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2023 Marvell
*
* Source file for Marvell SMC Interface
*
**/

#include <IndustryStandard/SmcLib.h>
#include <Library/ArmSmcLib.h>  // ArmCallSmc

UINTN SmcGetRamSize ( IN UINTN Node )
{
  ARM_SMC_ARGS ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_ID_DRAM_SIZE;
  ArmSmcArgs.Arg1 = Node;
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}
