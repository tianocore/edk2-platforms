/** @file
  Setup common lib for PEI phase.

  Copyright (C) 2015 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/CommonSetupLib.h>

/**
  Retrieve common setup configuration data.

  @param[out] CommonSetupOptions Pointer to the structure of COMMON_SETUP_OPTIONS,
                                 this pointer must be allocated with sizeof(COMMON_SETUP_OPTIONS)
                                 before being called.

  @retval EFI_SUCCESS            The common setup options are successfully retrieved.
  @retval EFI_INVALID_PARAMETER  NULL pointer.
  @return others                 Failed to retrieve common setup options.

**/
EFI_STATUS
EFIAPI
GetCommonSetupOptions (
  OUT COMMON_SETUP_OPTIONS  *CommonSetupOptions
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *VariablePpi;
  UINTN                            Size;

  if (CommonSetupOptions == NULL) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&VariablePpi
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Size   = sizeof (COMMON_SETUP_OPTIONS);
  Status = VariablePpi->GetVariable (
                          VariablePpi,
                          COMMON_SETUP_VARIABLE_NAME,
                          &gAmdPlatformSetupOptionsGuid,
                          NULL,
                          &Size,
                          CommonSetupOptions
                          );
  if ((Status == EFI_SUCCESS) && (Size == sizeof (COMMON_SETUP_OPTIONS))) {
    if (CommonSetupOptions->AllocatedSize != COMMON_SETUP_ALLOCATED_SIZE) {
      Status = LoadCommonSetupDefault (CommonSetupOptions);
    }
  } else {
    Status = LoadCommonSetupDefault (CommonSetupOptions);
  }

  return Status;
}
