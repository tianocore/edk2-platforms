/** @file
  HII Config Access protocol implementation of TCG2 configuration module.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/PlatformInfoHob.h>
#include <Guid/TpmInstance.h>
#include <IndustryStandard/TpmPtp.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/MailboxInterfaceLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>

#include "Tcg2ConfigImpl.h"

//
// Mailbox message of TPM PPI Request
//
#define MAILBOX_MESSAGE_TPM_PPI_REQUEST_TYPE     0x6
#define MAILBOX_MESSAGE_TPM_PPI_REQUEST_SUBTYPE  0xA

#define EFI_TCG2_EVENT_LOG_FORMAT_ALL  (EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 | EFI_TCG2_EVENT_LOG_FORMAT_TCG_2)

TPM_INSTANCE_ID  mTpmInstanceId[TPM_DEVICE_MAX + 1] = TPM_INSTANCE_ID_LIST;

TCG2_CONFIG_PRIVATE_DATA  *mTcg2ConfigPrivateDate;
TCG2_CONFIG_PRIVATE_DATA  mTcg2ConfigPrivateDateTemplate = {
  TCG2_CONFIG_PRIVATE_DATA_SIGNATURE,
  {
    Tcg2ExtractConfig,
    Tcg2RouteConfig,
    Tcg2Callback
  }
};

HII_VENDOR_DEVICE_PATH  mTcg2HiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TCG2_CONFIG_FORM_SET_GUID
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

  @param[in]   This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]   Request           A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param[out]  Progress          On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param[out]  Results           A null-terminated Unicode string in
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
Tcg2ExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT      EFI_STRING                      *Progress,
  OUT      EFI_STRING                      *Results
  )
{
  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  Raise the PP Clear Request via Non-secure Doorbell.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    Others                Other errors as indicated.

 */
