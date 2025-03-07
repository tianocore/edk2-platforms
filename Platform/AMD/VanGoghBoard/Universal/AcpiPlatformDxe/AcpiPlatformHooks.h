/** @file
  Sample ACPI Platform Driver

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_PLATFORM_HOOKS_H_
#define ACPI_PLATFORM_HOOKS_H_

//
// Statements that include other header files
//
#include <IndustryStandard/Acpi.h>
#include <Protocol/MpService.h>
#include <Library/UefiBootServicesTableLib.h>

#define AML_OPREGION_OP  0x80

/**
    Update the DSDT table.

    @param  TableHeader   The table to be set.

    @retval  EFI_SUCCESS  Update DSDT table sucessfully.

**/
EFI_STATUS
PatchDsdtTable (
  IN OUT   EFI_ACPI_DESCRIPTION_HEADER  *TableHeader
  );

/**
    Update the MADT table.

    @param  TableHeader   The table to be set.

    @retval  EFI_SUCCESS  Update MADT table successfully.

**/
EFI_STATUS
PatchMadtTable (
  IN OUT   EFI_ACPI_DESCRIPTION_HEADER  *TableHeader
  );

#endif
