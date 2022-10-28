/** @file
  Memory Detection for Virtual Machines.

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/QemuFwCfgLib.h>
#include "Platform.h"

/**
  Publish PEI core memory

  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64 Base;
  UINT64      Size;
  UINT64      RamTop;

  //
  // Determine the range of memory to use during PEI
  //
  Base = PcdGet64 (PcdSecPeiTempRamBase) + PcdGet32 (PcdSecPeiTempRamSize);
  RamTop = PcdGet64 (PcdUefiRamTop);
  Size = RamTop - Base;

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory (Base, Size);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "Publish Memory Initialize done.\n"));
  return Status;
}

/**
  Peform Memory Detection
  Publish system RAM and reserve memory regions
**/
VOID
InitializeRamRegions (
  VOID
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  LOONGARCH_MEMMAP_ENTRY  MemoryMapEntry;
  LOONGARCH_MEMMAP_ENTRY  *StartEntry;
  LOONGARCH_MEMMAP_ENTRY  *pEntry;
  UINTN                Processed;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/memmap error Status %d \n", __func__, __LINE__, Status));
    return ;
  }
  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    DEBUG ((DEBUG_ERROR, "no MemoryMapEntry FwCfgSize:%d\n", FwCfgSize));
    return ;
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
    if (pEntry->Type != EfiAcpiAddressRangeMemory) {
      continue;
    }

    AddMemoryRangeHob ( pEntry->BaseAddr, pEntry->BaseAddr + pEntry->Length);
  }
}
