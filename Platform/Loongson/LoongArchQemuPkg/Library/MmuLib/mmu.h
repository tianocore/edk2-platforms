/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Tlb or TLB     - Translation Lookaside Buffer
    - CSR            - Cpu State Register
    - PGDL           - Page Global Directory Low
    - PGDH           - Page Global Directory High
    - TLBIDX         - TLB Index
    - TLBREHI        - TLB Refill Entry High
    - PWCTL          - Page Walk Control
    - STLB           - Singular Page Size TLB
    - PS             - Page Size
**/
#ifndef MMU_H_
#define MMU_H_
/*page size 4k*/
#define DEFAULT_PAGE_SIZE         0x0c
#define LOONGARCH_CSR_PGDL        0x19 /* Page table base address when VA[47] = 0 */
#define LOONGARCH_CSR_PGDH        0x1a /* Page table base address when VA[47] = 1 */
#define LOONGARCH_CSR_TLBIDX      0x10 /* TLB Index, EHINV, PageSize, NP */
#define LOONGARCH_CSR_TLBEHI      0x11 /* TLB EntryHi */
#define LOONGARCH_CSR_TLBELO0     0x12 /* TLB EntryLo0 */
#define LOONGARCH_CSR_TLBELO1     0x13 /* TLB EntryLo1 */
#define LOONGARCH_CSR_TLBREHI     0x8e /* TLB refill entryhi */
#define LOONGARCH_CSR_PWCTL0      0x1c /* PWCtl0 */
#define LOONGARCH_CSR_PWCTL1      0x1d /* PWCtl1 */
#define LOONGARCH_CSR_STLBPGSIZE  0x1e
#define CSR_TLBIDX_SIZE_MASK      0x3f000000
#define CSR_TLBIDX_PS_SHIFT       24
#define CSR_TLBIDX_SIZE           CSR_TLBIDX_PS_SHIFT

#define  CSR_TLBREHI_PS_SHIFT     0
#define  CSR_TLBREHI_PS           0x3f

#define EFI_MEMORY_CACHETYPE_MASK     (EFI_MEMORY_UC  | \
                                       EFI_MEMORY_WC  | \
                                       EFI_MEMORY_WT  | \
                                       EFI_MEMORY_WB  | \
                                       EFI_MEMORY_UCE   \
                                       )

typedef struct {
  EFI_PHYSICAL_ADDRESS PhysicalBase;
  EFI_VIRTUAL_ADDRESS  VirtualBase;
  UINTN                Length;
  UINTN                Attributes;
} MEMORY_REGION_DESCRIPTOR;

// The total number of descriptors, including the final "end-of-table" descriptor.
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS (128)

extern CHAR8 HandleTlbRefill[], HandleTlbRefillEnd[];

/*
 Invalid corresponding TLB entries are based on the address given

 @param Address The address corresponding to the invalid page table entry

 @retval  none
*/
extern
VOID
LoongarchInvalidTlb (
  UINTN Address
  );

/*
 Set Tlb Refill function to hardware

 @param A0 The address of tlb refill function

 @retval  none
*/
extern
VOID
SetTlbRefillFuncBase (
  UINTN Address
  );

/*
  Set Cpu Status Register Page Size.

  @param  PageSize  Page Size.

  @retval  none
*/
extern
VOID
WriteCsrPageSize (
  UINTN PageSize
  );

/*
  Set Cpu Status Register TLBREFILL Page Size.

  @param  PageSize  Page Size.

  @retval  none
*/
extern
VOID
WriteCsrTlbRefillPageSize (
  UINTN PageSize
  );

/*
  Set Cpu Status Register STLB Page Size.

  @param PageSize  Page Size.

  @retval  VOID
*/
extern
VOID
WriteCsrStlbPageSize (
  UINTN PageSize
);

/*
  Write Csr PWCTL0 register.

  @param  Val  The value used to write to the PWCTL0 register

  @retval  none
*/
extern
VOID
LoongArchWriteqCsrPwctl0 (
  UINTN Val
  );

/*
  Write Csr PWCTL1 register.

  @param  Val  The value used to write to the PWCTL1 register

  @retval  none
*/
extern
VOID
LoongArchWriteqCsrPwctl1 (
  UINTN Val
  );

/*
  Write Csr PGDL register.

  @param  Val  The value used to write to the PGDL register

  @retval  none
*/
extern
VOID
LoongArchWriteqCsrPgdl (
  UINTN Val
  );

/*
  Write Csr PGDH register.

  @param  Val  The value used to write to the PGDH register

  @retval  none
*/
extern
VOID
LoongArchWriteqCsrPgdh (
  UINTN Val
  );

/*
  Exchange specified bit data with the Csr CRMD register.

  @param[IN]  Val   The value Exchanged with the CSR CRMD register.
  @param[IN]  Mask   Specifies the mask for swapping bits

  @retval  VOID
*/
extern
VOID
LoongArchXchgCsrCrmd (
  UINTN Val,
  UINTN Mask
  );

#endif // MMU_H_
