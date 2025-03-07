/** @file
  Implements Tpm2InstanceLibAmdFTpmPei.C

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>
#include <Ppi/AmdPspFtpmPpi.h>
#include <Library/AmdPspFtpmLib.h>
#include <Guid/TpmInstance.h>

// gEfiTpmDeviceInstanceTpm20AmdFtpmGuid
#define TPM_DEVICE_INTERFACE_TPM20_AMD_FTPM  \
  {0x286bf25a, 0xc2c3, 0x408c, {0xb3, 0xb4, 0x25, 0xe6, 0x75, 0x8b, 0x73, 0x17}}

/**
 * @brief gAmdFtpmFactoryResetPpiGuid callback, disable TPM
 *
 * @param PeiServices
 * @param NotifyDesc
 * @param InvokePpi
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
AmdFtpmFactoryResetCallback (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN  VOID                       *InvokePpi
  )
{
  UINTN       Size;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "AmdFtpmFactoryResetCallback\n"));
  // gEfiTpmDeviceInstanceNoneGuid GUID value used for PcdTpmInstanceGuid to indicate TPM is disabled.
  Size   = sizeof (gEfiTpmDeviceInstanceNoneGuid);
  Status = PcdSetPtrS (PcdTpmInstanceGuid, &Size, &gEfiTpmDeviceInstanceNoneGuid);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mAmdFtpmFactoryResetPpiCallback = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gAmdPspFtpmFactoryResetPpiGuid,
  AmdFtpmFactoryResetCallback
};

/**
  This service enables the sending of commands to the FTPM.

  @param[in]      InputParameterBlockSize  Size of the FTPM input parameter block.
  @param[in]      InputParameterBlock      Pointer to the FTPM input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the FTPM output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the FTPM output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
FTpmSubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  PSP_FTPM_PPI  *AmdFtpmPpi;
  EFI_STATUS    Status;

  Status = PeiServicesLocatePpi (&gAmdPspFtpmPpiGuid, 0, NULL, (VOID **)&AmdFtpmPpi);
  if (Status == EFI_SUCCESS) {
    return AmdFtpmPpi->Execute (AmdFtpmPpi, InputParameterBlock, InputParameterBlockSize, OutputParameterBlock, OutputParameterBlockSize);
  }

  return Status;
}

/**
  This service requests use FTPM.

  @retval EFI_SUCCESS      Get the control of FTPM chip.
  @retval EFI_NOT_FOUND    FTPM not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
FTpmRequestUseTpm (
  VOID
  )
{
  if ((PcdGet8 (PcdAmdPspSystemTpmConfig) == SYSTEM_TPM_CONFIG_PSP_FTPM) ||
      (PcdGet8 (PcdAmdPspSystemTpmConfig) == SYSTEM_TPM_CONFIG_HSP_FTPM))
  {
    // @todo Test fTPM functionality
    DEBUG ((DEBUG_INFO, "fTPM is used\n"));
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

TPM2_DEVICE_INTERFACE  mFTpmInternalTpm2Device = {
  TPM_DEVICE_INTERFACE_TPM20_AMD_FTPM,
  FTpmSubmitCommand,
  FTpmRequestUseTpm,
};

/**
  The function register FTPM instance.

  @retval EFI_SUCCESS   FTPM instance is registered, or system dose not surpport registr FTPM instance
**/
EFI_STATUS
EFIAPI
Tpm2InstanceLibAmdFTpmConstructor (
  VOID
  )
{
  EFI_STATUS    Status;
  PSP_FTPM_PPI  *AmdFtpmPpi;

  if ((PcdGet8 (PcdAmdPspSystemTpmConfig) == SYSTEM_TPM_CONFIG_PSP_FTPM) ||
      (PcdGet8 (PcdAmdPspSystemTpmConfig) == SYSTEM_TPM_CONFIG_HSP_FTPM))
  {
    //
    // AMD HSP f-TPM for EDK2 Core Base, Get the HSP PSP TcgEvetLog before BIOS
    //
    Status = PeiServicesLocatePpi (&gAmdPspFtpmPpiGuid, 0, NULL, (VOID **)&AmdFtpmPpi);
    if (Status == EFI_SUCCESS) {
      Status = Tpm2RegisterTpm2DeviceLib (&mFTpmInternalTpm2Device);
      if (Status == EFI_UNSUPPORTED) {
        //
        // Unsupported means platform policy does not need this instance enabled.
        //
        return EFI_SUCCESS;
      }
    }

    PeiServicesNotifyPpi (&mAmdFtpmFactoryResetPpiCallback);
  }

  return EFI_SUCCESS;
}
