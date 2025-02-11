/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  This function checks whether BMC is ready for transaction or not.

  @retval TRUE   The BMC is ready.
  @retval FALSE  The BMC is not ready.

**/
BOOLEAN
EFIAPI
PlatformBmcReady (
  VOID
  )
{
  //
  // This platform doesn't have a BMC READY GPIO line.
  //
  return TRUE;
}
