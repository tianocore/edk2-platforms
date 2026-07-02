/** @file
  Source code file for Report Firmware Volume (FV) library

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportFvLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Ppi/FirmwareVolumeInfo2.h>

// Use a FV pointer PCD to get a pointer to the FileSystemGuid in the FV header
#define PCD_TO_FV_HEADER_FILE_SYSTEM_GUID(Pcd)  (&((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN) PcdGet32 (Pcd))->FileSystemGuid)

/**
  Reports FVs necessary for MinPlarform pre-memory initialization.
**/
VOID
ReportPreMemFv (
  VOID
  )
{
  UINTN                              Index;
  EFI_PEI_PPI_DESCRIPTOR             *Descriptor;
  EFI_PEI_FIRMWARE_VOLUME_INFO2_PPI  *Ppi;
  EFI_STATUS                         Status;
  EFI_FIRMWARE_VOLUME_HEADER         *FvHeader;
  EFI_BOOT_MODE                      BootMode;

  Index      = 0;
  Descriptor = NULL;
  Ppi        = NULL;
  Status     = EFI_SUCCESS;
  FvHeader   = NULL;
  BootMode   = BOOT_WITH_FULL_CONFIGURATION;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    for (Index = 0; Status == EFI_SUCCESS; Index++) {
    Status = PeiServicesLocatePpi (&gEfiPeiFirmwareVolumeInfo2PpiGuid, Index, &Descriptor, (VOID **)&Ppi);
    if (!EFI_ERROR (Status)) {
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Ppi->FvInfo;
      DEBUG ((DEBUG_INFO, "Found FV at 0x%x, size 0x%x\n", FvHeader, FvHeader->FvLength));
    }
  }

    );

  //
  // FvBspPreMemory and FvPreMemory are required for all stages.
  //

  DEBUG ((DEBUG_INFO, "Install FlashFvBspPreMemory - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvBspPreMemoryBase), PcdGet32 (PcdFlashFvBspPreMemorySize)));
  PeiServicesInstallFvInfo2Ppi (
    PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvBspPreMemoryBase),
    (VOID *)(UINTN)PcdGet32 (PcdFlashFvBspPreMemoryBase),
    PcdGet32 (PcdFlashFvBspPreMemorySize),
    NULL,
    NULL,
    0
    );

  DEBUG ((DEBUG_INFO, "Install FlashFvPreMemory - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvPreMemoryBase), PcdGet32 (PcdFlashFvPreMemorySize)));
  PeiServicesInstallFvInfo2Ppi (
    PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvPreMemoryBase),
    (VOID *)(UINTN)PcdGet32 (PcdFlashFvPreMemoryBase),
    PcdGet32 (PcdFlashFvPreMemorySize),
    NULL,
    NULL,
    0
    );

  //
  // In API mode, do not publish FSP FV.
  //
  if (!PcdGetBool (PcdFspWrapperBootMode)) {
    //
    // FvFspT may be required for all stages
    //
    DEBUG ((DEBUG_INFO, "Install FlashFvFspT - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvFspTBase), PcdGet32 (PcdFlashFvFspTSize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvFspTBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvFspTBase),
      PcdGet32 (PcdFlashFvFspTSize),
      NULL,
      NULL,
      0
      );

    //
    // FvFspM required for stage 2 and above
    //
    if (PcdGet8 (PcdBootStage) >= 2) {
      DEBUG ((DEBUG_INFO, "Install FlashFvFspM - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvFspMBase), PcdGet32 (PcdFlashFvFspMSize)));
      PeiServicesInstallFvInfo2Ppi (
        PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvFspMBase),
        (VOID *)(UINTN)PcdGet32 (PcdFlashFvFspMBase),
        PcdGet32 (PcdFlashFvFspMSize),
        NULL,
        NULL,
        0
        );
    }
  }

  //
  // FvAdvanced not needed until stage 6
  //
  if (PcdGet8 (PcdBootStage) >= 6) {
    DEBUG ((DEBUG_INFO, "Install FlashFvAdvancedPreMemory - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvAdvancedPreMemoryBase), PcdGet32 (PcdFlashFvAdvancedPreMemorySize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvAdvancedPreMemoryBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvAdvancedPreMemoryBase),
      PcdGet32 (PcdFlashFvAdvancedPreMemorySize),
      NULL,
      NULL,
      0
      );
  }
}

/**
  Reports FVs for MinPlarform post-memory initialization.
  This function also publish FV HOBs to ensure DXE phase is aware of those FVs.
**/
VOID
ReportPostMemFv (
  VOID
  )
{
  UINTN                              Index;
  EFI_PEI_PPI_DESCRIPTOR             *Descriptor;
  EFI_PEI_FIRMWARE_VOLUME_INFO2_PPI  *Ppi;
  EFI_STATUS                         Status;
  EFI_FIRMWARE_VOLUME_HEADER         *FvHeader;

  Index      = 0;
  Descriptor = NULL;
  Ppi        = NULL;
  Status     = EFI_SUCCESS;
  FvHeader   = NULL;

  DEBUG_CODE (
    for (Index = 0; Status == EFI_SUCCESS; Index++) {
    Status = PeiServicesLocatePpi (&gEfiPeiFirmwareVolumeInfo2PpiGuid, Index, &Descriptor, (VOID **)&Ppi);
    if (!EFI_ERROR (Status)) {
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Ppi->FvInfo;
      DEBUG ((DEBUG_INFO, "Found FV at 0x%x, size 0x%x\n", FvHeader, FvHeader->FvLength));
    }
  }

    );

  //
  // FvFspS, FvPostMemory, and FvBsp may be required for completing stage 2
  //
  if (PcdGet8 (PcdBootStage) >= 2) {
    //
    // In API mode, do not publish FSP FV.
    //
    if (!PcdGetBool (PcdFspWrapperBootMode)) {
      DEBUG ((DEBUG_INFO, "Install FlashFvFspS - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvFspSBase), PcdGet32 (PcdFlashFvFspSSize)));
      PeiServicesInstallFvInfo2Ppi (
        PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvFspSBase),
        (VOID *)(UINTN)PcdGet32 (PcdFlashFvFspSBase),
        PcdGet32 (PcdFlashFvFspSSize),
        NULL,
        NULL,
        0
        );
    }

    DEBUG ((DEBUG_INFO, "Install FlashFvPostMemory - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvPostMemoryBase), PcdGet32 (PcdFlashFvPostMemorySize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvPostMemoryBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvPostMemoryBase),
      PcdGet32 (PcdFlashFvPostMemorySize),
      NULL,
      NULL,
      0
      );

    DEBUG ((DEBUG_INFO, "Build FlashFvPostMemory FV Hob at %Lx \n", (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashFvPostMemoryBase)));

    BuildFvHob (
      (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashFvPostMemoryBase),
      PcdGet32 (PcdFlashFvPostMemorySize)
      );

    DEBUG ((DEBUG_INFO, "Install FlashFvBsp - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvBspBase), PcdGet32 (PcdFlashFvBspSize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvBspBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvBspBase),
      PcdGet32 (PcdFlashFvBspSize),
      NULL,
      NULL,
      0
      );
  }

  //
  // FvUefiBoot required for completing stage 3
  //
  if (PcdGet8 (PcdBootStage) >= 3) {
    DEBUG ((DEBUG_INFO, "Install FlashFvUefiBoot - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvUefiBootBase), PcdGet32 (PcdFlashFvUefiBootSize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvUefiBootBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvUefiBootBase),
      PcdGet32 (PcdFlashFvUefiBootSize),
      NULL,
      NULL,
      0
      );

    DEBUG ((DEBUG_INFO, "Build FlashFvUefiBoot FV Hob at %Lx \n", (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashFvUefiBootBase)));

    BuildFvHob (
      (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashFvUefiBootBase),
      PcdGet32 (PcdFlashFvUefiBootSize)
      );
  }

  //
  // FvOsBoot required for completing stage 4
  //
  if (PcdGet8 (PcdBootStage) >= 4) {
    DEBUG ((DEBUG_INFO, "Install FlashFvOsBoot - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvOsBootBase), PcdGet32 (PcdFlashFvOsBootSize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvOsBootBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvOsBootBase),
      PcdGet32 (PcdFlashFvOsBootSize),
      NULL,
      NULL,
      0
      );

    DEBUG ((DEBUG_INFO, "Build FlashFvOsBoot FV Hob at %Lx \n", (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashFvUefiBootBase)));

    BuildFvHob (
      (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashFvOsBootBase),
      PcdGet32 (PcdFlashFvOsBootSize)
      );
  }

  //
  // FvSecurity required for completing stage 5
  //
  if (PcdGet8 (PcdBootStage) >= 5) {
    DEBUG ((DEBUG_INFO, "Install FlashFvSecurity - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvSecurityBase), PcdGet32 (PcdFlashFvSecuritySize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvSecurityBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvSecurityBase),
      PcdGet32 (PcdFlashFvSecuritySize),
      NULL,
      NULL,
      0
      );
  }

  //
  // FvAdvanced required for completing stage 6
  //
  if (PcdGet8 (PcdBootStage) >= 6) {
    DEBUG ((DEBUG_INFO, "Install FlashFvAdvanced - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvAdvancedBase), PcdGet32 (PcdFlashFvAdvancedSize)));
    PeiServicesInstallFvInfo2Ppi (
      PCD_TO_FV_HEADER_FILE_SYSTEM_GUID (PcdFlashFvAdvancedBase),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvAdvancedBase),
      PcdGet32 (PcdFlashFvAdvancedSize),
      NULL,
      NULL,
      0
      );
  }

  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT     |
     EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    (UINTN)PcdGet32 (PcdFlashAreaBaseAddress),
    (UINTN)PcdGet32 (PcdFlashAreaSize)
    );

  BuildMemoryAllocationHob (
    (UINTN)PcdGet32 (PcdFlashAreaBaseAddress),
    (UINTN)PcdGet32 (PcdFlashAreaSize),
    EfiMemoryMappedIO
    );
}
