/** @file

  Source file for the CN10K configuration UI driver.

  Copyright (c) 2022-2026 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ConfigUIDxe.h"
#include "ConfigUIDxeHii.h"

EFI_HANDLE                 DriverHandle  = NULL;
CN10K_CONFIG_PRIVATE_DATA  *mPrivateData = NULL;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_CONFIG_UI_DXE_GUID
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
  Return the current configuration to the HII configuration routing protocol.

  Only a single configuration request is supported; a request containing
  multiple GUID blocks is rejected. When the request contains no OFFSET
  element, a full request for the configuration variable is constructed.

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
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  CN10K_CONFIG_PRIVATE_DATA        *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;
  EFI_STRING                       StrPtr;

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

  PrivateData      = CN10K_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  // Check if multiple request
  if ((Request != NULL) && ((StrPtr = StrStr (Request, L"&GUID=")) != NULL)) {
    *Progress = StrPtr;
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize    = sizeof (CN10K_CONFIG_PRIVATE_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gConfigUIDxeFormSetGuid, CONFIG_VAR_STR, PrivateData->DriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

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
  Apply configuration changes submitted by the HII configuration routing
  protocol.

  Converts the configuration response into the NV data structure, compares it
  with the current values, updates the affected PCDs, and writes the new
  configuration to the non-volatile variable only when something changed.

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
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  CN10K_CONFIG_NV_DATA             *OldConfigData;
  CN10K_CONFIG_NV_DATA             NewConfigData;
  CN10K_CONFIG_PRIVATE_DATA        *Private;
  BOOLEAN                          Change = FALSE;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gConfigUIDxeFormSetGuid, CONFIG_VAR_STR)) {
    return EFI_NOT_FOUND;
  }

  Private       = CN10K_CONFIG_PRIVATE_FROM_THIS (This);
  ConfigRouting = Private->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize    = sizeof (CN10K_CONFIG_NV_DATA);
  OldConfigData = &Private->Configuration;
  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  Status = ConfigRouting->ConfigToBlock (
                            ConfigRouting,
                            Configuration,
                            (UINT8 *)&NewConfigData,
                            &BufferSize,
                            Progress
                            );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Compare new and old BMM configuration data and only do action for modified item to
  // avoid setting unnecessary non-volatile variable
  //

  if (OldConfigData->DisableWatchdog != NewConfigData.DisableWatchdog) {
    Change                         = TRUE;
    OldConfigData->DisableWatchdog = NewConfigData.DisableWatchdog;
    if (NewConfigData.DisableWatchdog) {
      PcdSetBoolS (PcdDisableWatchdog, TRUE);
    } else {
      PcdSetBoolS (PcdDisableWatchdog, FALSE);
    }
  }

  if (OldConfigData->DisablePci1Bus != NewConfigData.DisablePci1Bus) {
    Change                        = TRUE;
    OldConfigData->DisablePci1Bus = NewConfigData.DisablePci1Bus;
    if (NewConfigData.DisablePci1Bus) {
      PcdSetBoolS (PcdDisablePci1Bus, TRUE);
    } else {
      PcdSetBoolS (PcdDisablePci1Bus, FALSE);
    }
  }

  if (OldConfigData->DisableAtfTimers != NewConfigData.DisableAtfTimers) {
    Change                          = TRUE;
    OldConfigData->DisableAtfTimers = NewConfigData.DisableAtfTimers;
    if (NewConfigData.DisableAtfTimers) {
      PcdSetBoolS (PcdDisableAtfTimers, TRUE);
    } else {
      PcdSetBoolS (PcdDisableAtfTimers, FALSE);
    }
  }

  //
  // Store Buffer Storage back to EFI variable
  //
  if (Change) {
    Status = gRT->SetVariable (
                    CONFIG_VAR_STR,
                    &gConfigUIDxeFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (CN10K_CONFIG_NV_DATA),
                    &NewConfigData
                    );
  }

  return Status;
}

/**
  Handle browser actions for the configuration form.

  Mirrors the current form values into the watchdog, PCI1 bus, and ATF timer
  PCDs so that other drivers see them immediately. When the form is closed
  after a change, a popup asks the user to reset the platform.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 The type of action taken by the browser.
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
  CN10K_CONFIG_NV_DATA  Configuration;
  EFI_INPUT_KEY         Key;
  STATIC BOOLEAN        Changed = FALSE;

  if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && (Action != EFI_BROWSER_ACTION_FORM_CLOSE)) ||
      (ActionRequest == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  HiiGetBrowserData (&gConfigUIDxeFormSetGuid, CONFIG_VAR_STR, sizeof (CN10K_CONFIG_NV_DATA), (UINT8 *)&Configuration);
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;

  if (Configuration.DisableWatchdog) {
    PcdSetBoolS (PcdDisableWatchdog, TRUE);
  } else {
    PcdSetBoolS (PcdDisableWatchdog, FALSE);
  }

  if (Configuration.DisablePci1Bus) {
    PcdSetBoolS (PcdDisablePci1Bus, TRUE);
  } else {
    PcdSetBoolS (PcdDisablePci1Bus, FALSE);
  }

  if (Configuration.DisableAtfTimers) {
    PcdSetBoolS (PcdDisableAtfTimers, TRUE);
  } else {
    PcdSetBoolS (PcdDisableAtfTimers, FALSE);
  }

  if ((Action == EFI_BROWSER_ACTION_FORM_CLOSE) && (Changed)) {
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      L"Configuration changed, please reset platform to take effect!",
      NULL
      );
    Changed = FALSE;
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    Changed = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Entry point of the CN10K configuration UI driver.

  Installs the HII configuration access and device path protocols, publishes
  the compiled HII form, loads the configuration from the non-volatile
  variable (creating default values when absent), applies the values to the
  PCDs, and installs the config-applied protocol.

  @param[in]  ImageHandle  Image handle for this driver.
  @param[in]  SystemTable  Pointer to the system table.

  @retval  EFI_SUCCESS           The driver initialized successfully.
  @retval  EFI_OUT_OF_RESOURCES  A private data or HII package allocation
                                 failed.
  @retval  Others                A protocol installation or variable access
                                 failed.

**/
EFI_STATUS
EFIAPI
ConfigUIDxeInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  CN10K_CONFIG_NV_DATA             *Configuration;
  UINTN                            BufferSize;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (CN10K_CONFIG_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = CN10K_CINFIG_PRIVATE_SIGNATURE;

  mPrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig   = RouteConfig;
  mPrivateData->ConfigAccess.Callback      = DriverCallback;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPrivateData->HiiConfigRouting = HiiConfigRouting;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mPrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  mPrivateData->DriverHandle = DriverHandle;

  //
  // Publish our HII data
  //
  HiiHandle = HiiAddPackages (
                &gConfigUIDxeFormSetGuid,
                DriverHandle,
                ConfigUIBin,
                ConfigUIDxeStrings,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle = HiiHandle;

  //
  // Initialize configuration data
  //
  Configuration = &mPrivateData->Configuration;
  ZeroMem (Configuration, sizeof (CN10K_CONFIG_NV_DATA));

  BufferSize = sizeof (CN10K_CONFIG_NV_DATA);
  Status     = gRT->GetVariable (CONFIG_VAR_STR, &gConfigUIDxeFormSetGuid, NULL, &BufferSize, Configuration);
  if (EFI_ERROR (Status)) {
    Configuration->DisableWatchdog = 0;
    Configuration->DisablePci1Bus  = 0;
    //
    // Store zero data Buffer Storage to EFI variable
    //
    Status = gRT->SetVariable (
                    CONFIG_VAR_STR,
                    &gConfigUIDxeFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (CN10K_CONFIG_NV_DATA),
                    Configuration
                    );
  }

  if (Configuration->DisableWatchdog) {
    PcdSetBoolS (PcdDisableWatchdog, TRUE);
  } else {
    PcdSetBoolS (PcdDisableWatchdog, FALSE);
  }

  if (Configuration->DisablePci1Bus) {
    PcdSetBoolS (PcdDisablePci1Bus, TRUE);
  } else {
    PcdSetBoolS (PcdDisablePci1Bus, FALSE);
  }

  if (Configuration->DisableAtfTimers) {
    PcdSetBoolS (PcdDisableAtfTimers, TRUE);
  } else {
    PcdSetBoolS (PcdDisableAtfTimers, FALSE);
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gCN10KConfigAppliedProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
