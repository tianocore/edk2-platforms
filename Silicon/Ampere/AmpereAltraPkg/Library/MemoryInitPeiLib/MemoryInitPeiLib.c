/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/ArmMmuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

VOID
BuildMemoryTypeInformationHob (
  VOID
  );

STATIC
VOID
InitMmu (
  IN ARM_MEMORY_REGION_DESCRIPTOR *MemoryTable
  )
{
  VOID          *TranslationTableBase;
  UINTN         TranslationTableSize;
  RETURN_STATUS Status;

  // Note: Because we called PeiServicesInstallPeiMemory() before to call InitMmu()
  // the MMU Page Table resides in DRAM (even at the top of DRAM as it is the first
  // permanent memory allocation)
  //
  Status = ArmConfigureMmu (MemoryTable, &TranslationTableBase, &TranslationTableSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to enable MMU\n"));
  }

  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)(UINTN)TranslationTableBase,
    EFI_SIZE_TO_PAGES (TranslationTableSize) * EFI_PAGE_SIZE,
    EfiBootServicesData
    );
}

EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS UefiMemoryBase,
  IN UINT64               UefiMemorySize
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR *MemoryTable;
  UINTN                        Index;

  /* Get Virtual Memory Map from the Platform Library */
  ArmPlatformGetVirtualMemoryMap (&MemoryTable);

  Index = 0;
  while (MemoryTable[Index].Length != 0) {
    if (MemoryTable[Index].Attributes == ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        EFI_RESOURCE_ATTRIBUTE_PRESENT |
        EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
        EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_TESTED,
        MemoryTable[Index].PhysicalBase,
        MemoryTable[Index].Length
        );
    } else if (MemoryTable[Index].Attributes == ARM_MEMORY_REGION_ATTRIBUTE_DEVICE) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_MEMORY_MAPPED_IO,
        EFI_RESOURCE_ATTRIBUTE_PRESENT     |
        EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
        EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,
        MemoryTable[Index].PhysicalBase,
        MemoryTable[Index].Length
        );
    }
    Index++;
  }

  BuildMemoryAllocationHob (
    PcdGet64 (PcdFdBaseAddress),
    PcdGet32 (PcdFdSize),
    EfiRuntimeServicesData
    );

  InitMmu (MemoryTable);

  if (FeaturePcdGet (PcdPrePiProduceMemoryTypeInformationHob)) {
    // Optional feature that helps prevent EFI memory map fragmentation.
    BuildMemoryTypeInformationHob ();
  }

  return EFI_SUCCESS;
}
