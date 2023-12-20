/** @file
  This driver publishes a protocol that is used by the ACPI SMM Platform
  driver to notify the BMC of Power State transitions.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BmcAcpiSwChild.h"

/**
  This is the Stanalone MM driver entrypoint. This function intitializes
  the BMC ACPI SW Child protocol.

  @param[in] ImageHandle   ImageHandle of the loaded driver
  @param[in] SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS      If all services discovered.
  @retval Other            Failure in constructor.

**/
EFI_STATUS
EFIAPI
BmcAcpiSwChildMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return InitializeBmcAcpiSwChild ();
}
