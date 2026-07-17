/** @file
  Qualcomm platform memory configuration

  Copyright (c) 2011, ARM Limited. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - MMU  - Memory Management Unit
    - SMEM - Shared Memory
    - IMEM - Internal Memory
    - DTB  - Device Tree Blob
    - PASR - Partial Array Self Refresh
**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/QualcommSmemLib.h>
#include <Library/RamPartitionTableLib.h>

#include <PiPei.h>
#include <Pi/PiHob.h>
#include <Pi/PiBootMode.h>

#include "MemRegionInfo.h"
#include "PlatformConfiguration.h"

#define MAX_MEMORY_ENTRIES  (128)

/**
  Configure early MMU mappings for UART, DTB, system memory, SMEM, and IMEM.

  @retval  EFI_SUCCESS  MMU configured successfully.
  @retval  Other        ArmConfigureMmu() failed.

**/
STATIC EFI_STATUS
EarlyCacheInit (
  VOID
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  EarlyInitMemoryTable[] = {
    {
      .PhysicalBase = FixedPcdGet64 (PcdSerialRegisterBase),
      .VirtualBase  = FixedPcdGet64 (PcdSerialRegisterBase),
      .Length       = EFI_PAGE_SIZE,
      .Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
    },
    {
      .PhysicalBase = FixedPcdGet64 (PcdBootDtBase),
      .VirtualBase  = FixedPcdGet64 (PcdBootDtBase),
      .Length       = FixedPcdGet64 (PcdBootDtSize),
      .Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
    },
    {
      .PhysicalBase = FixedPcdGet64 (PcdSystemMemoryBase),
      .VirtualBase  = FixedPcdGet64 (PcdSystemMemoryBase),
      .Length       = FixedPcdGet64 (PcdSystemMemorySize),
      .Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
    },
    {
      .PhysicalBase = FixedPcdGet64 (PcdSmemBaseAddress),
      .VirtualBase  = FixedPcdGet64 (PcdSmemBaseAddress),
      .Length       = FixedPcdGet64 (PcdSmemSize),
      .Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED
    },
    {
      .PhysicalBase = FixedPcdGet64 (PcdIMemCookiesBase),
      .VirtualBase  = FixedPcdGet64 (PcdIMemCookiesBase),
      .Length       = FixedPcdGet64 (PcdIMemCookiesSize),
      .Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
    },
    { 0 } // End of table
  };

  return ArmConfigureMmu (EarlyInitMemoryTable, NULL, NULL);
}

/**
  Build resource descriptor and memory allocation HOBs for a memory region.

  @param[in]  MemRegion  Memory region descriptor to process.

  @retval  EFI_SUCCESS  HOBs built successfully.

**/
STATIC EFI_STATUS
BuildMemoryHob (
  IN MEM_REGION_INFO  *MemRegion
  )
{
  /* Make sure the region's end address doesn't exceed the MAX_ADDRESS) */
  ASSERT (MemRegion->MemBase < MAX_ADDRESS);
  ASSERT ((MemRegion->MemBase + MemRegion->MemSize - 1) <= MAX_ADDRESS);

  /* Build ResourceHob */
  if (MemRegion->BuildHobOption != AllocOnly) {
    BuildResourceDescriptorHob (
      MemRegion->ResourceType,
      MemRegion->ResourceAttribute,
      MemRegion->MemBase,
      MemRegion->MemSize
      );
  }

  if ((MemRegion->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) ||
      (MemRegion->MemoryType == EfiRuntimeServicesData))
  {
    /* Build MemoryAllocationHob */
    BuildMemoryAllocationHob (
      MemRegion->MemBase,
      MemRegion->MemSize,
      MemRegion->MemoryType
      );
  }

  return EFI_SUCCESS;
}

/**
  Build HOBs for all entries in the platform memory region table.

**/
STATIC VOID
AddMemRegionHobs (
  VOID
  )
{
  UINTN            Index;
  MEM_REGION_INFO  *MemRegions   = NULL;
  UINTN            MemRegionsCnt = 0;

  GetMemRegionCfgInfo (&MemRegions, &MemRegionsCnt);
  if ((MemRegions == NULL) || (MemRegionsCnt == 0)) {
    DEBUG ((DEBUG_ERROR, "UEFI Memory Map configuration not found\r\n"));
    ASSERT (MemRegions != NULL);
    ASSERT (MemRegionsCnt != 0);
    CpuDeadLoop ();
    return;
  }

  for (Index = 0; Index < MemRegionsCnt; Index++) {
    switch (MemRegions[Index].BuildHobOption) {
      case AllocOnly:
        BuildMemoryHob (&MemRegions[Index]);
        break;

      case AddMem:
        BuildMemoryHob (&MemRegions[Index]);
        break;

      case AddPeripheral:
        BuildMemoryHob (&MemRegions[Index]);
        break;

      case HobOnlyNoCacheSetting:
        BuildMemoryHob (&MemRegions[Index]);
        break;

      case NoBuildHob:
        break;

      case NoMap:
        break;

      case AddDynamicMem:
        break;

      case ErrorBuildHob:
      default:
        DEBUG ((DEBUG_ERROR, "Invalid BuildHob Option\n"));
        ASSERT (FALSE);
        CpuDeadLoop ();
        break;
    }
  }
}

