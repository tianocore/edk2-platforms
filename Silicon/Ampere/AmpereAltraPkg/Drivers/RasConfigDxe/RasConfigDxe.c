/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/MdeModuleHii.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NVParamLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <NVParamDef.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>

#include "RasConfigDxe.h"

CHAR16  RasConfigVarstoreDataName[] = L"RasConfigNVData";

EFI_HANDLE               mDriverHandle = NULL;
RAS_CONFIG_PRIVATE_DATA  *mPrivateData = NULL;

EFI_GUID  mRasConfigFormSetGuid = RAS_CONFIG_FORMSET_GUID;

//
// Default RAS Settings
//
#define RAS_DEFAULT_HARDWARE_EINJ_SUPPORT     0
#define RAS_DEFAULT_PCIE_AER_FW_FIRST         0
#define RAS_DEFAULT_BERT_SUPPORT              1
#define RAS_DEFAULT_SDEI_SUPPORT              0
#define RAS_DEFAULT_DDR_CE_THRESHOLD          1
#define RAS_DEFAULT_2P_CE_THRESHOLD           1
#define RAS_DEFAULT_PROCESSOR_CE_THRESHOLD    1
#define RAS_DEFAULT_DDR_LINK_ERROR_THRESHOLD  1

HII_VENDOR_DEVICE_PATH  mRasConfigHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    RAS_CONFIG_FORMSET_GUID
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
IsDdrCeWindowEnabled (
  VOID
  )
{
  UINT32      DdrCeWindow;
  UINT32      DdrCeControl;
  EFI_STATUS  Status;

  Status = NVParamGet (
             NV_SI_RO_BOARD_RAS_DDR_CE_WINDOW,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &DdrCeWindow
             );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = NVParamGet (
             NV_SI_RO_BOARD_RAS_DDR_CE_THC,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &DdrCeControl
             );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return ((DdrCeWindow != 0) && (DdrCeControl == 0)) ? TRUE : FALSE;
}

