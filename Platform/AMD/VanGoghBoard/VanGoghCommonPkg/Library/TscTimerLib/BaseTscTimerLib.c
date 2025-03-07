/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TscTimerLibInternal.h"

/**  Get TSC frequency.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  return InternalCalculateTscFrequency ();
}
