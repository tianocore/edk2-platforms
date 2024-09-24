/** @file
Do Platform Stage System Agent initialization.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiSaPolicyUpdate.h"
#include <Library/BaseMemoryLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/CpuPcieInfoFruLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiGetFvInfoLib.h>
#include <Library/PeiSiPolicyUpdateLib.h>
#include <Library/SiPolicyLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Protocol/GraphicsOutput.h>

#include <CpuPcieConfig.h>
#include <CpuPcieHob.h>
#include <IndustryStandard/Bmp.h>
#include <PolicyUpdateMacro.h>
#include <Guid/GraphicsInfoHob.h>

#include <VmdPeiConfig.h>
#include <Ppi/GraphicsPlatformPolicyPpi.h>


EFI_STATUS
EFIAPI
PeiGraphicsPolicyUpdateCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                          Status;
  EFI_PEI_GRAPHICS_INFO_HOB           *PlatformGraphicsOutput;
  EFI_PEI_HOB_POINTERS                 Hob;
  UINT8                               *HobStart;
  GRAPHICS_PEI_CONFIG                 *GtConfig;
  SI_POLICY_PPI                       *SiPolicyPpi;

  PlatformGraphicsOutput = NULL;
  HobStart = NULL;

  GtConfig = NULL;
  SiPolicyPpi = NULL;
  Status = PeiServicesLocatePpi (&gSiPolicyPpiGuid, 0, NULL, (VOID **) &SiPolicyPpi);
  ASSERT_EFI_ERROR(Status);

  Status = GetConfigBlock ((VOID *) SiPolicyPpi, &gGraphicsPeiConfigGuid, (VOID *) &GtConfig);
  ASSERT_EFI_ERROR(Status);

  Status = PeiServicesGetHobList ((VOID **) &Hob.Raw);
  HobStart = Hob.Raw;

  if (!EFI_ERROR (Status)) {
    if (HobStart != NULL) {
      if ((Hob.Raw = GetNextGuidHob (&gEfiGraphicsInfoHobGuid, HobStart)) != NULL) {
        DEBUG ((DEBUG_INFO, "Found EFI_PEI_GRAPHICS_INFO_HOB\n"));
        PlatformGraphicsOutput = GET_GUID_HOB_DATA (Hob.Guid);
      }
    }
  }

  if (PlatformGraphicsOutput != NULL) {
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.HorizontalResolution,   GtConfig->HorizontalResolution, PlatformGraphicsOutput->GraphicsMode.HorizontalResolution);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.VerticalResolution,     GtConfig->VerticalResolution,   PlatformGraphicsOutput->GraphicsMode.VerticalResolution);
  } else {
    DEBUG ((DEBUG_INFO, "Not able to find EFI_PEI_GRAPHICS_INFO_HOB\n"));
  }

  return Status;
}

STATIC
EFI_PEI_NOTIFY_DESCRIPTOR  mPeiGfxPolicyUpdateNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiGraphicsFramebufferReadyPpiGuid,
  PeiGraphicsPolicyUpdateCallback
};

/**
  UpdatePeiSaPolicy performs SA PEI Policy initialization

  @retval EFI_SUCCESS              The policy is installed and initialized.
**/
EFI_STATUS
EFIAPI
UpdatePeiSaPolicy (
  VOID
  )
{
  EFI_GUID                        BmpImageGuid;
  EFI_STATUS                      Status;
  EFI_GUID                        FileGuid;
  VOID                            *Buffer;
  UINT32                          Size;
  VOID                            *VmdVariablePtr;
  GRAPHICS_PEI_CONFIG             *GtConfig;
  SI_POLICY_PPI                   *SiPolicyPpi;
  VMD_PEI_CONFIG                  *VmdPeiConfig;
  EFI_PEI_PPI_DESCRIPTOR          *ReadyForGopConfigPpiDesc;
  VOID                            *VbtPtr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Blt;
  UINTN                            BltSize;
  UINTN                            Height;
  UINTN                            Width;

  DEBUG ((DEBUG_INFO, "Update PeiSaPolicyUpdate Pos-Mem Start\n"));

  Size  = 0;
  Blt   = NULL;
  BltSize = 0;

  GtConfig              = NULL;
  SiPolicyPpi           = NULL;
  VmdVariablePtr        = NULL;
  Buffer     = NULL;

  Status = PeiServicesLocatePpi (&gSiPolicyPpiGuid, 0, NULL, (VOID **) &SiPolicyPpi);
  ASSERT_EFI_ERROR (Status);

  Status = GetConfigBlock ((VOID *) SiPolicyPpi, &gGraphicsPeiConfigGuid, (VOID *) &GtConfig);
  ASSERT_EFI_ERROR(Status);



  VmdPeiConfig          = NULL;
  Status = GetConfigBlock ((VOID *) SiPolicyPpi, &gVmdPeiConfigGuid, (VOID *) &VmdPeiConfig);
  ASSERT_EFI_ERROR(Status);

  CopyMem(&BmpImageGuid, PcdGetPtr(PcdIntelGraphicsVbtFileGuid), sizeof(BmpImageGuid));

  if (!EFI_ERROR (Status)) {
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SkipFspGop,                      GtConfig->SkipFspGop,          0x0);
    Buffer = NULL;

    CopyMem(&FileGuid, &BmpImageGuid, sizeof(FileGuid));
    PeiGetSectionFromFv(FileGuid, &Buffer, &Size);
    if (Buffer == NULL) {
      DEBUG((DEBUG_ERROR, "Could not locate VBT\n"));
    }

    GtConfig->GraphicsConfigPtr = Buffer;
    DEBUG ((DEBUG_INFO, "Vbt Pointer from PeiGetSectionFromFv is 0x%x\n", GtConfig->GraphicsConfigPtr));
    DEBUG ((DEBUG_INFO, "Vbt Size from PeiGetSectionFromFv is 0x%x\n", Size));
    GET_POLICY ((VOID *) ((FSPS_UPD *) FspsUpd)->FspsConfig.GraphicsConfigPtr, GtConfig->GraphicsConfigPtr, VbtPtr);

    //
    // Install ReadyForGopConfig PPI to trigger PEI phase GopConfig callback.
    //
    ReadyForGopConfigPpiDesc = (EFI_PEI_PPI_DESCRIPTOR *) AllocateZeroPool (sizeof (EFI_PEI_PPI_DESCRIPTOR));
    if (ReadyForGopConfigPpiDesc == NULL) {
      ASSERT (FALSE);
      return EFI_OUT_OF_RESOURCES;
    }
    ReadyForGopConfigPpiDesc->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
    ReadyForGopConfigPpiDesc->Guid  = &gReadyForGopConfigPpiGuid;
    ReadyForGopConfigPpiDesc->Ppi   = VbtPtr;
    Status = PeiServicesInstallPpi (ReadyForGopConfigPpiDesc);

    Status = TranslateBmpToGopBlt (
              Buffer,
              Size,
              &Blt,
              &BltSize,
              &Height,
              &Width
              );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      Blt = NULL;
      Status = TranslateBmpToGopBlt (
                Buffer,
                Size,
                &Blt,
                &BltSize,
                &Height,
                &Width
                );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "TranslateBmpToGopBlt, Status = %r\n", Status));
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    //
    // Initialize Blt, BltSize
    //
    GtConfig->BltBufferAddress = Blt;

    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.BltBufferSize,   GtConfig->BltBufferSize,  BltSize);

    DEBUG ((DEBUG_INFO, "Calling mPeiGfxPolicyUpdateNotifyList\n"));
    Status = PeiServicesNotifyPpi (&mPeiGfxPolicyUpdateNotifyList);

  }

  //
  // VMD related settings from setup variable
  //
  COMPARE_AND_UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.VmdEnable,            VmdPeiConfig->VmdEnable,                       0);
  VmdPeiConfig->VmdVariablePtr = VmdVariablePtr;
  DEBUG ((DEBUG_INFO, "VmdVariablePtr from PeiGetSectionFromFv is 0x%x\n", VmdVariablePtr));

  return EFI_SUCCESS;
}
