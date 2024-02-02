/** @file
  SPI library header for abstraction of SPI HW registers accesses

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _SPI_ACCESS_LIB_H_
#define _SPI_ACCESS_LIB_H_


/**
  Returns SPI BAR0 value

  @retval  UINT32  PCH SPI BAR0 value
**/
UINT32
SpiGetBar0 (
  VOID
  );

/**
  Checks if PCH SPI Controler is present and available

  @retval TRUE    PCH SPI controller is avaialable
  @retval FALSE   PCH SPI controller is not available
**/
BOOLEAN
SpiIsControllerAvailable (
  VOID
  );

/**
  Checks if Device Attached Flash (DAF) mode is active

  @retval TRUE    SAF mode is active
  @retval FALSE   SAF mode is not active
**/
BOOLEAN
SpiIsSafModeActive (
  VOID
  );

#endif // _SPI_ACCESS_LIB_H_
