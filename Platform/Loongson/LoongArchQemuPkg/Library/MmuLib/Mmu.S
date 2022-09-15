#------------------------------------------------------------------------------
#
# LoongArch for LoongArch
#
# Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#-----------------------------------------------------------------------------

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include "Library/Cpu.h"
#include "mmu.h"

ASM_GLOBAL ASM_PFX(HandleTlbRefill)
ASM_GLOBAL HandleTlbRefillEnd
ASM_GLOBAL ASM_PFX(LoongarchInvalidTlb)
ASM_GLOBAL ASM_PFX(SetTlbRefillFuncBase)
ASM_GLOBAL ASM_PFX(WriteCsrPageSize)
ASM_GLOBAL ASM_PFX(WriteCsrTlbRefillPageSize)
ASM_GLOBAL ASM_PFX(WriteCsrStlbPageSize)
ASM_GLOBAL ASM_PFX(LoongArchWriteqCsrPwctl0)
ASM_GLOBAL ASM_PFX(LoongArchWriteqCsrPwctl1)
ASM_GLOBAL ASM_PFX(LoongArchWriteqCsrPgdl)
ASM_GLOBAL ASM_PFX(LoongArchWriteqCsrPgdh)
ASM_GLOBAL ASM_PFX(LoongArchXchgCsrCrmd)

#
#  Refill the page table.
#  @param  VOID
#  @retval  VOID
#

ASM_PFX(HandleTlbRefill):
  csrwr T0, LOONGARCH_CSR_TLBRSAVE
  csrrd T0, LOONGARCH_CSR_PGD
  lddir T0, T0, 3   #Put pud BaseAddress into T0
  lddir T0, T0, 2   #Put pmd BaseAddress into T0
  lddir T0, T0, 1   #Put pte BaseAddress into T0
  ldpte T0, 0
  ldpte T0, 1
  tlbfill
  csrrd T0, LOONGARCH_CSR_TLBRSAVE
  ertn
HandleTlbRefillEnd:

#
# Invalid corresponding TLB entries are based on the address given
# @param A0 The address corresponding to the invalid page table entry
# @retval  none
#

ASM_PFX(LoongarchInvalidTlb):
    invtlb  INVTLB_ADDR_GTRUE_OR_ASID, ZERO, A0
    jirl    ZERO, RA, 0

#
# Set Tlb Refill function to hardware
# @param A0 The address of tlb refill function
# @retval  none
#

ASM_PFX(SetTlbRefillFuncBase):
    csrwr   A0, LOONGARCH_CSR_TLBREBASE
    jirl    ZERO, RA,0

#
#  Set Cpu Status Register Page Size.
#  @param  A0  Page Size.
#  @retval  none
#

ASM_PFX(WriteCsrPageSize):
    li.d    T0, CSR_TLBIDX_SIZE
    sll.d   A0, A0, T0
    li.d    T0, CSR_TLBIDX_SIZE_MASK
    csrxchg A0, T0, LOONGARCH_CSR_TLBIDX
    jirl    ZERO, RA,0

#
#  Set Cpu Status Register TLBREFILL Page Size.
#  @param  A0  Page Size.
#  @retval  none
#

ASM_PFX(WriteCsrTlbRefillPageSize):
    li.d    T0, CSR_TLBREHI_PS_SHIFT
    sll.d   A0, A0, T0
    li.d    T0, CSR_TLBREHI_PS
    csrxchg A0, T0, LOONGARCH_CSR_TLBREHI
    jirl    ZERO, RA,0

#
#  Set Cpu Status Register STLB Page Size.
#  @param val  Page Size.
#  @retval  VOID
#

ASM_PFX(WriteCsrStlbPageSize):
    csrwr  A0, LOONGARCH_CSR_STLBPGSIZE
    jirl    ZERO, RA,0

#
#  Write Csr PWCTL0 register.
#  @param  A0  The value used to write to the PWCTL0 register
#  @retval  none
#

ASM_PFX(LoongArchWriteqCsrPwctl0):
    csrwr A0, LOONGARCH_CSR_PWCTL0
    jirl    ZERO, RA,0

#
#  Write Csr PWCTL1 register.
#  @param  A0  The value used to write to the PWCTL1 register
#  @retval  none
#

ASM_PFX(LoongArchWriteqCsrPwctl1):
    csrwr A0, LOONGARCH_CSR_PWCTL1
    jirl    ZERO, RA,0

#
#  Write Csr PGDL register.
#  @param  A0  The value used to write to the PGDL register
#  @retval  none
#

ASM_PFX(LoongArchWriteqCsrPgdl):
    csrwr A0, LOONGARCH_CSR_PGDL
    jirl    ZERO, RA,0

#
#  Write Csr PGDH register.
#  @param  A0  The value used to write to the PGDH register
#  @retval  none
#

ASM_PFX(LoongArchWriteqCsrPgdh):
    csrwr A0, LOONGARCH_CSR_PGDH
    jirl    ZERO, RA,0

#
#  Exchange specified bit data with the Csr CRMD register.
#  @param[IN]  A0   The value Exchanged with the CSR CRMD register.
#  @param[IN]  A1   Specifies the mask for swapping bits
#  @retval  VOID
#

ASM_PFX(LoongArchXchgCsrCrmd):
    csrxchg A0, A1, LOONGARCH_CSR_CRMD
    jirl    ZERO, RA,0
