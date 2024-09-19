/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformInfoHob.h>
#include <Guid/RootComplexConfigHii.h>
#include <Guid/RootComplexInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NVParamLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <NVParamDef.h>
#include <Platform/Ac01.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigKeyword.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>

#include "RootComplexConfigDxe.h"

BOOLEAN   mReadOnlyStrongOrdering;
CHAR16    mPcieNvparamVarstoreName[] = NVPARAM_VARSTORE_NAME;
CHAR16    gPcieVarstoreName[]        = ROOT_COMPLEX_CONFIG_VARSTORE_NAME;
EFI_GUID  gPcieFormSetGuid           = ROOT_COMPLEX_CONFIG_FORMSET_GUID;

SCREEN_PRIVATE_DATA  *mPrivateData = NULL;

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
    ROOT_COMPLEX_CONFIG_FORMSET_GUID
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

BOOLEAN
IsEmptyRC (
  IN AC01_ROOT_COMPLEX  *RootComplex
  )
{
  UINT8  Idx;

  for (Idx = PcieController0; Idx < MaxPcieController; Idx++) {
    if (RootComplex->Pcie[Idx].Active) {
      return FALSE;
    }
  }

  return TRUE;
}

AC01_ROOT_COMPLEX *
GetRootComplex (
  UINT8  Index
  )
{
  AC01_ROOT_COMPLEX  *RootComplexList;
  VOID               *Hob;

  Hob = GetFirstGuidHob (&gRootComplexInfoHobGuid);
  if (Hob == NULL) {
    return NULL;
  }

  RootComplexList = (AC01_ROOT_COMPLEX *)GET_GUID_HOB_DATA (Hob);
  return &RootComplexList[Index];
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
ExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT      EFI_STRING                      *Progress,
  OUT      EFI_STRING                      *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  SCREEN_PRIVATE_DATA              *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequest;
  UINTN                            Size;
  CHAR16                           *StrPointer;
  BOOLEAN                          AllocatedRequest;
  UINT8                            *VarStoreConfig;
  UINT32                           Value;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Request == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Initialize the local variables.
  //
  ConfigRequest    = NULL;
  Size             = 0;
  *Progress        = Request;
  AllocatedRequest = FALSE;

  PrivateData      = SCREEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (HiiIsConfigHdrMatch (Request, &gPcieFormSetGuid, mPcieNvparamVarstoreName)) {
    VarStoreConfig = (UINT8 *)&PrivateData->NVParamVarStoreConfig;
    ASSERT (VarStoreConfig != NULL);

    Status = NVParamGet (
               NV_SI_MESH_S0_CXG_RC_STRONG_ORDERING_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &Value
               );
    ASSERT_EFI_ERROR (Status);
    if (Value != 0) {
      PrivateData->NVParamVarStoreConfig.PcieStrongOrdering = TRUE;
    }

    Status = NVParamGet (
               NV_SI_MESH_S1_CXG_RC_STRONG_ORDERING_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &Value
               );
    ASSERT_EFI_ERROR (Status);
    if (Value != 0) {
      PrivateData->NVParamVarStoreConfig.PcieStrongOrdering = TRUE;
    }

    BufferSize = sizeof (NVPARAM_ROOT_COMPLEX_CONFIG_VARSTORE_DATA);
  } else if (HiiIsConfigHdrMatch (Request, &gPcieFormSetGuid, gPcieVarstoreName)) {
    VarStoreConfig = (UINT8 *)&PrivateData->VarStoreConfig;
    ASSERT (VarStoreConfig != NULL);

    //
    // Get Buffer Storage data from EFI variable.
    // Try to get the current setting from variable.
    //
    BufferSize = sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA);
    Status     = gRT->GetVariable (
                        gPcieVarstoreName,
                        &gPcieFormSetGuid,
                        NULL,
                        &BufferSize,
                        VarStoreConfig
                        );
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }
  } else {
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
      //
      // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
      // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
      //
      Size          = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      ASSERT (ConfigRequest != NULL);
      AllocatedRequest = TRUE;
      UnicodeSPrint (
        ConfigRequest,
        Size,
        L"%s&OFFSET=0&WIDTH=%016LX",
        Request,
        (UINT64)BufferSize
        );
    }
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               VarStoreConfig,
                               BufferSize,
                               Results,
                               Progress
                               );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
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
  SCREEN_PRIVATE_DATA              *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  UINT8                            *VarStoreConfig;
  UINT32                           Value;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = SCREEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress        = Configuration;

  if (HiiIsConfigHdrMatch (Configuration, &gPcieFormSetGuid, mPcieNvparamVarstoreName)) {
    VarStoreConfig = (UINT8 *)&PrivateData->NVParamVarStoreConfig;
    BufferSize     = sizeof (NVPARAM_ROOT_COMPLEX_CONFIG_VARSTORE_DATA);
  } else if (HiiIsConfigHdrMatch (Configuration, &gPcieFormSetGuid, gPcieVarstoreName)) {
    BufferSize     = sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA);
    VarStoreConfig = (UINT8 *)&PrivateData->VarStoreConfig;
  }

  ASSERT (VarStoreConfig != NULL);

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
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *)VarStoreConfig,
                               &BufferSize,
                               Progress
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check routing data in <ConfigHdr>.
  //
  if (HiiIsConfigHdrMatch (Configuration, &gPcieFormSetGuid, mPcieNvparamVarstoreName)) {
    Value = PrivateData->NVParamVarStoreConfig.PcieStrongOrdering ?
            STRONG_ORDERING_DEFAULT_NVPARAM_VALUE : 0;

    if (!mReadOnlyStrongOrdering) {
      //
      // Update whole 16 RCs.
      //
      Status = NVParamSet (
                 NV_SI_MESH_S0_CXG_RC_STRONG_ORDERING_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 Value
                 );
      ASSERT_EFI_ERROR (Status);

      //
      // No need to check slave present
      //
      Status = NVParamSet (
                 NV_SI_MESH_S1_CXG_RC_STRONG_ORDERING_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 Value
                 );
      ASSERT_EFI_ERROR (Status);
    }
  } else if (HiiIsConfigHdrMatch (Configuration, &gPcieFormSetGuid, gPcieVarstoreName)) {
    //
    // Store Buffer Storage back to variable
    //
    Status = gRT->SetVariable (
                    gPcieVarstoreName,
                    &gPcieFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA),
                    (ROOT_COMPLEX_CONFIG_VARSTORE_DATA *)VarStoreConfig
                    );
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
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN       EFI_BROWSER_ACTION              Action,
  IN       EFI_QUESTION_ID                 QuestionId,
  IN       UINT8                           Type,
  IN       EFI_IFR_TYPE_VALUE              *Value,
  OUT      EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  SCREEN_PRIVATE_DATA  *PrivateData;
  EFI_STATUS           Status;

  if (((Value == NULL) &&
       (Action != EFI_BROWSER_ACTION_FORM_OPEN) &&
       (Action != EFI_BROWSER_ACTION_FORM_CLOSE)) ||
      (ActionRequest == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = SCREEN_PRIVATE_FROM_THIS (This);

  Status = EFI_SUCCESS;

  switch (Action) {
    case EFI_BROWSER_ACTION_FORM_OPEN:
      break;

    case EFI_BROWSER_ACTION_FORM_CLOSE:
      break;

    case EFI_BROWSER_ACTION_DEFAULT_STANDARD:
    case EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING:
      if (QuestionId == SMMU_PMU_ID) {
        //
        // SMMU PMU
        //
        Value->u32 = 0;
        break;
      }

      if (QuestionId == STRONG_ORDERING_ID) {
        //
        // Strong Ordering
        //
        Value->u8 = STRONG_ORDERING_DEFAULT_OPTION_VALUE;
        break;
      }

      switch ((QuestionId - 0x8002) % MAX_EDITABLE_ELEMENTS) {
        case 0:
          Value->u8 = PcieRCActiveDefaultSetting ((QuestionId - 0x8002) / MAX_EDITABLE_ELEMENTS, PrivateData);
          break;

        case 1:
          Value->u8 = PcieRCDevMapLowDefaultSetting ((QuestionId - 0x8002) / MAX_EDITABLE_ELEMENTS, PrivateData);
          break;

        case 2:
          Value->u8 = PcieRCDevMapHighDefaultSetting ((QuestionId - 0x8002) / MAX_EDITABLE_ELEMENTS, PrivateData);
          break;
      }

      break;

    case EFI_BROWSER_ACTION_RETRIEVE:
    case EFI_BROWSER_ACTION_CHANGING:
    case EFI_BROWSER_ACTION_SUBMITTED:
      break;

    default:
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
}

/**
  This function return default settings for Dev Map Low.

  @param  RootComplex            RootComplex ID.
  @param  PrivateData            Private data.

  @retval Default dev settings.
**/
UINT8
PcieRCDevMapLowDefaultSetting (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  )
{
  AC01_ROOT_COMPLEX  *RootComplex = GetRootComplex (RCIndex);

  return RootComplex->DefaultDevMapLow;
}

/**
  This function return default settings for Dev Map High.

  @param  RootComplex            RootComplex ID.
  @param  PrivateData            Private data.

  @retval Default dev settings.
**/
UINT8
PcieRCDevMapHighDefaultSetting (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  )
{
  AC01_ROOT_COMPLEX  *RootComplex = GetRootComplex (RCIndex);

  return RootComplex->DefaultDevMapHigh;
}

BOOLEAN
PcieRCActiveDefaultSetting (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  )
{
  AC01_ROOT_COMPLEX  *RootComplex = GetRootComplex (RCIndex);

  return RootComplex->DefaultActive;
}

VOID *
CreateDevMapOptions (
  AC01_ROOT_COMPLEX  *RootComplex
  )
{
  EFI_STRING_ID  StringId;
  VOID           *OptionsOpCodeHandle;

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  StringId = RootComplex->Type == RootComplexTypeA ?
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE0) :
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE4);
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    DevMapMode1
    );

  StringId = RootComplex->Type == RootComplexTypeA ?
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE1) :
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE5);
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    DevMapMode2
    );

  StringId = RootComplex->Type == RootComplexTypeA ?
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE2) :
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE6);
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    DevMapMode3
    );

  StringId = RootComplex->Type == RootComplexTypeA ?
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE3) :
             STRING_TOKEN (STR_PCIE_BIFUR_SELECT_VALUE7);
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    StringId,
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    DevMapMode4
    );

  if (RootComplex->Type == RootComplexTypeA) {
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_PCIE_BIFUR_SELECT_AUTO),
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      DevMapModeAuto
      );
  }

  return OptionsOpCodeHandle;
}

