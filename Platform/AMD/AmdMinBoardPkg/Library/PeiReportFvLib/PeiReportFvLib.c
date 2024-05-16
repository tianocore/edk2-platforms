/** @file
  Source code file for Report Firmware Volume (FV) library for AMD platforms.

  @par Note:
    This source has the reference of MinPlatformPkgs's PeriReportFvLib.c module.

  Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportFvLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Ppi/FirmwareVolumeInfo.h>

/**
  Report Pre-Memory Firmware Volumes.

  This function reports the Pre-Memory Firmware Volumes to the Firmware Volume Hob.
**/
VOID
ReportPreMemFv (
  VOID
  )
{
  ///
  /// Note : FSP FVs except FSP-T FV are installed in IntelFsp2WrapperPkg in Dispatch mode.
  ///
  if (PcdGetBool (PcdFspWrapperBootMode)) {
    DEBUG ((DEBUG_INFO, "Install FlashFvFspT - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvFspTBase), PcdGet32 (PcdFlashFvFspTSize)));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvFspTBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvFspTBase),
      PcdGet32 (PcdFlashFvFspTSize),
      NULL,
      NULL,
      0
      );
  }

  DEBUG ((DEBUG_INFO, "Install FlashFvSecurity - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvSecurityBase), PcdGet32 (PcdFlashFvSecuritySize)));
  PeiServicesInstallFvInfo2Ppi (
    &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvSecurityBase))->FileSystemGuid),
    (VOID *)(UINTN)PcdGet32 (PcdFlashFvSecurityBase),
    PcdGet32 (PcdFlashFvSecuritySize),
    NULL,
    NULL,
    0
    );
  if (PcdGet8 (PcdBootStage) >= 6) {
    DEBUG ((
      DEBUG_INFO,
      "Install FlashFvAdvancedPreMemory - 0x%x, 0x%x\n",
      PcdGet32 (PcdFlashFvAdvancedPreMemoryBase),
      PcdGet32 (PcdFlashFvAdvancedPreMemorySize)
      ));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvAdvancedPreMemoryBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvAdvancedPreMemoryBase),
      PcdGet32 (PcdFlashFvAdvancedPreMemorySize),
      NULL,
      NULL,
      0
      );
  }
}

