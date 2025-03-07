/** @file
  Support FSP Wrapper MultiPhase process.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <FspEas.h>
#include <FspGlobalData.h>
#include <Ppi/Variable.h>
#include "../../Include/Library/FspWrapperPlatformMultiPhaseLib.h"
#include <FspsUpd.h>
#include <Protocol/SmbusHc.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmControl2.h>
#include <Protocol/Reset.h>
#include <MultiPhaseSiPhases.h>
#include <Library/UefiBootServicesTableLib.h>
#include <FspExportedInterfaceHob.h>
#include <Protocol/Smbios.h>
#include <Pi/PiHob.h>

extern EFI_GUID  gFspsUpdDataPointerAddressGuid;
extern EFI_GUID  gEfiSmmBase2ProtocolGuid;
extern EFI_GUID  gEfiSmmCommunicationProtocolGuid;
extern EFI_GUID  gEfiMmCommunication2ProtocolGuid;
extern EFI_GUID  gFchInitDonePolicyProtocolGuid;
extern EFI_GUID  gEfiVariableArchProtocolGuid;
extern EFI_GUID  gEfiSmmVariableProtocolGuid;
extern EFI_GUID  gSmmVariableWriteGuid;
extern EFI_GUID  gEfiHiiDatabaseProtocolGuid;
extern EFI_GUID  gEfiHiiStringProtocolGuid;
extern EFI_GUID  gEfiHiiConfigRoutingProtocolGuid;
extern EFI_GUID  gPspFlashAccSmmCommReadyProtocolGuid;
extern EFI_GUID  gFspSmmDependencyReadyProtocolGuid;
extern EFI_GUID  gFspHobGuid;
extern EFI_GUID  gFspExportedInterfaceHobGuid;

STATIC FSPS_UPD *volatile  FspsUpd;
static VOID                **mFspHobListPtr;

// The EDK 202208 Doesn't hold these structs.
typedef enum {
  EnumMultiPhaseGetVariableRequestInfo  = 0x2,
  EnumMultiPhaseCompleteVariableRequest = 0x3
} FSP_MULTI_PHASE_ACTION_23;

typedef enum {
  FspMultiPhaseMemInitApiIndex = 8
} FSP_API_INDEX_23;
///
/// Action definition for FspMultiPhaseSiInit API
///
typedef enum {
  EnumFspVariableRequestGetVariable         = 0x0,
  EnumFspVariableRequestGetNextVariableName = 0x1,
  EnumFspVariableRequestSetVariable         = 0x2,
  EnumFspVariableRequestQueryVariableInfo   = 0x3
} FSP_VARIABLE_REQUEST_TYPE;

#pragma pack(16)
typedef struct {
  IN     FSP_VARIABLE_REQUEST_TYPE    VariableRequest;
  IN OUT CHAR16                       *VariableName;
  IN OUT UINT64                       *VariableNameSize;
  IN OUT EFI_GUID                     *VariableGuid;
  IN OUT UINT32                       *Attributes;
  IN OUT UINT64                       *DataSize;
  IN OUT VOID                         *Data;
  OUT    UINT64                       *MaximumVariableStorageSize;
  OUT    UINT64                       *RemainingVariableStorageSize;
  OUT    UINT64                       *MaximumVariableSize;
} FSP_MULTI_PHASE_VARIABLE_REQUEST_INFO_PARAMS;

typedef struct {
  EFI_STATUS    VariableRequestStatus;
} FSP_MULTI_PHASE_COMPLETE_VARIABLE_REQUEST_PARAMS;

#pragma pack()

FSP_EXPORTED_INTERFACE_HOB  *ExportedInterfaceHob;

EFI_STATUS
EFIAPI
CallFspMultiPhaseEntry (
  IN VOID      *FspMultiPhaseParams,
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  );

/**
  Execute 32-bit FSP API entry code.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code.
  @param[in] Param2       The second parameter to pass to 32bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute32BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  );

/**
  Execute 64-bit FSP API entry code.

  @param[in] Function     The 64bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 64bit code.
  @param[in] Param2       The second parameter to pass to 64bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute64BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  );

/**
  Call FspsMultiPhase API.

  @param[in] FspsMultiPhaseParams - Parameters for MultiPhase API.
  @param[in] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in] ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @return EFI_UNSUPPORTED  - the requested FspsMultiPhase API is not supported.
  @return EFI_DEVICE_ERROR - the FSP header was not found.
  @return EFI status returned by FspsMultiPhase API.
**/
EFI_STATUS
EFIAPI
CallFspMultiPhaseEntry (
  IN VOID      *FspMultiPhaseParams,
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  )
{
  mFspHobListPtr = FspHobListPtr;
  FSP_INFO_HEADER  *FspHeader;
  //
  // FSP_MULTI_PHASE_INIT and FSP_MULTI_PHASE_SI_INIT API functions having same prototype.
  //
  UINTN                   FspMultiPhaseApiEntry;
  UINTN                   FspMultiPhaseApiOffset = 0;
  EFI_STATUS              Status;
  BOOLEAN                 InterruptState;
  BOOLEAN                 IsVariableServiceRequest;
  FSP_MULTI_PHASE_PARAMS  *FspMultiPhaseParamsPtr;

  FspMultiPhaseParamsPtr   = (FSP_MULTI_PHASE_PARAMS *)FspMultiPhaseParams;
  IsVariableServiceRequest = FALSE;
  if ((FspMultiPhaseParamsPtr->MultiPhaseAction == (int)EnumMultiPhaseGetVariableRequestInfo) ||
      (FspMultiPhaseParamsPtr->MultiPhaseAction == (int)EnumMultiPhaseCompleteVariableRequest))
  {
    IsVariableServiceRequest = TRUE;
  }

  if (ComponentIndex == FspMultiPhaseMemInitApiIndex) {
    FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspmBaseAddressInMemory));
    if (FspHeader == NULL) {
      return EFI_DEVICE_ERROR;
    } else if (FspHeader->SpecVersion < 0x24) {
      return EFI_UNSUPPORTED;
    }

    FspMultiPhaseApiOffset = FspHeader->FspMultiPhaseMemInitEntryOffset;
  } else if (ComponentIndex == FspMultiPhaseSiInitApiIndex) {
    FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspsBaseAddressInMemory));
    if (FspHeader == NULL) {
      return EFI_DEVICE_ERROR;
    } else if (FspHeader->SpecVersion < 0x22) {
      return EFI_UNSUPPORTED;
    } else if ((FspHeader->SpecVersion < 0x24) && (IsVariableServiceRequest == TRUE)) {
      return EFI_UNSUPPORTED;
    }

    FspMultiPhaseApiOffset = FspHeader->FspMultiPhaseSiInitEntryOffset;
  }

  if (FspMultiPhaseApiOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  FspMultiPhaseApiEntry = FspHeader->ImageBase + FspMultiPhaseApiOffset;
  InterruptState        = SaveAndDisableInterrupts ();
  if ((FspHeader->ImageAttribute & BIT2) == 0) {
    // BIT2: IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT
    Status = Execute32BitCode ((UINTN)FspMultiPhaseApiEntry, (UINTN)FspMultiPhaseParams, (UINTN)NULL);
  } else {
    Status = Execute64BitCode ((UINTN)FspMultiPhaseApiEntry, (UINTN)FspMultiPhaseParams, (UINTN)NULL);
  }

  SetInterruptState (InterruptState);

  DEBUG ((DEBUG_ERROR, "CallFspMultiPhaseEntry return Status %r \n", Status));

  return Status;
}

