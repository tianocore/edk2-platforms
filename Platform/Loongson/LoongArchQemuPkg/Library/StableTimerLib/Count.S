#------------------------------------------------------------------------------
#
# Count for LoongArch
#
# Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include "Library/Cpu.h"

ASM_GLOBAL ASM_PFX(CpuSetIP)
ASM_GLOBAL ASM_PFX(LoongArchReadTime)
ASM_GLOBAL ASM_PFX(LoongArchReadCpuCfg)

#
# Set cpu interrupts
# @param A0 The interrupt number
#

ASM_PFX(CpuSetIP):
    csrrd   T0, LOONGARCH_CSR_ECFG
    or      T0, T0, A0
    csrwr   T0, LOONGARCH_CSR_ECFG
    jirl    ZERO, RA, 0

#
#Gets the timer count value.
#@param[] VOID
#@retval  timer count value.
#

ASM_PFX(LoongArchReadTime):
    rdtime.d   A0, ZERO
    jirl    ZERO, RA, 0

#
#   Read Csr CPUCFG register.
#   @param   A0 Pointer to the variable used to store the CPUCFG register value.
#   @param   A1 Specifies the register number of the CPUCFG to read the data.
#   @retval  none
#

ASM_PFX(LoongArchReadCpuCfg):
    cpucfg T0, A1
    stptr.d T0, A0, 0
    jirl    ZERO, RA, 0
