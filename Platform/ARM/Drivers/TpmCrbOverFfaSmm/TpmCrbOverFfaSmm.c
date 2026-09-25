/** @file TpmCrbOverFfa StandaloneMm driver.

  Copyright (c) 2024-2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile
     - CRB - Command Response Buffer

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]
     - CRB over FF-A [https://developer.arm.com/documentation/den0138/latest/]

**/
#include <PiMm.h>

#include <Library/ArmFfaLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/TpmCrbFfaDeviceLib.h>

#include <Protocol/MmCommunication2.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/TpmPtp.h>

#include <Guid/Tpm2ServiceFfa.h>

/**
  Convert an EFI_STATUS value to a corresponding FF-A return code.

  This function maps EFI status codes to TPM2 FFA service response codes that
  are returned to the calling secure partition or normal world OS through the
  FFA direct messaging interface.

  @param[in]  Status                  The EFI_STATUS code to be converted.
  @param[in]  SuccessWithArgs         If success, return with result code.

  @retval TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED  If Status is EFI_SUCCESS and SuccessWithArgs == TRUE
  @retval TPM2_FFA_SUCCESS_OK                   If Status is EFI_SUCCESS and SuccessWithArgs == FALSE
  @retval others                                For all other status codes.
**/
STATIC
TPM2_FFA_STATUS
EfiStatusToTpm2FfaStatus (
  IN EFI_STATUS  Status,
  IN BOOLEAN     SuccessWithArgs
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      if (SuccessWithArgs) {
        return TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED;
      }

      return TPM2_FFA_SUCCESS_OK;
    case EFI_INVALID_PARAMETER:
      return TPM2_FFA_ERROR_INVARG;
    case EFI_UNSUPPORTED:
      return TPM2_FFA_ERROR_NOTSUP;
    case EFI_ACCESS_DENIED:
      return TPM2_FFA_ERROR_DENIED;
    case EFI_ALREADY_STARTED:
      return TPM2_FFA_ERROR_ALREADY;
    case EFI_DEVICE_ERROR:
      return TPM2_FFA_ERROR_INV_CRB_CTRL_DATA;
    default:
      DEBUG ((DEBUG_ERROR, "Not a valid status code: %r\n", Status));
      ASSERT (0);
      return TPM2_FFA_ERROR_INVARG;
  }
}

/**
  Set response data according to CRB over FF-A specificatiion.

  @param [in,out]  TpmArgs      Return arguments
  @param [in]      TpmStatus    Tpm Service Status
  @param [in]      Arg1
  @param [in]      Arg2
  @param [in]      Arg3

**/
STATIC
VOID
EFIAPI
SetResponseArgs (
  IN OUT ARM_FFA_ARGS     *TpmArgs,
  IN     TPM2_FFA_STATUS  TpmStatus,
  IN     UINTN            Arg1,
  IN     UINTN            Arg2,
  IN     UINTN            Arg3
  )
{
  ZeroMem (TpmArgs, sizeof (DIRECT_MSG_ARGS));

  TpmArgs->Arg4 = TpmStatus;
  TpmArgs->Arg5 = Arg1;
  TpmArgs->Arg6 = Arg2;
  TpmArgs->Arg7 = Arg3;
}

/**
  Return the version of the Tpm Service via FF-A interface that is available.

  See the CRB over FF-A spec 6.1.
  all of return values based on the specification.

  @param [in,out]  TpmArgs      Tpm service arguments

**/
STATIC
VOID
EFIAPI
TpmCrbOverFfaGetInterfaceVersion (
  IN OUT ARM_FFA_ARGS  *TpmArgs
  )
{
  UINTN  Version;

  Version = (1 << TPM2_FFA_SERVICE_MAJOR_VER_SHIFT) | (1 << TPM2_FFA_SERVICE_MINOR_VER_SHIFT);

  SetResponseArgs (TpmArgs, TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED, Version, 0x00, 0x00);
}

/**
  Return information on a given feature of the TPM service.

  See the CRB over FF-A spec 6.2.
  all of return values based on the specification.

  @param [in,out]  TpmArgs      Tpm service arguments

**/
STATIC
VOID
EFIAPI
TpmCrbOverFfaGetFeatureInfo (
  IN OUT ARM_FFA_ARGS  *TpmArgs
  )
{
  TPM2_FFA_STATUS  TpmStatus;

  switch (TpmArgs->Arg5) {
    case TPM_SERVICE_FEATURE_SUPPORT_NOTIFICATION:
      // Currently, StandaloneMm with TPM service doesn't support notification.
      TpmStatus = TPM2_FFA_ERROR_NOTSUP;
      break;
    default:
      TpmStatus = TPM2_FFA_ERROR_INVARG;
  }

  SetResponseArgs (TpmArgs, TpmStatus, 0x00, 0x00, 0x00);
}

/**
  Notifies the TPM service that a TPM command or TPM locality request
  is ready to be processed, and allows the TPM service to process it.

  See the CRB over FF-A spec 6.3.
  all of return values based on the specification.

  @param [in,out]  TpmArgs      Tpm service arguments

**/
STATIC
VOID
EFIAPI
TpmCrbOverFfaStart (
  IN OUT ARM_FFA_ARGS  *TpmArgs
  )
{
  EFI_STATUS       Status;
  TPM2_FFA_STATUS  TpmStatus;
  UINT8            Locality;

  Locality = (TpmArgs->Arg6 & TPM2_FFA_START_FUNC_LOCALITY_MASK);

  if (Locality > 4) {
    TpmStatus = TPM2_FFA_ERROR_DENIED;
    goto ErrorHandler;
  }

  Status    = TpmCrbFfaDeviceStart (TpmArgs->Arg5, Locality);
  TpmStatus = EfiStatusToTpm2FfaStatus (Status, FALSE);

ErrorHandler:
  SetResponseArgs (TpmArgs, TpmStatus, 0x00, 0x00, 0x00);
}