/**
  This function sets up the first elements of the form.
  @param  RootComplex            RootComplex ID.
  @param  PrivateData            Private data.

  @retval EFI_SUCCESS            The form is set up successfully.
**/
EFI_STATUS
PcieRCScreenSetup (
  IN UINTN                RCIndex,
  IN SCREEN_PRIVATE_DATA  *PrivateData
  )
{
  AC01_ROOT_COMPLEX   *RootComplex;
  CHAR16              Str[MAX_STRING_SIZE];
  EFI_IFR_GUID_LABEL  *EndLabel;
  EFI_IFR_GUID_LABEL  *StartLabel;
  UINT16              BifurHiVarOffset;
  UINT16              BifurLoVarOffset;
  UINT16              DisabledStatusVarOffset;
  UINT8               QuestionFlags, QuestionFlagsSubItem;
  VOID                *EndOpCodeHandle;
  VOID                *OptionsOpCodeHandle;
  VOID                *StartOpCodeHandle;

  RootComplex = GetRootComplex (RCIndex);

  // Initialize the container for dynamic opcodes
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);
  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  // Create Hii Extend Label OpCode as the start opcode
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_RC0_UPDATE + 2 * RCIndex;

  // Create Hii Extend Label OpCode as the end opcode
  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_RC0_END + 2 * RCIndex;

  // Create textbox to show the socket number which current Root Complex belongs to
  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_PCIE_SOCKET),
    STRING_TOKEN (STR_PCIE_SOCKET_HELP),
    HiiSetString (
      PrivateData->HiiHandle,
      0,
      (RootComplex->Socket) ? L"1" : L"0",
      NULL
      )
    );

  // Create textbox to show the Root Complex type
  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_PCIE_RC_TYPE),
    STRING_TOKEN (STR_PCIE_RC_TYPE_HELP),
    HiiSetString (
      PrivateData->HiiHandle,
      0,
      (RootComplex->Type == RootComplexTypeA) ? L"Root Complex Type-A" : L"Root Complex Type-B",
      NULL
      )
    );

  UnicodeSPrint (Str, sizeof (Str), L"Root Complex #%2d", RCIndex);

  DisabledStatusVarOffset = (UINT16)RC0_STATUS_OFFSET + sizeof (BOOLEAN) * RCIndex;
  BifurLoVarOffset        = (UINT16)RC0_BIFUR_LO_OFFSET + sizeof (UINT8) * RCIndex;
  BifurHiVarOffset        = (UINT16)RC0_BIFUR_HI_OFFSET + sizeof (UINT8) * RCIndex;

  QuestionFlags = EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_CALLBACK;
  if (  IsEmptyRC (RootComplex)
     || ((GetNumberOfActiveSockets () == 1) && (RootComplex->Socket == 1)))
  {
    //
    // Do not allow changing if none of Root Port underneath enabled
    // or slave Root Complex on 1P system.
    //
    QuestionFlags |= EFI_IFR_FLAG_READ_ONLY;
  }

  // Create the Root Complex Disable checkbox
  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                        // Container for dynamic created opcodes
    0x8002 + MAX_EDITABLE_ELEMENTS * RCIndex, // QuestionId (or "key")
    VARSTORE_ID,                              // VarStoreId
    DisabledStatusVarOffset,                  // VarOffset in Buffer Storage
    HiiSetString (
      PrivateData->HiiHandle,
      0,
      Str,
      NULL
      ),                                       // Prompt
    STRING_TOKEN (STR_PCIE_RC_STATUS_HELP),    // Help
    QuestionFlags,                             // QuestionFlags
    0,                                         // CheckBoxFlags
    NULL                                       // DefaultsOpCodeHandle
    );

  if (RootComplex->Type == RootComplexTypeA) {
    //
    // Create Option OpCode to display bifurcation for RootComplexTypeA
    //
    OptionsOpCodeHandle = CreateDevMapOptions (RootComplex);

    if (  (RootComplex->DefaultDevMapLow != 0)
       && (RootComplex->DefaultDevMapLow != DevMapModeAuto))
    {
      QuestionFlags |= EFI_IFR_FLAG_READ_ONLY;
    }

    HiiCreateOneOfOpCode (
      StartOpCodeHandle,                        // Container for dynamic created opcodes
      0x8003 + MAX_EDITABLE_ELEMENTS * RCIndex, // Question ID (or call it "key")
      VARSTORE_ID,                              // VarStore ID
      BifurLoVarOffset,                         // Offset in Buffer Storage
      STRING_TOKEN (STR_PCIE_RCA_BIFUR),        // Question prompt text
      STRING_TOKEN (STR_PCIE_RCA_BIFUR_HELP),   // Question help text
      QuestionFlags,                            // Question flag
      EFI_IFR_NUMERIC_SIZE_1,                   // Data type of Question Value
      OptionsOpCodeHandle,                      // Option Opcode list
      NULL                                      // Default Opcode is NULl
      );
  } else {
    //
    // Create Option OpCode to display bifurcation for RootComplexTypeB-Low
    //
    OptionsOpCodeHandle = CreateDevMapOptions (RootComplex);

    QuestionFlagsSubItem = QuestionFlags;
    if (RootComplex->DefaultDevMapLow != 0) {
      QuestionFlagsSubItem |= EFI_IFR_FLAG_READ_ONLY;
    }

    HiiCreateOneOfOpCode (
      StartOpCodeHandle,                         // Container for dynamic created opcodes
      0x8003 + MAX_EDITABLE_ELEMENTS * RCIndex,  // Question ID (or call it "key")
      VARSTORE_ID,                               // VarStore ID
      BifurLoVarOffset,                          // Offset in Buffer Storage
      STRING_TOKEN (STR_PCIE_RCB_LO_BIFUR),      // Question prompt text
      STRING_TOKEN (STR_PCIE_RCB_LO_BIFUR_HELP), // Question help text
      QuestionFlagsSubItem,                      // Question flag
      EFI_IFR_NUMERIC_SIZE_1,                    // Data type of Question Value
      OptionsOpCodeHandle,                       // Option Opcode list
      NULL                                       // Default Opcode is NULl
      );

    //
    // Create Option OpCode to display bifurcation for RootComplexTypeB-High
    //
    OptionsOpCodeHandle = CreateDevMapOptions (RootComplex);

    QuestionFlagsSubItem = QuestionFlags;
    if (RootComplex->DefaultDevMapHigh != 0) {
      QuestionFlagsSubItem |= EFI_IFR_FLAG_READ_ONLY;
    }

    HiiCreateOneOfOpCode (
      StartOpCodeHandle,                         // Container for dynamic created opcodes
      0x8004 + MAX_EDITABLE_ELEMENTS * RCIndex,  // Question ID (or call it "key")
      VARSTORE_ID,                               // VarStore ID
      BifurHiVarOffset,                          // Offset in Buffer Storage
      STRING_TOKEN (STR_PCIE_RCB_HI_BIFUR),      // Question prompt text
      STRING_TOKEN (STR_PCIE_RCB_HI_BIFUR_HELP), // Question help text
      QuestionFlagsSubItem,                      // Question flag
      EFI_IFR_NUMERIC_SIZE_1,                    // Data type of Question Value
      OptionsOpCodeHandle,                       // Option Opcode list
      NULL                                       // Default Opcode is NULl
      );
  }

  HiiUpdateForm (
    PrivateData->HiiHandle,     // HII handle
    &gPcieFormSetGuid,          // Formset GUID
    RC0_FORM_ID + RCIndex,      // Form ID
    StartOpCodeHandle,          // Label for where to insert opcodes
    EndOpCodeHandle             // Insert data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

/**
  This function sets up the first elements of the form.

  @param  PrivateData            Private data.

  @retval EFI_SUCCESS            The form is set up successfully.
**/
EFI_STATUS
PcieMainScreenSetup (
  IN SCREEN_PRIVATE_DATA  *PrivateData
  )
{
  VOID                *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *EndLabel;
  CHAR16              Str[MAX_STRING_SIZE];
  UINTN               RootComplex;
  SETUP_GOTO_DATA     *GotoItem = NULL;
  EFI_QUESTION_ID     GotoId;
  UINT8               QuestionFlags;

  // Initialize the container for dynamic opcodes
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);
  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  // Create Hii Extend Label OpCode as the start opcode
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_UPDATE;

  // Create Hii Extend Label OpCode as the end opcode
  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  QuestionFlags = EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED;

  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                       // Container for dynamic created opcodes
    SMMU_PMU_ID,                             // Question ID
    VARSTORE_ID,                             // VarStore ID
    (UINT16)SMMU_PMU_OFFSET,                 // Offset in Buffer Storage
    STRING_TOKEN (STR_PCIE_SMMU_PMU_PROMPT), // Question prompt text
    STRING_TOKEN (STR_PCIE_SMMU_PMU_HELP),   // Question help text
    QuestionFlags,
    0,
    NULL
    );

  if (mReadOnlyStrongOrdering) {
    QuestionFlags |= EFI_IFR_FLAG_READ_ONLY;
  }

  HiiCreateCheckBoxOpCode (
    StartOpCodeHandle,                              // Container for dynamic created opcodes
    STRONG_ORDERING_ID,                             // Question ID
    NVPARAM_VARSTORE_ID,                            // VarStore ID
    (UINT16)STRONG_ORDERING_OFFSET,                 // Offset in Buffer Storage
    STRING_TOKEN (STR_PCIE_STRONG_ORDERING_PROMPT), // Question prompt text
    STRING_TOKEN (STR_PCIE_STRONG_ORDERING_HELP),   // Question help text
    QuestionFlags,
    STRONG_ORDERING_DEFAULT_OPTION_VALUE,
    NULL
    );

  //
  // Create the a seperated line
  //
  HiiCreateTextOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_PCIE_FORM_SEPERATE_LINE),
    STRING_TOKEN (STR_PCIE_FORM_SEPERATE_LINE),
    STRING_TOKEN (STR_PCIE_FORM_SEPERATE_LINE)
    );

  // Create Goto form for each RootComplex
  for (RootComplex = 0; RootComplex < AC01_PCIE_MAX_ROOT_COMPLEX; RootComplex++) {
    GotoItem = AllocateZeroPool (sizeof (SETUP_GOTO_DATA));
    if (GotoItem == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    GotoItem->PciDevIdx = RootComplex;

    GotoId = GOTO_ID_BASE + (UINT16)RootComplex;

    // Update HII string
    UnicodeSPrint (Str, sizeof (Str), L"Root Complex #%2d", RootComplex);
    GotoItem->GotoStringId = HiiSetString (
                               PrivateData->HiiHandle,
                               0,
                               Str,
                               NULL
                               );
    GotoItem->GotoHelpStringId = STRING_TOKEN (STR_PCIE_GOTO_HELP);
    GotoItem->ShowItem         = TRUE;

    // Add goto control
    HiiCreateGotoOpCode (
      StartOpCodeHandle,
      RC0_FORM_ID + RootComplex,
      GotoItem->GotoStringId,
      GotoItem->GotoHelpStringId,
      EFI_IFR_FLAG_CALLBACK,
      GotoId
      );
  }

  HiiUpdateForm (
    PrivateData->HiiHandle,  // HII handle
    &gPcieFormSetGuid,       // Formset GUID
    FORM_ID,                 // Form ID
    StartOpCodeHandle,       // Label for where to insert opcodes
    EndOpCodeHandle          // Insert data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

VOID
NVParamVarstoreInit (
  VOID
  )
{
  BOOLEAN     BoardSettingValid;
  BOOLEAN     UserSettingValid;
  BOOLEAN     Update;
  EFI_STATUS  Status;
  UINT32      UserValue;
  UINT32      InitValue;

  mReadOnlyStrongOrdering = FALSE;

  // S0
  UserSettingValid = FALSE;
  Status           = NVParamGet (
                       NV_SI_MESH_S0_CXG_RC_STRONG_ORDERING_EN,
                       NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                       &UserValue
                       );
  if (!EFI_ERROR (Status)) {
    UserSettingValid = TRUE;
  }

  //
  // InitValue will be default value or board setting value.
  //
  BoardSettingValid = FALSE;
  Status            = NVParamGet (
                        NV_SI_RO_BOARD_MESH_S0_CXG_RC_STRONG_ORDERING_EN,
                        NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                        &InitValue
                        );
  if (!EFI_ERROR (Status) && (InitValue > 0)) {
    BoardSettingValid       = TRUE;
    mReadOnlyStrongOrdering = TRUE;
  } else {
    InitValue = STRONG_ORDERING_DEFAULT_NVPARAM_VALUE;
  }

  Update = TRUE;
  if (  (UserSettingValid && (UserValue == InitValue))
     || (!BoardSettingValid && UserSettingValid && (UserValue == 0)))
  {
    Update = FALSE;
  }

  if (Update) {
    Status = NVParamSet (
               NV_SI_MESH_S0_CXG_RC_STRONG_ORDERING_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               InitValue
               );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // No need to check slave present.
  //
  UserSettingValid = FALSE;
  Status           = NVParamGet (
                       NV_SI_MESH_S1_CXG_RC_STRONG_ORDERING_EN,
                       NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                       &UserValue
                       );
  if (!EFI_ERROR (Status)) {
    UserSettingValid = TRUE;
  }

  //
  // InitValue will be default value or board setting value.
  //
  BoardSettingValid = FALSE;
  Status            = NVParamGet (
                        NV_SI_RO_BOARD_MESH_S1_CXG_RC_STRONG_ORDERING_EN,
                        NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                        &InitValue
                        );
  if (!EFI_ERROR (Status) && (InitValue > 0)) {
    BoardSettingValid       = TRUE;
    mReadOnlyStrongOrdering = TRUE;
  } else {
    InitValue = STRONG_ORDERING_DEFAULT_NVPARAM_VALUE;
  }

  Update = TRUE;
  if (  (UserSettingValid && (UserValue == InitValue))
     || (!BoardSettingValid && UserSettingValid && (UserValue == 0)))
  {
    Update = FALSE;
  }

  if (Update) {
    Status = NVParamSet (
               NV_SI_MESH_S1_CXG_RC_STRONG_ORDERING_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               InitValue
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Build PCIe menu screen.

  @retval EFI_SUCCESS               The operation is successful.

  @retval Others                    An error occurred.
**/
EFI_STATUS
EFIAPI
RootComplexDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  AC01_ROOT_COMPLEX                    *RootComplex;
  BOOLEAN                              IsUpdated;
  EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL  *HiiKeywordHandler;
  EFI_HANDLE                           DriverHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL      *HiiConfigRouting;
  EFI_HII_DATABASE_PROTOCOL            *HiiDatabase;
  EFI_HII_HANDLE                       HiiHandle;
  EFI_HII_STRING_PROTOCOL              *HiiString;
  EFI_STATUS                           Status;
  ROOT_COMPLEX_CONFIG_VARSTORE_DATA    *VarStoreConfig;
  UINT8                                RCIndex;
  UINTN                                BufferSize;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (SCREEN_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature                  = SCREEN_PRIVATE_DATA_SIGNATURE;
  mPrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig   = RouteConfig;
  mPrivateData->ConfigAccess.Callback      = DriverCallback;

  //
  // Locate Hii Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPrivateData->HiiDatabase = HiiDatabase;

  //
  // Locate HiiString protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **)&HiiString
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPrivateData->HiiString = HiiString;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPrivateData->HiiConfigRouting = HiiConfigRouting;

  //
  // Locate keyword handler protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiConfigKeywordHandlerProtocolGuid,
                  NULL,
                  (VOID **)&HiiKeywordHandler
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPrivateData->HiiKeywordHandler = HiiKeywordHandler;

  DriverHandle = NULL;
  Status       = gBS->InstallMultipleProtocolInterfaces (
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
                &gPcieFormSetGuid,
                DriverHandle,
                RootComplexConfigDxeStrings,
                RootComplexConfigVfrBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle = HiiHandle;

  //
  // Initialize NVParam varstore configuration data
  //
  NVParamVarstoreInit ();

  //
  // Initialize efi varstore configuration data
  //
  VarStoreConfig = &mPrivateData->VarStoreConfig;
  ZeroMem (VarStoreConfig, sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA));

  // Get Buffer Storage data from EFI variable
  BufferSize = sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      gPcieVarstoreName,
                      &gPcieFormSetGuid,
                      NULL,
                      &BufferSize,
                      VarStoreConfig
                      );

  IsUpdated = FALSE;

  if (EFI_ERROR (Status)) {
    VarStoreConfig->SmmuPmu = 0; /* Disable by default */
    IsUpdated               = TRUE;
  }

  // Update board settings to menu
  for (RCIndex = 0; RCIndex < AC01_PCIE_MAX_ROOT_COMPLEX; RCIndex++) {
    RootComplex = GetRootComplex (RCIndex);

    if (EFI_ERROR (Status)) {
      VarStoreConfig->RCBifurcationLow[RCIndex]  = RootComplex->DefaultDevMapLow;
      VarStoreConfig->RCBifurcationHigh[RCIndex] = RootComplex->DefaultDevMapHigh;
      VarStoreConfig->RCStatus[RCIndex]          = RootComplex->Active;
      IsUpdated                                  = TRUE;
    }
  }

  if (IsUpdated) {
    // Update Buffer Storage
    Status = gRT->SetVariable (
                    gPcieVarstoreName,
                    &gPcieFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA),
                    VarStoreConfig
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = PcieMainScreenSetup (mPrivateData);
  ASSERT_EFI_ERROR (Status);

  for (RCIndex = 0; RCIndex < AC01_PCIE_MAX_ROOT_COMPLEX; RCIndex++) {
    Status = PcieRCScreenSetup (RCIndex, mPrivateData);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
