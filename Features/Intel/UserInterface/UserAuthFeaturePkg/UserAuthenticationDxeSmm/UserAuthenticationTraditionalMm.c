/** @file
  Entry point of UserAuthenticationSmm.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UserAuthenticationSmm.h"

/**
  Main entry for this driver.

  @param[in] ImageHandle  Image handle this driver.
  @param[in] SystemTable  Pointer to SystemTable.

  @retval EFI_SUCESS  This function always complete successfully.

**/
EFI_STATUS
EFIAPI
UserAuthenticationMmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return PasswordSmmInit ();
}
