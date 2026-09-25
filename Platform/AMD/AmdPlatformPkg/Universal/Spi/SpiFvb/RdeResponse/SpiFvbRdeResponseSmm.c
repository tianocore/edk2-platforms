/** @file

  FV block I/O protocol driver for RDE Response Table in BIOS Directory.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include "SpiFvbRdeResponse.h"

STATIC EFI_HANDLE  mSpiFvbHandle;

/**
  SPI firmware volume SMM driver EntryPoint.

  @param[in] ImageHandle    Driver Image Handle
  @param[in] MmSystemTable  MM System Table

  @retval EFI_SUCCESS           Driver initialization succeeded
  @retval all others            Driver initialization failed

**/
EFI_STATUS
EFIAPI
SpiFvbRdeResponseSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  // Retrieve SPI NOR flash driver
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSpiSmmNorFlashProtocolGuid,
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

  mSpiFvbHandle = NULL;
  Status        = gSmst->SmmInstallProtocolInterface (
                           &mSpiFvbHandle,
                           &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                           EFI_NATIVE_INTERFACE,
                           &mSpiFvbProtocol
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to install gEfiSmmFirmwareVolumeBlockProtocolGuid\n", __func__));
    return Status;
  }

  Status = gSmst->SmmInstallProtocolInterface (
                    &mSpiFvbHandle,
                    &gEfiRdeResponseFvbReadySmmProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );

  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
