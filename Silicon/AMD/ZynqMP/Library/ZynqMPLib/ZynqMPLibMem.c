/**
 * @file
 *
 * Memory initialization library used during SEC phase.
 *
 * Copyright (c) 2025, Linaro Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Derived from edk2/ArmPlatformPkg/Library/ArmPlatformLibNull/ArmPlatformLibNullMem.c
 *
 */

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (10)
#define ZYNQMP_PERIPHERALS_SIZE             (0x00010000UL)

VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  UINTN                         Index;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttributes;

  Index = 0;
  DEBUG ((DEBUG_INIT, "Building HobList for ZynqMP\n"));

  /* Build Hob list */
  ResourceAttributes = (
                        EFI_RESOURCE_ATTRIBUTE_PRESENT |
                        EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                        EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
                        EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
                        EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                        EFI_RESOURCE_ATTRIBUTE_TESTED
                        );

  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    ResourceAttributes,
    PcdGet64 (PcdSystemMemoryBase),
    PcdGet64 (PcdSystemMemorySize)
    );

  if (PcdGetBool (PcdUseExtraMemory)) {
    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      ResourceAttributes,
      PcdGet64 (PcdExtraMemoryBase),
      PcdGet64 (PcdExtraMemorySize)
      );
  }

  /* Add Reserved memory to Hob list (TF-A & OP-TEE if used) */
  if (PcdGetBool (PcdTfaInDram)) {
    DEBUG ((DEBUG_INIT, "Reserving Trusted Firmware-A region in DRAM\n"));
    BuildMemoryAllocationHob (
      PcdGet64 (PcdTfaMemoryBase),
      PcdGet64 (PcdTfaMemorySize),
      EfiReservedMemoryType
      );
  }

  if (PcdGetBool (PcdEnableOptee)) {
    DEBUG ((DEBUG_INIT, "Reserving OP-TEE region in DRAM\n"));
    BuildMemoryAllocationHob (
      PcdGet64 (PcdOpteeMemoryBase),
      PcdGet64 (PcdOpteeMemorySize),
      EfiReservedMemoryType
      );
  }

  /* Construct virtual memory table */
  DEBUG ((DEBUG_INIT, "Building Virtual Memory Table Definitions for ZynqMP\n"));
  VirtualMemoryTable = AllocatePool (
                         sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );
  if (VirtualMemoryTable == NULL) {
    return;
  }

  /* DDR Primary */
  VirtualMemoryTable[Index].PhysicalBase = FixedPcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].VirtualBase  = FixedPcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].Length       = FixedPcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  /* DDR Extra Memory */
  if (PcdGetBool (PcdUseExtraMemory)) {
    VirtualMemoryTable[++Index].PhysicalBase = FixedPcdGet64 (PcdExtraMemoryBase);
    VirtualMemoryTable[Index].VirtualBase    = FixedPcdGet64 (PcdExtraMemoryBase);
    VirtualMemoryTable[Index].Length         = FixedPcdGet64 (PcdExtraMemorySize);
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  }

  /* UART Peripherals */
  VirtualMemoryTable[++Index].PhysicalBase = FixedPcdGet64 (PcdSerialRegisterBase);
  VirtualMemoryTable[Index].VirtualBase    = FixedPcdGet64 (PcdSerialRegisterBase);
  VirtualMemoryTable[Index].Length         = ZYNQMP_PERIPHERALS_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /* GIC Interrupt */
  VirtualMemoryTable[++Index].PhysicalBase =  FixedPcdGet64 (PcdGicInterruptInterfaceBase);
  VirtualMemoryTable[Index].VirtualBase    =  FixedPcdGet64 (PcdGicInterruptInterfaceBase);
  VirtualMemoryTable[Index].Length         =  ZYNQMP_PERIPHERALS_SIZE;
  VirtualMemoryTable[Index].Attributes     =  ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  VirtualMemoryTable[++Index].PhysicalBase =  FixedPcdGet64 (PcdGicDistributorBase);
  VirtualMemoryTable[Index].VirtualBase    =  FixedPcdGet64 (PcdGicDistributorBase);
  VirtualMemoryTable[Index].Length         =  ZYNQMP_PERIPHERALS_SIZE;
  VirtualMemoryTable[Index].Attributes     =  ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /* SDHCI */
  VirtualMemoryTable[++Index].PhysicalBase = FixedPcdGet64 (PcdSdhciBase);
  VirtualMemoryTable[Index].VirtualBase    = FixedPcdGet64 (PcdSdhciBase);
  VirtualMemoryTable[Index].Length         = ZYNQMP_PERIPHERALS_SIZE;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /* End of Table */
  VirtualMemoryTable[++Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase    = 0;
  VirtualMemoryTable[Index].Length         = 0;
  VirtualMemoryTable[Index].Attributes     = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT ((Index) < MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}
