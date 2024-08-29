#
#  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

#include <AsmMacroLib.h>
#include <AutoGen.h>

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)

ASM_PFX(ArmPlatformPeiBootAction):

  // Invalidate the entire data cache hierarchy by set/way
  mrc   p15, 1, R6, c0, c0, 1   @ Read CLIDR
  ands  R3, R6, #0x7000000      @ Mask out all but Level of Coherency (LoC)
  mov   R3, R3, LSR #23         @ Cache level value (naturally aligned)
  beq   L_Finished
  mov   R10, #0

Loop1:
  add   R2, R10, R10, LSR #1    @ Work out 3xcachelevel
  mov   R12, R6, LSR R2         @ bottom 3 bits are the Cache type for this level
  and   R12, R12, #7            @ get those 3 bits alone
  cmp   R12, #2
  blt   L_Skip                  @ no cache or only instruction cache at this level
  mcr   p15, 2, R10, c0, c0, 0  @ write the Cache Size selection register (CSSELR)
  isb                           @ isb to sync the change to the CacheSizeID reg
  mrc   p15, 1, R12, c0, c0, 0  @ reads current Cache Size ID register (CCSIDR)
  and   R2, R12, #0x7           @ extract the line length field
  add   R2, R2, #4              @ add 4 for the line length offset (log2 16 bytes)
  movw  R4, #0x3ff
  ands  R4, R4, R12, LSR #3     @ R4 is the max number on the way size (right aligned)
  clz   R5, R4                  @ R5 is the bit position of the way size increment
  movw  R7, #0x7fff
  ands  R7, R7, R12, LSR #13    @ R7 is the max number of the index size (right aligned)

Loop2:
  mov   R9, R4                  @ R9 working copy of the max way size (right aligned)

Loop3:
  orr   R0, R10, R9, LSL R5     @ factor in the way number and cache number into R11
  orr   R0, R0, R7, LSL R2      @ factor in the index number

  mcr   p15, 0, r0, c7, c14, 2  @ Invalidate this line

  subs  R9, R9, #1              @ decrement the way number
  bge   Loop3
  subs  R7, R7, #1              @ decrement the index
  bge   Loop2
L_Skip:
  add   R10, R10, #2            @ increment the cache number
  cmp   R3, R10
  bgt   Loop1

L_Finished:
  dsb
  bx    lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
