/** @file

  Copyright (c) 2019, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HardwareInfoLib.h>

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  4

RETURN_STATUS
EFIAPI
SbsaQemuLibConstructor (
  VOID
  )
{
  UINT64         NewBase, CurBase;
  UINT64         NewSize, CurSize;
  UINT32         NumMemNodes;
  UINT32         Index;
  MemoryInfo     MemInfo;
  RETURN_STATUS  PcdStatus;

  NewBase = 0;
  NewSize = 0;

  NumMemNodes = GetMemNodeCount ();
  for (Index = 0; Index < NumMemNodes; Index++) {
    GetMemInfo (Index, &MemInfo);
    CurBase = MemInfo.AddressBase;
    CurSize = MemInfo.AddressSize;

    if ((NewBase > CurBase) || (NewBase == 0)) {
      NewBase = CurBase;
      NewSize = CurSize;
    }
  }

  // Make sure the start of DRAM matches our expectation
  ASSERT (FixedPcdGet64 (PcdSystemMemoryBase) == NewBase);
  PcdStatus = PcdSet64S (PcdSystemMemorySize, NewSize);
  ASSERT_RETURN_ERROR (PcdStatus);

  return RETURN_SUCCESS;
}

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR
                                    describing a Physical-to-Virtual Memory
                                    mapping. This array must be ended by a
                                    zero-filled entry. The allocated memory
                                    will not be freed.

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePool (
                         sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );

  if (VirtualMemoryTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Error: Failed AllocatePool()\n", __func__));
    return;
  }

  // System DRAM
  VirtualMemoryTable[0].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[0].VirtualBase  = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[0].Length       = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[0].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  DEBUG ((
    DEBUG_INFO,
    "%a: Dumping System DRAM Memory Map:\n"
    "\tPhysicalBase: 0x%lX\n"
    "\tVirtualBase: 0x%lX\n"
    "\tLength: 0x%lX\n",
    __func__,
    VirtualMemoryTable[0].PhysicalBase,
    VirtualMemoryTable[0].VirtualBase,
    VirtualMemoryTable[0].Length
    ));

  // Peripheral space before DRAM
  VirtualMemoryTable[1].PhysicalBase = 0x0;
  VirtualMemoryTable[1].VirtualBase  = 0x0;
  VirtualMemoryTable[1].Length       = VirtualMemoryTable[0].PhysicalBase;
  VirtualMemoryTable[1].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // Remap the FD region as normal executable memory
  VirtualMemoryTable[2].PhysicalBase = PcdGet64 (PcdFdBaseAddress);
  VirtualMemoryTable[2].VirtualBase  = VirtualMemoryTable[2].PhysicalBase;
  VirtualMemoryTable[2].Length       = FixedPcdGet32 (PcdFdSize);
  VirtualMemoryTable[2].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  // End of Table
  ZeroMem (&VirtualMemoryTable[3], sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  *VirtualMemoryMap = VirtualMemoryTable;
}