EFI_STATUS
RaiseTpm2PpClearRequest (
  VOID
  )
{
  EFI_STATUS                 Status;
  VOID                       *GuidHob;
  PLATFORM_INFO_HOB          *PlatformHob;
  PLATFORM_TPM2_PPI_REQUEST  *PpiRequest;
  MAILBOX_MESSAGE_DATA       Message;

  GuidHob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (GuidHob == NULL) {
    return EFI_DEVICE_ERROR;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
  PpiRequest  = PlatformHob->Tpm2Info.Tpm2CrbInterfaceParams.PpiRequest;

  //
  // Write the PPI request to the shared buffer "Current PPI Request"
  //
  PpiRequest->CurrentRequest = TCG2_PHYSICAL_PRESENCE_CLEAR;

  //
  // Ring the doorbell with the TPM-PPI message to register the PPI request
  //
  ZeroMem (&Message, sizeof (MAILBOX_MESSAGE_DATA));
  Message.Data = COMMON_MESSAGE_ENCODE (
                   MAILBOX_MESSAGE_TPM_PPI_REQUEST_TYPE,
                   MAILBOX_MESSAGE_TPM_PPI_REQUEST_SUBTYPE,
                   0
                   );
  Status = MailboxWrite (0, SMproDoorbellChannel2, &Message);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Raise PPI Request failed!\n"));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Perform TPM PP request.

  @param[in] PpRequest             Physical Presence request command.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    Others                Other errors as indicated.

**/
EFI_STATUS
SaveTcg2PpRequest (
  IN UINT8  PpRequest
  )
{
  EFI_STATUS  Status;

  switch (PpRequest) {
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      Status = RaiseTpm2PpClearRequest ();
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      break;

    default:
      // The PP Request is not implemented.
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Perform Tcg2 PCR Banks request request.

  @param[in] PCRBankIndex     PCR Bank Index.
  @param[in] Enable           Enable or disable this PCR Bank.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    Others                Other errors as indicated.

**/
EFI_STATUS
SaveTcg2PCRBanksRequest (
  IN UINTN    PCRBankIndex,
  IN BOOLEAN  Enable
  )
{
  EFI_STATUS                       Status;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  TpmHashAlgorithmBitmap;
  UINT32                           ActivePcrBanks;

  if (Enable) {
    mTcg2ConfigPrivateDate->PCRBanksDesired |= (0x1 << PCRBankIndex);
  } else {
    mTcg2ConfigPrivateDate->PCRBanksDesired &= ~(0x1 << PCRBankIndex);
  }

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &ActivePcrBanks);
  ASSERT_EFI_ERROR (Status);

  //
  // PP spec requirements:
  //    Firmware should check that all requested (set) hashing algorithms are supported with respective PCR banks.
  //    Firmware has to ensure that at least one PCR banks is active.
  // If not, an error is returned and no action is taken.
  //
  if (  (mTcg2ConfigPrivateDate->PCRBanksDesired == 0)
     || ((mTcg2ConfigPrivateDate->PCRBanksDesired & (~TpmHashAlgorithmBitmap)) != 0))
  {
    DEBUG ((DEBUG_ERROR, "PCR banks %x to allocate are not supported by TPM. Skip operation\n", mTcg2ConfigPrivateDate->PCRBanksDesired));
    return EFI_DEVICE_ERROR;
  }

  Status = Tpm2PcrAllocateBanks (NULL, TpmHashAlgorithmBitmap, mTcg2ConfigPrivateDate->PCRBanksDesired);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param[out] Progress           A pointer to a string filled in with the offset of
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
Tcg2RouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT      EFI_STRING                      *Progress
  )
{
  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  return EFI_NOT_FOUND;
}

/**
  Get HID string of TPM2 ACPI device object

  @param[in]  Hid               Points to HID String Buffer.
  @param[in]  Size              HID String size in bytes. Must >= TPM_HID_ACPI_SIZE

  @return                       HID String get status.

**/
EFI_STATUS
GetTpm2HID (
  IN OUT CHAR8  *Hid,
  IN     UINTN  Size
  )
{
  EFI_STATUS  Status;
  UINT32      ManufacturerID;
  UINT32      FirmwareVersion1;
  UINT32      FirmwareVersion2;
  BOOLEAN     PnpHID;

  PnpHID = TRUE;

  ZeroMem (Hid, Size);

  //
  // Get Manufacturer ID
  //
  Status = Tpm2GetCapabilityManufactureID (&ManufacturerID);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "TPM_PT_MANUFACTURER 0x%08x\n", ManufacturerID));
    //
    // ManufacturerID defined in TCG Vendor ID Registry
    // may tailed with 0x00 or 0x20
    //
    if (((ManufacturerID >> 24) == 0x00) || ((ManufacturerID >> 24) == 0x20)) {
      //
      //  HID containing PNP ID "NNN####"
      //   NNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem (Hid, &ManufacturerID, 3);
    } else {
      //
      //  HID containing ACP ID "NNNN####"
      //   NNNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem (Hid, &ManufacturerID, 4);
      PnpHID = FALSE;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_MANUFACTURER failed %x!\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  Status = Tpm2GetCapabilityFirmwareVersion (&FirmwareVersion1, &FirmwareVersion2);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_1 0x%x\n", FirmwareVersion1));
    DEBUG ((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_2 0x%x\n", FirmwareVersion2));
    //
    //   #### is Firmware Version 1
    //
    if (PnpHID) {
      AsciiSPrint (Hid + 3, TPM_HID_PNP_SIZE - 3, "%02d%02d", ((FirmwareVersion1 & 0xFFFF0000) >> 16), (FirmwareVersion1 & 0x0000FFFF));
    } else {
      AsciiSPrint (Hid + 4, TPM_HID_ACPI_SIZE - 4, "%02d%02d", ((FirmwareVersion1 & 0xFFFF0000) >> 16), (FirmwareVersion1 & 0x0000FFFF));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_FIRMWARE_VERSION_X failed %x!\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration
  for TCG2 version information.

  @param[in] Action             Specifies the type of action taken by the browser.
                                ASSERT if the Action is not EFI_BROWSER_ACTION_SUBMITTED.
  @param[in] QuestionId         A unique value which is sent to the original
                                exporting driver so that it can identify the type
                                of data to expect.
  @param[in] Type               The type of value for the question.
  @param[in] Value              A pointer to the data being sent to the original
                                exporting driver.

  @retval EFI_SUCCESS           The callback successfully handled the action.

**/
EFI_STATUS
Tcg2VersionInfoCallback (
  IN EFI_BROWSER_ACTION  Action,
  IN EFI_QUESTION_ID     QuestionId,
  IN UINT8               Type,
  IN EFI_IFR_TYPE_VALUE  *Value
  )
{
  EFI_INPUT_KEY  Key;
  UINT64         PcdTcg2PpiVersion;
  UINT8          PcdTpm2AcpiTableRev;

  ASSERT (Action == EFI_BROWSER_ACTION_SUBMITTED);

  if (QuestionId == KEY_TCG2_PPI_VERSION) {
    //
    // Get the PCD value after EFI_BROWSER_ACTION_SUBMITTED,
    // the SetVariable to TCG2_VERSION_NAME should have been done.
    // If the PCD value is not equal to the value set to variable,
    // the PCD is not DynamicHii type and does not map to the setup option.
    //
    PcdTcg2PpiVersion = 0;
    CopyMem (
      &PcdTcg2PpiVersion,
      PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer),
      AsciiStrSize ((CHAR8 *)PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer))
      );
    if (PcdTcg2PpiVersion != Value->u64) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"WARNING: PcdTcgPhysicalPresenceInterfaceVer is not DynamicHii type and does not map to this option!",
        L"The version configuring by this setup option will not work!",
        NULL
        );
    }
  } else if (QuestionId == KEY_TPM2_ACPI_REVISION) {
    //
    // Get the PCD value after EFI_BROWSER_ACTION_SUBMITTED,
    // the SetVariable to TCG2_VERSION_NAME should have been done.
    // If the PCD value is not equal to the value set to variable,
    // the PCD is not DynamicHii type and does not map to the setup option.
    //
    PcdTpm2AcpiTableRev = PcdGet8 (PcdTpm2AcpiTableRev);

    if (PcdTpm2AcpiTableRev != Value->u8) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"WARNING: PcdTpm2AcpiTableRev is not DynamicHii type and does not map to this option!",
        L"The Revision configuring by this setup option will not work!",
        NULL
        );
    }
  }

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out] ActionRequest      On return, points to the action requested by the
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
Tcg2Callback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN       EFI_BROWSER_ACTION              Action,
  IN       EFI_QUESTION_ID                 QuestionId,
  IN       UINT8                           Type,
  IN       EFI_IFR_TYPE_VALUE              *Value,
  OUT      EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  EFI_STATUS                Status;
  CHAR8                     HidStr[16];
  CHAR16                    UnHidStr[16];
  TCG2_CONFIG_PRIVATE_DATA  *Private;

  if ((This == NULL) || (Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = TCG2_CONFIG_PRIVATE_DATA_FROM_THIS (This);

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    //
    // Update TPM2 HID info
    //
    if (QuestionId == KEY_TPM_DEVICE) {
      Status = GetTpm2HID (HidStr, 16);

      if (EFI_ERROR (Status)) {
        //
        //  Fail to get TPM2 HID
        //
        HiiSetString (Private->HiiHandle, STRING_TOKEN (STR_TPM2_ACPI_HID_CONTENT), L"Unknown", NULL);
      } else {
        AsciiStrToUnicodeStrS (HidStr, UnHidStr, 16);
        HiiSetString (Private->HiiHandle, STRING_TOKEN (STR_TPM2_ACPI_HID_CONTENT), UnHidStr, NULL);
      }
    }

    return EFI_SUCCESS;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if (QuestionId == KEY_TPM_DEVICE) {
      return EFI_SUCCESS;
    }

    if (QuestionId == KEY_TPM2_OPERATION) {
      return SaveTcg2PpRequest (Value->u8);
    }

    if ((QuestionId >= KEY_TPM2_PCR_BANKS_REQUEST_0) && (QuestionId <= KEY_TPM2_PCR_BANKS_REQUEST_4)) {
      return SaveTcg2PCRBanksRequest (QuestionId - KEY_TPM2_PCR_BANKS_REQUEST_0, Value->b);
    }
  }

  if (Action == EFI_BROWSER_ACTION_SUBMITTED) {
    if ((QuestionId == KEY_TCG2_PPI_VERSION) || (QuestionId == KEY_TPM2_ACPI_REVISION)) {
      return Tcg2VersionInfoCallback (Action, QuestionId, Type, Value);
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Append Buffer With TpmAlgHash.

  @param[in] Buffer               Buffer to be appended.
  @param[in] BufferSize           Size of buffer.
  @param[in] TpmAlgHash           TpmAlgHash.

**/
VOID
AppendBufferWithTpmAlgHash (
  IN UINT16  *Buffer,
  IN UINTN   BufferSize,
  IN UINT32  TpmAlgHash
  )
{
  switch (TpmAlgHash) {
    case TPM_ALG_SHA1:
      if (Buffer[0] != 0) {
        StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
      }

      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA1");
      break;

    case TPM_ALG_SHA256:
      if (Buffer[0] != 0) {
        StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
      }

      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA256");
      break;

    case TPM_ALG_SHA384:
      if (Buffer[0] != 0) {
        StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
      }

      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA384");
      break;

    case TPM_ALG_SHA512:
      if (Buffer[0] != 0) {
        StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
      }

      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA512");
      break;

    case TPM_ALG_SM3_256:
      if (Buffer[0] != 0) {
        StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
      }

      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SM3_256");
      break;
  }
}

/**
  Fill Buffer With BootHashAlg.

  @param[in] Buffer               Buffer to be filled.
  @param[in] BufferSize           Size of buffer.
  @param[in] BootHashAlg          BootHashAlg.

**/
VOID
FillBufferWithBootHashAlg (
  IN UINT16  *Buffer,
  IN UINTN   BufferSize,
  IN UINT32  BootHashAlg
  )
{
  Buffer[0] = 0;
  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA1) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA1");
  }

  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA256) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA256");
  }

  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA384) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA384");
  }

  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA512) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA512");
  }

  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SM3_256) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"SM3_256");
  }
}