EFI_STATUS
RasConfigNvParamGet (
  OUT RAS_CONFIG_VARSTORE_DATA  *Configuration
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  Status = NVParamGet (
             NV_SI_HARDWARE_EINJ,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_HARDWARE_EINJ_SUPPORT;
    Status = NVParamSet (
               NV_SI_HARDWARE_EINJ,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasHardwareEinj = Value;

  Status = NVParamGet (
             NV_SI_RAS_PCIE_AER_FW_FIRST,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_PCIE_AER_FW_FIRST;
    Status = NVParamSet (
               NV_SI_RAS_PCIE_AER_FW_FIRST,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasPcieAerFwFirstEnabled = Value;

  Status = NVParamGet (
             NV_SI_RAS_BERT_ENABLED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_BERT_SUPPORT;
    Status = NVParamSet (
               NV_SI_RAS_BERT_ENABLED,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasBertEnabled = Value;

  Status = NVParamGet (
             NV_SI_RAS_SDEI_ENABLED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_SDEI_SUPPORT;
    Status = NVParamSet (
               NV_SI_RAS_SDEI_ENABLED,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasSdeiEnabled = Value;

  Status = NVParamGet (
             NV_SI_DDR_CE_RAS_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_DDR_CE_THRESHOLD;
    Status = NVParamSet (
               NV_SI_DDR_CE_RAS_THRESHOLD,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasDdrCeThreshold = Value;

  Status = NVParamGet (
             NV_SI_2P_CE_RAS_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_2P_CE_THRESHOLD;
    Status = NVParamSet (
               NV_SI_2P_CE_RAS_THRESHOLD,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->Ras2pCeThreshold = Value;

  Status = NVParamGet (
             NV_SI_CPM_CE_RAS_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_PROCESSOR_CE_THRESHOLD;
    Status = NVParamSet (
               NV_SI_CPM_CE_RAS_THRESHOLD,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasCpmCeThreshold = Value;

  Status = NVParamGet (
             NV_SI_LINK_ERR_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    Value  = RAS_DEFAULT_DDR_LINK_ERROR_THRESHOLD;
    Status = NVParamSet (
               NV_SI_LINK_ERR_THRESHOLD,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d NVParamSet() failed!\n", __func__, __LINE__));
      ASSERT_EFI_ERROR (Status);
      Value = 0;
    }
  }

  Configuration->RasLinkErrThreshold = Value;

  return EFI_SUCCESS;
}

EFI_STATUS
RasConfigNvParamSet (
  IN RAS_CONFIG_VARSTORE_DATA  *Configuration
  )
{
  EFI_STATUS  Status;

  Status = NVParamSet (
             NV_SI_HARDWARE_EINJ,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasHardwareEinj
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_RAS_PCIE_AER_FW_FIRST,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasPcieAerFwFirstEnabled
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_RAS_BERT_ENABLED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasBertEnabled
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_RAS_SDEI_ENABLED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasSdeiEnabled
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_DDR_CE_RAS_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasDdrCeThreshold
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_2P_CE_RAS_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->Ras2pCeThreshold
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_CPM_CE_RAS_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasCpmCeThreshold
             );
  ASSERT_EFI_ERROR (Status);

  Status = NVParamSet (
             NV_SI_LINK_ERR_THRESHOLD,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             NV_PERM_BIOS | NV_PERM_MANU,
             Configuration->RasLinkErrThreshold
             );
  ASSERT_EFI_ERROR (Status);

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
RasConfigExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT      EFI_STRING                      *Progress,
  OUT      EFI_STRING                      *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  RAS_CONFIG_PRIVATE_DATA          *PrivateData;
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

  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mRasConfigFormSetGuid, RasConfigVarstoreDataName)) {
    return EFI_NOT_FOUND;
  }

  PrivateData      = RAS_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get current setting from NVParam.
  //
  Status = RasConfigNvParamGet (&PrivateData->Configuration);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize    = sizeof (RAS_CONFIG_VARSTORE_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mRasConfigFormSetGuid, RasConfigVarstoreDataName, PrivateData->DriverHandle);
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
RasConfigRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT      EFI_STRING                      *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  RAS_CONFIG_PRIVATE_DATA          *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData      = RAS_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress        = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mRasConfigFormSetGuid, RasConfigVarstoreDataName)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get configuration data from NVParam
  //
  Status = RasConfigNvParamGet (&PrivateData->Configuration);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (RAS_CONFIG_VARSTORE_DATA);
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
  // Store configuration data back to NVParam
  //
  Status = RasConfigNvParamSet (&PrivateData->Configuration);
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
RasConfigCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN       EFI_BROWSER_ACTION              Action,
  IN       EFI_QUESTION_ID                 QuestionId,
  IN       UINT8                           Type,
  IN       EFI_IFR_TYPE_VALUE              *Value,
  OUT      EFI_BROWSER_ACTION_REQUEST      *ActionRequest
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

EFI_STATUS
UpdateRasConfigScreen (
  IN RAS_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS          Status;
  VOID                *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *EndLabel;

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
  if (StartLabel == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeOpCodeBuffer;
  }

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
  if (EndLabel == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeOpCodeBuffer;
  }

  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Create the numeric for DDR CE threshold
  //
  if (!IsDdrCeWindowEnabled ()) {
    HiiCreateNumericOpCode (
      StartOpCodeHandle,                              // Container for dynamic created opcodes
      0x8004,                                         // Question ID
      RAS_CONFIG_VARSTORE_ID,                         // VarStore ID
      (UINT16)RAS_DDR_CE_THRESHOLD_OFST,              // Offset in Buffer Storage
      STRING_TOKEN (STR_RAS_DDR_CE_THRESHOLD_PROMPT), // Question prompt text
      STRING_TOKEN (STR_RAS_DDR_CE_THRESHOLD_HELP),
      EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
      EFI_IFR_NUMERIC_SIZE_4,
      1,
      8192,
      1,
      NULL
      );
  }

  //
  // Create the numeric for 2P CE threshold
  //
  if (IsSlaveSocketActive ()) {
    HiiCreateNumericOpCode (
      StartOpCodeHandle,                             // Container for dynamic created opcodes
      0x8005,                                        // Question ID
      RAS_CONFIG_VARSTORE_ID,                        // VarStore ID
      (UINT16)RAS_2P_CE_THRESHOLD_OFST,              // Offset in Buffer Storage
      STRING_TOKEN (STR_RAS_2P_CE_THRESHOLD_PROMPT), // Question prompt text
      STRING_TOKEN (STR_RAS_2P_CE_THRESHOLD_HELP),
      EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED,
      EFI_IFR_NUMERIC_SIZE_4,
      1,
      8192,
      1,
      NULL
      );
  }

  Status = HiiUpdateForm (
             PrivateData->HiiHandle,  // HII handle
             &mRasConfigFormSetGuid,  // Formset GUID
             RAS_CONFIG_FORM_ID,      // Form ID
             StartOpCodeHandle,       // Label for where to insert opcodes
             EndOpCodeHandle          // Insert data
             );

FreeOpCodeBuffer:
  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}

EFI_STATUS
EFIAPI
RasConfigUnload (
  VOID
  )
{
  ASSERT (mPrivateData != NULL);

  if (mDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mRasConfigHiiVendorDevicePath,
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
RasConfigEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (RAS_CONFIG_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = RAS_CONFIG_PRIVATE_SIGNATURE;

  mPrivateData->ConfigAccess.ExtractConfig = RasConfigExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig   = RasConfigRouteConfig;
  mPrivateData->ConfigAccess.Callback      = RasConfigCallback;

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
                  &mRasConfigHiiVendorDevicePath,
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
                &mRasConfigFormSetGuid,
                mDriverHandle,
                RasConfigDxeStrings,
                RasConfigVfrBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mRasConfigHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &mPrivateData->ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle = HiiHandle;

  Status = UpdateRasConfigScreen (mPrivateData);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a %d Fail to update Memory Configuration screen \n",
      __func__,
      __LINE__
      ));
    RasConfigUnload ();
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return EFI_SUCCESS;
}
