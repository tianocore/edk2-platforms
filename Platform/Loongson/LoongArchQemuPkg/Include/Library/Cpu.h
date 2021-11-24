/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - EXC     - Exception
    - INT     - Interrupt
    - FPU     - Floating Point Unit
    - CSR     - CPU Status Register
    - READQ   - Read Quad Word
**/
#ifndef LOONGARCH_CPU_H_
#define LOONGARCH_CPU_H_

/* Exception types decoded by machdep exception decoder */
#define EXC_INT                     0       /* HW interrupt */
#define EXC_TLBL                    1       /* TLB miss on a load */
#define EXC_TLBS                    2       /* TLB miss on a store */
#define EXC_TLBI                    3       /* TLB miss on a ifetch */
#define EXC_TLBM                    4       /* TLB modified fault */
#define EXC_TLBRI                   5       /* TLB Read-Inhibit exception */
#define EXC_TLBXI                   6       /* TLB Execution-Inhibit exception */
#define EXC_TLBPE                   7       /* TLB Privilege Error */
#define EXC_ADE                     8       /* Address Error */
#define EXC_ALE                     9       /* Unalign Access */
#define EXC_OOB                     10      /* Out of bounds */
#define EXC_SYS                     11      /* System call */
#define EXC_BP                      12      /* Breakpoint */
#define EXC_INE                     13      /* Inst. Not Exist */
#define EXC_IPE                     14      /* Inst. Privileged Error */
#define EXC_FPDIS                   15      /* FPU Disabled */
#define EXC_LSXDIS                  16      /* LSX Disabled */
#define EXC_LASXDIS                 17      /* LASX Disabled */
#define EXC_FPE                     18      /* Floating Point Exception */
#define EXC_WATCH                   19      /* Watch address reference */
#define EXC_BAD                     255     /* Undecodeable */

#define COPY_SIGCODE    // copy sigcode above user stack in exec
#define ZERO                        $r0 /* wired zero */
#define RA                          $r1 /* return address */
#define GP                          $r2 /* global pointer - caller saved for PIC */
#define SP                          $r3 /* stack pointer */
#define V0                          $r4 /* return value - caller saved */
#define V1                          $r5
#define A0                          $r4 /* argument registers */
#define A1                          $r5
#define A2                          $r6
#define A3                          $r7
#define A4                          $r8 /* arg reg 64 bit; caller saved in 32 bit */
#define A5                          $r9
#define A6                          $r10
#define A7                          $r11
#define T0                          $r12 /* caller saved */
#define T1                          $r13
#define T2                          $r14
#define T3                          $r15
#define T4                          $r16 /* callee saved */
#define T5                          $r17
#define T6                          $r18
#define T7                          $r19
#define T8                          $r20 /* caller saved */
#define TP                          $r21 /* TLS */
#define FP                          $r22 /* frame pointer */
#define S0                          $r23 /* callee saved */
#define S1                          $r24
#define S2                          $r25
#define S3                          $r26
#define S4                          $r27
#define S5                          $r28
#define S6                          $r29
#define S7                          $r30
#define S8                          $r31 /* callee saved */

#define FCSR0                       $r0

//
// Location of the saved registers relative to ZERO.
// Usage is p->p_regs[XX].
//
#define RA_NUM                      1
#define GP_NUM                      2
#define SP_NUM                      3
#define A0_NUM                      4
#define A1_NUM                      5
#define A2_NUM                      6
#define A3_NUM                      7
#define A4_NUM                      8
#define A5_NUM                      9
#define A6_NUM                      10
#define A7_NUM                      11
#define T0_NUM                      12
#define T1_NUM                      13
#define T2_NUM                      14
#define T3_NUM                      15
#define T4_NUM                      16
#define T5_NUM                      17
#define T6_NUM                      18
#define T7_NUM                      19
#define T8_NUM                      20
#define TP_NUM                      21
#define FP_NUM                      22
#define S0_NUM                      23
#define S1_NUM                      24
#define S2_NUM                      25
#define S3_NUM                      26
#define S4_NUM                      27
#define S5_NUM                      28
#define S6_NUM                      29
#define S7_NUM                      30
#define S8_NUM                      31

#define FP0_NUM                     0
#define FP1_NUM                     1
#define FP2_NUM                     2
#define FP3_NUM                     3
#define FP4_NUM                     4
#define FP5_NUM                     5
#define FP6_NUM                     6
#define FP7_NUM                     7
#define FP8_NUM                     8
#define FP9_NUM                     9
#define FP10_NUM                    10
#define FP11_NUM                    11
#define FP12_NUM                    12
#define FP13_NUM                    13
#define FP14_NUM                    14
#define FP15_NUM                    15
#define FP16_NUM                    16
#define FP17_NUM                    17
#define FP18_NUM                    18
#define FP19_NUM                    19
#define FP20_NUM                    20
#define FP21_NUM                    21
#define FP22_NUM                    22
#define FP23_NUM                    23
#define FP24_NUM                    24
#define FP25_NUM                    25
#define FP26_NUM                    26
#define FP27_NUM                    27
#define FP28_NUM                    28
#define FP29_NUM                    29
#define FP30_NUM                    30
#define FP31_NUM                    31
#define FCSR_NUM                    32
#define FCC_NUM                     33

