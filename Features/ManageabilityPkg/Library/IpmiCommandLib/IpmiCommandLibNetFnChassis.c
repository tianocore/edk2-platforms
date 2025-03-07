/** @file
  IPMI Command - NetFnChassis.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>

#include <IndustryStandard/Ipmi.h>

/**
  This function returns information about which main chassis management functions are
  present and  what addresses are used to access those functions.

  @param [out]  GetChassisCapabilitiesResponse  Pointer to IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisCapabilities (
  OUT IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE  *GetChassisCapabilitiesResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetChassisCapabilitiesResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_GET_CAPABILITIES,
               NULL,
               0,
               (VOID *)GetChassisCapabilitiesResponse,
               &DataSize
               );
  return Status;
}

/**
  This function gets  information regarding the high-level status of the system
  chassis and main power subsystem.

  @param [out]  GetChassisStatusResponse  Pointer to IPMI_GET_CHASSIS_STATUS_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisStatus (
  OUT IPMI_GET_CHASSIS_STATUS_RESPONSE  *GetChassisStatusResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetChassisStatusResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_GET_STATUS,
               NULL,
               0,
               (VOID *)GetChassisStatusResponse,
               &DataSize
               );
  return Status;
}

/**
  This function sends command to control power up, power down, and reset.

  @param [in]   ChassisControlRequest  Pointer to IPMI_CHASSIS_CONTROL_REQUEST.
  @param [out]  CompletionCode         IPMI completetion code, refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiChassisControl (
  IN IPMI_CHASSIS_CONTROL_REQUEST  *ChassisControlRequest,
  OUT UINT8                        *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_CONTROL,
               (VOID *)ChassisControlRequest,
               sizeof (*ChassisControlRequest),
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function is used to configure the power restore policy.

  @param [in]   ChassisControlRequest   Pointer to IPMI_SET_POWER_RESTORE_POLICY_REQUEST.
  @param [out]  ChassisControlResponse  Pointer to IPMI_SET_POWER_RESTORE_POLICY_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetPowerRestorePolicy (
  IN  IPMI_SET_POWER_RESTORE_POLICY_REQUEST   *ChassisControlRequest,
  OUT IPMI_SET_POWER_RESTORE_POLICY_RESPONSE  *ChassisControlResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*ChassisControlResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_SET_POWER_RESTORE_POLICY,
               (VOID *)ChassisControlRequest,
               sizeof (*ChassisControlRequest),
               (VOID *)ChassisControlResponse,
               &DataSize
               );
  return Status;
}

/**
  This function is used to set parameters that direct the system boot
  following a system power up or reset.

  @param [in]   BootOptionsRequest   Pointer to IPMI_SET_BOOT_OPTIONS_REQUEST.
  @param [out]  BootOptionsResponse  Pointer to IPMI_SET_BOOT_OPTIONS_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetSystemBootOptions (
  IN  IPMI_SET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_SET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  )
{
  EFI_STATUS  Status;
  UINT32      RequestDataSize;
  UINT32      ResponseDataSize;

  ResponseDataSize = sizeof (*BootOptionsResponse);
  RequestDataSize  = sizeof (*BootOptionsRequest);

  switch (BootOptionsRequest->ParameterValid.Bits.ParameterSelector) {
    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SET_IN_PROGRESS:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_0);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SELECTOR:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_1);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SCAN:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_2);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_BMC_BOOT_FLAG:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_3);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_INFO:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_6);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_MAILBOX:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7);
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IpmiSubmitCommand (
             IPMI_NETFN_CHASSIS,
             IPMI_CHASSIS_SET_SYSTEM_BOOT_OPTIONS,
             (VOID *)BootOptionsRequest,
             RequestDataSize,
             (VOID *)BootOptionsResponse,
             &ResponseDataSize
             );
  return Status;
}

/**
  This function is used to retrieve the boot options set by the
  Set System Boot Options command.

  @param [in]   BootOptionsRequest   Pointer to IPMI_GET_BOOT_OPTIONS_REQUEST.
  @param [out]  BootOptionsResponse  Pointer to IPMI_GET_BOOT_OPTIONS_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemBootOptions (
  IN  IPMI_GET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_GET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  )
{
  EFI_STATUS  Status;
  UINT32      ResponseDataSize;

  ResponseDataSize = sizeof (*BootOptionsResponse);

  switch (BootOptionsRequest->ParameterSelector.Bits.ParameterSelector) {
    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SET_IN_PROGRESS:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_0);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SELECTOR:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_1);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SCAN:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_2);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_BMC_BOOT_FLAG:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_3);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_INFO:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_6);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_MAILBOX:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7);
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IpmiSubmitCommand (
             IPMI_NETFN_CHASSIS,
             IPMI_CHASSIS_GET_SYSTEM_BOOT_OPTIONS,
             (VOID *)BootOptionsRequest,
             sizeof (*BootOptionsRequest),
             (VOID *)BootOptionsResponse,
             &ResponseDataSize
             );
  return Status;
}
