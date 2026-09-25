/** @file

  FV block I/O protocol driver for RDE Response Table in BIOS directory.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "SpiFvbRdeResponse.h"

STATIC EFI_HANDLE  mSpiFvbHandle;

/**
  SPI firmware volume driver EntryPoint.

  @param[in] ImageHandle    Driver Image Handle
  @param[in] SystemTable    System Table

  @retval EFI_SUCCESS       Driver initialization succeeded
  @retval all others        Driver initialization failed

**/
EFI_STATUS
EFIAPI
SpiFvbRdeResponseDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  // Retrieve SPI NOR flash driver
  Status = gBS->LocateProtocol (
                  &gEfiSpiNorFlashProtocolGuid,
                  NULL,
                  (VOID **)&mSpiNorFlashProtocol
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InitFvbRdeResponse ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a - Failed to initial FVB parameters for RDE Response (Status = %r)\n", __func__, Status));
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &mSpiFvbHandle,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSpiFvbProtocol
                  );
  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