/**
  Set ConfigInfo according to TpmAlgHash.

  @param[in,out] Tcg2ConfigInfo       TCG2 config info.
  @param[in]     TpmAlgHash           TpmAlgHash.

**/
VOID
SetConfigInfo (
  IN OUT TCG2_CONFIGURATION_INFO  *Tcg2ConfigInfo,
  IN     UINT32                   TpmAlgHash
  )
{
  switch (TpmAlgHash) {
    case TPM_ALG_SHA1:
      Tcg2ConfigInfo->Sha1Supported = TRUE;
      break;

    case TPM_ALG_SHA256:
      Tcg2ConfigInfo->Sha256Supported = TRUE;
      break;

    case TPM_ALG_SHA384:
      Tcg2ConfigInfo->Sha384Supported = TRUE;
      break;

    case TPM_ALG_SHA512:
      Tcg2ConfigInfo->Sha512Supported = TRUE;
      break;

    case TPM_ALG_SM3_256:
      Tcg2ConfigInfo->Sm3Supported = TRUE;
      break;
  }
}

/**
  Fill Buffer With TCG2EventLogFormat.

  @param[in] Buffer               Buffer to be filled.
  @param[in] BufferSize           Size of buffer.
  @param[in] TCG2EventLogFormat   TCG2EventLogFormat.

**/
VOID
FillBufferWithTCG2EventLogFormat (
  IN UINT16  *Buffer,
  IN UINTN   BufferSize,
  IN UINT32  TCG2EventLogFormat
  )
{
  Buffer[0] = 0;
  if ((TCG2EventLogFormat & EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"TCG_1_2");
  }

  if ((TCG2EventLogFormat & EFI_TCG2_EVENT_LOG_FORMAT_TCG_2) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"TCG_2");
  }

  if ((TCG2EventLogFormat & (~EFI_TCG2_EVENT_LOG_FORMAT_ALL)) != 0) {
    if (Buffer[0] != 0) {
      StrCatS (Buffer, BufferSize / sizeof (CHAR16), L", ");
    }

    StrCatS (Buffer, BufferSize / sizeof (CHAR16), L"UNKNOWN");
  }
}

