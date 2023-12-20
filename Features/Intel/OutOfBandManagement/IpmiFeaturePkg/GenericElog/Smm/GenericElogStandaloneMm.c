/** @file
  Generic Event Log functions of StandaloneMm GenericElog driver.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "GenericElog.h"

/**
  The Driver Entry Point.

  @param[in] ImageHandle    The image handle of this driver
  @param[in] SystemTable    The pointer of EFI_MM_SYSTEM_TABLE

  @retval EFI_SUCCESS       The driver initialized successfully

**/
EFI_STATUS
EFIAPI
InitializeSmElogLayerStandaloneMm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return InitializeSmElogLayer ();
}
