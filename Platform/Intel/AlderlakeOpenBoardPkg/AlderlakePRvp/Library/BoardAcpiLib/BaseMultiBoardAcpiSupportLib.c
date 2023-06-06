/** @file
  Multi-Board ACPI Support Library

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BoardAcpiTableLib.h>
#include <Library/MultiBoardAcpiSupportLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

#include <PlatformBoardId.h>

EFI_STATUS
EFIAPI
AdlPBoardUpdateAcpiTable (
  IN OUT EFI_ACPI_COMMON_HEADER       *Table,
  IN OUT EFI_ACPI_TABLE_VERSION       *Version
  );

BOARD_ACPI_TABLE_FUNC  mAdlPBoardAcpiTableFunc = {
  AdlPBoardUpdateAcpiTable
};

/**
  The constructor function to register mAdlPBoardAcpiTableFunc function.

  @retval     EFI_SUCCESS  This constructor always return EFI_SUCCESS.
                           It will ASSERT on errors.
**/
EFI_STATUS
EFIAPI
AdlPBaseMultiBoardAcpiSupportLibConstructor (
  VOID
  )
{
  UINT8  SkuType;
  SkuType = PcdGet8 (PcdSkuType);

  if (SkuType==AdlPSkuType) {
    DEBUG ((DEBUG_INFO, "SKU_ID: 0x%x\n", LibPcdGetSku()));
    return RegisterBoardAcpiTableFunc (&mAdlPBoardAcpiTableFunc);
  }
  return EFI_SUCCESS;
}

