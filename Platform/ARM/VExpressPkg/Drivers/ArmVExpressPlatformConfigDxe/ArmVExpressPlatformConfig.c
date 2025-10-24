/** @file
*  Versatile Express platform configuration Dxe
*
*  Copyright (c) 2025, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "ArmVExpressPlatformConfig.h"
#include <Guid/ArmVExpressPlatformConfig.h>

CHAR16  VariableName[] = L"PlatformConfig";

EFI_HANDLE                                 DriverHandle  = NULL;
ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_DATA  *mPrivateData = NULL;
EFI_EVENT                                  mEvent        = NULL;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath0 = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    ARM_VEXPRESS_PLATFORM_CONFIG_GUID
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
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_DATA  *PrivateData;
  CHAR16                                     *StrPointer;
  UINTN                                      Size;
  BOOLEAN                                    AllocatedRequest;
  EFI_STRING                                 ConfigRequest;
  UINTN                                      BufferSize;
  EFI_STATUS                                 Status;
  EFI_STRING                                 ConfigRequestHdr;
  EFI_HII_CONFIG_ROUTING_PROTOCOL            *HiiConfigRouting;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  AllocatedRequest = FALSE;
  BufferSize       = sizeof (PLATFORM_CONFIG_DATA);

  if (Request == NULL) {
    // NULL Request indicates request for entire config database. Build a
    // Request string from the ConfigHdr with appended OFFSET and WIDTH.
    ConfigRequestHdr = HiiConstructConfigHdr (&gArmVExpressPlatformConfigGuid, VariableName, PrivateData->DriverHandle);
    if (ConfigRequestHdr == NULL) {
      ASSERT (ConfigRequestHdr != NULL);
      return EFI_UNSUPPORTED;
    }

    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      ASSERT (ConfigRequest != NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  } else {
    if (!HiiIsConfigHdrMatch (Request, &gArmVExpressPlatformConfigGuid, NULL)) {
      return EFI_NOT_FOUND;
    }

    // Check if Request specifies an element. If not then add OFFSET and WIDTH
    // for the entire config database.
    StrPointer = StrStr (Request, L"PATH");
    if (StrPointer == NULL) {
      ASSERT (StrPointer != NULL);
      return EFI_INVALID_PARAMETER;
    }

    if (StrStr (StrPointer, L"&") == NULL) {
      if (Request == NULL) {
        ASSERT (Request != NULL);
        return EFI_INVALID_PARAMETER;
      }

      Size          = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      if (ConfigRequest == NULL) {
        ASSERT (ConfigRequest != NULL);
        return EFI_OUT_OF_RESOURCES;
      }

      AllocatedRequest = TRUE;
      UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", Request, (UINT64)BufferSize);
    } else {
      ConfigRequest = Request;
    }
  }

  if (StrStr (ConfigRequest, L"OFFSET") == NULL) {
    ASSERT (StrStr (ConfigRequest, L"OFFSET") != NULL);

    if (AllocatedRequest) {
      FreePool (ConfigRequest);
    }

    return EFI_INVALID_PARAMETER;
  }

  // Use helper function BlockToConfig() to extract config data and produce
  // results string.
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               (UINT8 *)&PrivateData->PlatformConfig,
                               BufferSize,
                               Results,
                               Progress
                               );

  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  if (EFI_ERROR (Status)) {
    ASSERT (!EFI_ERROR (Status));
    return Status;
  }

  // Update Progress to point to the terminator in the Request string. If
  // Request was NULL then Progress should also be NULL.
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return EFI_SUCCESS;
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
  @retval EFI_DEVICE_ERROR       If value is 44, return error for testing.

**/
EFI_STATUS
EFIAPI
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_DATA  *PrivateData;
  UINTN                                      BufferSize;
  EFI_STATUS                                 Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL            *HiiConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  if (!HiiIsConfigHdrMatch (Configuration, &gArmVExpressPlatformConfigGuid, NULL)) {
    return EFI_NOT_FOUND;
  }

  BufferSize = sizeof (PLATFORM_CONFIG_DATA);
  Status     = gRT->GetVariable (VariableName, &gArmVExpressPlatformConfigGuid, NULL, &BufferSize, &mPrivateData->PlatformConfig);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT (Status == EFI_NOT_FOUND);
    return Status;
  }

  BufferSize = sizeof (PLATFORM_CONFIG_DATA);
  Status     = HiiConfigRouting->ConfigToBlock (
                                   HiiConfigRouting,
                                   Configuration,
                                   (UINT8 *)&PrivateData->PlatformConfig,
                                   &BufferSize,
                                   Progress
                                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  &gArmVExpressPlatformConfigGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (PLATFORM_CONFIG_DATA),
                  &mPrivateData->PlatformConfig
                  );
  ASSERT (!EFI_ERROR (Status));

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

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
ArmVExpressPlatformConfigInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  UINTN                            BufferSize;
  BOOLEAN                          ActionFlag;
  EFI_STRING                       ConfigRequestHdr;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_SIGNATURE;

  mPrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig   = RouteConfig;
  mPrivateData->ConfigAccess.Callback      = DriverCallback;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    ArmVExpressPlatformConfigUnload (ImageHandle);
    return Status;
  }

  mPrivateData->HiiConfigRouting = HiiConfigRouting;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath0,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mPrivateData->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    ArmVExpressPlatformConfigUnload (ImageHandle);
    return Status;
  }

  mPrivateData->DriverHandle = DriverHandle;

  //
  // Try to read NV config EFI variable first
  //
  ConfigRequestHdr = HiiConstructConfigHdr (&gArmVExpressPlatformConfigGuid, VariableName, DriverHandle);
  if (ConfigRequestHdr == NULL) {
    ASSERT (ConfigRequestHdr != NULL);
    ArmVExpressPlatformConfigUnload (ImageHandle);
    return EFI_UNSUPPORTED;
  }

  BufferSize = sizeof (PLATFORM_CONFIG_DATA);
  Status     = gRT->GetVariable (VariableName, &gArmVExpressPlatformConfigGuid, NULL, &BufferSize, &mPrivateData->PlatformConfig);

  if (EFI_ERROR (Status)) {
    Status = gRT->SetVariable (
                    VariableName,
                    &gArmVExpressPlatformConfigGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (PLATFORM_CONFIG_DATA),
                    &mPrivateData->PlatformConfig
                    );
    if (EFI_ERROR (Status)) {
      ASSERT (!EFI_ERROR (Status));
      ArmVExpressPlatformConfigUnload (ImageHandle);
      return Status;
    }

    ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    if (!ActionFlag) {
      ArmVExpressPlatformConfigUnload (ImageHandle);
      return EFI_INVALID_PARAMETER;
    }

    BufferSize = sizeof (PLATFORM_CONFIG_DATA);
    Status     = gRT->GetVariable (VariableName, &gArmVExpressPlatformConfigGuid, NULL, &BufferSize, &mPrivateData->PlatformConfig);
    if (EFI_ERROR (Status)) {
      ArmVExpressPlatformConfigUnload (ImageHandle);
      return Status;
    }
  } else {
    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    if (!ActionFlag) {
      ArmVExpressPlatformConfigUnload (ImageHandle);
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Publish our HII data
  //
  HiiHandle = HiiAddPackages (
                &gArmVExpressPlatformConfigGuid,
                DriverHandle,
                ArmVExpressPlatformConfigStrings,
                PlatformConfigVfrBin,
                NULL
                );
  if (HiiHandle == NULL) {
    ArmVExpressPlatformConfigUnload (ImageHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle = HiiHandle;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  EfiEventEmptyFunction,
                  NULL,
                  &gEfiIfrRefreshIdOpGuid,
                  &mEvent
                  );
  if (EFI_ERROR (Status)) {
    ArmVExpressPlatformConfigUnload (ImageHandle);
  }

  return Status;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
ArmVExpressPlatformConfigUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mPrivateData == NULL) {
    ASSERT (mPrivateData != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (mEvent != NULL) {
    gBS->CloseEvent (mEvent);
  }

  if (DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mHiiVendorDevicePath0,
           &gEfiHiiConfigAccessProtocolGuid,
           &mPrivateData->ConfigAccess,
           NULL
           );
    DriverHandle = NULL;
  }

  if (mPrivateData->HiiHandle != NULL) {
    HiiRemovePackages (mPrivateData->HiiHandle);
  }

  FreePool (mPrivateData);
  mPrivateData = NULL;

  return EFI_SUCCESS;
}
