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

ASM_GLOBAL ASM_PFX(LoongarchWriteqKs0)
ASM_GLOBAL ASM_PFX(LoongarchReadqKs0)

#
# Write Csr KS0 register.
# @param A0 The value used to write to the KS0 register
# @retval  none
#

ASM_PFX(LoongarchWriteqKs0):
    csrwr   A0, LOONGARCH_CSR_KS0
    jirl    ZERO, RA,0

#
# Write Csr KS0 register.
# @param A0 Pointer to the variable used to store the KS0 register value
# @retval  none
#

ASM_PFX(LoongarchReadqKs0):
    csrrd T0, LOONGARCH_CSR_KS0
    stptr.d T0, A0, 0
    jirl    ZERO, RA,0
    jirl    ZERO, RA,0
