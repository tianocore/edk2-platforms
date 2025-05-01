/** @file
  This will be invoked only once. It will call FspMemoryInit API,
  register TemporaryRamDonePpi to call TempRamExit API, and register MemoryDiscoveredPpi
  notify to call FspSiliconInit API.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/FspWrapperHobProcessLib.h>
#include "../Include/Library/FspWrapperMultiPhaseProcessLib.h"
#include "../Include/Library/FspWrapperPlatformMultiPhaseLib.h"
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Ppi/FspSiliconInitDone.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/TemporaryRamDone.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>
#include <Library/FspWrapperApiTestLib.h>
#include <FspEas.h>
#include <FspStatusCode.h>
#include <FspGlobalData.h>
#include <Library/LzmaCustomDecompressLib/LzmaDecompressLibInternal.h>
#include <FspExportedInterfaceHob.h>

extern EFI_PEI_NOTIFY_DESCRIPTOR  mS3EndOfPeiNotifyDesc;
extern EFI_GUID                   gFspHobGuid;

STATIC CONST EFI_GUID  SmmDriverVolumeFileName =
{
  0x82DFABE7, 0xCD0E, 0x44D3, { 0xAF, 0xBE, 0x46, 0x82, 0x21, 0xD1, 0x08, 0xC4 }
};

/**
  This function handles S3 resume task at the end of PEI.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mS3EndOfPeiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  S3EndOfPeiNotify
};

/**
  This function handles S3 resume task at the end of PEI.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  NOTIFY_PHASE_PARAMS  NotifyPhaseParams;
  EFI_STATUS           Status;

  DEBUG ((DEBUG_INFO, "S3EndOfPeiNotify enter\n"));

  NotifyPhaseParams.Phase = EnumInitPhaseAfterPciEnumeration;
  Status                  = CallFspNotifyPhase (&NotifyPhaseParams);
  DEBUG ((DEBUG_INFO, "FSP S3NotifyPhase AfterPciEnumeration status: 0x%x\n", Status));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FSP S3NotifyPhase AfterPciEnumeration requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  Status                  = CallFspNotifyPhase (&NotifyPhaseParams);
  DEBUG ((DEBUG_INFO, "FSP S3NotifyPhase ReadyToBoot status: 0x%x\n", Status));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FSP S3NotifyPhase ReadyToBoot requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  NotifyPhaseParams.Phase = EnumInitPhaseEndOfFirmware;
  Status                  = CallFspNotifyPhase (&NotifyPhaseParams);
  DEBUG ((DEBUG_INFO, "FSP S3NotifyPhase EndOfFirmware status: 0x%x\n", Status));

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FSP S3NotifyPhase EndOfFirmware requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  return EFI_SUCCESS;
}

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS      Return Hob list produced by FSP successfully.
**/
EFI_STATUS
EFIAPI
FspSiliconInitDoneGetFspHobList (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  FSP_SILICON_INIT_DONE_PPI  *This,
  OUT VOID                       **FspHobList
  );

FSP_SILICON_INIT_DONE_PPI  mFspSiliconInitDonePpi = {
  FspSiliconInitDoneGetFspHobList
};