VOID
EFIAPI
OnRuntimeServiceReady (
  EFI_EVENT  Event,
  VOID       *Extra
  )
{
  gBS->CloseEvent (Event);
  DEBUG ((DEBUG_ERROR, "Runtime Service ready.\n"));
  FSP_MULTI_PHASE_PARAMS  FspMultiPhaseParams;

  FspMultiPhaseParams.MultiPhaseAction   = EnumMultiPhaseExecutePhase;
  FspMultiPhaseParams.PhaseIndex         = EnumMultiPhaseAmdRuntimeServicesReadyPhase;
  FspMultiPhaseParams.MultiPhaseParamPtr = NULL;
 #if 1
  ExportedInterfaceHob->GetVariable         = gST->RuntimeServices->GetVariable;
  ExportedInterfaceHob->GetNextVariableName = gST->RuntimeServices->GetNextVariableName;
  ExportedInterfaceHob->SetVariable         = gST->RuntimeServices->SetVariable;
  ExportedInterfaceHob->QueryVariableInfo   = gST->RuntimeServices->QueryVariableInfo;
  ASSERT (gST->RuntimeServices->GetVariable && gST->RuntimeServices->SetVariable);
  VOID        *HiiProtocol;
  EFI_STATUS  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, &HiiProtocol);
  ASSERT (Status == EFI_SUCCESS);
  ExportedInterfaceHob->HiiProtocol = HiiProtocol;
  Status                            = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, &HiiProtocol);
  ASSERT (Status == EFI_SUCCESS);
  ExportedInterfaceHob->HiiStringProtocol = HiiProtocol;
  Status                                  = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, &HiiProtocol);
  ASSERT (Status == EFI_SUCCESS);
  ExportedInterfaceHob->HiiConfigRoutingProtocol = HiiProtocol;
 #endif
  CallFspMultiPhaseEntry (&FspMultiPhaseParams, NULL, FspMultiPhaseSiInitApiIndex);
}

