/** @file
  Platform PEI driver

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FwCfg    - Firmeware Config
    - Tlb      - Translation Lookaside Buffer
**/
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "Library/Cpu.h"
#include "pte.h"
#include "page.h"
#include "mmu.h"
#include <Library/QemuFwCfgLib.h>
#include "MmuLibCore.h"
#include <Library/CacheMaintenanceLib.h>
#include <Library/MmuLib.h>

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on your platform.

  @param[out]   VirtualMemoryMap    Array of MEMORY_REGION_DESCRIPTOR
                                    describing a Physical-to-Virtual Memory
                                    mapping. This array must be ended by a
                                    zero-filled entry. The allocated memory
                                    will not be freed.
**/
VOID
GetMemoryMapFromFwCfg (
  OUT MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{

  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  LOONGARCH_MEMMAP_ENTRY  MemoryMapEntry;
  LOONGARCH_MEMMAP_ENTRY  *StartEntry;
  LOONGARCH_MEMMAP_ENTRY  *pEntry;
  UINTN                Processed;
  MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINTN  Index = 0;
  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePool (
                         sizeof (MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );
  VirtualMemoryTable[Index].PhysicalBase = 0x10000000;
  VirtualMemoryTable[Index].VirtualBase  = VirtualMemoryTable[Index].PhysicalBase;
  VirtualMemoryTable[Index].Length       = 0x10000000;
  VirtualMemoryTable[Index].Attributes   = PAGE_VALID | PLV_KERNEL |  CACHE_SUC | PAGE_DIRTY | PAGE_GLOBAL;
  ++Index;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/memmap error Status %d \n", __func__, __LINE__, Status));
    ZeroMem (&VirtualMemoryTable[Index], sizeof (MEMORY_REGION_DESCRIPTOR));
    *VirtualMemoryMap = VirtualMemoryTable;
    return ;
  }
  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    DEBUG ((DEBUG_ERROR, "no MemoryMapEntry FwCfgSize:%d\n", FwCfgSize));
  }

  QemuFwCfgSelectItem (FwCfgItem);
  StartEntry = AllocatePages  (EFI_SIZE_TO_PAGES (FwCfgSize));
  QemuFwCfgReadBytes (FwCfgSize, StartEntry);
  for (Processed = 0; Processed < (FwCfgSize / sizeof MemoryMapEntry); Processed++) {
    pEntry = StartEntry + Processed;
    if (pEntry->Length == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "MemmapEntry Base %p length %p  type %d\n", pEntry->BaseAddr, pEntry->Length, pEntry->Type));
    VirtualMemoryTable[Index].PhysicalBase = pEntry->BaseAddr;
    VirtualMemoryTable[Index].VirtualBase  = VirtualMemoryTable[Index].PhysicalBase;
    VirtualMemoryTable[Index].Length       = pEntry->Length;
    VirtualMemoryTable[Index].Attributes   = PAGE_VALID | PLV_KERNEL |  CACHE_CC | PAGE_DIRTY | PAGE_GLOBAL;
    ++Index;
  }

  FreePages (StartEntry, EFI_SIZE_TO_PAGES (FwCfgSize));
  // End of Table
  ZeroMem (&VirtualMemoryTable[Index], sizeof (MEMORY_REGION_DESCRIPTOR));
  *VirtualMemoryMap = VirtualMemoryTable;
  return ;
}

