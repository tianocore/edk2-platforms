/** @file
  CCX TSC implements one instance of Timer Library.

  Copyright (C) 2008 - 2025, Advanced Micro Devices, Inc. All rights reserved
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/HobLib.h>
#include "TscTimerLibPrivate.h"

extern EFI_GUID  gAmdTscFrequencyGuid;

/**
  Get TSC frequency from TSC frequency GUID HOB, if the HOB is not found, build it.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  EFI_HOB_GUID_TYPE              *GuidHob;
  VOID                           *DataInHob;
  AMD_TSC_FREQUENCY_INFORMATION  FrequencyInfoData;
  AMD_TSC_FREQUENCY_INFORMATION  *FrequencyInfo;

  FrequencyInfo = &FrequencyInfoData;

  //
  // Get TSC frequency from TSC frequency GUID HOB.
  //
  GuidHob = GetFirstGuidHob (&gAmdTscFrequencyGuid);
  if (GuidHob != NULL) {
    FrequencyInfo = (AMD_TSC_FREQUENCY_INFORMATION *)GET_GUID_HOB_DATA (GuidHob);
    if (FrequencyInfo->ApmInfoEdx.Field.TscInvariant != 0) {
      return FrequencyInfo->TscFrequency;
    } else if (AsmReadMsr64 (MSR_PSTATE_0 + FrequencyInfo->CurrentPState) == FrequencyInfo->CurrentPStateReg) {
      return FrequencyInfo->TscFrequency;
    }
  } else {
    //
    // TSC frequency GUID HOB is not found, build it.
    //
    DataInHob = BuildGuidHob (
                  &gAmdTscFrequencyGuid,
                  sizeof (AMD_TSC_FREQUENCY_INFORMATION)
                  );
    if (DataInHob != NULL) {
      FrequencyInfo = (AMD_TSC_FREQUENCY_INFORMATION *)DataInHob;
    }
  }

  InternalGetTscFrequencyInformation (FrequencyInfo);

  return FrequencyInfo->TscFrequency;
}
