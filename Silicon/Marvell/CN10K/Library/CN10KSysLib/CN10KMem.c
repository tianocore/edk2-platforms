/** @file

  Source file for the Marvell CN10K memory map library.

  Copyright (c) 2015-2016, Cavium Inc. All rights reserved.<BR>
  Copyright (c) 2011-2014, ARM Limited. All rights reserved.<BR>
  Copyright (c) 2026, Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/SmcLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>
#include <Library/MemoryAllocationLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  129

// DDR attributes
#define DDR_ATTRIBUTES_CACHED    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED  ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]  VirtualMemoryMap   Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_ATTRIBUTES  CacheAttributes;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINT64                        VirtualMemoryTableSize;
  UINT64                        MemoryBase;
  UINT64                        MemorySize;
  UINTN                         Index = 0;
  UINTN                         Node;
  UINTN                         NodeCount;
  EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttributes;

  ASSERT (VirtualMemoryMap != NULL);

  NodeCount = SmcNodeCount ();

  DEBUG ((DEBUG_INFO, "%a: NodeCount: %d\n", __FUNCTION__, NodeCount));

  VirtualMemoryTableSize = sizeof (ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS;
  VirtualMemoryTable     = AllocatePages (EFI_SIZE_TO_PAGES (VirtualMemoryTableSize));

  if (VirtualMemoryTable == NULL) {
    return;
  }

  CacheAttributes = DDR_ATTRIBUTES_CACHED;

  ResourceAttributes =
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED;

  VirtualMemoryTable[Index].PhysicalBase = PcdGet64 (PcdFdBaseAddress);
  VirtualMemoryTable[Index].VirtualBase  = PcdGet64 (PcdFdBaseAddress);
  VirtualMemoryTable[Index].Length       = PcdGet32 (PcdFdSize);
  VirtualMemoryTable[Index].Attributes   = CacheAttributes;
  Index++;

  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    ResourceAttributes,
    PcdGet64 (PcdFdBaseAddress),
    PcdGet32 (PcdFdSize)
    );

  for (Node = 0; Node < NodeCount; Node++) {
    MemoryBase = Node * FixedPcdGet64 (PcdNodeDramBase);
    MemorySize = SmcGetRamSize (Node);

    MemoryBase += (Node == 0) ? PcdGet64 (PcdSystemMemoryBase) : 0;
    MemorySize -= (Node == 0) ? PcdGet64 (PcdSystemMemoryBase) : 0;

    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      ResourceAttributes,
      MemoryBase,
      MemorySize
      );

    DEBUG ((DEBUG_LOAD | DEBUG_INFO, "Memory %lx @ %lx\n", MemorySize, MemoryBase));
    VirtualMemoryTable[Index].PhysicalBase = MemoryBase;
    VirtualMemoryTable[Index].VirtualBase  = MemoryBase;
    VirtualMemoryTable[Index].Length       = MemorySize;
    VirtualMemoryTable[Index].Attributes   = CacheAttributes;

    Index++;
  }

  for (Node = 0; Node < NodeCount; Node++) {
    VirtualMemoryTable[Index].PhysicalBase = FixedPcdGet64 (PcdIoBaseAddress) +
                                             Node * FixedPcdGet64 (PcdNodeIoBaseAddress);
    VirtualMemoryTable[Index].VirtualBase = FixedPcdGet64 (PcdIoBaseAddress) +
                                            Node * FixedPcdGet64 (PcdNodeIoBaseAddress);
    VirtualMemoryTable[Index].Length     = FixedPcdGet64 (PcdIoSize);
    VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    DEBUG ((
      DEBUG_LOAD | DEBUG_INFO,
      "IO %lx @ %lx\n",
      VirtualMemoryTable[Index].Length,
      VirtualMemoryTable[Index].PhysicalBase
      ));

    Index++;
  }

  // End of Table
  VirtualMemoryTable[Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase  = 0;
  VirtualMemoryTable[Index].Length       = 0;
  VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  *VirtualMemoryMap = VirtualMemoryTable;
}
