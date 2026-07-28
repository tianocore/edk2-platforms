/** @file
  GetTimerCountus implementation - required for DTFramework.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>

#define MICROSECONDS_PER_SECOND  1000000ULL

/**
  Returns the current performance counter value in microseconds.

  Retrieves the performance counter frequency on the first call and caches
  it for subsequent calls. Validates that the frequency is within an
  acceptable range and that the counter is an up-counter before performing
  the conversion.

  @retval  0    The frequency is invalid or the counter is not supported.
  @retval  !0   The current performance counter value in microseconds.
**/
UINT64
GetTimerCountus (
  VOID
  )
{
  UINT32  Factor;
  UINT64  Frequency;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);
  Factor    = (UINT32)(Frequency / MICROSECONDS_PER_SECOND);
  if (Factor == 0) {
    return 0;
  }

  return DivU64x32 (GetPerformanceCounter (), Factor);
}

