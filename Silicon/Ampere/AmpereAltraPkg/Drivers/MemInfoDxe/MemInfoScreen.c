/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "MemInfoScreen.h"

#define MAX_STRING_SIZE  64
#define GB_SCALE_FACTOR  (1024*1024*1024)
#define MB_SCALE_FACTOR  (1024*1024)

EFI_GUID  gMemInfoFormSetGuid = MEM_INFO_FORM_SET_GUID;

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
    MEM_INFO_FORM_SET_GUID
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

EFI_HANDLE                    DriverHandle  = NULL;
MEM_INFO_SCREEN_PRIVATE_DATA  *mPrivateData = NULL;

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
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.
**/
EFI_STATUS
EFIAPI
ExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT      EFI_STRING                      *Progress,
  OUT      EFI_STRING                      *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  MEM_INFO_SCREEN_PRIVATE_DATA     *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  UINTN                            Size;
  CHAR16                           *StrPointer;
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

  PrivateData      = MEM_INFO_SCREEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (MEM_INFO_VARSTORE_DATA);
  Status     = MemInfoNvparamGet (&PrivateData->VarStoreConfig);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (Request == NULL) {
    //
    // Request is set to NULL, construct full request string.
    //

    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gMemInfoFormSetGuid, MEM_INFO_VARSTORE_NAME, PrivateData->DriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  } else {
    //
    // Check routing data in <ConfigHdr>.
    // Note: if only one Storage is used, then this checking could be skipped.
    //
    if (!HiiIsConfigHdrMatch (Request, &gMemInfoFormSetGuid, NULL)) {
      return EFI_NOT_FOUND;
    }

    //
    // Set Request to the unified request string.
    //
    ConfigRequest = Request;

    //
    // Check whether Request includes Request Element.
    //
    if (StrStr (Request, L"OFFSET") == NULL) {
      //
      // Check Request Element does exist in Request String
      //
      StrPointer = StrStr (Request, L"PATH");
      if (StrPointer == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      if (StrStr (StrPointer, L"&") == NULL) {
        Size          = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
        ConfigRequest = AllocateZeroPool (Size);
        ASSERT (ConfigRequest != NULL);
        AllocatedRequest = TRUE;
        UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", Request, (UINT64)BufferSize);
      }
    }
  }

  //
  // Check if requesting Name/Value storage
  //
  if (StrStr (ConfigRequest, L"OFFSET") == NULL) {
    //
    // Don't have any Name/Value storage names
    //
    Status = EFI_SUCCESS;
  } else {
    //
    // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
    //
    Status = HiiConfigRouting->BlockToConfig (
                                 HiiConfigRouting,
                                 ConfigRequest,
                                 (UINT8 *)&PrivateData->VarStoreConfig,
                                 BufferSize,
                                 Results,
                                 Progress
                                 );
  }

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  if (ConfigRequestHdr != NULL) {
    FreePool (ConfigRequestHdr);
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
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT      EFI_STRING                      *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  MEM_INFO_SCREEN_PRIVATE_DATA     *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = MEM_INFO_SCREEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress        = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gMemInfoFormSetGuid, NULL)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get Buffer Storage data from NVParam
  //
  Status = MemInfoNvparamGet (&PrivateData->VarStoreConfig);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if configuring Name/Value storage
  //
  if (StrStr (Configuration, L"OFFSET") == NULL) {
    //
    // Don't have any Name/Value storage names
    //
    return EFI_SUCCESS;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (MEM_INFO_VARSTORE_DATA);
  Status     = HiiConfigRouting->ConfigToBlock (
                                   HiiConfigRouting,
                                   Configuration,
                                   (UINT8 *)&PrivateData->VarStoreConfig,
                                   &BufferSize,
                                   Progress
                                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Store Buffer Storage back to NVParam
  //
  Status = MemInfoNvparamSet (&PrivateData->VarStoreConfig);

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
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.
**/
EFI_STATUS
EFIAPI
DriverCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN       EFI_BROWSER_ACTION              Action,
  IN       EFI_QUESTION_ID                 QuestionId,
  IN       UINT8                           Type,
  IN       EFI_IFR_TYPE_VALUE              *Value,
  OUT      EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  if (  (  (Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN)
        && (Action != EFI_BROWSER_ACTION_FORM_CLOSE))
     || (ActionRequest == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  switch (Action) {
    case EFI_BROWSER_ACTION_FORM_OPEN:
    case EFI_BROWSER_ACTION_FORM_CLOSE:
      break;

    case EFI_BROWSER_ACTION_DEFAULT_STANDARD:
    case EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING:
    {
      switch (QuestionId) {
        case MEM_INFO_DDR_SPEED_SEL_QUESTION_ID:
          //
          // DDR speed selection default to auto
          //
          Value->u32 = 0;
          break;

        case MEM_INFO_FORM_PERFORMANCE_ECC_QUESTION_ID:
          //
          // ECC mode default to be enabled
          //
          Value->u32 = EccAuto;
          break;

        case MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_DE_QUESTION_ID:
          //
          // ErrCtrl_DE default to be enabled
          //
          Value->u32 = ErrCtlrDeEnable;
          break;

        case MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_FI_QUESTION_ID:
          //
          // ErrCtrl_FI default to be enabled
          //
          Value->u32 = ErrCtlrDeEnable;
          break;

        case MEM_INFO_DDR_SLAVE_32BIT_QUESTION_ID:
          //
          // Slave's 32bit region to be disabled
          //
          Value->u32 = 0;
          break;

        case MEM_INFO_DDR_SCRUB_PATROL_QUESTION_ID:
          Value->u32 = DDR_DEFAULT_SCRUB_PATROL_DURATION;
          break;

        case MEM_INFO_DDR_DEMAND_SCRUB_QUESTION_ID:
          Value->u32 = DDR_DEFAULT_DEMAND_SCRUB;
          break;

        case MEM_INFO_DDR_WRITE_CRC_QUESTION_ID:
          Value->u32 = DDR_DEFAULT_WRITE_CRC;
          break;

        case MEM_INFO_FGR_MODE_QUESTION_ID:
          Value->u32 = DDR_DEFAULT_FGR_MODE;
          break;

        case MEM_INFO_REFRESH2X_MODE_QUESTION_ID:
          Value->u32 = DDR_DEFAULT_REFRESH2X_MODE;
          break;

        case MEM_INFO_FORM_NVDIMM_MODE_SEL_QUESTION_ID:
          Value->u32 = DDR_DEFAULT_NVDIMM_MODE_SEL;
          break;
      }

      break;
    }

    case EFI_BROWSER_ACTION_RETRIEVE:
    case EFI_BROWSER_ACTION_CHANGING:
    case EFI_BROWSER_ACTION_SUBMITTED:
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateMemInfo (
  PLATFORM_INFO_HOB  *PlatformHob
  )
{
  MEM_INFO_SCREEN_PRIVATE_DATA  *PrivateData = mPrivateData;
  CHAR16                        Str[MAX_STRING_SIZE];
  EFI_HOB_RESOURCE_DESCRIPTOR   *ResHob;
  UINT64                        Size;

  /* Update Total memory */
  UnicodeSPrint (Str, sizeof (Str), L"%d GB", PlatformHob->DramInfo.TotalSize / GB_SCALE_FACTOR);
  HiiSetString (
    PrivateData->HiiHandle,
    STRING_TOKEN (STR_MEM_INFO_TOTAL_MEM_VALUE),
    Str,
    NULL
    );

  /* Update effective memory */
  Size   = 0;
  ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (ResHob != NULL) {
    if ((ResHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)) {
      Size += ResHob->ResourceLength;
    }

    ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, (VOID *)((UINTN)ResHob + ResHob->Header.HobLength));
  }

  UnicodeSPrint (Str, sizeof (Str), L"%d GB", Size / GB_SCALE_FACTOR);
  HiiSetString (
    PrivateData->HiiHandle,
    STRING_TOKEN (STR_MEM_INFO_EFFECT_MEM_VALUE),
    Str,
    NULL
    );

  /* Update current DDR speed */
  UnicodeSPrint (Str, sizeof (Str), L"%d MHz", PlatformHob->DramInfo.MaxSpeed);
  HiiSetString (
    PrivateData->HiiHandle,
    STRING_TOKEN (STR_MEM_INFO_CURRENT_SPEED_VALUE),
    Str,
    NULL
    );

  return EFI_SUCCESS;
}

EFI_STATUS
AddMemorySpeedSelection (
  PLATFORM_INFO_HOB  *PlatformHob,
  VOID               *StartOpCodeHandle
  )
{
  VOID  *OptionsOpCodeHandle;

  //
  // Create Option OpCode to display speed configuration
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_VALUE0),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    0
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_VALUE1),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2133
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_VALUE2),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2400
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_VALUE3),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2666
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_VALUE4),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2933
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_VALUE5),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    3200
    );

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                                   // Container for dynamic created opcodes
    MEM_INFO_DDR_SPEED_SEL_QUESTION_ID,                  // Question ID (or call it "key")
    MEM_INFO_VARSTORE_ID,                                // VarStore ID
    (UINT16)MEM_INFO_DDR_SPEED_SEL_OFFSET,               // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_PROMPT),     // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_SPEED_SELECT_HELP),       // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED, // Question flag
    EFI_IFR_NUMERIC_SIZE_4,                              // Data type of Question Value
    OptionsOpCodeHandle,                                 // Option Opcode list
    NULL                                                 // Default Opcode is NULl
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  return EFI_SUCCESS;
}