/**
  Report Post-Memory Firmware Volumes.

  This function reports the Post-Memory Firmware Volumes to the Firmware Volume Hob.
**/
VOID
ReportPostMemFv (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_BOOT_MODE             BootMode;
  EFI_HOB_FIRMWARE_VOLUME3  *Hob3;
  EFI_HOB_FIRMWARE_VOLUME   *Hob;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Note : FSP FVs except FSP-T FV are installed in IntelFsp2WrapperPkg in Dispatch mode.
  ///

  ///
  /// Build HOB for DXE
  ///
  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    ///
    /// Prepare the recovery service
    ///
  } else {
    DEBUG ((DEBUG_INFO, "Install FlashFvPostMemory - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvPostMemoryBase), PcdGet32 (PcdFlashFvPostMemorySize)));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvPostMemoryBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvPostMemoryBase),
      PcdGet32 (PcdFlashFvPostMemorySize),
      NULL,
      NULL,
      0
      );

    if (PcdGet64 (PcdAmdFlashFvUefiBootBase) >= BASE_4GB) {
      Hob  = NULL;
      Hob3 = NULL;
      DEBUG ((DEBUG_INFO, "Found FvUefiBoot FV above 4GB, creating FV HOBs.\n"));
      Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME), (VOID **)&Hob);
      if (!EFI_ERROR (Status)) {
        Hob->BaseAddress = PcdGet64 (PcdAmdFlashFvUefiBootBase);
        Hob->Length      = PcdGet32 (PcdFlashFvUefiBootSize);
      }

      Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV3, sizeof (EFI_HOB_FIRMWARE_VOLUME3), (VOID **)&Hob3);
      if (!EFI_ERROR (Status)) {
        Hob3->BaseAddress          = PcdGet64 (PcdAmdFlashFvUefiBootBase);
        Hob3->Length               = PcdGet32 (PcdFlashFvUefiBootSize);
        Hob3->AuthenticationStatus = 0;
        Hob3->ExtractedFv          = FALSE;
      }
    } else {
      DEBUG ((DEBUG_INFO, "Install FlashFvUefiBoot - 0x%lx, 0x%x\n", PcdGet64 (PcdAmdFlashFvUefiBootBase), PcdGet32 (PcdFlashFvUefiBootSize)));
      PeiServicesInstallFvInfo2Ppi (
        &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet64 (PcdAmdFlashFvUefiBootBase))->FileSystemGuid),
        (VOID *)(UINTN)PcdGet64 (PcdAmdFlashFvUefiBootBase),
        PcdGet32 (PcdFlashFvUefiBootSize),
        NULL,
        NULL,
        0
        );
    }

    if (PcdGet64 (PcdAmdFlashFvOsBootBase) >= BASE_4GB) {
      Hob  = NULL;
      Hob3 = NULL;
      DEBUG ((DEBUG_INFO, "Found FvOsBoot FV above 4GB, creating FV HOBs.\n"));
      Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME), (VOID **)&Hob);
      if (!EFI_ERROR (Status)) {
        Hob->BaseAddress = PcdGet64 (PcdAmdFlashFvOsBootBase);
        Hob->Length      = PcdGet32 (PcdFlashFvOsBootSize);
      }

      Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV3, sizeof (EFI_HOB_FIRMWARE_VOLUME3), (VOID **)&Hob3);
      if (!EFI_ERROR (Status)) {
        Hob3->BaseAddress          = PcdGet64 (PcdAmdFlashFvOsBootBase);
        Hob3->Length               = PcdGet32 (PcdFlashFvOsBootSize);
        Hob3->AuthenticationStatus = 0;
        Hob3->ExtractedFv          = FALSE;
      }
    } else {
      DEBUG ((DEBUG_INFO, "Install FlashFvOsBoot - 0x%lx, 0x%x\n", PcdGet64 (PcdAmdFlashFvOsBootBase), PcdGet32 (PcdFlashFvOsBootSize)));
      PeiServicesInstallFvInfo2Ppi (
        &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet64 (PcdAmdFlashFvOsBootBase))->FileSystemGuid),
        (VOID *)(UINTN)PcdGet64 (PcdAmdFlashFvOsBootBase),
        PcdGet32 (PcdFlashFvOsBootSize),
        NULL,
        NULL,
        0
        );
    }

    if (PcdGet8 (PcdBootStage) >= 6) {
      if (PcdGet64 (PcdAmdFlashFvAdvancedBase) >= BASE_4GB) {
        Hob  = NULL;
        Hob3 = NULL;
        DEBUG ((DEBUG_INFO, "Found FvAdvanced FV above 4GB, creating FV HOBs.\n"));
        Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME), (VOID **)&Hob);
        if (!EFI_ERROR (Status)) {
          Hob->BaseAddress = PcdGet64 (PcdAmdFlashFvAdvancedBase);
          Hob->Length      = PcdGet32 (PcdFlashFvAdvancedSize);
        }

        Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV3, sizeof (EFI_HOB_FIRMWARE_VOLUME3), (VOID **)&Hob3);
        if (!EFI_ERROR (Status)) {
          Hob3->BaseAddress          = PcdGet64 (PcdAmdFlashFvAdvancedBase);
          Hob3->Length               = PcdGet32 (PcdFlashFvAdvancedSize);
          Hob3->AuthenticationStatus = 0;
          Hob3->ExtractedFv          = FALSE;
        }
      } else {
        DEBUG ((DEBUG_INFO, "Install FlashFvAdvanced - 0x%lx, 0x%x\n", PcdGet64 (PcdAmdFlashFvAdvancedBase), PcdGet32 (PcdFlashFvAdvancedSize)));
        PeiServicesInstallFvInfo2Ppi (
          &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet64 (PcdAmdFlashFvAdvancedBase))->FileSystemGuid),
          (VOID *)(UINTN)PcdGet64 (PcdAmdFlashFvAdvancedBase),
          PcdGet32 (PcdFlashFvAdvancedSize),
          NULL,
          NULL,
          0
          );
      }
    }
  }

  if (PcdGet64 (PcdAmdFlashFvAdvancedSecurityBase) >= BASE_4GB) {
    Hob  = NULL;
    Hob3 = NULL;
    DEBUG ((DEBUG_INFO, "Found FvAdvancedSecurity FV above 4GB, creating FV HOBs.\n"));
    Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV, sizeof (EFI_HOB_FIRMWARE_VOLUME), (VOID **)&Hob);
    if (!EFI_ERROR (Status)) {
      Hob->BaseAddress = PcdGet64 (PcdAmdFlashFvAdvancedSecurityBase);
      Hob->Length      = PcdGet32 (PcdAmdFlashFvAdvancedSecuritySize);
    }

    Status = PeiServicesCreateHob (EFI_HOB_TYPE_FV3, sizeof (EFI_HOB_FIRMWARE_VOLUME3), (VOID **)&Hob3);
    if (!EFI_ERROR (Status)) {
      Hob3->BaseAddress          = PcdGet64 (PcdAmdFlashFvAdvancedSecurityBase);
      Hob3->Length               = PcdGet32 (PcdAmdFlashFvAdvancedSecuritySize);
      Hob3->AuthenticationStatus = 0;
      Hob3->ExtractedFv          = FALSE;
    }
  } else {
    DEBUG ((DEBUG_INFO, "Install FvAdvancedSecurity - 0x%lx, 0x%x\n", PcdGet64 (PcdAmdFlashFvAdvancedSecurityBase), PcdGet32 (PcdAmdFlashFvAdvancedSecuritySize)));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet64 (PcdAmdFlashFvAdvancedSecurityBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet64 (PcdAmdFlashFvAdvancedSecurityBase),
      PcdGet32 (PcdAmdFlashFvAdvancedSecuritySize),
      NULL,
      NULL,
      0
      );
  }

  //
  // Report resource HOB for flash FV
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
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
