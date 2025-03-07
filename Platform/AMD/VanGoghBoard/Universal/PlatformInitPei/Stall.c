/** @file
  Implements Stall.C
  Produce Stall Ppi.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/Stall.h>
#include <Library/IoLib.h>
#include "CommonHeader.h"

#define B_FCH_ACPI_PM1_TMR_VAL      0xFFFFFF            // The timer value mask
#define V_FCH_ACPI_PM1_TMR_MAX_VAL  0x1000000           // The timer is 24 bit overflow

/**

  Waits for at least the given number of microseconds.

  @param PeiServices     General purpose services available to every PEIM.
  @param This            PPI instance structure.
  @param Microseconds    Desired length of time to wait.

  @retval EFI_SUCCESS    If the desired amount of time was passed.

*/
EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI  *This,
  IN UINTN                    Microseconds
  )
{
  UINTN   Ticks;
  UINTN   Counts;
  UINT16  AcpiTimerPort;
  UINT32  CurrentTick;
  UINT32  OriginalTick;
  UINT32  RemainingTick;

  if (Microseconds == 0) {
    return EFI_SUCCESS;
  }

  AcpiTimerPort = MmioRead16 (ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REG64);

  OriginalTick  = IoRead32 (AcpiTimerPort);
  OriginalTick &= (V_FCH_ACPI_PM1_TMR_MAX_VAL - 1);
  CurrentTick   = OriginalTick;

  //
  // The timer frequency is 3.579545MHz, so 1 ms corresponds to 3.58 clocks
  //
  Ticks = Microseconds * 358 / 100 + OriginalTick + 1;

  //
  // The loops needed for timer overflow
  //
  Counts = (UINTN)RShiftU64 ((UINT64)Ticks, 24);

  //
  // Remaining clocks within one loop
  //
  RemainingTick = Ticks & 0xFFFFFF;

  //
  // Do not intend to use TMROF_STS bit of register PM1_STS, because this add extra
  // one I/O operation, and may generate SMI
  //
  while (Counts != 0) {
    CurrentTick = IoRead32 (AcpiTimerPort) & B_FCH_ACPI_PM1_TMR_VAL;
    if (CurrentTick <= OriginalTick) {
      Counts--;
    }

    OriginalTick = CurrentTick;
  }

  while ((RemainingTick > CurrentTick) && (OriginalTick <= CurrentTick)) {
    OriginalTick = CurrentTick;
    CurrentTick  = IoRead32 (AcpiTimerPort) & B_FCH_ACPI_PM1_TMR_VAL;
  }

  return EFI_SUCCESS;
}
