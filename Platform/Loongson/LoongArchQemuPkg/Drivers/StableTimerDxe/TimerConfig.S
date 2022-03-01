#------------------------------------------------------------------------------
#
# Timer Cfg for LoongArch
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

ASM_GLOBAL ASM_PFX(LoongarchWriteqTmcfg)
ASM_GLOBAL ASM_PFX(LoongarchWriteqTintclr)

#
# Write Csr TMCFG register.
# @param A0 The value used to write to the TMCFG register
# @retval  none
#

ASM_PFX(LoongarchWriteqTmcfg):
    csrwr   A0, LOONGARCH_CSR_TMCFG
    jirl    ZERO, RA,0

#
# Write Csr TINTCLR register.
# @param A0 The value used to write to the TINTCLR register
# @retval  none
#

ASM_PFX(LoongarchWriteqTintclr):
    csrwr   A0, LOONGARCH_CSR_TINTCLR
    jirl    ZERO, RA,0
