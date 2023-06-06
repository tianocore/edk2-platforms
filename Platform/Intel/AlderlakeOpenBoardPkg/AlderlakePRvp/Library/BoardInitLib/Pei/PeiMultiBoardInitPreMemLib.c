/** @file
  PEI Multi-Board Initialization in Pre-Memory PEI Library

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BoardInitLib.h>
#include <Library/MultiBoardInitSupportLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/BoardConfigLib.h>
#include <PlatformBoardId.h>
#include <Library/PeiServicesLib.h>

EFI_STATUS
EFIAPI
AdlPBoardDetect (
  VOID
  );

EFI_STATUS
EFIAPI
AdlPMultiBoardDetect (
  VOID
  );

EFI_BOOT_MODE
EFIAPI
AdlPBoardBootModeDetect (
  VOID
  );

EFI_STATUS
EFIAPI
AdlPBoardDebugInit (
  VOID
  );

EFI_STATUS
EFIAPI
AdlPBoardInitBeforeMemoryInit (
  VOID
  );


EFI_STATUS
EFIAPI
AdlPBoardPatchConfigurationDataPreMemCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

static EFI_PEI_NOTIFY_DESCRIPTOR mOtherBoardPatchConfigurationDataPreMemNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPatchConfigurationDataPreMemPpiGuid,
  AdlPBoardPatchConfigurationDataPreMemCallback
};

BOARD_DETECT_FUNC  mAdlPBoardDetectFunc = {
  AdlPMultiBoardDetect
};

BOARD_PRE_MEM_INIT_FUNC  mAdlPBoardPreMemInitFunc = {
  AdlPBoardDebugInit,
  AdlPBoardBootModeDetect,
  AdlPBoardInitBeforeMemoryInit,
  NULL, // BoardInitBeforeTempRamExit
  NULL, // BoardInitAfterTempRamExit
};

EFI_STATUS
EFIAPI
AdlPMultiBoardDetect (
  VOID
  )
{
  UINT8  SkuType;
  DEBUG ((DEBUG_INFO, " In AdlPMultiBoardDetect \n"));

  AdlPBoardDetect ();

  SkuType = PcdGet8 (PcdSkuType);
  if (SkuType==AdlPSkuType) {
    RegisterBoardPreMemInit (&mAdlPBoardPreMemInitFunc);
    PeiServicesNotifyPpi (&mOtherBoardPatchConfigurationDataPreMemNotifyList);
  } else {
    DEBUG ((DEBUG_WARN,"Not a Valid Alderlake P Board\n"));
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiAdlPMultiBoardInitPreMemLibConstructor (
  VOID
  )
{
  return RegisterBoardDetect (&mAdlPBoardDetectFunc);
}