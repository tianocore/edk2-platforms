/** @file
  Qcom UFS HC DXE driver

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/UfsHostControllerPlatform.h>

//
// Ufs Host Controller Platform Protocol Instance
//
// Note here that it is assumed that the prior stage XBL has already
// initialized the UFS controller.
//
EDKII_UFS_HC_PLATFORM_PROTOCOL gQcomUfsHc = {
    EDKII_UFS_HC_PLATFORM_PROTOCOL_VERSION,
    NULL,
    NULL,
    EdkiiUfsCardRefClkFreqObsolete,
    TRUE,
    TRUE
};

/**
  The Entry Point of module. It follows the standard UEFI driver model.

  @param[in] ImageHandle       The firmware allocated handle for the EFI image.
  @param[in] SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval Other                Some error occurs when executing this entry point

**/
EFI_STATUS
EFIAPI
QcomUfsHcDriverEntry (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
{
  EFI_STATUS               Status;

  Status = gBS->InstallProtocolInterface (
             &ImageHandle,
             &gEdkiiUfsHcPlatformProtocolGuid,
             EFI_NATIVE_INTERFACE,
             &gQcomUfsHc
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install UFS platform protocol, error: %r \n",
      Status));
    return Status;
  }

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeUfs,
             NonDiscoverableDeviceDmaTypeNonCoherent,
             NULL,
             NULL,
             1,
             PcdGet32 (PcdQcomUfsHcDxeBaseAddress),
             PcdGet32 (PcdQcomUfsHcDxeSize)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to register UFS device, error: %r \n", Status));
  }

  return Status;
}
