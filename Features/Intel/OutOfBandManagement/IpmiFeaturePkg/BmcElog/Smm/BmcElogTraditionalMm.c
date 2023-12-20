/** @file
  Entry Piont of Bmc Elog SMM Driver.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BmcElog.h"

/**
  InitializeBmcElogLayerSmm.

  @param[in] ImageHandle   ImageHandle of the loaded driver
  @param[in] SystemTable   Pointer to the System Table

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
InitializeSmBmcElogLayerSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return InitializeSmBmcElogLayer ();
}
