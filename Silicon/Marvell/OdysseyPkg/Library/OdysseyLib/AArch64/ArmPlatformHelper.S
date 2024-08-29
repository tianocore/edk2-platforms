/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2023 Marvell
*
* Source file for Marvell ARM Platform library
* Based on ArmPlatformPkg/Library/ArmPlatformLibNull
**/

#include <AsmMacroLib.h>
#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>
#include <IndustryStandard/SmcLib.h>

/* x1 - node number
 */

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)
GCC_ASM_EXPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_EXPORT(ArmPlatformGetPrimaryCoreMpId)
GCC_ASM_EXPORT(ArmPlatformGetCorePosition)
GCC_ASM_EXPORT(ArmGetCpuCountPerCluster)

GCC_ASM_IMPORT(mDeviceTreeBaseAddress)
GCC_ASM_IMPORT(mSystemMemoryEnd)

ASM_FUNC(ArmPlatformPeiBootAction)
  // Save the boot parameter to a global variable
  adr   x10, mDeviceTreeBaseAddress
  str   x1, [x10]

  adr   x1, PrimaryCoreMpid
  str   w0, [x1]
  ldr   x0, =MV_SMC_ID_DRAM_SIZE
  mov   x1, xzr
  smc   #0
  sub   x0, x0, #1           // Last valid address
  // if mSystemMemoryEnd wasn't gethered from SMC call, get it from PCDs
  cmp   x0, #0xffffffffffffffff
  bne   done
  // if mSystemMemoryEnd wasn't gethered from SMC call, get it from PCDs
  MOV64 (x0, FixedPcdGet64(PcdSystemMemoryBase) + FixedPcdGet64(PcdSystemMemorySize) - 1)
done:
  adr   x1, mSystemMemoryEnd
  str   x0, [x1]             // Set mSystemMemoryEnd

  ret


//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_FUNC(ArmPlatformGetPrimaryCoreMpId)
  MOV32(w0, FixedPcdGet32(PcdArmPrimaryCore))
  ret

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
  MOV32  (w1, FixedPcdGet32 (PcdArmPrimaryCoreMask))
  and   x0, x0, x1
  MOV32  (w1, FixedPcdGet32 (PcdArmPrimaryCore))
  cmp   w0, w1
  mov   x0, #1
  mov   x1, #0
  csel  x0, x0, x1, eq
  ret

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformGetCorePosition)
/*
  Affinity Level 0: single thread 0
  Affinity Level 1: clustering 0(
  Affinity Level 2: number of clusters up to 64 (CN10K)/ 80 (Odyssey)/ 16 (Iliad)
  Affinity Level 3: number of chip 0
  LinearId = Aff2
*/
  and   x0, x0, #ARM_CORE_AFF2
  lsr   x0, x0, #16
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED

PrimaryCoreMpid:  .word    0x0
