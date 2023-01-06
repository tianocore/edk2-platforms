/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Pgd or Pgd or PGD    - Page Global Directory
    - Pud or Pud or PUD    - Page Upper Directory
    - Pmd or Pmd or PMD    - Page Middle Directory
    - Pte or pte or PTE    - Page Table Entry
    - Val or VAL or val    - Value
    - Dir    - Directory
**/
#ifndef PAGE_H_
#define PAGE_H_

#define MAX_VA_BITS                         47
#define PGD_WIDE                            (8)
#define PUD_WIDE                            (9)
#define PMD_WIDE                            (9)
#define PTE_WIDE                            (9)

#define ENTRYS_PER_PGD                      (1 << PGD_WIDE)
#define ENTRYS_PER_PUD                      (1 << PUD_WIDE)
#define ENTRYS_PER_PMD                      (1 << PMD_WIDE)
#define ENTRYS_PER_PTE                      (1 << PTE_WIDE)

#define PGD_SHIFT                           (PUD_SHIFT + PUD_WIDE)
#define PUD_SHIFT                           (PMD_SHIFT + PMD_WIDE)
#define PMD_SHIFT                           (EFI_PAGE_SHIFT + PTE_WIDE)
#define PTE_SHIFT                           (EFI_PAGE_SHIFT)

#define PGD_SIZE                            (1UL << PGD_SHIFT)
#define PUD_SIZE                            (1UL << PUD_SHIFT)
#define PMD_SIZE                            (1UL << PMD_SHIFT)

#define PGD_MASK                            (~(PGD_SIZE-1))
#define PUD_MASK                            (~(PUD_SIZE-1))
#define PMD_MASK                            (~(PMD_SIZE-1))
#define PAGE_MASK                           (~(EFI_PAGE_SIZE - 1))
#define PFN_MASK                            (~(((UINTN)(1) << (EFI_PAGE_SHIFT)) - 1) & \
                                             (((UINTN)(1) << (PAGE_PFN_END_SHIFT)) - 1))

#define HUGEP_PAGE_MASK                     (~(((UINTN)(1) << (PMD_SHIFT)) - 1) & \
                                             (((UINTN)(1) << (PAGE_PFN_END_SHIFT)) - 1))

typedef struct { UINTN PgdVal; } PGD;
typedef struct { UINTN PudVal; } PUD;
typedef struct { UINTN PmdVal; } PMD;
typedef struct { UINTN PteVal; } PTE;
/**
  Gets the value of the page global directory table entry.

  @param  x    Page global directory struct variables.

  @retval   the value of the page global directory table entry.
 **/
#define PGD_VAL(x)                          ((x).PgdVal)
/**
  Gets the value of the page upper directory table entry.

  @param  x    Page upper directory struct variables.

  @retval  the value of the page upper directory table entry.
 **/
#define PUD_VAL(x)                          ((x).PudVal)
/**
  Gets the value of the page middle directory table entry.

  @param  x    Page middle directory struct variables.

  @retval  the value of the page middle directory table entry.
 **/
#define PMD_VAL(x)                          ((x).PmdVal)
/**
  Gets the value of the page table entry.

  @param  x    Page table entry struct variables.

  @retval  the value of the page table entry.
 **/
#define PTE_VAL(x)                          ((x).PteVal)

#define PGD_TABLE_SIZE                      (ENTRYS_PER_PGD * sizeof(PGD))
#define PUD_TABLE_SIZE                      (ENTRYS_PER_PUD * sizeof(PUD))
#define PMD_TABLE_SIZE                      (ENTRYS_PER_PMD * sizeof(PMD))
#define PTE_TABLE_SIZE                      (ENTRYS_PER_PTE * sizeof(PTE))
/**
  Gets the physical address of the record in the page table entry.

  @param  x    Page table entry struct variables.

  @retval  the value of the physical address.
 **/
#define GET_PAGE_ATTRIBUTES(x)              (UINTN) {(PTE_VAL(x) & ~PFN_MASK)}
/**
  Gets the virtual address of the next block of the specified virtual address
  that is aligned with the size of the global page directory mapping.

  @param  Address  Specifies the virtual address.
  @param  End    The end address of the memory region.

  @retval   the specified virtual address  of the next block.
 **/
#define PGD_ADDRESS_END(Address, End)                  \
({                                                     \
  UINTN Boundary = ((Address) + PGD_SIZE) & PGD_MASK;  \
  (Boundary - 1 < (End) - 1)? Boundary: (End);         \
})
/**
  Gets the virtual address of the next block of the specified virtual address
  that is aligned with the size of the page upper directory mapping.

  @param  Address  Specifies the virtual address.
  @param  End    The end address of the memory region.

  @retval   the specified virtual address  of the next block.
 **/
#define PUD_ADDRESS_END(Address, End)                  \
({                                                     \
  UINTN Boundary = ((Address) + PUD_SIZE) & PUD_MASK;  \
  (Boundary - 1 < (End) - 1)? Boundary: (End);         \
})
/**
  Gets the virtual address of the next block of the specified virtual address
  that is aligned with the size of the page middle directory mapping.

  @param  Address  Specifies the virtual address.
  @param  End    The end address of the memory region.

  @retval   the specified virtual address  of the next block.
 **/
