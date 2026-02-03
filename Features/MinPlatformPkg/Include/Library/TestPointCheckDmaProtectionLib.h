/** @file TestPointDmaProtectionLib.h

  Library to provide VTD related implementation for TestPointCheckLib

  Copyright (c) Microsoft Corporation.
**/

#ifndef _TEST_POINT_CHECK_DMA_PROTECTION_LIB_H_
#define _TEST_POINT_CHECK_DMA_PROTECTION_LIB_H_

/**
  This service checks if VTD DMA protection is supported.

  @retval EFI_SUCCESS         DMA protection is supported.
  @retval other               DMA protection is nor supported.
**/
EFI_STATUS
EFIAPI
TestPointVtdEngine (
  VOID
  );

#endif
