/** @file
  Common Setup Library.

  Copyright (C) 2015 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/CommonSetupLib.h>

/**
  Set common setup configuration data in Variable, **ONLY** available in DXE.

  @param[in] CommonSetupOptions  Pointer to the structure of COMMON_SETUP_OPTIONS,
                                 this pointer must be allocated with sizeof(COMMON_SETUP_OPTIONS)
                                 before being called.

  @retval EFI_SUCCESS            The common setup options are successfully saved to Variable.
  @retval EFI_INVALID_PARAMETER  NULL pointer.
  @retval others                 Failed to save the struct to Variable.

**/
EFI_STATUS
EFIAPI
SetCommonSetupOptions (
  IN COMMON_SETUP_OPTIONS  *CommonSetupOptions
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if ((CommonSetupOptions == NULL) || (CommonSetupOptions->AllocatedSize != COMMON_SETUP_ALLOCATED_SIZE)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = sizeof (COMMON_SETUP_OPTIONS);
  Status     = gRT->SetVariable (
                      COMMON_SETUP_VARIABLE_NAME,
                      &gAmdPlatformSetupOptionsGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      BufferSize,
                      (VOID *)CommonSetupOptions
                      );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if (CommonSetupOptions == NULL) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = sizeof (COMMON_SETUP_OPTIONS);
  Status     = gRT->GetVariable (
                      COMMON_SETUP_VARIABLE_NAME,
                      &gAmdPlatformSetupOptionsGuid,
                      NULL,
                      &BufferSize,
                      (VOID *)CommonSetupOptions
                      );
  if ((Status == EFI_SUCCESS) && (BufferSize == sizeof (COMMON_SETUP_OPTIONS))) {
    if (CommonSetupOptions->AllocatedSize != COMMON_SETUP_ALLOCATED_SIZE) {
      Status = LoadCommonSetupDefault (CommonSetupOptions);
    }
  } else {
    Status = LoadCommonSetupDefault (CommonSetupOptions);
  }

  return Status;
}