#define PMD_ADDRESS_END(Address, End)                  \
({                                                     \
  UINTN Boundary = ((Address) + PMD_SIZE) & PMD_MASK;  \
  (Boundary - 1 < (End) - 1)? Boundary: (End);         \
})
/**
  Get Specifies the virtual address corresponding to the index of the page global directory table entry.

  @param  Address  Specifies the virtual address.

  @retval   the index of the page global directory table entry.
 **/
#define PGD_INDEX(Address)                  (((Address) >> PGD_SHIFT) & (ENTRYS_PER_PGD-1))
/**
  Get Specifies the virtual address corresponding to the index of the page upper directory table entry.

  @param  Address  Specifies the virtual address.
  @param  End    The end address of the memory region.

  @retval   the index of the page upper directory table entry.
 **/
#define PUD_INDEX(Address)                  (((Address) >> PUD_SHIFT) & (ENTRYS_PER_PUD - 1))
/**
  Get Specifies the virtual address corresponding to the index of the page middle directory table entry.

  @param  Address  Specifies the virtual address.

  @retval   the index of the page middle directory table entry.
 **/
#define PMD_INDEX(Address)                  (((Address) >> PMD_SHIFT) & (ENTRYS_PER_PMD - 1))
/**
  Get Specifies the virtual address corresponding to the index of the page table entry.

  @param  Address  Specifies the virtual address.

  @retval   the index of the page table entry.
 **/
#define PTE_INDEX(Address)                  (((Address) >> EFI_PAGE_SHIFT) & (ENTRYS_PER_PTE - 1))

/**
  Calculates the value of the page table entry based on the specified virtual address and properties.

  @param  Address  Specifies the virtual address.
  @param  Attributes  Specifies the Attributes.

  @retval    the value of the page table entry.
 **/
#define MAKE_PTE(Address, Attributes)       (PTE){((((Address) >> EFI_PAGE_SHIFT) << 12) | (Attributes))}
/**
  Get Global bit from Attributes

  @param  Attributes  Specifies the Attributes.
 * */
#define GET_GLOBALBIT(Attributes)           ((Attributes & PAGE_GLOBAL) >> PAGE_GLOBAL_SHIFT)
/**
  Calculates the value of the Huge page table entry based on the specified virtual address and properties.

  @param  Address  Specifies the virtual address.
  @param  Attributes  Specifies the Attributes.

  @retval    the value of the HUGE page table entry.
 **/
#define MAKE_HUGE_PTE(Address, Attributes)  (((((Address) >> PMD_SHIFT) << PMD_SHIFT) | \
                                             ((Attributes) | (GET_GLOBALBIT(Attributes) << PAGE_HGLOBAL_SHIFT) | \
                                             PAGE_HUGE)))

 /**
  Check whether the large page table entry is.

  @param  Val The value of the page table entry.

  @retval    1   Is huge page table entry.
  @retval    0   Isn't huge page table entry.
 **/
#define IS_HUGE_PAGE(Val)                   ((((Val) & PAGE_HUGE) == PAGE_HUGE) && \
                                             (((Val) & PAGE_HGLOBAL) == PAGE_HGLOBAL))

#define HUGE_PAGE_SIZE                      (PMD_SIZE)

 /**
  Check that the global page directory table entry is empty.

  @param  pgd   the global page directory struct variables.

  @retval    1   Is huge page table entry.
  @retval    0   Isn't huge page table entry.
 **/
STATIC
inline
UINTN
pgd_none (
  IN PGD pgd
  )
{
  return (PGD_VAL(pgd) == (UINTN)PcdGet64(PcdInvalidPud));
}

 /**
  Check that the page upper directory table entry is empty.

  @param  pud   Page upper directory struct variables.

  @retval    1   Is huge page table entry.
  @retval    0   Isn't huge page table entry.
 **/
STATIC
inline
UINTN
pud_none (
  IN PUD pud
  )
{
  return (PUD_VAL(pud) == (UINTN)PcdGet64 (PcdInvalidPmd));
}

 /**
  Check that the page middle directory table entry is empty.

  @param  pmd   Page middle directory struct variables.

  @retval    1   Is huge page table entry.
  @retval    0   Isn't huge page table entry.
 **/
STATIC
inline
UINTN
pmd_none (
  IN PMD pmd
  )
{
  return (PMD_VAL(pmd) == (UINTN)PcdGet64(PcdInvalidPte));
}
 /**
  Check that the page  table entry is empty.

  @param  pmd   Page table entry struct variables.

  @retval    1   Is huge page table entry.
  @retval    0   Isn't huge page table entry.
 **/
STATIC
inline
UINTN
pte_none (
  IN PTE pte
  )
{
  return (!(PTE_VAL(pte) & (~PAGE_VALID)));
}
#endif // PAGE_H_
