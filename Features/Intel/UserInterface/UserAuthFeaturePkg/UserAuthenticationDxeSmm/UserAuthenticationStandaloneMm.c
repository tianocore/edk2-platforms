/** @file
  Entry point of UserAuthenticationStandaloneMm.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UserAuthenticationSmm.h"

/**
  NULL implement for Lock password variables.
  Relies on UserAuthenticationVariableLockDxe to lock the password variables.

  @retval EFI_SUCCESS  Always return success.

**/
EFI_STATUS
LockPasswordVariable (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
UserAuthenticationMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return PasswordSmmInit ();
}