/**
  This function publish the TCG2 configuration Form for TPM device.

  @param[in, out]  PrivateData   Points to TCG2 configuration private data.

  @retval EFI_SUCCESS            HII Form is installed for this network device.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallTcg2ConfigForm (
  IN OUT TCG2_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  UINTN                           Index;
  TPML_PCR_SELECTION              Pcrs;
  CHAR16                          TempBuffer[1024];
  TCG2_CONFIGURATION_INFO         Tcg2ConfigInfo;

  DriverHandle = NULL;
  ConfigAccess = &PrivateData->ConfigAccess;
  Status       = gBS->InstallMultipleProtocolInterfaces (
                        &DriverHandle,
                        &gEfiDevicePathProtocolGuid,
                        &mTcg2HiiVendorDevicePath,
                        &gEfiHiiConfigAccessProtocolGuid,
                        ConfigAccess,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->DriverHandle = DriverHandle;

  //
  // Publish the HII package list
  //
  HiiHandle = HiiAddPackages (
                &gTcg2ConfigFormSetGuid,
                DriverHandle,
                Tcg2ConfigDxeStrings,
                Tcg2ConfigBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mTcg2HiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           ConfigAccess,
           NULL
           );

    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle = HiiHandle;

  //
  // Update static data
  //
  switch (PrivateData->TpmDeviceDetected) {
    case TPM_DEVICE_NULL:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_DEVICE_STATE_CONTENT), L"Not Found", NULL);
      break;

    case TPM_DEVICE_2_0_DTPM:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_DEVICE_STATE_CONTENT), L"TPM 2.0", NULL);
      break;

    default:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_DEVICE_STATE_CONTENT), L"Unknown", NULL);
      break;
  }

  ZeroMem (&Tcg2ConfigInfo, sizeof (Tcg2ConfigInfo));
  Status = Tpm2GetCapabilityPcrs (&Pcrs);
  if (EFI_ERROR (Status)) {
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM2_ACTIVE_HASH_ALGO_CONTENT), L"[Unknown]", NULL);
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM2_SUPPORTED_HASH_ALGO_CONTENT), L"[Unknown]", NULL);
  } else {
    TempBuffer[0] = 0;
    for (Index = 0; Index < Pcrs.count; Index++) {
      if (!IsZeroBuffer (Pcrs.pcrSelections[Index].pcrSelect, Pcrs.pcrSelections[Index].sizeofSelect)) {
        AppendBufferWithTpmAlgHash (TempBuffer, sizeof (TempBuffer), Pcrs.pcrSelections[Index].hash);
      }
    }

    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM2_ACTIVE_HASH_ALGO_CONTENT), TempBuffer, NULL);

    TempBuffer[0] = 0;
    for (Index = 0; Index < Pcrs.count; Index++) {
      AppendBufferWithTpmAlgHash (TempBuffer, sizeof (TempBuffer), Pcrs.pcrSelections[Index].hash);
      SetConfigInfo (&Tcg2ConfigInfo, Pcrs.pcrSelections[Index].hash);
    }

    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM2_SUPPORTED_HASH_ALGO_CONTENT), TempBuffer, NULL);
  }

  FillBufferWithBootHashAlg (TempBuffer, sizeof (TempBuffer), PcdGet32 (PcdTcg2HashAlgorithmBitmap));
  HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_BIOS_HASH_ALGO_CONTENT), TempBuffer, NULL);

  //
  // Tcg2 Capability
  //
  FillBufferWithTCG2EventLogFormat (TempBuffer, sizeof (TempBuffer), PrivateData->ProtocolCapability.SupportedEventLogs);
  HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_SUPPORTED_EVENT_LOG_FORMAT_CONTENT), TempBuffer, NULL);

  FillBufferWithBootHashAlg (TempBuffer, sizeof (TempBuffer), PrivateData->ProtocolCapability.HashAlgorithmBitmap);
  HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_HASH_ALGO_BITMAP_CONTENT), TempBuffer, NULL);

  UnicodeSPrint (TempBuffer, sizeof (TempBuffer), L"%d", PrivateData->ProtocolCapability.NumberOfPCRBanks);
  HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_NUMBER_OF_PCR_BANKS_CONTENT), TempBuffer, NULL);

  FillBufferWithBootHashAlg (TempBuffer, sizeof (TempBuffer), PrivateData->ProtocolCapability.ActivePcrBanks);
  HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_ACTIVE_PCR_BANKS_CONTENT), TempBuffer, NULL);

  //
  // Update TPM device interface type
  //
  if (PrivateData->TpmDeviceDetected == TPM_DEVICE_2_0_DTPM) {
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_DEVICE_INTERFACE_STATE_CONTENT), L"PTP CRB", NULL);
  }

  //
  // Set ConfigInfo, to control the check box.
  //
  Status = gRT->SetVariable (
                  TCG2_STORAGE_INFO_NAME,
                  &gTcg2ConfigFormSetGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (Tcg2ConfigInfo),
                  &Tcg2ConfigInfo
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tcg2ConfigDriver: Fail to set TCG2_STORAGE_INFO_NAME\n"));
  }

  return EFI_SUCCESS;
}

/**
  This function removes TCG2 configuration Form.

  @param[in, out]  PrivateData   Points to TCG2 configuration private data.

**/
VOID
UninstallTcg2ConfigForm (
  IN OUT TCG2_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  //
  // Uninstall HII package list
  //
  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }

  //
  // Uninstall HII Config Access Protocol
  //
  if (PrivateData->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           PrivateData->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mTcg2HiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &PrivateData->ConfigAccess,
           NULL
           );
    PrivateData->DriverHandle = NULL;
  }

  FreePool (PrivateData);
}
