/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Dir    - Directory
**/
#ifndef  MMU_LIB_CORE_H_
#define  MMU_LIB_CORE_H_
/**
  Iterates through the page directory to initialize it.

  @param  Dst  A pointer to the directory of the page to initialize.
  @param  Num  The number of page directories to initialize.
  @param  Src  A pointer to the data used to initialize the page directory.

  @retval VOID.
**/
VOID
PageDirInit (
  IN VOID *dest,
  IN UINTN Count,
  IN VOID *src
  );

/**
  Page tables are established from memory-mapped tables.

  @param  MemoryRegion   A pointer to a memory-mapped table entry.

  @retval     EFI_SUCCESS   The page table was created successfully.
  @retval     EFI_OUT_OF_RESOURCES  Page table  establishment failed due to resource exhaustion.
**/
EFI_STATUS
FillTranslationTable (
  IN  MEMORY_REGION_DESCRIPTOR  *MemoryRegion
  );
#endif // MMU_LIB_CORE_H_
