/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/HobLib.h>
#include "TscTimerLibInternal.h"

/**  Get TSC frequency from TSC frequency GUID HOB, if the HOB is not found, build it.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;
  UINT64             TscFrequency;

  //
  // Get TSC frequency from TSC frequency GUID HOB.
  //
  GuidHob = GetFirstGuidHob (&gAmdCommonPkgTscFrequencyGuid);
  if (GuidHob != NULL) {
    DataInHob    = GET_GUID_HOB_DATA (GuidHob);
    TscFrequency = *(UINT64 *)DataInHob;
    return TscFrequency;
  }

  //
  // TSC frequency GUID HOB is not found, build it.
  //

  TscFrequency = InternalCalculateTscFrequency ();
  //
  // TscFrequency is now equal to the number of TSC counts per second, build GUID HOB for it.
  //
  BuildGuidDataHob (
    &gAmdCommonPkgTscFrequencyGuid,
    &TscFrequency,
    sizeof (UINT64)
    );

  return TscFrequency;
}
