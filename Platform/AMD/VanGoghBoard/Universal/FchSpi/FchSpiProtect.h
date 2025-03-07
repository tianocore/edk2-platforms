/** @file
  Implements FchSpiProtect.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FCH_SPI_PROTECT_H_
#define FCH_SPI_PROTECT_H_

/**

   Fch Spi Protect Lock

   @param UINTN SpiMmioBase

**/
EFI_STATUS
EFIAPI
FchSpiProtect_Lock (
  IN UINTN  SpiMmioBase
  );

/**

   Fch Spi Protect UnLock

   @param UINTN SpiMmioBase

**/
EFI_STATUS
EFIAPI
FchSpiProtect_UnLock (
  IN UINTN  SpiMmioBase
  );

#endif
