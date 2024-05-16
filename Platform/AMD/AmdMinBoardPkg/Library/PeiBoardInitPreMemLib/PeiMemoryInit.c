/** @file

Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMemoryInit.h"
#include "AmdMemoryInfoHob.h"

/**
  A Callback routine only AmdMemoryInfoHob is ready.

  @param[in]  PeiServices       General purpose services available to every PEIM.
  @param[in]  NotifyDescriptor  The descriptor for the notification event.
  @param[in]  Ppi               The context of the notification.

  @retval EFI_SUCCESS   Platform Pre Memory initialization is successful.
          EFI_STATUS    Various failure from underlying routine calls.
**/
EFI_STATUS
EFIAPI
EndofAmdMemoryInfoHobPpiGuidCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_PLATFORM_MEMORY_SIZE_PPI    *PlatformMemorySizePpi;
  EFI_STATUS                      Status;
  UINT64                          MemorySize;
  AMD_MEMORY_INFO_HOB             *AmdMemoryInfoHob;
  AMD_MEMORY_RANGE_DESCRIPTOR     *AmdMemoryInfoRange;
  EFI_HOB_GUID_TYPE               *GuidHob;
  EFI_PEI_HOB_POINTERS            Hob;
  UINTN                           Index;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramHobDescriptorBlock;
  EFI_PHYSICAL_ADDRESS            SmramBaseAddress;
  UINT8                           SmramRanges;
  UINTN                           DataSize;

  SmramBaseAddress = 0;
  SmramRanges      = 0;

  // Locate AMD_MEMORY_INFO_HOB Guided HOB and retrieve data
  AmdMemoryInfoHob = NULL;
  GuidHob          = GetFirstGuidHob (&gAmdMemoryInfoHobGuid);
  if (GuidHob != NULL) {
    AmdMemoryInfoHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (AmdMemoryInfoHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Error: Could not locate AMD_MEMORY_INFO_HOB.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "AMD_MEMORY_INFO_HOB at 0x%X\n", AmdMemoryInfoHob));
  DEBUG ((DEBUG_INFO, "  Version: 0x%X\n", AmdMemoryInfoHob->Version));
  DEBUG ((DEBUG_INFO, "  NumberOfDescriptor: 0x%X\n", AmdMemoryInfoHob->NumberOfDescriptor));

  //
  // Build Descriptors
  //
  DEBUG ((DEBUG_INFO, "\nAMD HOB Descriptors:"));
  for (Index = 0; Index < AmdMemoryInfoHob->NumberOfDescriptor; Index++) {
    AmdMemoryInfoRange = (AMD_MEMORY_RANGE_DESCRIPTOR *)&(AmdMemoryInfoHob->Ranges[Index]);

    DEBUG ((DEBUG_INFO, "\n Index: %d\n", Index));
    DEBUG ((DEBUG_INFO, "   Base: 0x%lX\n", AmdMemoryInfoRange->Base));
    DEBUG ((DEBUG_INFO, "   Size: 0x%lX\n", AmdMemoryInfoRange->Size));
    DEBUG ((DEBUG_INFO, "   Attribute: 0x%X\n", AmdMemoryInfoRange->Attribute));

    switch (AmdMemoryInfoRange->Attribute) {
      case AMD_MEMORY_ATTRIBUTE_AVAILABLE:
        if (AmdMemoryInfoRange->Base < SIZE_4GB) {
          SmramRanges = 1u;
          // Set SMRAM base at heighest range below 4GB
          SmramBaseAddress = AmdMemoryInfoRange->Base + AmdMemoryInfoRange->Size - FixedPcdGet32 (PcdAmdSmramAreaSize);
          BuildResourceDescriptorHob (
            EFI_RESOURCE_MEMORY_RESERVED,
            (EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
            SmramBaseAddress,
            FixedPcdGet32 (PcdAmdSmramAreaSize)
            );
          DEBUG ((
            DEBUG_INFO,
            "SMRAM RESERVED_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
            SmramBaseAddress,
            FixedPcdGet32 (PcdAmdSmramAreaSize)
            ));

          AmdMemoryInfoRange->Size -= FixedPcdGet32 (PcdAmdSmramAreaSize);
        }

        if (AmdMemoryInfoRange->Size) {
          BuildResourceDescriptorHob (
            EFI_RESOURCE_SYSTEM_MEMORY,
            SYSTEM_MEMORY_ATTRIBUTES,
            AmdMemoryInfoRange->Base,
            AmdMemoryInfoRange->Size
            );

          DEBUG ((
            DEBUG_INFO,
            "SYSTEM_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
            AmdMemoryInfoRange->Base,
            AmdMemoryInfoRange->Size
            ));
        }

        break;

      case AMD_MEMORY_ATTRIBUTE_MMIO:
        BuildResourceDescriptorHob (
          EFI_RESOURCE_MEMORY_MAPPED_IO,
          MEMORY_MAPPED_IO_ATTRIBUTES,
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          );

        DEBUG ((
          DEBUG_INFO,
          "MMIO: Base = 0x%lX, Size = 0x%lX\n",
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          ));
        break;

      case AMD_MEMORY_ATTRIBUTE_RESERVED:
      case AMD_MEMORY_ATTRIBUTE_UMA:
      default:
        BuildResourceDescriptorHob (
          EFI_RESOURCE_MEMORY_RESERVED,
          0,
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          );

        DEBUG ((
          DEBUG_INFO,
          "RESERVED_MEMORY: Base = 0x%lX, Size = 0x%lX\n",
          AmdMemoryInfoRange->Base,
          AmdMemoryInfoRange->Size
          ));
        break;
    }
  }

  ASSERT (SmramRanges > 0);
  DataSize  = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK);
  DataSize += ((SmramRanges - 1) * sizeof (EFI_SMRAM_DESCRIPTOR));

  Hob.Raw = BuildGuidHob (
              &gEfiSmmSmramMemoryGuid,
              DataSize
              );
  ASSERT (Hob.Raw);

  SmramHobDescriptorBlock                              = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)(Hob.Raw);
  SmramHobDescriptorBlock->NumberOfSmmReservedRegions  = SmramRanges;
  SmramHobDescriptorBlock->Descriptor[0].PhysicalStart = SmramBaseAddress;
  SmramHobDescriptorBlock->Descriptor[0].CpuStart      = SmramBaseAddress;
  SmramHobDescriptorBlock->Descriptor[0].PhysicalSize  = FixedPcdGet32 (PcdAmdSmramAreaSize);
  SmramHobDescriptorBlock->Descriptor[0].RegionState   = EFI_SMRAM_CLOSED | EFI_CACHEABLE;

  Status = PeiServicesLocatePpi (
             &gPeiPlatformMemorySizePpiGuid,
             0,
             NULL,
             (VOID **)&PlatformMemorySizePpi
             );
  ASSERT_EFI_ERROR (Status);

  Status = PlatformMemorySizePpi->GetPlatformMemorySize (
                                    PeiServices,
                                    PlatformMemorySizePpi,
                                    &MemorySize
                                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error(%r) in getting Platform Memory size.\n",
      __func__,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "Installing PeiMemory, BaseAddress = 0x%x, Size = 0x%x\n",
    0,
    MemorySize
    ));
  Status = PeiServicesInstallPeiMemory (0, MemorySize);
  ASSERT_EFI_ERROR (Status);
  return Status;
}
