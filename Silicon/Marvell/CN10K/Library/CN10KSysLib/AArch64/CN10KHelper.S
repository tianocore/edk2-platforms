#
#  Copyright (c) 2015-2016, Cavium Inc. All rights reserved.
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#  Copyright (c) 2026, Marvell. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent

#include <IndustryStandard/SmcLib.h>
#include <AsmMacroLib.h>
#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

/* x1 - node number
 */

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)

GCC_ASM_IMPORT(mSystemMemoryEnd)
GCC_ASM_IMPORT(mDeviceTreeBaseAddress)

ASM_FUNC(ArmPlatformPeiBootAction)
  // Save the Device Tree pointer to a global variable
  adr   x10, mDeviceTreeBaseAddress
  str   x1, [x10]

  ldr   x0, =MV_SMC_ID_DRAM_SIZE
  mov   x1, xzr
  smc   #0
  sub   x0, x0, #1           // Last valid address
  adr   x1, mSystemMemoryEnd
  str   x0, [x1]             // Set mSystemMemoryEnd

  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
