/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include "TscTimerLibInternal.h"

UINT64  mTscFrequency;

/** The constructor function determines the actual TSC frequency.

  First, Get TSC frequency from system configuration table with TSC frequency GUID,
  if the table is not found, install it.
  This function will always return EFI_SUCCESS.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeTscTimerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT64      *TscFrequency;

  TscFrequency = NULL;
  //
  // Get TSC frequency from system configuration table with TSC frequency GUID.
  //
  Status = EfiGetSystemConfigurationTable (&gAmdCommonPkgTscFrequencyGuid, (VOID **)&TscFrequency);
  if (Status == EFI_SUCCESS) {
    ASSERT (TscFrequency != NULL);
    mTscFrequency = *TscFrequency;
    return EFI_SUCCESS;
  }

  //
  // TSC frequency GUID system configuration table is not found, install it.
  //

  Status = gBS->AllocatePool (EfiBootServicesData, sizeof (UINT64), (VOID **)&TscFrequency);
  ASSERT_EFI_ERROR (Status);

  *TscFrequency = InternalCalculateTscFrequency ();
  //
  // TscFrequency now points to the number of TSC counts per second, install system configuration table for it.
  //
  gBS->InstallConfigurationTable (&gAmdCommonPkgTscFrequencyGuid, TscFrequency);

  mTscFrequency = *TscFrequency;
  return EFI_SUCCESS;
}

/**  Get TSC frequency.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  return mTscFrequency;
}
