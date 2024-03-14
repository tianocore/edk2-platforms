/** @file
  Source code file for OpenBoard Platform Init PEI module

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Core/Pei/PeiMain.h>
#include <Ppi/GraphicsPlatformPolicyPpi.h>
#include <Library/PeiGetFvInfoLib.h>

EFI_STATUS
EFIAPI
GetPeiPlatformLidStatus (
  OUT LID_STATUS  *CurrentLidStatus
  );

EFI_STATUS
EFIAPI
GetVbtData (
  OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
  OUT UINT32               *VbtSize
  );

PEI_GRAPHICS_PLATFORM_POLICY_PPI PeiGraphicsPlatform = {
  PEI_GRAPHICS_PLATFORM_POLICY_REVISION,
  GetPeiPlatformLidStatus,
  GetVbtData
};

EFI_PEI_PPI_DESCRIPTOR  mPeiGraphicsPlatformPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiGraphicsPlatformPpiGuid,
  &PeiGraphicsPlatform
};

EFI_STATUS
EFIAPI
GetVbtData (
  OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
  OUT UINT32               *VbtSize
  )
{
  EFI_GUID                        FileGuid;
  EFI_GUID                        BmpImageGuid;
  VOID                            *Buffer;
  UINT32                          Size;

  Size    = 0;
  Buffer  = NULL;


  DEBUG((DEBUG_INFO, "GetVbtData Entry\n"));

  CopyMem (&BmpImageGuid, PcdGetPtr(PcdIntelGraphicsVbtFileGuid), sizeof(BmpImageGuid));

  CopyMem(&FileGuid, &BmpImageGuid, sizeof(FileGuid));
  PeiGetSectionFromFv(FileGuid, &Buffer, &Size);
  if (Buffer == NULL) {
    DEBUG((DEBUG_ERROR, "Could not locate VBT\n"));
  } else {
    DEBUG ((DEBUG_INFO, "GetVbtData Buffer is 0x%x\n", Buffer));
    DEBUG ((DEBUG_INFO, "GetVbtData Size is 0x%x\n", Size));
    *VbtAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
    *VbtSize    = Size;
  }
  DEBUG((DEBUG_INFO, "GetVbtData exit\n"));

  return EFI_SUCCESS;
}


/**
  This function will return Lid Status in PEI phase.

  @param[out] CurrentLidStatus

  @retval     EFI_SUCCESS
  @retval     EFI_UNSUPPORTED
**/

EFI_STATUS
EFIAPI
GetPeiPlatformLidStatus (
  OUT LID_STATUS  *CurrentLidStatus
  )
{
  *CurrentLidStatus = LidOpen;
  return EFI_SUCCESS;
}

/**
  Platform Init PEI module entry point

  @param[in]  FileHandle           Not used.
  @param[in]  PeiServices          General purpose services available to every PEIM.

  @retval     EFI_SUCCESS          The function completes successfully
  @retval     EFI_OUT_OF_RESOURCES Insufficient resources to create database
**/
EFI_STATUS
EFIAPI
OpenBoardPlatformInitPostMemEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                       Status;
  PEI_CORE_INSTANCE                *PrivateData;
  UINTN                            CurrentFv;
  PEI_CORE_FV_HANDLE               *CoreFvHandle;
  VOID                             *HobData;

  //
  // Build a HOB to show current FV location for SA policy update code to consume.
  //
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);
  CurrentFv = PrivateData->CurrentPeimFvCount;
  CoreFvHandle = &(PrivateData->Fv[CurrentFv]);

  HobData = BuildGuidHob (
             &gPlatformInitFvLocationGuid,
             sizeof (VOID *)
             );
  ASSERT (HobData != NULL);
  CopyMem (HobData, (VOID *) &CoreFvHandle, sizeof (VOID *));

  //
  // Install mPeiGraphicsPlatformPpi
  //
  DEBUG ((DEBUG_INFO, "Install mPeiGraphicsPlatformPpi \n"));
  Status = PeiServicesInstallPpi (&mPeiGraphicsPlatformPpi);

  return Status;
}
