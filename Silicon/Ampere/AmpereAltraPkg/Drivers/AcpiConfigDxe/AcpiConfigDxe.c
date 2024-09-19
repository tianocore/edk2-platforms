/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <AcpiConfigNVDataStruct.h>
#include <Guid/AcpiConfigHii.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformInfoHob.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>

#include "AcpiConfigDxe.h"

#define ACPI_VARSTORE_ATTRIBUTES  EFI_VARIABLE_BOOTSERVICE_ACCESS |\
                                 EFI_VARIABLE_RUNTIME_ACCESS     | \
                                 EFI_VARIABLE_NON_VOLATILE

CHAR16  AcpiVarstoreDataName[] = L"AcpiConfigNVData";

EFI_HANDLE                mDriverHandle = NULL;
ACPI_CONFIG_PRIVATE_DATA  *mPrivateData = NULL;

HII_VENDOR_DEVICE_PATH  mAcpiConfigHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    ACPI_CONFIGURATION_FORMSET_GUID
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
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  ACPI_CONFIG_PRIVATE_DATA         *PrivateData;
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

  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gAcpiConfigFormSetGuid, AcpiVarstoreDataName)) {
    return EFI_NOT_FOUND;
  }

  PrivateData      = ACPI_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      AcpiVarstoreDataName,
                      &gAcpiConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      &PrivateData->Configuration
                      );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize    = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (
                         &gAcpiConfigFormSetGuid,
                         AcpiVarstoreDataName,
                         PrivateData->DriverHandle
                         );
    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
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
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  ACPI_CONFIG_PRIVATE_DATA         *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = ACPI_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress        = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gAcpiConfigFormSetGuid, AcpiVarstoreDataName)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      AcpiVarstoreDataName,
                      &gAcpiConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      &PrivateData->Configuration
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
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

  //
  // Store Buffer Storage back to EFI variable
  //
  Status = gRT->SetVariable (
                  AcpiVarstoreDataName,
                  &gAcpiConfigFormSetGuid,
                  ACPI_VARSTORE_ATTRIBUTES,
                  sizeof (ACPI_CONFIG_VARSTORE_DATA),
                  &PrivateData->Configuration
                  );

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
DriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  if (  (  (Value == NULL)
        && (Action != EFI_BROWSER_ACTION_FORM_OPEN)
        && (Action != EFI_BROWSER_ACTION_FORM_CLOSE))
     || (ActionRequest == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdateCPPCConfig (
  IN ACPI_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS  Status;
  CHAR8       Buffer[64];

  ASSERT (PrivateData != NULL);

  AsciiSPrint (Buffer, sizeof (Buffer), "\\_SB.CPCE");
  Status = AcpiAmlObjectUpdateInteger (
             PrivateData->AcpiSdtProtocol,
             PrivateData->AcpiTableHandle,
             Buffer,
             PrivateData->Configuration.AcpiCppcEnable
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

STATIC
EFI_STATUS
UpdateLPIConfig (
  IN ACPI_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS  Status;
  CHAR8       Buffer[64];

  ASSERT (PrivateData != NULL);

  AsciiSPrint (Buffer, sizeof (Buffer), "\\_SB.LPIE");
  Status = AcpiAmlObjectUpdateInteger (
             PrivateData->AcpiSdtProtocol,
             PrivateData->AcpiTableHandle,
             Buffer,
             PrivateData->Configuration.AcpiLpiEnable
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

STATIC
VOID
UpdateAcpiOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_SDT_PROTOCOL        *AcpiSdtProtocol;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  EFI_ACPI_HANDLE              TableHandle;
  UINTN                        TableKey;
  UINTN                        TableIndex;

  ASSERT (mPrivateData != NULL);

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&AcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI table protocol\n"));
    return;
  }

  mPrivateData->AcpiSdtProtocol = AcpiSdtProtocol;

  TableIndex = 0;
  Status     = AcpiLocateTableBySignature (
                 AcpiSdtProtocol,
                 EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
                 &TableIndex,
                 &Table,
                 &TableKey
                 );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = AcpiSdtProtocol->OpenSdt (TableKey, &TableHandle);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return;
  }

  mPrivateData->AcpiTableHandle = TableHandle;

  Status = UpdateCPPCConfig (mPrivateData);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = UpdateLPIConfig (mPrivateData);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Close DSDT Table
  //
  AcpiSdtProtocol->Close (TableHandle);
  AcpiUpdateChecksum ((UINT8 *)Table, Table->Length);
}

STATIC
EFI_STATUS
AcpiConfigUnload (
  VOID
  )
{
  ASSERT (mPrivateData != NULL);

  if (mDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mAcpiConfigHiiVendorDevicePath,
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
EFIAPI
AcpiConfigEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  UINTN                            BufferSize;
  ACPI_CONFIG_VARSTORE_DATA        *Configuration;
  BOOLEAN                          ActionFlag;
  EFI_STRING                       ConfigRequestHdr;
  EFI_EVENT                        ReadyToBootEvent;
  PLATFORM_INFO_HOB                *PlatformHob;
  VOID                             *Hob;

  //
  // Initialize the local variables.
  //
  ConfigRequestHdr = NULL;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (ACPI_CONFIG_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = ACPI_CONFIG_PRIVATE_SIGNATURE;

  mPrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig   = RouteConfig;
  mPrivateData->ConfigAccess.Callback      = DriverCallback;

  //
  // Get the Platform HOB
  //
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    AcpiConfigUnload ();
    return EFI_DEVICE_ERROR;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  mPrivateData->PlatformHob = PlatformHob;

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
                  &mAcpiConfigHiiVendorDevicePath,
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
                &gAcpiConfigFormSetGuid,
                mDriverHandle,
                AcpiConfigDxeStrings,
                AcpiConfigVfrBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mAcpiConfigHiiVendorDevicePath,
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
  ZeroMem (Configuration, sizeof (ACPI_CONFIG_VARSTORE_DATA));

  //
  // Try to read NV config EFI variable first
  //
  ConfigRequestHdr = HiiConstructConfigHdr (&gAcpiConfigFormSetGuid, AcpiVarstoreDataName, mDriverHandle);
  ASSERT (ConfigRequestHdr != NULL);

  BufferSize = sizeof (ACPI_CONFIG_VARSTORE_DATA);
  Status     = gRT->GetVariable (AcpiVarstoreDataName, &gAcpiConfigFormSetGuid, NULL, &BufferSize, Configuration);
  if (EFI_ERROR (Status)) {
    //
    // Store zero data Buffer Storage to EFI variable
    //
    Status = gRT->SetVariable (
                    AcpiVarstoreDataName,
                    &gAcpiConfigFormSetGuid,
                    ACPI_VARSTORE_ATTRIBUTES,
                    sizeof (ACPI_CONFIG_VARSTORE_DATA),
                    Configuration
                    );
    if (EFI_ERROR (Status)) {
      AcpiConfigUnload ();
      return Status;
    }

    //
    // EFI variable for NV config doesn't exit, we should build this variable
    // based on default values stored in IFR
    //
    ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    if (!ActionFlag) {
      AcpiConfigUnload ();
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // EFI variable does exist and Validate Current Setting
    //
    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    if (!ActionFlag) {
      AcpiConfigUnload ();
      return EFI_INVALID_PARAMETER;
    }
  }

  FreePool (ConfigRequestHdr);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UpdateAcpiOnReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to create ready to boot event %r!\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}