EFI_STATUS
AddFgrModeSelection (
  PLATFORM_INFO_HOB  *PlatformHob,
  VOID               *StartOpCodeHandle
  )
{
  VOID  *OptionsOpCodeHandle;

  //
  // Create Option OpCode to display FGR mode configuration
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_FGR_MODE_VALUE0),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    0
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_FGR_MODE_VALUE1),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    1
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_FGR_MODE_VALUE2),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2
    );

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                                   // Container for dynamic created opcodes
    MEM_INFO_FGR_MODE_QUESTION_ID,                       // Question ID (or call it "key")
    MEM_INFO_VARSTORE_ID,                                // VarStore ID
    (UINT16)MEM_INFO_FGR_MODE_OFFSET,                    // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_FGR_MODE_PROMPT),         // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_FGR_MODE_HELP),           // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED, // Question flag
    EFI_IFR_NUMERIC_SIZE_4,                              // Data type of Question Value
    OptionsOpCodeHandle,                                 // Option Opcode list
    NULL                                                 // Default Opcode is NULl
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  return EFI_SUCCESS;
}

EFI_STATUS
AddDimmListInfo (
  PLATFORM_INFO_HOB  *PlatformHob,
  VOID               *StartOpCodeHandle
  )
{
  MEM_INFO_SCREEN_PRIVATE_DATA  *PrivateData = mPrivateData;
  CHAR16                        Str[MAX_STRING_SIZE], Str1[MAX_STRING_SIZE];
  UINTN                         Count;
  PLATFORM_DIMM_INFO            *DimmInfo;
  EFI_STRING_ID                 StringId;

  //
  // Display DIMM list info
  //
  HiiCreateSubTitleOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_DIMM_INFO),
    0,
    0,
    0
    );

  for (Count = 0; Count < PlatformHob->DimmList.BoardDimmSlots; Count++) {
    DimmInfo = &PlatformHob->DimmList.Dimm[Count].Info;
    switch (DimmInfo->DimmType) {
      case UDIMM:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"UDIMM");
        break;

      case RDIMM:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"RDIMM");
        break;

      case SODIMM:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"SODIMM");
        break;

      case LRDIMM:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"LRDIMM");
        break;

      case RSODIMM:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"RSODIMM");
        break;

      case NVRDIMM:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"NV-RDIMM");
        break;

      default:
        UnicodeSPrint (Str, sizeof (Str), L"Unknown Type");
    }

    if (DimmInfo->DimmStatus == DIMM_INSTALLED_OPERATIONAL) {
      UnicodeSPrint (Str1, sizeof (Str1), L"Slot %2d: %d GB %s Installed&Operational", Count + 1, DimmInfo->DimmSize, Str);
    } else if (DimmInfo->DimmStatus == DIMM_NOT_INSTALLED) {
      UnicodeSPrint (Str1, sizeof (Str1), L"Slot %2d: Not Installed", Count + 1, PlatformHob->DimmList.Dimm[Count].NodeId);
    } else if (DimmInfo->DimmStatus == DIMM_INSTALLED_NONOPERATIONAL) {
      UnicodeSPrint (Str1, sizeof (Str1), L"Slot %2d: Installed&Non-Operational", Count + 1, PlatformHob->DimmList.Dimm[Count].NodeId);
    } else {
      UnicodeSPrint (Str1, sizeof (Str1), L"Slot %2d: Installed&Failed", Count + 1, PlatformHob->DimmList.Dimm[Count].NodeId);
    }

    StringId = HiiSetString (PrivateData->HiiHandle, 0, Str1, NULL);

    HiiCreateSubTitleOpCode (
      StartOpCodeHandle,
      StringId,
      0,
      0,
      0
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MemInfoMainScreen (
  PLATFORM_INFO_HOB  *PlatformHob
  )
{
  MEM_INFO_SCREEN_PRIVATE_DATA  *PrivateData = mPrivateData;
  EFI_IFR_GUID_LABEL            *StartLabel;
  EFI_IFR_GUID_LABEL            *EndLabel;
  VOID                          *StartOpCodeHandle;
  VOID                          *EndOpCodeHandle;
  EFI_STATUS                    Status;

  Status = UpdateMemInfo (PlatformHob);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Buffer Storage data from EFI variable
  //
  Status = MemInfoNvparamGet (&PrivateData->VarStoreConfig);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize the container for dynamic opcodes
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_UPDATE;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Create a total mem title
  //
  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_TOTAL_MEM),
    STRING_TOKEN (STR_MEM_INFO_TOTAL_MEM),
    STRING_TOKEN (STR_MEM_INFO_TOTAL_MEM_VALUE)
    );

  //
  // Create a effective mem title
  //
  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_EFFECT_MEM),
    STRING_TOKEN (STR_MEM_INFO_EFFECT_MEM),
    STRING_TOKEN (STR_MEM_INFO_EFFECT_MEM_VALUE)
    );

  //
  // Create a current speed title
  //
  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_CURRENT_SPEED),
    STRING_TOKEN (STR_MEM_INFO_CURRENT_SPEED),
    STRING_TOKEN (STR_MEM_INFO_CURRENT_SPEED_VALUE)
    );

  if (IsSlaveSocketActive ()) {
    //
    // Display enable slave's 32bit region
    //
    HiiCreateCheckBoxOpCode (
      StartOpCodeHandle,                                    // Container for dynamic created opcodes
      MEM_INFO_DDR_SLAVE_32BIT_QUESTION_ID,                 // Question ID
      MEM_INFO_VARSTORE_ID,                                 // VarStore ID
      (UINT16)MEM_INFO_ERR_SLAVE_32BIT_OFFSET,              // Offset in Buffer Storage
      STRING_TOKEN (STR_MEM_INFO_ENABLE_32GB_SLAVE_PROMPT), // Question prompt text
      STRING_TOKEN (STR_MEM_INFO_ENABLE_32GB_SLAVE_HELP),   // Question help text
      EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
      0,
      NULL
      );
  }

  Status = AddMemorySpeedSelection (PlatformHob, StartOpCodeHandle);
  ASSERT_EFI_ERROR (Status);

  Status = AddFgrModeSelection (PlatformHob, StartOpCodeHandle);
  ASSERT_EFI_ERROR (Status);

  //
  // Create a Goto OpCode to ras memory configuration
  //
  HiiCreateGotoOpCode (
    StartOpCodeHandle,                                 // Container for dynamic created opcodes
    MEM_INFO_FORM_PERFORMANCE_ID,                      // Target Form ID
    STRING_TOKEN (STR_MEM_INFO_PERFORMANCE_FORM),      // Prompt text
    STRING_TOKEN (STR_MEM_INFO_PERFORMANCE_FORM_HELP), // Help text
    0,                                                 // Question flag
    MEM_INFO_FORM_PERFORMANCE_QUESTION_ID              // Question ID
    );

  //
  // Create a Goto OpCode to nvdimm-n configuration
  //
  HiiCreateGotoOpCode (
    StartOpCodeHandle,                            // Container for dynamic created opcodes
    MEM_INFO_FORM_NVDIMM_ID,                      // Target Form ID
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_FORM),      // Prompt text
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_FORM_HELP), // Help text
    0,                                            // Question flag
    MEM_INFO_FORM_NVDIMM_QUESTION_ID              // Question ID
    );

  Status = AddDimmListInfo (PlatformHob, StartOpCodeHandle);
  ASSERT_EFI_ERROR (Status);

  HiiUpdateForm (
    PrivateData->HiiHandle,  // HII handle
    &gMemInfoFormSetGuid,    // Formset GUID
    MEM_INFO_FORM_ID,        // Form ID
    StartOpCodeHandle,       // Label for where to insert opcodes
    EndOpCodeHandle          // Insert data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}

EFI_STATUS
MemInfoMainPerformanceScreen (
  PLATFORM_INFO_HOB  *PlatformHob
  )
{
  EFI_STATUS                    Status;
  MEM_INFO_SCREEN_PRIVATE_DATA  *PrivateData = mPrivateData;
  VOID                          *StartOpCodeHandle;
  VOID                          *OptionsEccOpCodeHandle, *OptionsScrubOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *EndLabel;
  EFI_STRING_ID                 StringId;
  CHAR16                        Str[MAX_STRING_SIZE];
  UINTN                         Idx;

  Status = EFI_SUCCESS;

  //
  // Initialize the container for dynamic opcodes
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_UPDATE;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  /* Display ECC mode selection */
  OptionsEccOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsEccOpCodeHandle != NULL);

  UnicodeSPrint (Str, sizeof (Str), L"Disabled");
  StringId = HiiSetString (PrivateData->HiiHandle, 0, Str, NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsEccOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    0
    );

  UnicodeSPrint (Str, sizeof (Str), L"SECDED");
  StringId = HiiSetString (PrivateData->HiiHandle, 0, Str, NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsEccOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    1
    );

  UnicodeSPrint (Str, sizeof (Str), L"Symbol");
  StringId = HiiSetString (PrivateData->HiiHandle, 0, Str, NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsEccOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2
    );

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                                   // Container for dynamic created opcodes
    MEM_INFO_FORM_PERFORMANCE_ECC_QUESTION_ID,           // Question ID (or call it "key")
    MEM_INFO_VARSTORE_ID,                                // VarStore ID
    (UINT16)MEM_INFO_ECC_MODE_SEL_OFFSET,                // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_ENABLE_ECC_PROMPT),       // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_ENABLE_ECC_HELP),         // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED, // Question flag
    EFI_IFR_NUMERIC_SIZE_4,                              // Data type of Question Value
    OptionsEccOpCodeHandle,                              // Option Opcode list
    NULL                                                 // Default Opcode is NULl
    );

  /*
   * Display ErrCtrl options
   */
  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                                    // Container for dynamic created opcodes
    MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_DE_QUESTION_ID,    // Question ID
    MEM_INFO_VARSTORE_ID,                                 // VarStore ID
    (UINT16)MEM_INFO_ERR_CTRL_DE_MODE_SEL_OFFSET,         // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_ENABLE_ERRCTRL_DE_PROMPT), // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_ENABLE_ERRCTRL_DE_HELP),   // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
    0,
    NULL
    );

  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                                    // Container for dynamic created opcodes
    MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_FI_QUESTION_ID,    // Question ID
    MEM_INFO_VARSTORE_ID,                                 // VarStore ID
    (UINT16)MEM_INFO_ERR_CTRL_FI_MODE_SEL_OFFSET,         // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_ENABLE_ERRCTRL_FI_PROMPT), // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_ENABLE_ERRCTRL_FI_HELP),   // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
    0,
    NULL
    );

  /* Display Scrub Patrol selection */
  OptionsScrubOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsScrubOpCodeHandle != NULL);

  UnicodeSPrint (Str, sizeof (Str), L"Disabled");
  StringId = HiiSetString (PrivateData->HiiHandle, 0, Str, NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsScrubOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    0
    );

  for (Idx = 1; Idx <= MAX_NUMBER_OF_HOURS_IN_A_DAY; Idx++) {
    UnicodeSPrint (Str, sizeof (Str), L"%d", Idx);
    StringId = HiiSetString (
                 PrivateData->HiiHandle,
                 0,
                 Str,
                 NULL
                 );
    HiiCreateOneOfOptionOpCode (
      OptionsScrubOpCodeHandle,
      StringId,
      0,
      EFI_IFR_NUMERIC_SIZE_4,
      Idx
      );
  }

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                                   // Container for dynamic created opcodes
    MEM_INFO_DDR_SCRUB_PATROL_QUESTION_ID,               // Question ID (or call it "key")
    MEM_INFO_VARSTORE_ID,                                // VarStore ID
    (UINT16)MEM_INFO_DDR_SCRUB_OFFSET,                   // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_ENABLE_SCRUB),            // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_ENABLE_SCRUB_HELP),       // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED, // Question flag
    EFI_IFR_NUMERIC_SIZE_4,                              // Data type of Question Value
    OptionsScrubOpCodeHandle,                            // Option Opcode list
    NULL                                                 // Default Opcode is NULl
    );

  /*
   * Display Demand Scrub options
   */
  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                                      // Container for dynamic created opcodes
    MEM_INFO_DDR_DEMAND_SCRUB_QUESTION_ID,                  // Question ID
    MEM_INFO_VARSTORE_ID,                                   // VarStore ID
    (UINT16)MEM_INFO_DDR_DEMAND_SCRUB_OFFSET,               // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_ENABLE_DEMAND_SCRUB_PROMPT), // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_ENABLE_DEMAND_SCRUB_HELP),   // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
    0,
    NULL
    );

  /*
   * Display Write CRC options
   */
  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                                   // Container for dynamic created opcodes
    MEM_INFO_DDR_WRITE_CRC_QUESTION_ID,                  // Question ID
    MEM_INFO_VARSTORE_ID,                                // VarStore ID
    (UINT16)MEM_INFO_DDR_WRITE_CRC_OFFSET,               // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_ENABLE_WRITE_CRC_PROMPT), // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_ENABLE_WRITE_CRC_HELP),   // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
    0,
    NULL
    );

  /*
   * Display CVE-2020-10255 options
   */
  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                                 // Container for dynamic created opcodes
    MEM_INFO_REFRESH2X_MODE_QUESTION_ID,               // Question ID
    MEM_INFO_VARSTORE_ID,                              // VarStore ID
    (UINT16)MEM_INFO_REFRESH2X_MODE_OFFSET,            // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_REFRESH2X_MODE_PROMPT), // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_REFRESH2X_MODE_HELP),   // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
    0,
    NULL
    );

  HiiUpdateForm (
    PrivateData->HiiHandle,              // HII handle
    &gMemInfoFormSetGuid,                // Formset GUID
    MEM_INFO_FORM_PERFORMANCE_ID,        // Form ID
    StartOpCodeHandle,                   // Label for where to insert opcodes
    EndOpCodeHandle                      // Insert data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsEccOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsScrubOpCodeHandle);

  return Status;
}

