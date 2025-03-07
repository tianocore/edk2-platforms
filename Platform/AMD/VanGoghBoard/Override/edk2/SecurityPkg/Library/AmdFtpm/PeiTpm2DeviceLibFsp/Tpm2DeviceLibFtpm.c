/** @file
  This library is TPM2 device router. Platform can register multi TPM2 instance to it
  via PcdTpmInstanceGuid. Platform need make choice that which one will be final one.
  At most one TPM2 instance can be finally registered, and other will return unsupported.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2017, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Pi/PiPeiCis.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/AmdPspFtpmPpi.h>

/**
  The constructor function for this library.

  @param  None

  @retval EFI_SUCCESS   This library is ready for use.

**/

extern EFI_GUID  gAmdPspFtpmPpiGuid;

EFI_STATUS
EFIAPI
Tpm2DeviceLibConstructor (
  VOID
  )
{
  PSP_FTPM_PPI  *PspFtpmPpi;

  return (*GetPeiServicesTablePointer ())->LocatePpi (
                                             GetPeiServicesTablePointer (),
                                             &gAmdPspFtpmPpiGuid,
                                             0,
                                             NULL,
                                             (VOID **)&PspFtpmPpi
                                             );
}

/**
  This service enables the sending of commands to the TPM2.

  @param[in]  InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]  InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]  OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
Tpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if ((NULL == InputParameterBlock) || (NULL == OutputParameterBlock) || (0 == InputParameterBlockSize)) {
    DEBUG ((DEBUG_ERROR, "Buffer == NULL or InputParameterBlockSize == 0\n"));
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  PSP_FTPM_PPI  *PspFtpmPpi;

  Status = (*GetPeiServicesTablePointer ())->LocatePpi (
                                               GetPeiServicesTablePointer (),
                                               &gAmdPspFtpmPpiGuid,
                                               0,
                                               NULL,
                                               (VOID **)&PspFtpmPpi
                                               );
  if (!EFI_ERROR (Status)) {
    Status = PspFtpmPpi->Execute (
                           PspFtpmPpi,
                           (VOID *)InputParameterBlock,
                           InputParameterBlockSize,
                           (VOID *)OutputParameterBlock,
                           OutputParameterBlockSize
                           );
  }

  return Status;
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  UINTN         FtpmStatus;
  PSP_FTPM_PPI  *PspFtpmPpi;
  EFI_STATUS    Status = (*GetPeiServicesTablePointer ())->LocatePpi (
                                                             GetPeiServicesTablePointer (),
                                                             &gAmdPspFtpmPpiGuid,
                                                             0,
                                                             NULL,
                                                             (VOID **)&PspFtpmPpi
                                                             );

  return Status || PspFtpmPpi->CheckStatus (PspFtpmPpi, &FtpmStatus);
}

/**
  This service register TPM2 device.

  @Param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          This TPM2 device is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
  @retval EFI_ALREADY_STARTED  System already register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN TPM2_DEVICE_INTERFACE  *Tpm2Device
  )
{
  return EFI_UNSUPPORTED;
}
