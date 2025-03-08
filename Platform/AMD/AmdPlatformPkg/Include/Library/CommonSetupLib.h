/** @file
  The header file for the setup common lib.

  Copyright (C) 2019 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef COMMON_SETUP_LIB_H_
#define COMMON_SETUP_LIB_H_

#include <Uefi.h>
#include <Universal/CommonSetupDxe/CommonSetupOptions.h>

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
  );

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
  );

/**
  Load default value for common setup configuration.

  @param[out] CommonSetupOptions Pointer to the structure of COMMON_SETUP_OPTIONS,
                                 this pointer must be allocated with sizeof(COMMON_SETUP_OPTIONS)
                                 before being called.

  @retval EFI_SUCCESS            The common setup options are successfully loaded to default value.

**/
EFI_STATUS
EFIAPI
LoadCommonSetupDefault (
  OUT COMMON_SETUP_OPTIONS  *CommonSetupOptions
  );

#endif // COMMON_SETUP_LIB_H_
