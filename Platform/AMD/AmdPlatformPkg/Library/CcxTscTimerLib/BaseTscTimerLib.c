/** @file
  CCX TSC implements one instance of Timer Library.

  Copyright (C) 2008 - 2025, Advanced Micro Devices, Inc. All rights reserved
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TscTimerLibPrivate.h"

/**
  Get TSC frequency.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  AMD_TSC_FREQUENCY_INFORMATION  FrequencyInfo;

  InternalGetTscFrequencyInformation (&FrequencyInfo);
  return FrequencyInfo.TscFrequency;
}
