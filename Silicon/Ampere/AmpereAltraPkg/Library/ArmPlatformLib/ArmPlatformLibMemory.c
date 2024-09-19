/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "PlatformMemoryMap.h"

/* Number of Virtual Memory Map Descriptors */
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  50

/* DDR attributes */
#define DDR_ATTRIBUTES_CACHED    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED  ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  UINTN                         Index = 0;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINT32                        NumRegion;
  UINTN                         Count;
  VOID                          *Hob;
  PLATFORM_INFO_HOB             *PlatformHob;

  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR *)AllocatePages (EFI_SIZE_TO_PAGES (sizeof (ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
  if (VirtualMemoryTable == NULL) {
    return;
  }

  /* For Address space 0x1000_0000_0000 to 0x1001_00FF_FFFF
   *  - Device memory
   */
  VirtualMemoryTable[Index].PhysicalBase = AC01_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase  = AC01_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length       = AC01_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /* For Address space 0x5000_0000_0000 to 0x5001_00FF_FFFF
   *  - Device memory
   */
  if (IsSlaveSocketActive ()) {
    VirtualMemoryTable[++Index].PhysicalBase = AC01_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  }

  /*
   *  - PCIe RCA0 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA0_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCA0_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCA0_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCA1 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA1_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCA1_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCA1_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCA2 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA2_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCA2_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCA2_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCA3 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA3_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCA3_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCA3_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCB0 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB0_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCB0_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCB0_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCB1 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB1_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCB1_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCB1_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCB2 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB2_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCB2_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCB2_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - PCIe RCB3 Device memory
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB3_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_RCB3_DEVICE_MEMORY_S0_BASE;
  VirtualMemoryTable[Index].Length         = AC01_RCB3_DEVICE_MEMORY_S0_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  if (IsSlaveSocketActive ()) {
    // Slave socket exist

    /*
     *  - PCIe RCA0 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA0_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCA0_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCA0_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCA1 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA1_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCA1_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCA1_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCA2 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA2_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCA2_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCA2_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCA3 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCA3_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCA3_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCA3_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCB0 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB0_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCB0_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCB0_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCB1 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB1_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCB1_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCB1_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCB2 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB2_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCB2_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCB2_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    /*
     *  - PCIe RCB3 Device memory
     */
    VirtualMemoryTable[++Index].PhysicalBase = AC01_RCB3_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].VirtualBase    = AC01_RCB3_DEVICE_MEMORY_S1_BASE;
    VirtualMemoryTable[Index].Length         = AC01_RCB3_DEVICE_MEMORY_S1_SIZE;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  }

  /*
   *  - BERT memory region
   */
  VirtualMemoryTable[++Index].PhysicalBase = AC01_BERT_MEMORY_BASE;
  VirtualMemoryTable[Index].VirtualBase    = AC01_BERT_MEMORY_BASE;
  VirtualMemoryTable[Index].Length         = AC01_BERT_MEMORY_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /*
   *  - DDR memory region
   */
  NumRegion = PlatformHob->DramInfo.NumRegion;
  Count     = 0;
  while (NumRegion-- > 0) {
    if (PlatformHob->DramInfo.NvdRegion[Count]) {
      /* Skip NVDIMM Region */
      Count++;
      continue;
    }

    VirtualMemoryTable[++Index].PhysicalBase = PlatformHob->DramInfo.Base[Count];
    VirtualMemoryTable[Index].VirtualBase    = PlatformHob->DramInfo.Base[Count];
    VirtualMemoryTable[Index].Length         = PlatformHob->DramInfo.Size[Count];
    VirtualMemoryTable[Index].Attributes     = DDR_ATTRIBUTES_CACHED;
    if (PlatformHob->DramInfo.Base[Count] == PcdGet64 (PcdMmBufferBase)) {
      //
      // Set uncached attribute for MM region
      //
      VirtualMemoryTable[Index].Attributes = DDR_ATTRIBUTES_UNCACHED;
    }

    Count++;
  }

  /* End of Table */
  VirtualMemoryTable[++Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase    = 0;
  VirtualMemoryTable[Index].Length         = 0;
  VirtualMemoryTable[Index].Attributes     = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT ((Index + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}
