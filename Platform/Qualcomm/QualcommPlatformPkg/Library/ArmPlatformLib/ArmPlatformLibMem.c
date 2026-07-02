/** @file
  Qualcomm platform memory configuration

  Copyright (c) 2011, ARM Limited. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
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
#include <Library/RamPartitionTableLib.h>
#include <Library/SmemLib.h>

#include <PiPei.h>
#include <Pi/PiHob.h>
#include <Pi/PiBootMode.h>

#include <MemRegionInfo.h>
#include <PlatformConfiguration.h>

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
  Build HOB for the memory region

  @param  MemRegion       pointer to the memory region

  @retval EFI_SUCCESS     Successfully retrieves memory base and size
  @retval EFI_INVALID_PARAMETER  The RAM partition table is invalid
**/
STATIC EFI_STATUS
BuildMemoryHob (
  IN MEM_REGION_INFO  *pMemRegion
  )
{
  /* Make sure the region's end address doesn't exceed the MAX_ADDRESS) */
  ASSERT (pMemRegion->MemBase < MAX_ADDRESS);
  ASSERT ((pMemRegion->MemBase + pMemRegion->MemSize - 1) <= MAX_ADDRESS);

  /* Build ResourceHob */
  if (pMemRegion->BuildHobOption != AllocOnly) {
    BuildResourceDescriptorHob (
      pMemRegion->ResourceType,
      pMemRegion->ResourceAttribute,
      pMemRegion->MemBase,
      pMemRegion->MemSize
      );
  }

  if ((pMemRegion->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) ||
      (pMemRegion->MemoryType == EfiRuntimeServicesData))
  {
    /* Build MemoryAllocationHob */
    BuildMemoryAllocationHob (
      pMemRegion->MemBase,
      pMemRegion->MemSize,
      pMemRegion->MemoryType
      );
  }

  return EFI_SUCCESS;
}

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
    return; /* For KW */
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
        /* Don't do anything, only cache is initialized */
        break;

      case NoMap:
        /* Don't do anything */
        break;

      case AddDynamicMem:
        /* Don't do anything */
        break;

      case ErrorBuildHob:
      default:
        DEBUG ((DEBUG_ERROR, "Invalid BuildHob Option\n"));
        ASSERT (FALSE);
        CpuDeadLoop ();
        break;
    } /* end of switch */
  } /* end of for */
}

#define MAX_MEMORY_ENTRIES  (128)

EFI_STATUS
EFIAPI
GeneratePageTableRegionMap (
  IN MEM_REGION_INFO               *pMemRegions,
  IN UINTN                         RegionsCnt,
  IN ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  STATIC ARM_MEMORY_REGION_DESCRIPTOR  MemoryTable[MAX_MEMORY_ENTRIES];
  ARM_MEMORY_REGION_ATTRIBUTES         CacheAttributes;
  UINTN                                MemRgnCnt;
  UINTN                                MemoryTableIndex = 0;

  SetMem (MemoryTable, sizeof (MemoryTable), 0);

  // Sanity check
  if ((RegionsCnt >= MAX_MEMORY_ENTRIES) || (pMemRegions == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CacheAttributes = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  for (MemRgnCnt = 0; MemRgnCnt < RegionsCnt; MemRgnCnt++) {
    // Skip entries which explicitly ask to be added as HOB only
    // Also skip entries that are marked as NoMap so a hole is created.
    BUILD_HOB_OPTION_TYPE  HobValue = pMemRegions[MemRgnCnt].BuildHobOption;
    if ((HobValue == HobOnlyNoCacheSetting) || (HobValue == NoMap) || (HobValue == AddDynamicMem)) {
      continue;
    }

    // Fill the new entry
    MemoryTable[MemoryTableIndex].PhysicalBase = pMemRegions[MemRgnCnt].MemBase;
    MemoryTable[MemoryTableIndex].VirtualBase  = pMemRegions[MemRgnCnt].MemBase;
    MemoryTable[MemoryTableIndex].Length       = pMemRegions[MemRgnCnt].MemSize;

    if (pMemRegions[MemRgnCnt].CacheAttributes == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) {
      MemoryTable[MemoryTableIndex].Attributes = CacheAttributes;
    } else {
      MemoryTable[MemoryTableIndex].Attributes = pMemRegions[MemRgnCnt].CacheAttributes;
    }

    MemoryTableIndex++;

    if (MemoryTableIndex >= MAX_MEMORY_ENTRIES) {
      return EFI_INVALID_PARAMETER;
    }
  }

  // End of Table
  MemoryTable[MemoryTableIndex].PhysicalBase = 0;
  MemoryTable[MemoryTableIndex].VirtualBase  = 0;
  MemoryTable[MemoryTableIndex].Length       = 0;
  MemoryTable[MemoryTableIndex].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  WriteBackInvalidateDataCacheRange ((VOID *)FixedPcdGet64 (PcdFdBaseAddress), FixedPcdGet64 (PcdSystemMemoryUefiRegionSize));
  WriteBackInvalidateDataCacheRange ((VOID *)FixedPcdGet64 (PcdBootDtBase), FixedPcdGet64 (PcdBootDtSize));

  ArmDisableCachesAndMmu ();
  ArmInvalidateTlb ();

  *VirtualMemoryMap = MemoryTable;

  return EFI_SUCCESS;
}

STATIC VOID
ArmPLatformSetupDebugBuffer (
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

STATIC
EFI_STATUS
LoadPlatformConfigFromDeviceTree (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

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
  ArmPLatformSetupDebugBuffer ();

  SmemInit ();
  DEBUG ((DEBUG_INFO, "SmemInit\n"));

  Status = EarlyCacheInit ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "EarlyCacheInit failed\n"));
    goto ExitError;
  }

  Status = InitRamPartitionTableLib ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "InitRamPartitionTableLib failed\n"));
    goto ExitError;
  }

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