EFI_PEI_PPI_DESCRIPTOR  mPeiFspSiliconInitDonePpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gFspSiliconInitDonePpiGuid,
  &mFspSiliconInitDonePpi
};

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS      Return Hob list produced by FSP successfully.
**/
EFI_STATUS
EFIAPI
FspSiliconInitDoneGetFspHobList (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  FSP_SILICON_INIT_DONE_PPI  *This,
  OUT VOID                       **FspHobList
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  if (GuidHob != NULL) {
    *FspHobList = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Get the FSP S UPD Data address

  @return FSP-S UPD Data Address
**/
UINTN
GetFspsUpdDataAddress (
  VOID
  )
{
  if (PcdGet64 (PcdFspsUpdDataAddress64) != 0) {
    return (UINTN)PcdGet64 (PcdFspsUpdDataAddress64);
  } else {
    return (UINTN)PcdGet32 (PcdFspsUpdDataAddress);
  }
}

/**
  This function is for FSP dispatch mode to perform post FSP-S process.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Status returned by PeiServicesInstallPpi ()
**/
EFI_STATUS
EFIAPI
FspsWrapperEndOfPeiNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS  Status;

  //
  // This step may include platform specific process in some boot loaders so
  // aligning the same behavior between API and Dispatch modes.
  // Note: In Dispatch mode no FspHobList so passing NULL to function and
  //       expecting function will handle it.
  //
  PostFspsHobProcess (NULL);

  //
  // Install FspSiliconInitDonePpi so that any other driver can consume this info.
  //
  Status = PeiServicesInstallPpi (&mPeiFspSiliconInitDonePpi);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mFspsWrapperEndOfPeiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  FspsWrapperEndOfPeiNotify
};

/**
  This function is called after PEI core discover memory and finish migration.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiMemoryDiscoveredNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mPeiMemoryDiscoveredNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  PeiMemoryDiscoveredNotify
};

extern
RETURN_STATUS
EFIAPI
LzmaGuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
;

extern
RETURN_STATUS
EFIAPI
LzmaGuidedSectionExtraction (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  OUT       VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  )
;

/**
  This function is called after PEI core discover memory and finish migration.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiMemoryDiscoveredNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  FSP_INFO_HEADER      *FspsHeaderPtr;
  UINT64               TimeStampCounterStart;
  EFI_STATUS           Status;
  VOID                 *FspHobListPtr;
  EFI_HOB_GUID_TYPE    *GuidHob;
  FSPS_UPD_COMMON      *FspsUpdDataPtr;
  UINTN                *SourceData;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;
  EFI_FV_FILE_INFO     FileInfo;
  UINT32               FvIndex = 0;
  UINT32               DecompressedSmmFvSize, TemporaryBufferSize;
  VOID                 *DecompressedFv, *TemporaryBuffer;
  EFI_BOOT_MODE        BootMode;

  //
  // Get boot mode
  //
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "PeiMemoryDiscoveredNotify enter\n"));
  FspsUpdDataPtr = NULL;

  VOID  *FspsBaseAddressInMem = (VOID *)(UINTN)PcdGet32 (PcdFspsBaseAddressInMemory);

  FspsHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader ((EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdFspsBaseAddressInMemory));
  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)(UINTN)FspsBaseAddressInMem,
    (UINT64)PcdGet32 (PcdFspsRegionSize),
    EfiACPIMemoryNVS
    );
  FspsHeaderPtr->ImageBase = (UINTN)FspsBaseAddressInMem;

  if (BootMode != BOOT_ON_S3_RESUME) {
    // Get SMM Driver Volume from flash, and extract/deflate it.
    while (PeiServicesFfsFindNextVolume (FvIndex, &VolumeHandle) != EFI_NOT_FOUND) {
      if (PeiServicesFfsFindFileByName (&SmmDriverVolumeFileName, VolumeHandle, &FileHandle) == EFI_SUCCESS) {
        break;
      }

      FvIndex++;
    }

    ASSERT (FileHandle != NULL);
    PeiServicesFfsGetFileInfo (FileHandle, &FileInfo);
    DEBUG ((DEBUG_INFO, "SMM Driver File:%p,0x%x bytes.\n", FileInfo.Buffer, FileInfo.BufferSize));
    UINT16  Attribute;
    Status = LzmaGuidedSectionGetInfo (
               FileInfo.Buffer,
               &DecompressedSmmFvSize,
               &TemporaryBufferSize,
               &Attribute
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Cannot Get LZMA Section info!\n"));
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

    DEBUG ((DEBUG_INFO, "FV Decompress size:%d\n", DecompressedSmmFvSize));
    EFI_PHYSICAL_ADDRESS  PhysicalAddress;
    Status =  PeiServicesAllocatePages (
                EfiBootServicesData,
                EFI_SIZE_TO_PAGES (DecompressedSmmFvSize),
                &PhysicalAddress
                );
    DecompressedFv = (VOID *)(UINTN)PhysicalAddress;
    Status        |= PeiServicesAllocatePages (
                       EfiBootServicesData,
                       EFI_SIZE_TO_PAGES (TemporaryBufferSize),
                       &PhysicalAddress
                       );
    TemporaryBuffer = (VOID *)(UINTN)PhysicalAddress;
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Cannot allocate memory!%r\n", Status));
      return EFI_UNSUPPORTED;
    }

    UINT32  AuthenticationStatus;
    Status = LzmaGuidedSectionExtraction (FileInfo.Buffer, &DecompressedFv, TemporaryBuffer, &AuthenticationStatus);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Cannot Decompress LZMA Section!:%r\n", Status));
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

    PeiServicesFreePages (PhysicalAddress, EFI_SIZE_TO_PAGES (TemporaryBufferSize));

    FSP_EXPORTED_INTERFACE_HOB  *ExportedInterfaceHob;
    GuidHob = GetFirstGuidHob (&gFspHobGuid);
    if (GuidHob != NULL) {
      GuidHob              = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
      GuidHob              = GetNextGuidHob (&gFspExportedInterfaceHobGuid, GuidHob);
      ExportedInterfaceHob = GET_GUID_HOB_DATA (GuidHob);
      DEBUG ((DEBUG_INFO, "FSP Exported interface HOB:%p\n", ExportedInterfaceHob));
      DEBUG ((DEBUG_INFO, "FSP DecompressedFv:%p\n", (UINT8 *)DecompressedFv+0x10)); // Skip RAW section.
      ExportedInterfaceHob->SmmDriverVolume     = (UINT8 *)DecompressedFv+0x10;      // Skip RAW section.
      ExportedInterfaceHob->SmmDriverVolumeSize = DecompressedSmmFvSize;
    }
  }

  DEBUG ((DEBUG_INFO, "FspsHeaderPtr - 0x%x\n", FspsHeaderPtr));
  if (FspsHeaderPtr == NULL) {
    return EFI_UNSUPPORTED;
  }

  if ((GetFspsUpdDataAddress () == 0) && (FspsHeaderPtr->CfgRegionSize != 0) && (FspsHeaderPtr->CfgRegionOffset != 0)) {
    //
    // Copy default FSP-S UPD data from Flash
    //
    FspsUpdDataPtr = (FSPS_UPD_COMMON *)AllocateZeroPool ((UINTN)FspsHeaderPtr->CfgRegionSize);
    ASSERT (FspsUpdDataPtr != NULL);
    SourceData = (UINTN *)((UINTN)FspsHeaderPtr->ImageBase + (UINTN)FspsHeaderPtr->CfgRegionOffset);
    CopyMem (FspsUpdDataPtr, SourceData, (UINTN)FspsHeaderPtr->CfgRegionSize);
  } else {
    FspsUpdDataPtr = (FSPS_UPD_COMMON *)GetFspsUpdDataAddress ();
    ASSERT (FspsUpdDataPtr != NULL);
  }

  UpdateFspsUpdData ((VOID *)FspsUpdDataPtr);

  TimeStampCounterStart = AsmReadTsc ();
  PERF_START_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  Status = CallFspSiliconInit ((VOID *)FspsUpdDataPtr);

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FspSiliconInitApi requested reset %r\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  if ((Status != FSP_STATUS_VARIABLE_REQUEST) && EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - Failed to execute FspSiliconInitApi(), Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "FspSiliconInit status: %r\n", Status));

  if (Status == FSP_STATUS_VARIABLE_REQUEST) {
    //
    // call to Variable request handler
    //
    FspWrapperVariableRequestHandler (&FspHobListPtr, FspMultiPhaseSiInitApiIndex);
  }

  //
  // See if MultiPhase process is required or not
  //
  FspWrapperMultiPhaseHandler (&FspHobListPtr, FspMultiPhaseSiInitApiIndex);    // FspS MultiPhase

  PERF_END_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  DEBUG ((DEBUG_INFO, "Total time spent executing FspSiliconInitApi: %d millisecond\n", DivU64x32 (GetTimeInNanoSecond (AsmReadTsc () - TimeStampCounterStart), 1000000)));

  Status = TestFspSiliconInitApiOutput ((VOID *)NULL);
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR - TestFspSiliconInitApiOutput () fail, Status = %r\n", Status));
  }

  //
  // Now FspHobList complete, process it
  //
  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  ASSERT (GuidHob != NULL);
  FspHobListPtr = *(VOID **)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "FspHobListPtr - 0x%x\n", FspHobListPtr));
  PostFspsHobProcess (FspHobListPtr);

  //
  // Install FspSiliconInitDonePpi so that any other driver can consume this info.
  //
  Status = PeiServicesInstallPpi (&mPeiFspSiliconInitDonePpi);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Do FSP initialization in API mode.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
FspsWrapperInitApiMode (
  VOID
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;

  //
  // Register MemoryDiscovered Notify to run FspSiliconInit
  //
  Status = PeiServicesNotifyPpi (&mPeiMemoryDiscoveredNotifyDesc);
  ASSERT_EFI_ERROR (Status);

  //
  // Register EndOfPei Notify for S3 to run FSP NotifyPhase
  //
  PeiServicesGetBootMode (&BootMode);
  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesNotifyPpi (&mS3EndOfPeiNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Do FSP initialization in Dispatch mode.

  @retval FSP initialization status.
**/
EFI_STATUS
FspsWrapperInitDispatchMode (
  VOID
  )
{
  EFI_STATUS                                             Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI  *MeasurementExcludedFvPpi;
  EFI_PEI_PPI_DESCRIPTOR                                 *MeasurementExcludedPpiList;
  EFI_BOOT_MODE                                          BootMode;

  PeiServicesGetBootMode (&BootMode);
  Status = EFI_SUCCESS;

  if (BootMode != BOOT_ON_S3_RESUME) {
    MeasurementExcludedFvPpi = AllocatePool (sizeof (*MeasurementExcludedFvPpi));
    if (MeasurementExcludedFvPpi == NULL) {
      ASSERT (MeasurementExcludedFvPpi != NULL);
      return EFI_OUT_OF_RESOURCES;
    }
    MeasurementExcludedFvPpi->Count          = 1;
    MeasurementExcludedFvPpi->Fv[0].FvBase   = PcdGet32 (PcdFspsBaseAddressInMemory);
    MeasurementExcludedFvPpi->Fv[0].FvLength = PcdGet32 (PcdFspsRegionSize);

    MeasurementExcludedPpiList = AllocatePool (sizeof (*MeasurementExcludedPpiList));
    if (MeasurementExcludedPpiList == NULL) {
      ASSERT (MeasurementExcludedPpiList != NULL);
      FreePool (MeasurementExcludedFvPpi);
      return EFI_OUT_OF_RESOURCES;
    }
    MeasurementExcludedPpiList->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
    MeasurementExcludedPpiList->Guid  = &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;
    MeasurementExcludedPpiList->Ppi   = MeasurementExcludedFvPpi;

    Status = PeiServicesInstallPpi (MeasurementExcludedPpiList);
    ASSERT_EFI_ERROR (Status);

    EFI_FIRMWARE_VOLUME_HEADER  *FspsBaseAddressInMem = (VOID *)(UINTN)PcdGet32 (PcdFspsBaseAddressInMemory);
    //
    // FSP-S Wrapper running in Dispatch mode and reports FSP-S FV to PEI dispatcher.
    //
    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *)(UINTN)FspsBaseAddressInMem,
      (UINT32)PcdGet32 (PcdFspsRegionSize),
      NULL,
      NULL
      );

    VOID  *FspoDxeBaseAddressInMem = (VOID *)(UINTN)PcdGet32 (PcdFspoDxeBaseAddressInMemory);
    PeiServicesInstallFvInfoPpi (
      NULL,
      FspoDxeBaseAddressInMem,
      (UINT32)PcdGet32 (PcdFspoDxeRegionSize),
      NULL,
      NULL
      );
    BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)FspsBaseAddressInMem, PcdGet32 (PcdFspsRegionSize));
    BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)FspoDxeBaseAddressInMem, PcdGet32 (PcdFspoDxeRegionSize));

    //
    // Register EndOfPei Nofity to run post FSP-S process.
    //
    Status = PeiServicesNotifyPpi (&mFspsWrapperEndOfPeiNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  This is the entrypoint of PEIM.

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FspsWrapperPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  DEBUG ((DEBUG_INFO, "FspsWrapperPeimEntryPoint\n"));

 #ifndef COMPRESS_FSP_REGION
  CopyMem ((VOID *)(UINTN)PcdGet32 (PcdFspoDxeBaseAddressInMemory), (VOID *)(UINTN)PcdGet32 (PcdFspoDxeBaseAddressInFlash), PcdGet32 (PcdFspoDxeRegionSize));
  CopyMem ((VOID *)(UINTN)PcdGet32 (PcdFspsBaseAddressInMemory), (VOID *)(UINTN)PcdGet32 (PcdFspsBaseAddressInFlash), PcdGet32 (PcdFspsRegionSize));
 #endif
  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdFspsBaseAddressInMemory),
    (UINT64)PcdGet32 (PcdFspsRegionSize),
    EfiACPIMemoryNVS
    );

  BuildMemoryAllocationHob (
    (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdFspoDxeBaseAddressInMemory),
    (UINT64)PcdGet32 (PcdFspoDxeRegionSize),
    EfiACPIMemoryNVS
    );

  if (PcdGet8 (PcdFspModeSelection) == 1) {
    FspsWrapperInitApiMode ();
  } else {
    FspsWrapperInitDispatchMode ();
  }

  return EFI_SUCCESS;
}