/**
  Return the control address of locality 0 and per-locality size of
  the CRB region.

  @param [in,out]  TpmArgs      Tpm service arguments

**/
STATIC
VOID
EFIAPI
TpmCrbOverFfaGetCrbInfo (
  IN OUT ARM_FFA_ARGS  *TpmArgs
  )
{
  EFI_STATUS            Status;
  TPM2_FFA_STATUS       TpmStatus;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINTN                 Size;
  UINTN                 SizeValue;

  Status    = TpmCrbFfaDeviceGetCrbInfo (0, &BaseAddress, &Size);
  TpmStatus = EfiStatusToTpm2FfaStatus (Status, TRUE);

  if (EFI_ERROR (Status)) {
    goto ErrorHandler;
  } else {
    BaseAddress += OFFSET_OF (PTP_CRB_REGISTERS, CrbControlRequest);
    switch (Size) {
      case SIZE_4KB:
        SizeValue = TPM2_FFA_CRB_REGION_SIZE_4K;
        break;
      case SIZE_16KB:
        SizeValue = TPM2_FFA_CRB_REGION_SIZE_16K;
        break;
      case SIZE_64KB:
        SizeValue = TPM2_FFA_CRB_REGION_SIZE_64K;
        break;
      default:
        TpmStatus = TPM2_FFA_ERROR_INVARG;
        goto ErrorHandler;
    }

    SetResponseArgs (TpmArgs, TpmStatus, BaseAddress, SizeValue, 0x00);
  }

  return;

ErrorHandler:
  SetResponseArgs (TpmArgs, TpmStatus, 0x00, 0x00, 0x00);
}

/**
  Parse the Tpm Servie reqeust via FF-A and
  Generate response for the request.

  @param  [in]     DispatchHandle   The unique handle assigned to this handler
                                    by MmiHandlerRegister().
  @param  [in]     Context          Points to an optional handler context which
                                    was specified when the handler was registered.
  @param  [in,out] CommBuffer       A pointer to a collection of data in memory
                                    that will be conveyed from a non-MM environment
                                    into an MM environment.
  @param  [in,out] CommBufferSize   The size of the CommBuffer.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER     Invalid Command Buffer.

**/
STATIC
EFI_STATUS
EFIAPI
TpmCrbOverFfaHandler (
  IN     EFI_HANDLE DispatchHandle,
  IN     CONST VOID *Context, OPTIONAL
  IN OUT VOID                     *CommBuffer,
  IN OUT UINTN                    *CommBufferSize
  )
{
  ARM_FFA_ARGS     *TpmArgs;
  UINTN            Operation;
  TPM2_FFA_STATUS  TpmStatus;

  if ((CommBufferSize == NULL) || (*CommBufferSize < sizeof (ARM_FFA_ARGS))) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TpmArgs   = CommBuffer;
  Operation = TpmArgs->Arg4;

  switch (Operation) {
    case TPM2_FFA_GET_INTERFACE_VERSION:
      TpmCrbOverFfaGetInterfaceVersion (TpmArgs);
      break;
    case TPM2_FFA_GET_FEATURE_INFO:
      TpmCrbOverFfaGetFeatureInfo (TpmArgs);
      break;
    case TPM2_FFA_START:
      TpmCrbOverFfaStart (TpmArgs);
      break;
    case TPM2_FFA_GET_CRB_INFO:
      TpmCrbOverFfaGetCrbInfo (TpmArgs);
      break;

    /*
     * Notification is based on NPI or SRI infra structure.
     * However, S-EL0 application couldn't register ISR according to FF-A
     * specification (See FF-A spec 14.4.1 Usage) and StandaloneMm is
     * typical S-EL0 application. Hence, return NOTSUP for notification requests.
     */
    case TPM2_FFA_REGISTER_FOR_NOTIFICATION:
    case TPM2_FFA_UNREGISTER_FROM_NOTIFICATION:
    case TPM2_FFA_FINISH_NOTIFIED:
      SetResponseArgs (TpmArgs, TPM2_FFA_ERROR_NOTSUP, 0x00, 0x00, 0x00);
      break;
    default:
      DEBUG ((DEBUG_ERROR, "Invalid function id... 0x%llx\n", Operation));
      ASSERT (0);
      SetResponseArgs (TpmArgs, TPM2_FFA_ERROR_INVARG, 0x00, 0x00, 0x00);
  }

  TpmStatus = TpmArgs->Arg4;

  if ((TpmStatus != TPM2_FFA_SUCCESS_OK) &&
      (TpmStatus != TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED))
  {
    DEBUG ((
      DEBUG_ERROR,
      "Failed for operation(0x%x). TpmStatus: 0x%x\n",
      Operation,
      TpmStatus
      ));
  }

  return EFI_SUCCESS;
}

/**
  The entry point of TpmCrbOverFfa Driver.

  @param  [in] ImageHandle    The image handle of the Standalone MM Driver.
  @param  [in] MmSystemTable  A pointer to the MM System Table.

  @retval EFI_SUCCESS
  @retval Others              Error.
**/
EFI_STATUS
EFIAPI
TpmCrbOverFfaDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DispatchHandle;

  if (!TpmCrbFfaDeviceIsAvailable ()) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: FF-A CRB device is not avaiable.\n",
      __func__
      ));
    return EFI_DEVICE_ERROR;
  }

  Status = gMmst->MmiHandlerRegister (
                    TpmCrbOverFfaHandler,
                    &gTpm2ServiceFfaGuid,
                    &DispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to register TPM CRB over FF-A Service... Status: %r\n",
      __func__,
      Status
      ));
  }

  return Status;
}
