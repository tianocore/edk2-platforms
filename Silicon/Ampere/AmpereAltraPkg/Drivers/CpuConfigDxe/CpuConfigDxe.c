/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/CpuConfigHii.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformManagerHii.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NVParamLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <NVParamDef.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>

#include "CpuConfigDxe.h"

//
// Default settings definitions
//
#define NV_SI_SUBNUMA_MODE_DEFAULT   CPU_SUBNUMA_MODE_MONO
#define WA_ERRATUM_1542419_DEFAULT   0x00 /* Disable I-Cache coherency */
#define NEAR_ATOMIC_DISABLE_DEFAULT  0x00 /* Enable Near Atomic */
#define CPU_SLC_REPLACE_POLICY       0x00 /* eLRU */

EFI_HANDLE               mDriverHandle = NULL;
CPU_CONFIG_PRIVATE_DATA  *mPrivateData = NULL;

HII_VENDOR_DEVICE_PATH  mCpuConfigHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    CPU_CONFIGURATION_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

STATIC
EFI_STATUS
CpuNvParamGet (
  OUT CPU_VARSTORE_DATA  *Configuration
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  ASSERT (Configuration != NULL);

  Status = NVParamGet (
             NV_SI_SUBNUMA_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d Fail to get NVParam, %r\n", __func__, __LINE__, Status));
    Configuration->CpuSubNumaMode = SUBNUMA_MODE_MONOLITHIC;
  } else {
    Configuration->CpuSubNumaMode = Value;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CpuNvParamSet (
  IN CPU_VARSTORE_DATA  *Configuration
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  ASSERT (Configuration != NULL);

  Status = NVParamGet (
             NV_SI_SUBNUMA_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status) || (Value != Configuration->CpuSubNumaMode)) {
    Status = NVParamSet (
               NV_SI_SUBNUMA_MODE,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Configuration->CpuSubNumaMode
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a %d Fail to set NVParam, %r\n", __func__, __LINE__, Status));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetupDefaultSettings (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  //
  // Subnuma Mode
  //
  Status = NVParamGet (
             NV_SI_SUBNUMA_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Status = NVParamSet (
               NV_SI_SUBNUMA_MODE,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               NV_SI_SUBNUMA_MODE_DEFAULT
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // ARM ERRATA 1542419 workaround
  //
  Status = NVParamSet (
             NV_SI_ERRATUM_1542419_WA,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             WA_ERRATUM_1542419_DEFAULT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Near atomic
  //
  Status = NVParamSet (
             NV_SI_NEAR_ATOMIC_DISABLE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             NEAR_ATOMIC_DISABLE_DEFAULT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // SLC Replacement Policy
  //
  Status = NVParamSet (
             NV_SI_HNF_AUX_CTL_32_63,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             CPU_SLC_REPLACE_POLICY
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
CpuConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  CPU_CONFIG_PRIVATE_DATA          *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  UINTN                            Size;
  BOOLEAN                          AllocatedRequest;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the local variables.
  //
  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  Size             = 0;
  *Progress        = Request;
  AllocatedRequest = FALSE;

  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gCpuConfigFormSetGuid, CPU_CONFIG_VARIABLE_NAME)) {
    return EFI_NOT_FOUND;
  }

  PrivateData      = CPU_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  BufferSize = sizeof (CPU_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      CPU_CONFIG_VARIABLE_NAME,
                      &gCpuConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      &PrivateData->Configuration
                      );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get current setting from NVParam.
  //
  Status = CpuNvParamGet (&PrivateData->Configuration);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize    = sizeof (CPU_VARSTORE_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gCpuConfigFormSetGuid, CPU_CONFIG_VARIABLE_NAME, PrivateData->DriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    if (ConfigRequest == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               (UINT8 *)&PrivateData->Configuration,
                               BufferSize,
                               Results,
                               Progress
                               );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
CpuConfigRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT      EFI_STRING                      *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  CPU_CONFIG_PRIVATE_DATA          *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = CPU_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress        = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gCpuConfigFormSetGuid, CPU_CONFIG_VARIABLE_NAME)) {
    return EFI_NOT_FOUND;
  }

  BufferSize = sizeof (CPU_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      CPU_CONFIG_VARIABLE_NAME,
                      &gCpuConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      &PrivateData->Configuration
                      );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get configuration data from NVParam
  //
  Status = CpuNvParamGet (&PrivateData->Configuration);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (CPU_VARSTORE_DATA);
  Status     = HiiConfigRouting->ConfigToBlock (
                                   HiiConfigRouting,
                                   Configuration,
                                   (UINT8 *)&PrivateData->Configuration,
                                   &BufferSize,
                                   Progress
                                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
                  CPU_CONFIG_VARIABLE_NAME,
                  &gCpuConfigFormSetGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (CPU_VARSTORE_DATA),
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Store configuration data back to NVParam
  //
  Status = CpuNvParamSet (&PrivateData->Configuration);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
CpuConfigCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN       EFI_BROWSER_ACTION              Action,
  IN       EFI_QUESTION_ID                 QuestionId,
  IN       UINT8                           Type,
  IN       EFI_IFR_TYPE_VALUE              *Value,
  OUT      EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  CPU_VARSTORE_DATA  *Configuration;

  if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && (Action != EFI_BROWSER_ACTION_FORM_CLOSE)) ||
      (ActionRequest == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    Configuration = AllocateZeroPool (sizeof (CPU_VARSTORE_DATA));
    ASSERT (Configuration != NULL);
    if (!HiiGetBrowserData (&gCpuConfigFormSetGuid, CPU_CONFIG_VARIABLE_NAME, sizeof (CPU_VARSTORE_DATA), (UINT8 *)Configuration)) {
      FreePool (Configuration);
      return EFI_NOT_FOUND;
    }

    //
    // Disable SLC as L3$ option if the CPU configuration is 1P monolithic mode
    //
    if (!IsSlaveSocketActive () && (Configuration->CpuSubNumaMode == CPU_SUBNUMA_MODE_MONO)) {
      Configuration->CpuSlcAsL3Permitted = CPU_SLC_AS_L3_PERMITTED_YES;
    } else {
      Configuration->CpuSlcAsL3Permitted = CPU_SLC_AS_L3_PERMITTED_NO;
      Configuration->CpuSlcAsL3          = CPU_SLC_AS_L3_DISABLE;
    }

    //
    // Pass changed uncommitted data back to Form Browser
    //
    HiiSetBrowserData (&gCpuConfigFormSetGuid, CPU_CONFIG_VARIABLE_NAME, sizeof (CPU_VARSTORE_DATA), (UINT8 *)Configuration, NULL);
    FreePool (Configuration);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
CpuConfigUnload (
  VOID
  )
{
  ASSERT (mPrivateData != NULL);

  if (mDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mCpuConfigHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &mPrivateData->ConfigAccess,
           NULL
           );
    mDriverHandle = NULL;
  }

  if (mPrivateData->HiiHandle != NULL) {
    HiiRemovePackages (mPrivateData->HiiHandle);
  }

  FreePool (mPrivateData);
  mPrivateData = NULL;

  return EFI_SUCCESS;
}

EFI_STATUS
CpuConfigDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  BOOLEAN                          ActionFlag;
  CPU_VARSTORE_DATA                *Configuration;
  EFI_STRING                       ConfigRequestHdr;
  UINTN                            BufferSize;

  //
  // With the fresh system, the NVParam value is invalid (0xFFFFFFFF).
  // It causes reading from the NVParam is failed.
  // So, the NVParam should be setting with default values if any params is invalid.
  //
  Status = SetupDefaultSettings ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (CPU_CONFIG_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = CPU_CONFIG_PRIVATE_SIGNATURE;

  mPrivateData->ConfigAccess.ExtractConfig = CpuConfigExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig   = CpuConfigRouteConfig;
  mPrivateData->ConfigAccess.Callback      = CpuConfigCallback;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPrivateData->HiiConfigRouting = HiiConfigRouting;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mCpuConfigHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mPrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  mPrivateData->DriverHandle = mDriverHandle;

  //
  // Publish our HII data
  //
  HiiHandle = HiiAddPackages (
                &gCpuConfigFormSetGuid,
                mDriverHandle,
                CpuConfigDxeStrings,
                CpuConfigVfrBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mCpuConfigHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &mPrivateData->ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle = HiiHandle;

  //
  // Initialize configuration data
  //
  Configuration = &mPrivateData->Configuration;
  ZeroMem (Configuration, sizeof (CPU_VARSTORE_DATA));

  //
  // Try to read NV config EFI variable first
  //
  ConfigRequestHdr = HiiConstructConfigHdr (&gCpuConfigFormSetGuid, CPU_CONFIG_VARIABLE_NAME, mDriverHandle);
  ASSERT (ConfigRequestHdr != NULL);

  BufferSize = sizeof (CPU_VARSTORE_DATA);
  Status     = gRT->GetVariable (CPU_CONFIG_VARIABLE_NAME, &gCpuConfigFormSetGuid, NULL, &BufferSize, Configuration);
  if (EFI_ERROR (Status)) {
    //
    // Store zero data Buffer Storage to EFI variable
    //
    Status = gRT->SetVariable (
                    CPU_CONFIG_VARIABLE_NAME,
                    &gCpuConfigFormSetGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (CPU_VARSTORE_DATA),
                    Configuration
                    );
    if (EFI_ERROR (Status)) {
      CpuConfigUnload ();
      return Status;
    }

    //
    // EFI variable for NV config doesn't exit, we should build this variable
    // based on default values stored in IFR
    //
    ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    if (!ActionFlag) {
      CpuConfigUnload ();
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // EFI variable does exist and Validate Current Setting
    //
    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    if (!ActionFlag) {
      CpuConfigUnload ();
      return EFI_INVALID_PARAMETER;
    }
  }

  FreePool (ConfigRequestHdr);

  //
  // Limit SLC as L3$ to only 1P monolithic mode
  //
  if (!IsSlaveSocketActive () && (Configuration->CpuSubNumaMode == CPU_SUBNUMA_MODE_MONO)) {
    Configuration->CpuSlcAsL3Permitted = CPU_SLC_AS_L3_PERMITTED_YES;
  } else {
    Configuration->CpuSlcAsL3Permitted = CPU_SLC_AS_L3_PERMITTED_NO;
    Configuration->CpuSlcAsL3          = CPU_SLC_AS_L3_DISABLE;
  }

  Status = gRT->SetVariable (
                  CPU_CONFIG_VARIABLE_NAME,
                  &gCpuConfigFormSetGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (CPU_VARSTORE_DATA),
                  Configuration
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
