/** @file

   Copyright (c) 2022 - 2025, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Initialize SA DXE Policy

  @param[in] ImageHandle          Image handle of this driver.

  @retval EFI_SUCCESS             Initialization complete.
  @retval EFI_UNSUPPORTED         The chipset is unsupported by this driver.
  @retval EFI_OUT_OF_RESOURCES    Do not have enough resources to initialize the driver.
  @retval EFI_DEVICE_ERROR        Device error, driver exits abnormally.
**/
EFI_STATUS
EFIAPI
SaPolicyInitDxe (
  IN EFI_HANDLE           ImageHandle
  );

/**
  Initilize Intel Silicon DXE Platform Policy

  @param[in] ImageHandle        Image handle of this driver.

  @retval EFI_SUCCESS           Initialization complete.
  @exception EFI_UNSUPPORTED    The chipset is unsupported by this driver.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.
  @retval EFI_DEVICE_ERROR      Device error, driver exits abnormally.
**/
EFI_STATUS
EFIAPI
SiliconPolicyInitDxe (
  IN EFI_HANDLE       ImageHandle
  );

/**
  Performs silicon late policy update.

  The meaning of Policy is defined by silicon code.
  It could be the raw data, a handle, a Protocol, etc.

  The input Policy must be returned by SiliconPolicyDoneLate().

  In FSP or non-FSP path, the board may use additional way to get
  the silicon policy data field based upon the input Policy.

  @param[in, out] Policy       Pointer to policy.

  @return the updated policy.
**/
VOID *
EFIAPI
SiliconPolicyUpdateLate (
  IN OUT VOID *Policy
  )
{
  EFI_STATUS           Status;

  Status = EFI_SUCCESS;

  //
  // SystemAgent Dxe Platform Policy Initialization
  //
  Status = SaPolicyInitDxe (gImageHandle);
  DEBUG ((DEBUG_INFO, "SystemAgent Dxe Platform Policy Initialization status: %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return NULL;
}
