/** @file
  This library abstract how to access TPM CRB over FF-A accesses hardware device.

Copyright (c) 2026, Arm ltd. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

/**
  Issue TPM command or locality request to FF-A CRB device.

  @param[in]      Qualifier                 Qualifier.
  @param[in]      Locality                  Locality.

  @retval EFI_SUCCESS               The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_INVALID_PARAMETER     Invalid arguments.
  @retval EFI_DEVICE_ERROR          CRB control data or locality conrol data is not valid.
  @retval EFI_ACCESS_DENIED         locality requests or command processing at given locality is disabled.

**/
EFI_STATUS
EFIAPI
TpmCrbFfaDeviceStart (
  IN UINT8  Qualifier,
  IN UINT8  Locality
  );

/**
  Get CRB information.

  @param[in]      Locality                     Locality.
  @param[out]     BaseAddress                  CRB base address.
  @param[out]     Size                         CRB region size.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER                Invalid arguments.

**/
EFI_STATUS
EFIAPI
TpmCrbFfaDeviceGetCrbInfo (
  IN UINT8                  Locality,
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINTN                 *Size
  );

/**
  Check whether FF-A CRB device is available

  @retval TRUE    Available.
  @retval FALSE   Unavailable.

**/
BOOLEAN
EFIAPI
TpmCrbFfaDeviceIsAvailable (
  VOID
  );
