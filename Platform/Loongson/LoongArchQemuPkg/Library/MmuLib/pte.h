/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Tlb or TLB     - Translation Lookaside Buffer
    - HGLOBAL     - Huge Global
    - PFN       - Page Frame number
    - EXEC       - Execute
    - PLV       - Privilege Level
    - RPLV       - Restricted Privilege Level
    - SUC       - Strong-ordered UnCached
    - CC       - Coherent Cached
    - WUC       - Weak-ordered UnCached
**/
#ifndef PTE_H_
#define PTE_H_
/*Page table property definitions */
#define  PAGE_VALID_SHIFT    0
#define  PAGE_DIRTY_SHIFT    1
#define  PAGE_PLV_SHIFT      2  /* 2~3, two bits */
#define  CACHE_SHIFT         4  /* 4~5, two bits */
#define  PAGE_GLOBAL_SHIFT   6
#define  PAGE_HUGE_SHIFT     6  /* HUGE is a PMD bit */

#define  PAGE_HGLOBAL_SHIFT  12 /* HGlobal is a PMD bit */
#define  PAGE_PFN_SHIFT      12
#define  PAGE_PFN_END_SHIFT  48
#define  PAGE_NO_READ_SHIFT  61
#define  PAGE_NO_EXEC_SHIFT  62
#define  PAGE_RPLV_SHIFT     63

/* Used by TLB hardware (placed in EntryLo*) */
#define PAGE_VALID           ((UINTN)(1) << PAGE_VALID_SHIFT)
#define PAGE_DIRTY           ((UINTN)(1) << PAGE_DIRTY_SHIFT)
#define PAGE_PLV             ((UINTN)(3) << PAGE_PLV_SHIFT)
#define PAGE_GLOBAL          ((UINTN)(1) << PAGE_GLOBAL_SHIFT)
#define PAGE_HUGE            ((UINTN)(1) << PAGE_HUGE_SHIFT)
#define PAGE_HGLOBAL         ((UINTN)(1) << PAGE_HGLOBAL_SHIFT)
#define PAGE_NO_READ         ((UINTN)(1) << PAGE_NO_READ_SHIFT)
#define PAGE_NO_EXEC         ((UINTN)(1) << PAGE_NO_EXEC_SHIFT)
#define PAGE_RPLV            ((UINTN)(1) << PAGE_RPLV_SHIFT)
#define CACHE_MASK           ((UINTN)(3) << CACHE_SHIFT)
#define PFN_SHIFT            (EFI_PAGE_SHIFT - 12 + PAGE_PFN_SHIFT)

#define PLV_KERNEL           0
#define PLV_USER             3

#define PAGE_USER            (PLV_USER << PAGE_PLV_SHIFT)
#define PAGE_KERNEL          (PLV_KERN << PAGE_PLV_SHIFT)

#define CACHE_SUC            (0 << CACHE_SHIFT) /* Strong-ordered UnCached */
#define CACHE_CC             (1 << CACHE_SHIFT) /* Coherent Cached */
#define CACHE_WUC            (2 << CACHE_SHIFT) /* Weak-ordered UnCached */
#endif // PTE_H_
