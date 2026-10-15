/** @file
  Source code file for OpenBoard Platform Init PEI module

   Copyright (c) 2022 - 2026, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
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
  EFI_PEI_FV_HANDLE                 FvHandle;
  EFI_PEI_FILE_HANDLE               CurrentFileHandle;
  UINTN                             FvInstance;
  VOID                             *HobData;

  //
  // Build a HOB to show current FV location for SA policy update code to consume.
  //
  FvHandle = NULL;
  for (FvInstance = 0; ; FvInstance++) {
    Status = PeiServicesFfsFindNextVolume (FvInstance, &FvHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    CurrentFileHandle = NULL;
    while (!EFI_ERROR (PeiServicesFfsFindNextFile (EFI_FV_FILETYPE_ALL, FvHandle, &CurrentFileHandle))) {
      if (CurrentFileHandle == FileHandle) {
        break;
      }
    }

    if (CurrentFileHandle == FileHandle) {
      Status = EFI_SUCCESS;
      break;
    }
  }
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HobData = BuildGuidHob (
             &gPlatformInitFvLocationGuid,
             sizeof (EFI_PEI_FV_HANDLE)
             );
  ASSERT (HobData != NULL);
  CopyMem (HobData, (VOID *) &FvHandle, sizeof (EFI_PEI_FV_HANDLE));

  //
  // Install mPeiGraphicsPlatformPpi
  //
  DEBUG ((DEBUG_INFO, "Install mPeiGraphicsPlatformPpi \n"));
  Status = PeiServicesInstallPpi (&mPeiGraphicsPlatformPpi);

  return Status;
}
