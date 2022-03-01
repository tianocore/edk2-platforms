#------------------------------------------------------------------------------
#
# Fpu for LoongArch
#
# Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#  @par Glossary:
#    - CsrEuen   - Cpu Status Register Extern Unit Enable
#    - FPEN      - FPU Enable
#    - fpu or fp or FP   - Float Point Unit
#-----------------------------------------------------------------------------
#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif
#include "Library/Cpu.h"
#include "CpuDxe.h"

ASM_GLOBAL ASM_PFX(InitFpu)
ASM_GLOBAL ASM_PFX(LoongArchEnableFpu)
ASM_GLOBAL ASM_PFX(LoongArchDisableFpu)

#
#  Load the FPU with signalling NANS.  This bit pattern we're using has
#  the property that no matter whether considered as single or as double
#  precision represents signaling NANS.
#
#  The value to initialize FCSR0 to comes in $A0.
#

ASM_PFX(InitFpu):
  li.d    T1, CSR_EUEN_FPEN
  csrxchg T1, T1, LOONGARCH_CSR_EUEN

  movgr2fcsr      FCSR0, A0
  li.d    T1, -1                          # SNaN
  movgr2fr.d      $f0, T1
  movgr2fr.d      $f1, T1
  movgr2fr.d      $f2, T1
  movgr2fr.d      $f3, T1
  movgr2fr.d      $f4, T1
  movgr2fr.d      $f5, T1
  movgr2fr.d      $f6, T1
  movgr2fr.d      $f7, T1
  movgr2fr.d      $f8, T1
  movgr2fr.d      $f9, T1
  movgr2fr.d      $f10, T1
  movgr2fr.d      $f11, T1
  movgr2fr.d      $f12, T1
  movgr2fr.d      $f13, T1
  movgr2fr.d      $f14, T1
  movgr2fr.d      $f15, T1
  movgr2fr.d      $f16, T1
  movgr2fr.d      $f17, T1
  movgr2fr.d      $f18, T1
  movgr2fr.d      $f19, T1
  movgr2fr.d      $f20, T1
  movgr2fr.d      $f21, T1
  movgr2fr.d      $f22, T1
  movgr2fr.d      $f23, T1
  movgr2fr.d      $f24, T1
  movgr2fr.d      $f25, T1
  movgr2fr.d      $f26, T1
  movgr2fr.d      $f27, T1
  movgr2fr.d      $f28, T1
  movgr2fr.d      $f29, T1
  movgr2fr.d      $f30, T1
  movgr2fr.d      $f31, T1

  jirl ZERO, RA, 0

#
#  Enables  floating-point unit
#  @param  VOID
#  @retval  VOID
#

ASM_PFX(LoongArchEnableFpu):
    li.d    T0, 1
    li.d    T1, CSR_EUEN_FPEN_SHIFT
    sll.d   T0, T0, T1
    csrxchg T0, T0, LOONGARCH_CSR_EUEN
    jirl    ZERO, RA,0

#
#  Disable  floating-point unit
#  @param  VOID
#  @retval  VOID
#

ASM_PFX(LoongArchDisableFpu):
    li.d    T0, 1
    li.d    T1, CSR_EUEN_FPEN_SHIFT
    sll.d   T0, T0, T1
    csrxchg ZERO, T0, LOONGARCH_CSR_EUEN
    jirl    ZERO, RA,0