/**
  Convert the platform memory region table to an ARM MMU descriptor array.

  @param[in]   MemRegions       Array of memory region descriptors.
  @param[in]   RegionsCnt       Number of entries in MemRegions.
  @param[out]  VirtualMemoryMap Receives the ARM MMU descriptor array.

  @retval  EFI_SUCCESS           Array built successfully.
  @retval  EFI_INVALID_PARAMETER Invalid input or table overflow.

**/
EFI_STATUS
EFIAPI
GeneratePageTableRegionMap (
  IN MEM_REGION_INFO               *MemRegions,
  IN UINTN                         RegionsCnt,
  IN ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  STATIC ARM_MEMORY_REGION_DESCRIPTOR  MemoryTable[MAX_MEMORY_ENTRIES];
  ARM_MEMORY_REGION_ATTRIBUTES         CacheAttributes;
  UINTN                                MemoryRegionCount;
  UINTN                                Index;

  Index = 0;

  SetMem (MemoryTable, sizeof (MemoryTable), 0);

  // Sanity check
  if ((RegionsCnt >= MAX_MEMORY_ENTRIES) || (MemRegions == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CacheAttributes = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  for (MemoryRegionCount = 0; MemoryRegionCount < RegionsCnt; MemoryRegionCount++) {
    // Skip entries which explicitly ask to be added as HOB only
    // Also skip entries that are marked as NoMap so a hole is created.
    BUILD_HOB_OPTION_TYPE  HobValue = MemRegions[MemoryRegionCount].BuildHobOption;
    if ((HobValue == HobOnlyNoCacheSetting) || (HobValue == NoMap) || (HobValue == AddDynamicMem)) {
      continue;
    }

    // Fill the new entry
    MemoryTable[Index].PhysicalBase = MemRegions[MemoryRegionCount].MemBase;
    MemoryTable[Index].VirtualBase  = MemRegions[MemoryRegionCount].MemBase;
    MemoryTable[Index].Length       = MemRegions[MemoryRegionCount].MemSize;

    if (MemRegions[MemoryRegionCount].CacheAttributes == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) {
      MemoryTable[Index].Attributes = CacheAttributes;
    } else {
      MemoryTable[Index].Attributes = MemRegions[MemoryRegionCount].CacheAttributes;
    }

    Index++;

    if (Index >= MAX_MEMORY_ENTRIES) {
      return EFI_INVALID_PARAMETER;
    }
  }

  // End of Table
  MemoryTable[Index].PhysicalBase = 0;
  MemoryTable[Index].VirtualBase  = 0;
  MemoryTable[Index].Length       = 0;
  MemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  WriteBackInvalidateDataCacheRange ((VOID *)FixedPcdGet64 (PcdFdBaseAddress), FixedPcdGet64 (PcdSystemMemoryUefiRegionSize));
  WriteBackInvalidateDataCacheRange ((VOID *)FixedPcdGet64 (PcdBootDtBase), FixedPcdGet64 (PcdBootDtSize));

  ArmDisableCachesAndMmu ();
  ArmInvalidateTlb ();

  *VirtualMemoryMap = MemoryTable;

  return EFI_SUCCESS;
}

/**
  Reserve the Trace32 DDR debug buffer as EFI_RESOURCE_MEMORY_RESERVED.

**/
STATIC VOID
ArmPlatformSetupDebugBuffer (
  VOID
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,
    FixedPcdGet64 (PcdTrace32DdrBase),
    FixedPcdGet64 (PcdTrace32DdrSize)
    );

  BuildMemoryAllocationHob (
    FixedPcdGet64 (PcdTrace32DdrBase),
    FixedPcdGet64 (PcdTrace32DdrSize),
    EfiReservedMemoryType
    );
  return;
}

/**
  Load platform memory configuration from the boot device tree.

  @retval  EFI_UNSUPPORTED  Not yet implemented.

**/
STATIC
EFI_STATUS
LoadPlatformConfigFromDeviceTree (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Print all RAM partition information using RamPartitionTableLib API.

**/
STATIC
VOID
DisplayRamPartitionInformation (
  VOID
  )
{
  EFI_STATUS       Status;
  UINTN            MemoryCapacity;
  UINTN            StartAddress;
  UINT8            HighBankBit;
  UINT32           MinPasrSize;
  MEM_REGION_INFO  PartitionTable[RAM_NUM_PART_ENTRIES];
  MEM_REGION_INFO  PreloadTable[RAM_NUM_PART_ENTRIES];
  MEM_REGION_INFO  FdEntry;
  UINTN            EntryCount;
  UINTN            Index;

  DEBUG ((DEBUG_INFO, "===================\n"));
  DEBUG ((DEBUG_INFO, "RAM Partition Table\n"));
  DEBUG ((DEBUG_INFO, "===================\n"));

  //
  // Total physical memory (available partitions)
  //
  MemoryCapacity = 0;
  Status         = RamPartitionGetTotalPhysicalMemory (&MemoryCapacity);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "  Total Physical Memory    : %lu MB\n", (UINT64)(MemoryCapacity >> 20)));
  } else {
    DEBUG ((DEBUG_WARN, "  Total Physical Memory    : unavailable (%r)\n", Status));
  }

  //
  // Total installed physical memory (raw partition table, all entries)
  //
  MemoryCapacity = 0;
  Status         = RamPartitionGetInstalledPhysicalMemory (&MemoryCapacity);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "  Installed Physical Memory: %lu MB\n", (UINT64)(MemoryCapacity >> 20)));
  } else {
    DEBUG ((DEBUG_WARN, "  Installed Physical Memory: unavailable (%r)\n", Status));
  }

  //
  // Total installed SDRAM memory (SysMemory + SDRAM category only)
  //
  MemoryCapacity = 0;
  Status         = RamPartitionGetInstalledSdramMemory (&MemoryCapacity);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "  Installed SDRAM Memory   : %lu MB\n", (UINT64)(MemoryCapacity >> 20)));
  } else {
    DEBUG ((DEBUG_WARN, "  Installed SDRAM Memory   : unavailable (%r)\n", Status));
  }

  //
  // Lowest physical start address across all partitions
  //
  StartAddress = 0;
  Status       = RamPartitionGetLowestPhysicalStartAddress (&StartAddress);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "  Lowest Physical Start    : 0x%016lx\n", (UINT64)StartAddress));
  } else {
    DEBUG ((DEBUG_WARN, "  Lowest Physical Start    : unavailable (%r)\n", Status));
  }

  //
  // Highest bank bit
  //
  HighBankBit = 0;
  Status      = RamPartitionGetHighestBankBit (&HighBankBit);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "  Highest Bank Bit         : %u\n", (UINT32)HighBankBit));
  } else {
    DEBUG ((DEBUG_WARN, "  Highest Bank Bit         : unavailable (%r)\n", Status));
  }

  //
  // Minimum PASR size
  //
  MinPasrSize = 0;
  Status      = RamPartitionGetMinPasrSize (&MinPasrSize);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "  Min PASR Size            : %u MB\n", MinPasrSize));
  } else {
    DEBUG ((DEBUG_WARN, "  Min PASR Size            : unavailable (%r)\n", Status));
  }

  //
  // Usable RAM partition entries (SDRAM SysMemory, available to UEFI)
  //
  SetMem (PartitionTable, sizeof (PartitionTable), 0);
  EntryCount = RAM_NUM_PART_ENTRIES;
  Status     = RamPartitionGetRamPartitions (&EntryCount, PartitionTable);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "\n  RAM Partitions (%u entries):\n", (UINT32)EntryCount));
    for (Index = 0; Index < EntryCount; Index++) {
      DEBUG ((
        DEBUG_INFO,
        "    [%u] %-20a  Base: 0x%016lx  Size: 0x%016lx (%lu MB)\n",
        (UINT32)Index,
        PartitionTable[Index].Name,
        PartitionTable[Index].MemBase,
        PartitionTable[Index].MemSize,
        PartitionTable[Index].MemSize >> 20
        ));
    }
  } else {
    DEBUG ((DEBUG_WARN, "  RAM Partitions           : unavailable (%r)\n", Status));
  }

  //
  // Preloaded RAM partition entries
  //
  SetMem (PreloadTable, sizeof (PreloadTable), 0);
  EntryCount = RAM_NUM_PART_ENTRIES;
  Status     = RamPartitionGetPreLoadedRamPartitions (&EntryCount, PreloadTable);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "\n  Preloaded RAM Partitions (%u entries):\n", (UINT32)EntryCount));
    if (EntryCount == 0) {
      DEBUG ((DEBUG_INFO, "    (none)\n"));
    }

    for (Index = 0; Index < EntryCount; Index++) {
      DEBUG ((
        DEBUG_INFO,
        "    [%u] %-20a  Base: 0x%016lx  Size: 0x%016lx (%lu MB)\n",
        (UINT32)Index,
        PreloadTable[Index].Name,
        PreloadTable[Index].MemBase,
        PreloadTable[Index].MemSize,
        PreloadTable[Index].MemSize >> 20
        ));
    }
  } else {
    DEBUG ((DEBUG_WARN, "  Preloaded RAM Partitions : unavailable (%r)\n", Status));
  }

  //
  // Partition entry containing the UEFI FD base address
  //
  SetMem (&FdEntry, sizeof (FdEntry), 0);
  Status = RamPartitionGetPartitionEntryByAddr (FixedPcdGet64 (PcdFdBaseAddress), &FdEntry);
  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "\n  Partition containing FD base (0x%016lx):\n"
      "    Name : %a\n"
      "    Base : 0x%016lx\n"
      "    Size : 0x%016lx (%lu MB)\n",
      FixedPcdGet64 (PcdFdBaseAddress),
      FdEntry.Name,
      FdEntry.MemBase,
      FdEntry.MemSize,
      FdEntry.MemSize >> 20
      ));
  } else {
    DEBUG ((
      DEBUG_WARN,
      "\n  Partition containing FD base (0x%016lx): not found (%r)\n",
      FixedPcdGet64 (PcdFdBaseAddress),
      Status
      ));
  }

  DEBUG ((DEBUG_INFO, "========================================\n\n"));
}

