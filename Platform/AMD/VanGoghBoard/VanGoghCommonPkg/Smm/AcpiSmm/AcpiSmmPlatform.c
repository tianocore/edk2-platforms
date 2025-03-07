/** @file
ACPISMM Driver implementation file.

This is QNC Smm platform driver

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2019 Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AcpiSmmPlatform.h>

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param[in] Size   Size of memory to allocate.

  @return       Allocated address for output.

**/
VOID *
AllocateAcpiNvsMemoryBelow4G (
  IN UINTN  Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  Pages,
                  &Address
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Buffer = (VOID *)(UINTN)Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Reserved S3 memory for InstallS3Memory

  @retval  EFI_OUT_OF_RESOURCES     Insufficient resources to complete function.
  @retval  EFI_SUCCESS              Function has completed successfully.

**/
EFI_STATUS
EFIAPI
ReservedS3Memory (
  UINTN  SystemMemoryLength

  )

{
  VOID                            *GuidHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  VOID                            *AcpiReservedBase;

  UINTN                   TsegIndex;
  UINTN                   TsegSize;
  UINTN                   TsegBase;
  RESERVED_ACPI_S3_RANGE  *AcpiS3Range;

  DEBUG ((DEBUG_INFO, "ReservedS3Memory, SystemMemoryLength: 0x%08X\n", SystemMemoryLength));
  //
  // Get Hob list for SMRAM desc
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  ASSERT (GuidHob != NULL);
  DEBUG ((DEBUG_INFO, "gEfiSmmPeiSmramMemoryReserveGuid: 0x%X \n", (UINTN)GuidHob));
  DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (DescriptorBlock != NULL);

  //
  // Use the hob to get SMRAM capabilities
  //
  TsegIndex = DescriptorBlock->NumberOfSmmReservedRegions - 1;
  DEBUG ((DEBUG_INFO, "DescriptorBlock->NumberOfSmmReservedRegions: 0x%X\n", DescriptorBlock->NumberOfSmmReservedRegions));
  DEBUG ((DEBUG_INFO, "TsegIndex: 0x%X\n", TsegIndex));
  ASSERT (TsegIndex <= (MAX_SMRAM_RANGES - 1));
  TsegBase = (UINTN)DescriptorBlock->Descriptor[TsegIndex].PhysicalStart;
  TsegSize = (UINTN)DescriptorBlock->Descriptor[TsegIndex].PhysicalSize;

  DEBUG ((DEBUG_INFO, "SMM  Base: %08X\n", TsegBase));
  DEBUG ((DEBUG_INFO, "SMM  Size: %08X\n", TsegSize));

  //
  // Now find the location of the data structure that is used to store the address
  // of the S3 reserved memory.
  //
  AcpiS3Range = (RESERVED_ACPI_S3_RANGE *)(UINTN)(TsegBase + RESERVED_ACPI_S3_RANGE_OFFSET);
  DEBUG ((DEBUG_INFO, "AcpiS3Range: %08X\n", (UINTN)AcpiS3Range));
  //
  // Allocate reserved ACPI memory for S3 resume.  Pointer to this region is
  // stored in SMRAM in the first page of TSEG.
  //
  AcpiReservedBase = AllocateAcpiNvsMemoryBelow4G (PcdGet32 (PcdS3AcpiReservedMemorySize));
  DEBUG ((DEBUG_INFO, "AcpiReservedBase: %08X\n", (UINTN)AcpiReservedBase));
  ASSERT (AcpiReservedBase != NULL);
  if (AcpiReservedBase != NULL) {
    AcpiS3Range->AcpiReservedMemoryBase = (UINT32)(UINTN)AcpiReservedBase;
    AcpiS3Range->AcpiReservedMemorySize = PcdGet32 (PcdS3AcpiReservedMemorySize);
  }

  AcpiS3Range->SystemMemoryLength = (UINT32)SystemMemoryLength;

  DEBUG ((DEBUG_INFO, "S3 Memory  Base:    %08X\n", AcpiS3Range->AcpiReservedMemoryBase));
  DEBUG ((DEBUG_INFO, "S3 Memory  Size:    %08X\n", AcpiS3Range->AcpiReservedMemorySize));
  DEBUG ((DEBUG_INFO, "S3 SysMemoryLength: %08X\n", AcpiS3Range->SystemMemoryLength));

  return EFI_SUCCESS;
}

/**
  Initializes the SMM S3 Handler Driver.

  @param[in]  ImageHandle     The image handle of Sleep State Wake driver.
  @param[in]  SystemTable     The starndard EFI system table.

  @retval  EFI_OUT_OF_RESOURCES     Insufficient resources to complete function.
  @retval  EFI_SUCCESS              Function has completed successfully.
  @retval  Other                    Error occured during execution.

**/
EFI_STATUS
EFIAPI
InitAcpiSmmPlatform (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  EFI_STATUS                    Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL  *AcpiNvsProtocol = NULL;
  UINTN                         MemoryLength;
  EFI_PEI_HOB_POINTERS          Hob;

  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  (VOID **)&AcpiNvsProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Calculate the system memory length by memory hobs
  //
  MemoryLength = 0x100000;
  Hob.Raw      = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  ASSERT (Hob.Raw != NULL);
  while ((Hob.Raw != NULL) && (!END_OF_HOB_LIST (Hob))) {
    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      //
      // Skip the memory region below 1MB
      //
      if (Hob.ResourceDescriptor->PhysicalStart >= 0x100000) {
        MemoryLength += (UINTN)Hob.ResourceDescriptor->ResourceLength;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  Status = ReservedS3Memory (MemoryLength);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
