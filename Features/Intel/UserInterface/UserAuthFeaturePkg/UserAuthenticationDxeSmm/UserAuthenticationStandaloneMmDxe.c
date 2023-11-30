/** @file
  This Driver mainly locks password variables.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include "UserAuthenticationVariable.h"

/**
  User Authentication Standalone Mm Dxe driver entry point.

  @param[in] ImageHandle  The image handle.
  @param[in] SystemTable  The system table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @return  other         Contain some other errors.

**/
EFI_STATUS
EFIAPI
UserAuthenticationStandaloneMmDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return LockPasswordVariable ();
}
