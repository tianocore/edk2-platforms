/** @file
  Build FV related hobs for platform.

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiPei.h"
#include "Platform.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>

/**
  Publish PEI & DXE (Decompressed) Memory based FVs to let PEI
  and DXE know about them.

  @retval EFI_SUCCESS   Platform PEI FVs were initialized successfully.
**/
EFI_STATUS
PeiFvInitialization (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "Platform PEI Firmware Volume Initialization\n"));

  //
  // Create a memory allocation HOB for the PEI FV.
  //
  BuildMemoryAllocationHob (
    PcdGet64 (PcdSecPeiTempRamBase),
    PcdGet32 (PcdSecPeiTempRamSize),
    EfiBootServicesData
    );

  //
  // Let DXE know about the DXE FV
  //
  BuildFvHob (PcdGet64 (PcdFlashDxeFvBase), PcdGet32 (PcdFlashDxeFvSize));

  //
  // Let PEI know about the DXE FV so it can find the DXE Core
  //
  DEBUG ((DEBUG_INFO, "DXEFV base:%p size:%x\n", (VOID *) (UINTN)PcdGet64 (PcdFlashDxeFvBase),
    PcdGet32 (PcdFlashDxeFvSize)));
  PeiServicesInstallFvInfoPpi (
    NULL,
    (VOID *) (UINTN)PcdGet64 (PcdFlashDxeFvBase),
    PcdGet32 (PcdFlashDxeFvSize),
    NULL,
    NULL
    );

  return EFI_SUCCESS;
}