/**
  Return the virtual memory map for this platform.

  @param[out]  VirtualMemoryMap  Receives a pointer to the ARM MMU descriptor array.

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  EFI_STATUS       Status         = EFI_UNSUPPORTED;
  MEM_REGION_INFO  *mMemRegions   = NULL;
  UINTN            mNumMemRegions = 0;
  BOOLEAN          UsedStaticMap  = FALSE;

  DEBUG ((DEBUG_INFO, "ArmPlatformGetVirtualMemoryMap\n"));

  /* Reserved DDR Region for T32 CMM Script */
  ArmPlatformSetupDebugBuffer ();

  QualcommSmemInit ();
  DEBUG ((DEBUG_INFO, "SmemInit\n"));

  Status = EarlyCacheInit ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "EarlyCacheInit failed\n"));
    goto ExitError;
  }

  Status = RamPartitionInitRamPartitionTableLib();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "InitRamPartitionTableLib failed\n"));
    goto ExitError;
  }

  DisplayRamPartitionInformation();

  /* Try DT-based platform configuration first */
  Status = LoadPlatformConfigFromDeviceTree ();
  if (Status == EFI_UNSUPPORTED) {
    DEBUG ((DEBUG_WARN, "DT platform configuration unsupported, falling back to static memory map\n"));
    Status = LoadStaticPlatformCfg ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "LoadStaticPlatformCfg failed\n"));
      goto ExitError;
    }

    UsedStaticMap = TRUE;
  } else if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "LoadPlatformConfigFromDeviceTree failed\n"));
    goto ExitError;
  }

  /* Validate memory regions against RAM partition table, including
   * the static map fallback. */
  Status = ValidateParsedMemoryRegions ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ValidateParsedMemoryRegions failed\n"));
    goto ExitError;
  }

  if (UsedStaticMap == TRUE) {
    Status = AddUpperMemoryFromRamPartitions ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "AddUpperMemoryFromRamPartitions failed\n"));
      goto ExitError;
    }
  } else {
    Status = UpdateSystemMemoryRegions ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UpdateSystemMemoryRegions failed\n"));
      goto ExitError;
    }
  }

  AddMemRegionHobs ();

  Status = GetMemRegionCfgInfo (&mMemRegions, &mNumMemRegions);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "GetMemRegionCfgInfo failed\n"));
    goto ExitError;
  }

  if ((mMemRegions == NULL) || (mNumMemRegions == 0)) {
    ASSERT (mMemRegions != NULL);
    ASSERT (mNumMemRegions > 0);
    DEBUG ((DEBUG_ERROR, "GetMemRegionCfgInfo Invalid\n"));
    goto ExitError;
  }

  Status = GeneratePageTableRegionMap (mMemRegions, mNumMemRegions, VirtualMemoryMap);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "GeneratePageTableRegionMap failed\n"));
    goto ExitError;
  }

  DEBUG ((DEBUG_ERROR, "ArmPlatformGetVirtualMemoryMap Exit\n"));
  return;

ExitError:
  DEBUG ((DEBUG_ERROR, "ArmPlatformGetVirtualMemoryMap Error\n"));
  ASSERT (Status == EFI_SUCCESS);
  CpuDeadLoop ();
  return;
}
