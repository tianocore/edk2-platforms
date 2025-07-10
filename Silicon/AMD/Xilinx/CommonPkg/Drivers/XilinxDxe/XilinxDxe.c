/**
 * @file
 *
 * Platform initialization driver used during DXE phase.
 *
 * Copyright (c) 2025, Linaro Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 */

#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
XilinxEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "Registering SDHCI device\n"));

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeSdhci,
             NonDiscoverableDeviceDmaTypeNonCoherent,
             NULL,
             NULL,
             1,
             PcdGet64 (PcdSdhciBase),
             SIZE_4KB
           );

  DEBUG((DEBUG_INFO, "SDHCI Registration Status: %r\n", Status));

  return Status;
}