EFI_STATUS
MemInfoMainNvdimmScreen (
  PLATFORM_INFO_HOB  *PlatformHob
  )
{
  EFI_STATUS                    Status;
  MEM_INFO_SCREEN_PRIVATE_DATA  *PrivateData;
  VOID                          *StartOpCodeHandle;
  VOID                          *OptionsOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *EndLabel;
  CHAR16                        Str[MAX_STRING_SIZE];

  Status      = EFI_SUCCESS;
  PrivateData = mPrivateData;

  if (PlatformHob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the container for dynamic opcodes
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_UPDATE;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Update Current NVDIMM-N Mode title Socket0
  //
  switch (PlatformHob->DramInfo.NvdimmMode[0]) {
    case 0:
      UnicodeSPrint (Str, sizeof (Str), L"%s", L"Non-NVDIMM");
      break;

    case 1:
      UnicodeSPrint (Str, sizeof (Str), L"%s", L"Non-Hashed");
      break;

    case 2:
      UnicodeSPrint (Str, sizeof (Str), L"%s", L"Hashed");
      break;

    default:
      UnicodeSPrint (Str, sizeof (Str), L"%s", L"Unknown");
      break;
  }

  HiiSetString (
    PrivateData->HiiHandle,
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK0_VALUE),
    Str,
    NULL
    );

  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK0),
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK0),
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK0_VALUE)
    );

  //
  // Update Current NVDIMM-N Mode title Socket1
  //
  if (IsSlaveSocketActive ()) {
    switch (PlatformHob->DramInfo.NvdimmMode[1]) {
      case 0:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"Non-NVDIMM");
        break;

      case 1:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"Non-Hashed");
        break;

      case 2:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"Hashed");
        break;

      default:
        UnicodeSPrint (Str, sizeof (Str), L"%s", L"Unknown");
        break;
    }

    HiiSetString (
      PrivateData->HiiHandle,
      STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK1_VALUE),
      Str,
      NULL
      );

    HiiCreateTextOpCode (
      StartOpCodeHandle,
      STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK1),
      STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK1),
      STRING_TOKEN (STR_MEM_INFO_NVDIMM_CUR_MODE_SK1_VALUE)
      );
  }

  //
  // Create Option OpCode to NVDIMM-N Mode Selection
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  //
  // Create OpCode to NVDIMM-N Mode Selection
  //
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_MODE_SEL_VALUE0),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    0
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_MODE_SEL_VALUE1),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    1
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_MODE_SEL_VALUE2),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    2
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_MODE_SEL_VALUE3),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    3
    );

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                                   // Container for dynamic created opcodes
    MEM_INFO_FORM_NVDIMM_MODE_SEL_QUESTION_ID,           // Question ID (or call it "key")
    MEM_INFO_VARSTORE_ID,                                // VarStore ID
    (UINT16)MEM_INFO_NVDIMM_MODE_SEL_OFFSET,             // Offset in Buffer Storage
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_MODE_SEL_PROMPT),  // Question prompt text
    STRING_TOKEN (STR_MEM_INFO_NVDIMM_MODE_SEL_HELP),    // Question help text
    EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED, // Question flag
    EFI_IFR_NUMERIC_SIZE_4,                              // Data type of Question Value
    OptionsOpCodeHandle,                                 // Option Opcode list
    NULL                                                 // Default Opcode is NULl
    );

  HiiUpdateForm (
    PrivateData->HiiHandle,              // HII handle
    &gMemInfoFormSetGuid,                // Formset GUID
    MEM_INFO_FORM_NVDIMM_ID,             // Form ID
    StartOpCodeHandle,                   // Label for where to insert opcodes
    EndOpCodeHandle                      // Insert data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  return Status;
}

