/** @file
  BMC Configuration screen

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/BmcConfigHii.h>
#include <Guid/MdeModuleHii.h>
#include <IndustryStandard/Ipmi.h>
#include <IndustryStandard/IpmiNetFnApp.h>
#include <IndustryStandard/IpmiNetFnTransport.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "BmcConfigDxe.h"

//
// HII Handle for BMC Configuration package
//
EFI_HII_HANDLE  mHiiHandle;

/**
  Get BMC LAN Information of specific channel.

  @param[in]     BmcChannel            BMC Channel to retrieve LAN information.
  @param[out]    BmcIpAddress          Pointer to buffer to receive BMC IPv4 address.
  @param[out]    BmcSubnetMask         Pointer to buffer to receive BMC subnet mask.

  @retval EFI_SUCCESS                  The command byte stream was successfully submit to the device
                                       and a response was successfully received.
  @retval EFI_INVALID_PARAMETER        BmcIpAddress or BmcSubnetMask is NULL.
  @retval other                        Failed to get BMC LAN info.
**/
EFI_STATUS
EFIAPI
IpmiGetBmcLanInfo (
  IN  UINT8                 BmcChannel,
  OUT IPMI_LAN_IP_ADDRESS   *BmcIpAddress,
  OUT IPMI_LAN_SUBNET_MASK  *BmcSubnetMask
  )
{
  EFI_STATUS                                      Status;
  IPMI_GET_CHANNEL_INFO_REQUEST                   GetChannelInfoRequest;
  IPMI_GET_CHANNEL_INFO_RESPONSE                  GetChannelInfoResponse;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   GetConfigurationParametersRequest;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse;
  UINT32                                          ResponseSize;

  if ((BmcIpAddress == NULL) || (BmcSubnetMask == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get Channel Information
  //
  ZeroMem (&GetChannelInfoRequest, sizeof (GetChannelInfoRequest));
  GetChannelInfoRequest.ChannelNumber.Bits.ChannelNo = BmcChannel;
  ResponseSize                                       = sizeof (GetChannelInfoResponse);

  Status = IpmiGetChannelInfo (&GetChannelInfoRequest, &GetChannelInfoResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get BMC channel info\n", __func__));
    return Status;
  }

  //
  // Check for LAN interface
  //
  if (  EFI_ERROR (Status)
     || (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL)
     || (GetChannelInfoResponse.MediumType.Bits.ChannelMediumType != IPMI_CHANNEL_MEDIA_TYPE_802_3_LAN))
  {
    return EFI_NOT_FOUND;
  }

  GetConfigurationParametersResponse = AllocateZeroPool (
                                         sizeof (*GetConfigurationParametersResponse)
                                         + sizeof (IPMI_LAN_IP_ADDRESS)
                                         );
  if (GetConfigurationParametersResponse == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get LAN IP Address
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BmcChannel;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanIpAddress;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (IPMI_LAN_IP_ADDRESS);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN configuration parameter\n", __func__));
    goto Exit;
  }

  if (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    BmcIpAddress->IpAddress,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (IPMI_LAN_IP_ADDRESS)
    );

  //
  // Get Subnet Mask
  //
  ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
  GetConfigurationParametersRequest.ChannelNumber.Uint8 = BmcChannel;
  GetConfigurationParametersRequest.ParameterSelector   = IpmiLanSubnetMask;
  GetConfigurationParametersRequest.SetSelector         = 0;
  GetConfigurationParametersRequest.BlockSelector       = 0;

  ResponseSize = sizeof (*GetConfigurationParametersResponse) + sizeof (IPMI_LAN_SUBNET_MASK);

  Status = IpmiGetLanConfigurationParameters (&GetConfigurationParametersRequest, GetConfigurationParametersResponse, &ResponseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get the LAN configuration parameter\n", __func__));
    goto Exit;
  }

  if (GetChannelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  CopyMem (
    BmcSubnetMask->IpAddress,
    GetConfigurationParametersResponse->ParameterData,
    sizeof (IPMI_LAN_SUBNET_MASK)
    );

Exit:
  FreePool (GetConfigurationParametersResponse);
  return Status;
}

/**
  This function updates the BMC information.

  @param[in] VOID

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
UpdateBmcConfigForm (
  VOID
  )
{
  EFI_IFR_GUID_LABEL           *EndLabel;
  EFI_IFR_GUID_LABEL           *StartLabel;
  EFI_STATUS                   Status;
  IPMI_GET_DEVICE_ID_RESPONSE  DeviceId;
  IPMI_LAN_IP_ADDRESS          BmcIpAddress;
  IPMI_LAN_SUBNET_MASK         BmcSubnetMask;
  UINT16                       StrBuf[MAX_STRING_SIZE];
  UINT8                        BmcChannel;
  VOID                         *EndOpCodeHandle;
  VOID                         *StartOpCodeHandle;

  Status = IpmiGetDeviceId (&DeviceId);
  if (  !EFI_ERROR (Status)
     && (DeviceId.CompletionCode == IPMI_COMP_CODE_NORMAL))
  {
    //
    // Firmware Revision
    //
    UnicodeSPrint (
      StrBuf,
      sizeof (StrBuf),
      L"%d.%02d",
      DeviceId.FirmwareRev1.Bits.MajorFirmwareRev,
      BcdToDecimal8 (DeviceId.MinorFirmwareRev)
      );
    HiiSetString (mHiiHandle, STRING_TOKEN (STR_BMC_FIRMWARE_REV_VALUE), StrBuf, NULL);

    //
    // IPMI Version
    //
    UnicodeSPrint (
      StrBuf,
      sizeof (StrBuf),
      L"%d.%d",
      DeviceId.SpecificationVersion & 0x0F,
      (DeviceId.SpecificationVersion >> 4) & 0x0F
      );
    HiiSetString (mHiiHandle, STRING_TOKEN (STR_BMC_IPMI_VER_VALUE), StrBuf, NULL);
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

  for (BmcChannel = IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_1; BmcChannel < IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_11; BmcChannel++) {
    ZeroMem (&BmcIpAddress, sizeof (BmcIpAddress));
    ZeroMem (&BmcSubnetMask, sizeof (BmcSubnetMask));
    Status = IpmiGetBmcLanInfo (BmcChannel, &BmcIpAddress, &BmcSubnetMask);
    if (  EFI_ERROR (Status) || (BmcIpAddress.IpAddress[0] == 0)) {
      continue;
    }

    UnicodeSPrint (
      StrBuf,
      sizeof (StrBuf),
      L"%d.%d.%d.%d",
      BmcIpAddress.IpAddress[0],
      BmcIpAddress.IpAddress[1],
      BmcIpAddress.IpAddress[2],
      BmcIpAddress.IpAddress[3]
      );

    HiiCreateTextOpCode (
      StartOpCodeHandle,
      STRING_TOKEN (STR_BMC_IP_ADDRESS_LABEL),
      STRING_TOKEN (STR_BMC_IP_ADDRESS_LABEL),
      HiiSetString (mHiiHandle, 0, StrBuf, NULL)
      );

    HiiUpdateForm (
      mHiiHandle,                 // HII handle
      &gBmcConfigFormSetGuid,     // Formset GUID
      MAIN_FORM_ID,               // Form ID
      StartOpCodeHandle,          // Label for where to insert opcodes
      EndOpCodeHandle             // Insert data
      );

    break;
  }

  return Status;
}

/**
  The user Entry Point for the BMC Configuration driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BmcConfigEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle;

  Status       = EFI_SUCCESS;
  DriverHandle = NULL;

  //
  // Publish our HII data
  //
  mHiiHandle = HiiAddPackages (
                 &gBmcConfigFormSetGuid,
                 DriverHandle,
                 BmcConfigDxeStrings,
                 BmcConfigVfrBin,
                 NULL
                 );
  if (mHiiHandle == NULL) {
    ASSERT (mHiiHandle != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UpdateBmcConfigForm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Failed to update the BMC Configuration screen\n", __func__));
  }

  return Status;
}
