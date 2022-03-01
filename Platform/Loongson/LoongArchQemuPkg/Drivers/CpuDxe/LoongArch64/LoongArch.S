#------------------------------------------------------------------------------
#
# LoongArch for LoongArch
#
# Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#  @par Glossary:
#    - CsrEuen      - Cpu Status Register Extern Unit Enable
#    - fpu    - Float Point Unit
#    - LOONGARCH   - Loongson Arch
#    - Ebase - Exception Base Address
#-----------------------------------------------------------------------------

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include "Library/Cpu.h"
#include "CpuDxe.h"

#define RSIZE   8       /* 64 bit mode register size */
#define RLOGSIZE 3

ASM_GLOBAL ASM_PFX(Exception_handler)
ASM_GLOBAL ASM_PFX(LoongArchException)
ASM_GLOBAL ASM_PFX(SetEbase)
ASM_GLOBAL ASM_PFX(LoongArchReadqCsrEuen)
ASM_GLOBAL ASM_PFX(LoongArchWriteqCsrEuen)

#
#   Main exception handler. Not really a leaf routine but not a normal
#   function either. Save away the entire cpu state end enter exception mode.
#

ASM_PFX(Exception_handler):
    csrrd   SP, LOONGARCH_CSR_KS1

    addi.d  T0, $r0, -0x10
    and     SP, SP, T0
    addi.d  SP, SP,  -((CSR_NUM + BASE_NUM + FP_BASE_NUM)  * RSIZE)

    st.d   RA, SP, RA_NUM * RSIZE
    st.d   GP, SP, GP_NUM * RSIZE
    st.d   A0, SP, A0_NUM * RSIZE
    st.d   A1, SP, A1_NUM * RSIZE
    st.d   A2, SP, A2_NUM * RSIZE
    st.d   A3, SP, A3_NUM * RSIZE
    st.d   A4, SP, A4_NUM * RSIZE
    st.d   A5, SP, A5_NUM * RSIZE
    st.d   A6, SP, A6_NUM * RSIZE
    st.d   A7, SP, A7_NUM * RSIZE
    st.d   T1, SP, T1_NUM * RSIZE
    st.d   T2, SP, T2_NUM * RSIZE
    st.d   T3, SP, T3_NUM * RSIZE
    st.d   T4, SP, T4_NUM * RSIZE
    st.d   T5, SP, T5_NUM * RSIZE
    st.d   T6, SP, T6_NUM * RSIZE
    st.d   T7, SP, T7_NUM * RSIZE
    st.d   T8, SP, T8_NUM * RSIZE
    st.d   TP, SP, TP_NUM * RSIZE
    st.d   FP, SP, FP_NUM * RSIZE
    st.d   S0, SP, S0_NUM * RSIZE
    st.d   S1, SP, S1_NUM * RSIZE
    st.d   S2, SP, S2_NUM * RSIZE
    st.d   S3, SP, S3_NUM * RSIZE
    st.d   S4, SP, S4_NUM * RSIZE
    st.d   S5, SP, S5_NUM * RSIZE
    st.d   S6, SP, S6_NUM * RSIZE
    st.d   S7, SP, S7_NUM * RSIZE
    st.d   S8, SP, S8_NUM * RSIZE

    #
    #  save T0/SP from scratch registers on stack
    #
    csrrd  T0, LOONGARCH_CSR_KS0
    st.d   T0, SP, T0_NUM * RSIZE
    csrrd  T0, LOONGARCH_CSR_KS1
    st.d   T0, SP, SP_NUM * RSIZE

    csrrd   T0, LOONGARCH_CSR_CRMD
    st.d    T0, SP, (LOONGARCH_CSR_CRMD + BASE_NUM)  * RSIZE
    csrrd   T0, LOONGARCH_CSR_PRMD
    st.d    T0, SP, (LOONGARCH_CSR_PRMD + BASE_NUM)  * RSIZE
    csrrd   T0, LOONGARCH_CSR_ECFG
    st.d    T0, SP, (LOONGARCH_CSR_ECFG + BASE_NUM) * RSIZE
    csrrd   T0, LOONGARCH_CSR_ESTAT
    st.d    T0, SP, (LOONGARCH_CSR_ESTAT + BASE_NUM)  * RSIZE
    csrrd   T0, LOONGARCH_CSR_EPC
    st.d    T0, SP, (LOONGARCH_CSR_EPC+ BASE_NUM)    * RSIZE
    csrrd   T0, LOONGARCH_CSR_BADV
    st.d    T0, SP, (LOONGARCH_CSR_BADV + BASE_NUM)  * RSIZE
    csrrd   T0, LOONGARCH_CSR_BADI
    st.d    T0, SP, (LOONGARCH_CSR_BADI + BASE_NUM)  * RSIZE
    csrrd   T0, LOONGARCH_CSR_EUEN
    st.d    T0, SP, (LOONGARCH_CSR_EUEN + BASE_NUM)  * RSIZE

    #
    #  Save FPU context
    #
    ori     T1, ZERO, CSR_EUEN_FPEN
    and     T2, T0, T1
    beqz    T2, 1f

    fst.d   $f0,  SP, (FP0_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f1,  SP, (FP1_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f2,  SP, (FP2_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f3,  SP, (FP3_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f4,  SP, (FP4_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f5,  SP, (FP5_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f6,  SP, (FP6_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f7,  SP, (FP7_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f8,  SP, (FP8_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f9,  SP, (FP9_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f10, SP, (FP10_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f11, SP, (FP11_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f12, SP, (FP12_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f13, SP, (FP13_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f14, SP, (FP14_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f15, SP, (FP15_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f16, SP, (FP16_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f17, SP, (FP17_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f18, SP, (FP18_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f19, SP, (FP19_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f20, SP, (FP20_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f21, SP, (FP21_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f22, SP, (FP22_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f23, SP, (FP23_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f24, SP, (FP24_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f25, SP, (FP25_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f26, SP, (FP26_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f27, SP, (FP27_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f28, SP, (FP28_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f29, SP, (FP29_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f30, SP, (FP30_NUM + FP_BASE_INDEX) * RSIZE
    fst.d   $f31, SP, (FP31_NUM + FP_BASE_INDEX) * RSIZE

    movfcsr2gr      T3, FCSR0
    st.d            T3, SP, (FCSR_NUM + FP_BASE_INDEX) * RSIZE
    movcf2gr        T3, $fcc0
    or              T2, T3, ZERO
    movcf2gr        T3, $fcc1
    bstrins.d       T2, T3, 0xf, 0x8
    movcf2gr        T3, $fcc2
    bstrins.d       T2, T3, 0x17, 0x10
    movcf2gr        T3, $fcc3
    bstrins.d       T2, T3, 0x1f, 0x18
    movcf2gr        T3, $fcc4
    bstrins.d       T2, T3, 0x27, 0x20
    movcf2gr        T3, $fcc5
    bstrins.d       T2, T3, 0x2f, 0x28
    movcf2gr        T3, $fcc6
    bstrins.d       T2, T3, 0x37, 0x30
    movcf2gr        T3, $fcc7
    bstrins.d       T2, T3, 0x3f, 0x38
    st.d            T2, SP, (FCC_NUM + FP_BASE_INDEX)  * RSIZE
1:
    or      A0, SP, ZERO
    bl      CommonExceptionEntry
    /*disable interrupt*/
    li.d     T0, (1 << 2)
    csrxchg ZERO, T0, LOONGARCH_CSR_CRMD

    ld.d    T0, SP, (LOONGARCH_CSR_PRMD + BASE_NUM) * RSIZE
    csrwr   T0, LOONGARCH_CSR_PRMD
    ld.d    T0, SP, (LOONGARCH_CSR_ECFG + BASE_NUM) * RSIZE
    csrwr   T0, LOONGARCH_CSR_ECFG
    ld.d    T0, SP, (LOONGARCH_CSR_EPC + BASE_NUM) * RSIZE
    csrwr   T0, LOONGARCH_CSR_EPC

    ld.d    T0, SP, (LOONGARCH_CSR_EUEN + BASE_NUM) * RSIZE
    ori     T1, ZERO, CSR_EUEN_FPEN
    and     T2, T0, T1
    beqz    T2, 2f

    #
    #  check previous FP state
    #  restore FP contect if FP enabled
    #
    fld.d   $f0,  SP, (FP0_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f1,  SP, (FP1_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f2,  SP, (FP2_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f3,  SP, (FP3_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f4,  SP, (FP4_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f5,  SP, (FP5_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f6,  SP, (FP6_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f7,  SP, (FP7_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f8,  SP, (FP8_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f9,  SP, (FP9_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f10, SP, (FP10_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f11, SP, (FP11_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f12, SP, (FP12_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f13, SP, (FP13_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f14, SP, (FP14_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f15, SP, (FP15_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f16, SP, (FP16_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f17, SP, (FP17_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f18, SP, (FP18_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f19, SP, (FP19_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f20, SP, (FP20_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f21, SP, (FP21_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f22, SP, (FP22_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f23, SP, (FP23_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f24, SP, (FP24_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f25, SP, (FP25_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f26, SP, (FP26_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f27, SP, (FP27_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f28, SP, (FP28_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f29, SP, (FP29_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f30, SP, (FP30_NUM + FP_BASE_INDEX) * RSIZE
    fld.d   $f31, SP, (FP31_NUM + FP_BASE_INDEX) * RSIZE

    ld.d    T0, SP, (FCSR_NUM + FP_BASE_INDEX) * RSIZE
    movgr2fcsr      FCSR0, T0
    ld.d    T0, SP, (FCC_NUM + FP_BASE_INDEX) * RSIZE
    bstrpick.d      T1, T0, 7, 0
    movgr2cf        $fcc0, T1
    bstrpick.d      T1, T0, 15, 8
    movgr2cf        $fcc1, T1
    bstrpick.d      T1, T0, 23, 16
    movgr2cf        $fcc2, T1
    bstrpick.d      T1, T0, 31, 24
    movgr2cf        $fcc3, T1
    bstrpick.d      T1, T0, 39, 32
    movgr2cf        $fcc4, T1
    bstrpick.d      T1, T0, 47, 40
    movgr2cf        $fcc5, T1
    bstrpick.d      T1, T0, 55, 48
    movgr2cf        $fcc6, T1
    bstrpick.d      T1, T0, 63, 56
    movgr2cf        $fcc7, T1
2:
    ld.d    RA, SP, RA_NUM * RSIZE
    ld.d    GP, SP, GP_NUM * RSIZE
    ld.d    A0, SP, A0_NUM * RSIZE
    ld.d    A1, SP, A1_NUM * RSIZE
    ld.d    A2, SP, A2_NUM * RSIZE
    ld.d    A3, SP, A3_NUM * RSIZE
    ld.d    A4, SP, A4_NUM * RSIZE
    ld.d    A5, SP, A5_NUM * RSIZE
    ld.d    A6, SP, A6_NUM * RSIZE
    ld.d    A7, SP, A7_NUM * RSIZE
    ld.d    T0, SP, T0_NUM * RSIZE
    ld.d    T1, SP, T1_NUM * RSIZE
    ld.d    T2, SP, T2_NUM * RSIZE
    ld.d    T3, SP, T3_NUM * RSIZE
    ld.d    T4, SP, T4_NUM * RSIZE
    ld.d    T5, SP, T5_NUM * RSIZE
    ld.d    T6, SP, T6_NUM * RSIZE
    ld.d    T7, SP, T7_NUM * RSIZE
    ld.d    T8, SP, T8_NUM * RSIZE
    ld.d    TP, SP, TP_NUM * RSIZE
    ld.d    FP, SP, FP_NUM * RSIZE
    ld.d    S0, SP, S0_NUM * RSIZE
    ld.d    S1, SP, S1_NUM * RSIZE
    ld.d    S2, SP, S2_NUM * RSIZE
    ld.d    S3, SP, S3_NUM * RSIZE
    ld.d    S4, SP, S4_NUM * RSIZE
    ld.d    S5, SP, S5_NUM * RSIZE
    ld.d    S6, SP, S6_NUM * RSIZE
    ld.d    S7, SP, S7_NUM * RSIZE
    ld.d    S8, SP, S8_NUM * RSIZE

    ld.d    SP, SP, SP_NUM * RSIZE
    ertn

#
#   Exception trampoline copied down to RAM after initialization.
#

ASM_PFX(LoongArchException):
    csrwr   T0, LOONGARCH_CSR_KS0
    csrwr   SP, LOONGARCH_CSR_KS1
    pcaddi   T0, 0
    ld.d     T0, T0, 16
    jirl    ZERO, T0, 0
    nop
1:
    .quad  Exception_handler
.globl  LoongArchExceptionEnd
LoongArchExceptionEnd:

#
#   Set Exception Base Address.
#

ASM_PFX(SetEbase):
    #
    #  clear Vint cofigure
    #  all exceptions share the same interrupt entry
    #
    csrrd   T0, LOONGARCH_CSR_ECFG
    li.d    T1, ~0x70000
    and     T0, T0, T1
    csrwr   T0, LOONGARCH_CSR_ECFG

    #  set ebase
    csrwr   A0, LOONGARCH_CSR_EBASE
    jirl    ZERO, RA, 0

#
#  Read Csr EUEN register.
#  @param   A0 Pointer to the variable used to store the EUEN register value
#  @retval  none
#

ASM_PFX(LoongArchReadqCsrEuen):
    csrrd T0, LOONGARCH_CSR_EUEN
    stptr.d T0, A0, 0
    jirl    ZERO, RA,0

#
#  Write Csr EUEN register.
#  @param  A0  The value used to write to the EUEN register
#  @retval  none
#

ASM_PFX(LoongArchWriteqCsrEuen):
    csrwr A0, LOONGARCH_CSR_EUEN
    jirl    ZERO, RA,0