/**
  This function sets up the first elements of the form.
  @param  PrivateData            Private data.
  @retval EFI_SUCCESS            The form is set up successfully.
**/
EFI_STATUS
MemInfoScreenSetup (
  VOID
  )
{
  EFI_STATUS         Status;
  VOID               *Hob;
  PLATFORM_INFO_HOB  *PlatformHob;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return EFI_DEVICE_ERROR;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  Status = MemInfoMainScreen (PlatformHob);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MemInfoMainPerformanceScreen (PlatformHob);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MemInfoMainNvdimmScreen (PlatformHob);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MemInfoScreenInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  BOOLEAN                          ActionFlag;
  EFI_STRING                       ConfigRequestHdr;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (MEM_INFO_SCREEN_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = MEM_INFO_SCREEN_PRIVATE_DATA_SIGNATURE;

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
                &gMemInfoFormSetGuid,
                DriverHandle,
                MemInfoDxeStrings,
                MemInfoScreenVfrBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle = HiiHandle;

  //
  // Try to read NV config EFI variable first
  //
  ConfigRequestHdr = HiiConstructConfigHdr (
                       &gMemInfoFormSetGuid,
                       MEM_INFO_VARSTORE_NAME,
                       DriverHandle
                       );
  ASSERT (ConfigRequestHdr != NULL);

  //
  // Validate Current Setting
  //
  ActionFlag = HiiValidateSettings (ConfigRequestHdr);
  if (!ActionFlag) {
    MemInfoScreenUnload (ImageHandle);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (ConfigRequestHdr);

  Status = MemInfoScreenSetup ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

EFI_STATUS
MemInfoScreenUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  ASSERT (mPrivateData != NULL);

  if (DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mHiiVendorDevicePath,
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
