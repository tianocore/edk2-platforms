#------------------------------------------------------------------------------
#
# Start for LoongArch
#
# Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#  @par Glossary:
#    - CSR - CPU Status Register
#    - EBASE - Exception Base Address
#------------------------------------------------------------------------------
#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <Library/Cpu.h>

ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_GLOBAL ASM_PFX(DeadLoop)

.text
ASM_PFX(_ModuleEntryPoint):
    /* configure reset ebase */
    la.pcrel    T0, DeadLoop
    csrwr   T0, LOONGARCH_CSR_EBASE

    /*disable interrupt*/
    li.d    T0, (1 << 2)
    csrxchg ZERO, T0, LOONGARCH_CSR_CRMD

    /* read physical cpu number id */
    csrrd   T0, LOONGARCH_CSR_CPUNUM
    andi    T0, T0, 0x3ff
    li.d    A0, BOOTCORE_ID  //0
    bne     T0, A0, slave_main

call_centry:
    /*call C function make sure parameter true*/
    li.d    A1, FixedPcdGet64(PcdSecPeiTempRamBase) + FixedPcdGet32(PcdSecPeiTempRamSize)  # stack base
    li.d    A0, FixedPcdGet64(PcdFlashPeiFvBase) # PEI Fv base
    move    SP, A1
    addi.d  SP, SP, -0x8
    bl      SecCoreStartupWithStack

slave_main:
    # clear mailbox
    li.d      T1, LOONGSON_CSR_MAIL_BUF0
    iocsrwr.d ZERO, T1

    # enable IPI interrupt
    li.d      T0, (1 << 12)
    csrxchg   T0, T0, LOONGARCH_CSR_ECFG

    addi.d    T0, ZERO, -1
    li.d      T1, LOONGSON_IOCSR_IPI_EN
    iocsrwr.w T0, T1

    li.d      T1, LOONGSON_CSR_MAIL_BUF0

1:
    # wait for wakeup
    idle 0
    nop
    iocsrrd.w T0, T1
    beqz      T0, 1b

    # read and clear ipi interrupt
    li.d      T1, LOONGSON_IOCSR_IPI_STATUS
    iocsrrd.w T0, T1
    li.d      T1, LOONGSON_IOCSR_IPI_CLEAR
    iocsrwr.w T0, T1

    # disable IPI interrupt
    li.d      T0, (1 << 12)
    csrxchg   ZERO, T0, LOONGARCH_CSR_ECFG

    # read mail buf and jump to specified entry
    li.d      T1, LOONGSON_CSR_MAIL_BUF0
    iocsrrd.d T0, T1
    or        RA, T0, ZERO
    jirl      ZERO, RA, 0x0

.align 12
ASM_PFX(DeadLoop):
    b DeadLoop