/**
  FSP Wrapper MultiPhase Handler

  @param[in, out] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in]      ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @retval EFI_UNSUPPORTED   Specific MultiPhase action was not supported.
  @retval EFI_SUCCESS       MultiPhase action were completed successfully.

**/
EFI_STATUS
EFIAPI
FspWrapperMultiPhaseHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  )
{
  EFI_STATUS              Status;
  FSP_MULTI_PHASE_PARAMS  FspMultiPhaseParams;
  UINT32                  Index;
  EFI_HANDLE              Handle      = NULL;
  VOID                    *FspsUpdHob = GetFirstGuidHob (&gFspsUpdDataPointerAddressGuid);

  if ( FspsUpdHob != NULL ) {
    FspsUpd = ((FSPS_UPD *)(UINTN)(*(UINT32 *)GET_GUID_HOB_DATA (FspsUpdHob)));
  }

  FspsUpd->FspsConfig.nv_storage_variable_base    = PcdGet32 (PcdFlashNvStorageVariableBase);
  FspsUpd->FspsConfig.nv_storage_variable_size    = PcdGet32 (PcdFlashNvStorageVariableSize);
  FspsUpd->FspsConfig.nv_storage_ftw_working_base = PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  FspsUpd->FspsConfig.nv_storage_ftw_working_size = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  FspsUpd->FspsConfig.nv_storage_ftw_spare_base   = PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  FspsUpd->FspsConfig.nv_storage_ftw_spare_size   = PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  for (Index = 1; Index <= EnumMultiPhaseAmdCpmDxeTableReadyPhase; Index++) {
    //
    // Platform actions can be added in below function for each component and phase before returning control back to FSP.
    //
    FspWrapperPlatformMultiPhaseHandler (FspHobListPtr, ComponentIndex, Index);

    FspMultiPhaseParams.MultiPhaseAction   = EnumMultiPhaseExecutePhase;
    FspMultiPhaseParams.PhaseIndex         = Index;
    FspMultiPhaseParams.MultiPhaseParamPtr = NULL;
    Status                                 = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);

    //
    // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
    //
    if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
      DEBUG ((DEBUG_INFO, "FspMultiPhaseApi-0x%x requested reset %r\n", ComponentIndex, Status));
      CallFspWrapperResetSystem ((UINTN)Status);
    }

    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_ERROR, " FSP Multi Phase Silicon Phase #2 init done. Installing Protocol.\n"));
  DEBUG ((DEBUG_ERROR, " *FspHobListPtr:%011p\n", *FspHobListPtr));
  VOID  *ExportedInterfaceRawHob = GetNextGuidHob (&gFspExportedInterfaceHobGuid, *FspHobListPtr);

  DEBUG ((DEBUG_ERROR, " ExportedInterfaceRawHob:%011p\n", ExportedInterfaceRawHob));
  if ( ExportedInterfaceRawHob != NULL) {
    ExportedInterfaceHob = GET_GUID_HOB_DATA (ExportedInterfaceRawHob);
  } else {
    DEBUG ((DEBUG_ERROR, " Cannot found Exported Interface HOB!\n"));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_ERROR, "ExportedInterfaceHob:%011p\n", ExportedInterfaceHob));
  if ( FspsUpd != NULL ) {
    DEBUG ((DEBUG_ERROR, "FSP-S UPD Ptr:%011p\n", FspsUpd));
    // SMBUS Protocol
    if (ExportedInterfaceHob->SmbusProtocol != 0) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiSmbusHcProtocolGuid,
                      ExportedInterfaceHob->SmbusProtocol,
                      NULL
                      );
      Handle  = NULL;
      Status |= gBS->InstallProtocolInterface (
                       &Handle,
                       &gFchInitDonePolicyProtocolGuid,
                       EFI_NATIVE_INTERFACE,
                       NULL
                       );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install SMBUS Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "SMBUS operation address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    // SMRAM Access 2 Protocol
    if (ExportedInterfaceHob->SmmAccessProtocol != 0) {
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiSmmAccess2ProtocolGuid,
                      ExportedInterfaceHob->SmmAccessProtocol,
                      NULL
                      );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install SMRAM Access Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "SMRAM access address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    // SMRAM Control 2 Protocol
    if (ExportedInterfaceHob->SmmControl2Protocol != 0) {
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiSmmControl2ProtocolGuid,
                      ExportedInterfaceHob->SmmControl2Protocol,
                      NULL
                      );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install SMRAM Control Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "SMRAM control address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    // SMM Related Protocol
    if (ExportedInterfaceHob->SmmBase2Protocol != 0) {
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiSmmBase2ProtocolGuid,
                      ExportedInterfaceHob->SmmBase2Protocol,
                      NULL
                      );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install SMM Base 2 Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "SMM Base 2 Protocol address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    if (ExportedInterfaceHob->SmmCommunicationProtocol != 0) {
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiSmmCommunicationProtocolGuid,
                      ExportedInterfaceHob->SmmCommunicationProtocol,
                      NULL
                      );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install SMM Communication Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "SMM Communication Protocol address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    if (ExportedInterfaceHob->MmCommunication2Protocol != 0) {
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiMmCommunication2ProtocolGuid,
                      ExportedInterfaceHob->MmCommunication2Protocol,
                      NULL
                      );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install MM Communication 2 Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "MM Communication 2 Protocol address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    if (ExportedInterfaceHob->PspFtpmProtocol != 0) {
      Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gAmdPspFtpmProtocolGuid,
                      ExportedInterfaceHob->PspFtpmProtocol,
                      NULL
                      );
      if ( EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install PSP fTPM Protocol!\n"));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "PSP fTPM Protocol address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    if (ExportedInterfaceHob->FchResetSystem != 0) {
      gST->RuntimeServices->ResetSystem = ExportedInterfaceHob->FchResetSystem;
      Handle                            = NULL;
      gBS->InstallProtocolInterface (
             &Handle,
             &gEfiResetArchProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );
    } else {
      DEBUG ((DEBUG_ERROR, "Runtime Reset address is 0!\n"));
      return EFI_UNSUPPORTED;
    }

    // Install SMBIOS Protocol.
    EFI_SMBIOS_PROTOCOL  *SmbiosProtocol;
    VOID                 **SmbiosTableAddress = ExportedInterfaceHob->SmbiosPointers;
    Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&SmbiosProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SMBIOS Protocol not found!\n"));
      return EFI_NOT_FOUND;
    }

    for (UINT32 Count = 0; Count < MAX_SMBIOS_TABLE_COUNT; Count++) {
      if (SmbiosTableAddress[Count]) {
        EFI_SMBIOS_TABLE_HEADER  *Header = (VOID *)((UINTN)SmbiosTableAddress[Count]);
        Header->Handle = SMBIOS_HANDLE_PI_RESERVED; // Re-allocate one.
        Status         = SmbiosProtocol->Add (SmbiosProtocol, NULL, &Header->Handle, Header);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Failed to add SMBIOS Entry #%d @0x%x:%r!\n", Count, SmbiosTableAddress[Count], Status));
          break;
        }

        DEBUG ((DEBUG_INFO, "Added SMBIOS Entry #%d @0x%x\n", Count, SmbiosTableAddress[Count]));
      }
    }

    // Set PcdAmdSmmCommunicationAddress.
    PcdSet64S (PcdAmdSmmCommunicationAddress, ExportedInterfaceHob->PcdAmdSmmCommunicationAddress);
    PcdSet64S (PcdAmdS3LibPrivateDataAddress, ExportedInterfaceHob->PcdAmdS3LibPrivateDataAddress);
    PcdSet64S (PcdAmdS3LibTableAddress, ExportedInterfaceHob->PcdAmdS3LibTableAddress);
    PcdSet64S (PcdAmdS3LibTableSize, ExportedInterfaceHob->PcdAmdS3LibTableSize);
    PcdSet64S (PcdS3BootScriptTablePrivateDataPtr, ExportedInterfaceHob->S3BootScriptTablePrivateDataPtr);
    PcdSet64S (PcdS3BootScriptTablePrivateSmmDataPtr, ExportedInterfaceHob->S3BootScriptTablePrivateSmmDataPtr);
    DEBUG ((
      DEBUG_INFO,
      "PcdS3BootScriptTablePrivateDataPtr:%011p,PcdS3BootScriptTablePrivateSmmDataPtr:%011p\n",
      ExportedInterfaceHob->S3BootScriptTablePrivateDataPtr,
      ExportedInterfaceHob->S3BootScriptTablePrivateSmmDataPtr
      ));
    DEBUG ((DEBUG_INFO, "Offset:%p,%p\n", &ExportedInterfaceHob->S3BootScriptTablePrivateDataPtr, &ExportedInterfaceHob->S3BootScriptTablePrivateSmmDataPtr));
    ASSERT (
      EfiNamedEventListen (
        &gFspSmmDependencyReadyProtocolGuid,
        TPL_CALLBACK,
        OnRuntimeServiceReady,
        NULL,
        NULL
        ) == EFI_SUCCESS
      );
    gBS->InstallProtocolInterface (
           &Handle,
           &gPspFlashAccSmmCommReadyProtocolGuid,
           EFI_NATIVE_INTERFACE,
           NULL
           );
    // Install Smm Variable Write protocol.
    Handle = NULL;
    gBS->InstallMultipleProtocolInterfaces (
           &Handle,
           &gEfiSmmVariableProtocolGuid,
           NULL,
           &gSmmVariableWriteGuid,
           NULL,
           &gEfiLockBoxProtocolGuid,
           NULL,
           NULL
           );
  } else {
    DEBUG ((DEBUG_ERROR, "FspsUpdHob is NULL!\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
