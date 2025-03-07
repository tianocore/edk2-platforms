/** @file
  Implements FchSpiProtect.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

#include "FchSpiProtect.h"

/**

   Fch Spi Protect Lock

   @param SpiMmioBase

**/
EFI_STATUS
EFIAPI
FchSpiProtect_Lock (
  IN UINTN  SpiMmioBase
  )
{
  if (!(MmioRead8 (SpiMmioBase + 2) & 0xC0)) {
    // Check BIT7+BIT6
    return EFI_SUCCESS;
  } else {
    MmioWrite8 (SpiMmioBase + 9, 0x6);                                // PrefixOpCode WRITE_ENABLE
    MmioWrite8 (SpiMmioBase + 2, MmioRead8 (SpiMmioBase + 2) & 0x3F); // Clear BIT7+BIT6
    if (MmioRead8 (SpiMmioBase + 2) & 0xC0) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**

   Fch Spi Protect UnLock

   @param SpiMmioBase

**/
EFI_STATUS
EFIAPI
FchSpiProtect_UnLock (
  IN UINTN  SpiMmioBase
  )
{
  if ((MmioRead8 (SpiMmioBase + 2) & 0xC0) || (6 != MmioRead8 (SpiMmioBase + 9))) {
    return EFI_SUCCESS;
  } else {
    MmioWrite8 (SpiMmioBase + 9, 0x0);
    MmioWrite8 (SpiMmioBase + 2, MmioRead8 (SpiMmioBase + 2) | 0xC0); // Set BIT7+BIT6
  }

  return EFI_SUCCESS;
}
