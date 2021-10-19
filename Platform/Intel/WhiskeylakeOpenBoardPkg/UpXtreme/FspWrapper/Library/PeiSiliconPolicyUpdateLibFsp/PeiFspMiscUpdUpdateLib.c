/** @file
  Implementation of Fsp Misc UPD Initialization.


  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiLib.h>
#include <Library/ConfigBlockLib.h>
#include <Library/PeiServicesLib.h>

#include <FspEas.h>
#include <FspmUpd.h>
#include <FspsUpd.h>

#include <Library/DebugLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/PciLib.h>
#include <Guid/MemoryOverwriteControl.h>
#include <PchAccess.h>
#include <Platform.h>

#include "PeiMiscPolicyUpdate.h"

/**
  Performs FSP Misc UPD initialization.

  @param[in,out]    FspmUpd                 Pointer to FSPM_UPD Data.

  @retval           EFI_SUCCESS             FSP UPD Data is updated.
  @retval           EFI_NOT_FOUND           An instance of gEfiPeiReadOnlyVariable2PpiGuid
                                            could not be located.
  @retval           EFI_OUT_OF_RESOURCES    Insufficent resources to allocate a memory buffer.
**/
EFI_STATUS
EFIAPI
PeiFspMiscUpdUpdatePreMem (
  IN OUT FSPM_UPD    *FspmUpd
  )
{
  EFI_STATUS                        Status;
  UINTN                             FspNvsBufferSize;
  VOID                              *FspNvsBufferPtr;

  FspNvsBufferPtr   = NULL;
  FspNvsBufferSize  = 0;
  Status = PeiGetLargeVariable (L"FspNvsBuffer", &gFspNvsBufferVariableGuid, &FspNvsBufferPtr, &FspNvsBufferSize);
  if (Status == EFI_SUCCESS) {
    FspmUpd->FspmArchUpd.NvsBufferPtr = FspNvsBufferPtr;
  } else {
    DEBUG ((DEBUG_INFO, "Cannot create FSP NVS Buffer, UEFI variable does not exist (this is likely a first boot)\n"));
    FspmUpd->FspmArchUpd.NvsBufferPtr = NULL;
  }

  FspmUpd->FspmConfig.TsegSize              = FixedPcdGet32 (PcdTsegSize);
  FspmUpd->FspmConfig.CpuRatio              = 0;
  FspmUpd->FspmConfig.CaVrefConfig          = PcdGet8 (PcdMrcCaVrefConfig);
  FspmUpd->FspmConfig.PlatformMemorySize    = PEI_MIN_MEMORY_SIZE;
  FspmUpd->FspmConfig.PcdSerialDebugLevel   = 3;
  FspmUpd->FspmConfig.SafeMode              = 0;
  FspmUpd->FspmConfig.PeciC10Reset          = 0;

  return EFI_SUCCESS;
}


