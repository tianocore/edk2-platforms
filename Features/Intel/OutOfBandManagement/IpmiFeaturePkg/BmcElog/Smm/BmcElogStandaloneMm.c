/** @file
  Entry Piont of Bmc Elog Standalone MM Driver.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BmcElog.h"

/**
  InitializeBmcElogLayerStandaloneMm.

  @param[in] ImageHandle   ImageHandle of the loaded driver
  @param[in] SystemTable   Pointer to the MM System Table

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
InitializeSmBmcElogLayerStandaloneMm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return InitializeSmBmcElogLayer ();
}
