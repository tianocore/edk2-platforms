/** @file TestPointCheckDmaProtectionLibNull.c

  NULL instance of TestPointCheckDmaProtection Library.

  Copyright (c) Microsoft Corporation.
**/

#include <Uefi/UefiBaseType.h>

/**
  This service checks if VTD DMA protection is supported.

  @retval EFI_SUCCESS         DMA protection is supported.
  @retval other               DMA protection is nor supported.
**/
EFI_STATUS
EFIAPI
TestPointVtdEngine (
  VOID
  ) {
  return EFI_UNSUPPORTED;
}