/**
  Create a page table and initialize the MMU.

  @param[] VOID

  @retval  VOID
**/
EFIAPI
VOID
ConfigureMmu (VOID)
{
  PGD *SwapperPageDir = NULL;
  PGD *InvalidPgd = NULL;
  PUD *InvalidPudTable = NULL;
  PMD *InvalidPmdTable = NULL;
  PTE *InvalidPteTable = NULL;
  MEMORY_REGION_DESCRIPTOR  *MemoryTable = NULL;
  RETURN_STATUS PcdStatus;
  UINTN PgdShift = PGD_SHIFT;
  UINTN PgdWide = PGD_WIDE;
  UINTN PudShift = PUD_SHIFT;
  UINTN PudWide = PUD_WIDE;
  UINTN PmdShift = PMD_SHIFT;
  UINTN PmdWide = PMD_WIDE;
  UINTN PteShift = PTE_SHIFT;
  UINTN PteWide = PTE_WIDE;
  UINTN PageEnable = 1 << 4;
  VOID *TlbReEntry;

  SwapperPageDir = AllocatePages (EFI_SIZE_TO_PAGES (PGD_TABLE_SIZE));
  InvalidPgd = AllocatePages (EFI_SIZE_TO_PAGES (PGD_TABLE_SIZE));
  InvalidPudTable = AllocatePages (EFI_SIZE_TO_PAGES (PUD_TABLE_SIZE));
  InvalidPmdTable = AllocatePages (EFI_SIZE_TO_PAGES (PMD_TABLE_SIZE));
  InvalidPteTable = AllocatePages (EFI_SIZE_TO_PAGES (PTE_TABLE_SIZE));
  ZeroMem (InvalidPteTable, PTE_TABLE_SIZE);

  if ((!InvalidPgd) ||
      (!InvalidPudTable) ||
      (!InvalidPmdTable) ||
      (!InvalidPteTable))
  {
    goto FreeTranslationTable;
  }

  /*pgd init*/
  PageDirInit (SwapperPageDir , ENTRYS_PER_PGD, InvalidPudTable);
  /*pgd init*/
  PageDirInit (InvalidPgd, ENTRYS_PER_PGD, InvalidPudTable);
  /*pud init*/
  PageDirInit (InvalidPudTable, ENTRYS_PER_PUD, InvalidPmdTable);
  /*pmd init*/
  PageDirInit (InvalidPmdTable, ENTRYS_PER_PMD, InvalidPteTable);
  GetMemoryMapFromFwCfg (&MemoryTable);

  PcdStatus |= PcdSet64S (PcdSwapPageDir, (UINTN)SwapperPageDir);
  PcdStatus |= PcdSet64S (PcdInvalidPgd, (UINTN)InvalidPgd);
  PcdStatus |= PcdSet64S (PcdInvalidPud, (UINTN)InvalidPudTable);
  PcdStatus |= PcdSet64S (PcdInvalidPmd, (UINTN)InvalidPmdTable);
  PcdStatus |= PcdSet64S (PcdInvalidPte, (UINTN)InvalidPteTable);
  ASSERT_RETURN_ERROR (PcdStatus);

  while (MemoryTable->Length != 0) {
    DEBUG ((DEBUG_VERBOSE, "%a %d VirtualBase %p VirtualEnd %p Attributes %p .\n", __func__, __LINE__,
      MemoryTable->VirtualBase,
      (MemoryTable->Length + MemoryTable->VirtualBase),
      MemoryTable->Attributes));

    PcdStatus = FillTranslationTable (MemoryTable);
    if (EFI_ERROR (PcdStatus)) {
      goto FreeTranslationTable;
    }
    MemoryTable++;
  }

  if (PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT0) {
    LoongArchSetMemoryAttributes (0, EFI_PAGE_SIZE, EFI_MEMORY_RP | EFI_MEMORY_XP | EFI_MEMORY_WP);
  }

  TlbReEntry = AllocatePages (1);
  if (TlbReEntry == NULL) {
    goto FreeTranslationTable;
  }
  CopyMem ((char *)TlbReEntry, HandleTlbRefill, (HandleTlbRefillEnd - HandleTlbRefill));
  InvalidateInstructionCacheRange ((VOID *)(UINTN)HandleTlbRefill, (UINTN)(HandleTlbRefillEnd - HandleTlbRefill));

  DEBUG ((DEBUG_VERBOSE,
    "%a  %d PteShift %d PteWide %d PmdShift %d PmdWide %d PudShift %d PudWide %d PgdShift %d PgdWide %d.\n",
    __func__, __LINE__,
    PteShift, PteWide, PmdShift, PmdWide,PudShift, PudWide, PgdShift, PgdWide));

  SetTlbRefillFuncBase ((UINTN)TlbReEntry);
  /*set page size*/
  WriteCsrPageSize (DEFAULT_PAGE_SIZE);
  WriteCsrStlbPageSize (DEFAULT_PAGE_SIZE);
  WriteCsrTlbRefillPageSize (DEFAULT_PAGE_SIZE);

  LoongArchWriteqCsrPwctl0 ((PteShift | PteWide << 5 | PmdShift << 10 | PmdWide << 15 | PudShift << 20 | PudWide << 25
              ));
  LoongArchWriteqCsrPwctl1 (PgdShift | PgdWide << 6);
  LoongArchWriteqCsrPgdl ((UINTN)SwapperPageDir);
  LoongArchWriteqCsrPgdh ((UINTN)InvalidPgd);

  DEBUG ((DEBUG_INFO, "%a %d Enable Mmu Start PageBassAddress %p.\n", __func__, __LINE__, SwapperPageDir));
  LoongArchXchgCsrCrmd ( PageEnable, 1 << 4);
  DEBUG ((DEBUG_INFO, "%a %d Enable Mmu End.\n", __func__, __LINE__));

  return ;

FreeTranslationTable:
  if (SwapperPageDir) {
    FreePages (SwapperPageDir, EFI_SIZE_TO_PAGES (PGD_TABLE_SIZE));
  }

  if (InvalidPgd) {
    FreePages (InvalidPgd, EFI_SIZE_TO_PAGES (PGD_TABLE_SIZE));
  }

  if (InvalidPudTable) {
    FreePages (InvalidPudTable, EFI_SIZE_TO_PAGES (PUD_TABLE_SIZE));
  }

  if (InvalidPmdTable) {
    FreePages (InvalidPmdTable, EFI_SIZE_TO_PAGES (PMD_TABLE_SIZE));
  }

  if (InvalidPteTable) {
    FreePages (InvalidPteTable, EFI_SIZE_TO_PAGES (PTE_TABLE_SIZE));
  }

  PcdSet64S (PcdSwapPageDir, (UINTN)0);
  PcdSet64S (PcdInvalidPgd, (UINTN)0);
  PcdSet64S (PcdInvalidPud, (UINTN)0);
  PcdSet64S (PcdInvalidPmd, (UINTN)0);
  PcdSet64S (PcdInvalidPte, (UINTN)0);

  return ;
}