#ifdef __ASSEMBLY__
#define _ULCAST_
#define _U64CAST_
#else
#define _ULCAST_ (unsigned long)
#define _U64CAST_ (u64)
#endif

#define LOONGARCH_CSR_CRMD          0
#define LOONGARCH_CSR_PRMD          1
#define LOONGARCH_CSR_EUEN          2
#define CSR_EUEN_LBTEN_SHIFT        3
#define CSR_EUEN_LBTEN              (_ULCAST_(0x1) << CSR_EUEN_LBTEN_SHIFT)
#define CSR_EUEN_LASXEN_SHIFT       2
#define CSR_EUEN_LASXEN             (_ULCAST_(0x1) << CSR_EUEN_LASXEN_SHIFT)
#define CSR_EUEN_LSXEN_SHIFT        1
#define CSR_EUEN_LSXEN              (_ULCAST_(0x1) << CSR_EUEN_LSXEN_SHIFT)
#define CSR_EUEN_FPEN_SHIFT         0
#define CSR_EUEN_FPEN               (_ULCAST_(0x1) << CSR_EUEN_FPEN_SHIFT)
#define LOONGARCH_CSR_ECFG          4

/* Exception status */
#define LOONGARCH_CSR_ESTAT         5
#define CSR_ESTAT_ESUBCODE_SHIFT    22
#define CSR_ESTAT_ESUBCODE_WIDTH    9
#define CSR_ESTAT_ESUBCODE          (_ULCAST_(0x1ff) << CSR_ESTAT_ESUBCODE_SHIFT)
#define CSR_ESTAT_EXC_SHIFT         16
#define CSR_ESTAT_EXC_WIDTH         6
#define CSR_ESTAT_EXC               (_ULCAST_(0x3f) << CSR_ESTAT_EXC_SHIFT)
#define CSR_ESTAT_IS_SHIFT          0
#define CSR_ESTAT_IS_WIDTH          15
#define CSR_ESTAT_IS                (_ULCAST_(0x7fff) << CSR_ESTAT_IS_SHIFT)

#define LOONGARCH_CSR_EPC           6
#define LOONGARCH_CSR_BADV          7
#define LOONGARCH_CSR_BADINST       8
#define LOONGARCH_CSR_BADI          8
#define LOONGARCH_CSR_EBASE         0xc     /* Exception entry base address */
#define LOONGARCH_CSR_CPUNUM        0x20    /* CPU core number */

/* register number save in stack on exception */
#define FP_BASE_NUM                 34
#define BASE_NUM                    32
#define CSR_NUM                     10
#define FP_BASE_INDEX               (CSR_NUM + BASE_NUM)
#define BOOTCORE_ID                 0

#define LOONGSON_IOCSR_IPI_STATUS   0x1000
#define LOONGSON_IOCSR_IPI_EN       0x1004
#define LOONGSON_IOCSR_IPI_SET      0x1008
#define LOONGSON_IOCSR_IPI_CLEAR    0x100c
#define LOONGSON_CSR_MAIL_BUF0      0x1020
#define LOONGSON_CSR_MAIL_BUF1      0x1028
#define LOONGSON_CSR_MAIL_BUF2      0x1030
#define LOONGSON_CSR_MAIL_BUF3      0x1038

/* Bit Domains for CFG registers */
#define LOONGARCH_CPUCFG4           0x4
#define LOONGARCH_CPUCFG5           0x5

/* Kscratch registers */
#define LOONGARCH_CSR_KS0           0x30
#define LOONGARCH_CSR_KS1           0x31

/* Stable timer registers */
#define LOONGARCH_CSR_TMCFG         0x41
#define LOONGARCH_CSR_TMCFG_EN      (1ULL << 0)
#define LOONGARCH_CSR_TMCFG_PERIOD  (1ULL << 1)
#define LOONGARCH_CSR_TMCFG_TIMEVAL (0x3fffffffffffULL << 2)
#define LOONGARCH_CSR_TVAL          0x42    /* Timer value */
#define LOONGARCH_CSR_CNTC          0x43    /* Timer offset */
#define LOONGARCH_CSR_TINTCLR       0x44    /* Timer interrupt clear */

/* TLB refill exception base address */
#define LOONGARCH_CSR_TLBREBASE     0x88
#define LOONGARCH_CSR_TLBRSAVE      0x8b    /* KScratch for TLB refill exception */
#define LOONGARCH_CSR_PGD           0x1b    /* Page table base */

/* Invalid addr with global=1 or matched asid in current tlb */
#define INVTLB_ADDR_GTRUE_OR_ASID   0x6

/* Bits 8 and 9 of FPU Status Register specify the rounding mode */
#define FPU_CSR_RM                  0x300
#define FPU_CSR_RN                  0x000   /* nearest */
#define FPU_CSR_RZ                  0x100   /* towards zero */
#define FPU_CSR_RU                  0x200   /* towards +Infinity */
#define FPU_CSR_RD                  0x300   /* towards -Infinity */

#endif
