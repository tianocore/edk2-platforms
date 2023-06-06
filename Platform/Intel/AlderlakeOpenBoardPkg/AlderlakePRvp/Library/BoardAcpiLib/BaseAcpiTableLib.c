/** @file
  Board ACPI Table Library

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BoardAcpiTableLib.h>

/**
  Update ACPI Table

  @param[in, out]  Table    Pointer to table, need to be update in Acpi table.
  @param[in, out]  Version  ACPI table version

  @retval     EFI_SUCCESS   The function always return successfully.
**/
EFI_STATUS
EFIAPI
AdlPBoardUpdateAcpiTable (
  IN OUT EFI_ACPI_COMMON_HEADER       *Table,
  IN OUT EFI_ACPI_TABLE_VERSION       *Version
  )
{
  return EFI_SUCCESS;
}

