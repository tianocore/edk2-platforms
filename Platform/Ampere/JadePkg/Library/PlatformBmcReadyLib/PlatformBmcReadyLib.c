/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/GpioLib.h>
#include <Library/PcdLib.h>

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
  // The BMC is considered ready if its GPIO pin is set to a logic high level.
  //
  return GpioReadBit (FixedPcdGet8 (PcdBmcReadyGpio)) == 0x1;
}
