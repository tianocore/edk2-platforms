/** @file
  Source code file for Report Firmware Volume (FV) library for AMD platforms.

  @par Note:
    This source has the reference of MinPlatformPkgs's PeriReportFvLib.c module.

  Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportFvLib.h>

/**
  Report Pre-Memory Firmware Volumes.

  This function reports the Pre-Memory Firmware Volumes to the Firmware Volume Hob.
**/
VOID
ReportPreMemFv (
  VOID
  )
{
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
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error(%r): Failed to get boot mode.\n", __func__, Status));
    return;
  }

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
    DEBUG ((DEBUG_INFO, "Install FlashFvUefiBoot - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvUefiBootBase), PcdGet32 (PcdFlashFvUefiBootSize)));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvUefiBootBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvUefiBootBase),
      PcdGet32 (PcdFlashFvUefiBootSize),
      NULL,
      NULL,
      0
      );

    DEBUG ((DEBUG_INFO, "Install FlashFvOsBoot - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvOsBootBase), PcdGet32 (PcdFlashFvOsBootSize)));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvOsBootBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvOsBootBase),
      PcdGet32 (PcdFlashFvOsBootSize),
      NULL,
      NULL,
      0
      );

    if (PcdGet8 (PcdBootStage) >= 6) {
      DEBUG ((DEBUG_INFO, "Install FlashFvAdvanced - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvAdvancedBase), PcdGet32 (PcdFlashFvAdvancedSize)));
      PeiServicesInstallFvInfo2Ppi (
        &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvAdvancedBase))->FileSystemGuid),
        (VOID *)(UINTN)PcdGet32 (PcdFlashFvAdvancedBase),
        PcdGet32 (PcdFlashFvAdvancedSize),
        NULL,
        NULL,
        0
        );
    }

    DEBUG ((DEBUG_INFO, "Install FvAdvancedSecurity - 0x%x, 0x%x\n", PcdGet32 (PcdFlashFvAdvancedSecurityBase), PcdGet32 (PcdAmdFlashFvAdvancedSecuritySize)));
    PeiServicesInstallFvInfo2Ppi (
      &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdFlashFvAdvancedSecurityBase))->FileSystemGuid),
      (VOID *)(UINTN)PcdGet32 (PcdFlashFvAdvancedSecurityBase),
      PcdGet32 (PcdAmdFlashFvAdvancedSecuritySize),
      NULL,
      NULL,
      0
      );
    // Create the System Memory HOB for the firmware
    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_TESTED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,
      PcdGet32 (PcdFlashFvAdvancedBase),
      (PcdGet32 (PcdFlashFvPreMemoryBase) + PcdGet32 (PcdFlashFvPreMemorySize)) - PcdGet32 (PcdFlashFvAdvancedBase)
      );
    BuildMemoryAllocationHob (
      PcdGet32 (PcdFlashFvAdvancedBase),
      (PcdGet32 (PcdFlashFvPreMemoryBase) + PcdGet32 (PcdFlashFvPreMemorySize)) - PcdGet32 (PcdFlashFvAdvancedBase),
      EfiBootServicesData
      );
  }
}
