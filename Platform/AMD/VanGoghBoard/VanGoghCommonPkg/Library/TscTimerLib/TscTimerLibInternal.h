/** @file
  Header file internal to ACPI TimerLib.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TSC_TIMER_LIB_INTERNAL_H_
#define TSC_TIMER_LIB_INTERNAL_H_

#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>

#include <Library/IoLib.h>
#define ACPI_MMIO_BASE   0xFED80000ul
#define PMIO_BASE        0x300  // DWORD
#define FCH_PMIOA_REG64  0x64   // AcpiPmTmrBlk

/**  Get TSC frequency.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  );

/**  Calculate TSC frequency.

  The TSC counting frequency is determined by comparing how far it counts
  during a 1ms period as determined by the ACPI timer. The ACPI timer is
  used because it counts at a known frequency.
  If ACPI I/O space not enabled, this function will enable it. Then the
  TSC is sampled, followed by waiting for 3579 clocks of the ACPI timer, or 1ms.
  The TSC is then sampled again. The difference multiplied by 1000 is the TSC
  frequency. There will be a small error because of the overhead of reading
  the ACPI timer. An attempt is made to determine and compensate for this error.

  @return The number of TSC counts per second.

**/
UINT64
InternalCalculateTscFrequency (
  VOID
  );

#endif
