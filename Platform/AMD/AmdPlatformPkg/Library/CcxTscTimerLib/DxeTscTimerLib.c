/** @file
  CCX TSC implements one instance of Timer Library.

  Copyright (C) 2008 - 2025, Advanced Micro Devices, Inc. All rights reserved
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include "TscTimerLibPrivate.h"

extern EFI_GUID  gAmdTscFrequencyGuid;

AMD_TSC_FREQUENCY_INFORMATION  mFrequencyInfo;

/**
  The constructor function determines the actual TSC frequency.

  First, Get TSC frequency from system configuration table with TSC frequency GUID,
  if the table is not found, install it.
  This function will always return EFI_SUCCESS.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval     EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeTscTimerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  AMD_TSC_FREQUENCY_INFORMATION  *FrequencyInfo;
  AMD_TSC_FREQUENCY_INFORMATION  *FrequencyInfoRt;

  FrequencyInfo = NULL;
  //
  // Get TSC frequency from system configuration table with TSC frequency GUID.
  //
  Status = EfiGetSystemConfigurationTable (&gAmdTscFrequencyGuid, (VOID **)&FrequencyInfo);
  if ((Status == EFI_SUCCESS) && (FrequencyInfo != NULL)) {
    if (FrequencyInfo->ApmInfoEdx.Field.TscInvariant == 0) {
      if (AsmReadMsr64 (MSR_PSTATE_0 + FrequencyInfo->CurrentPState) != FrequencyInfo->CurrentPStateReg) {
        InternalGetTscFrequencyInformation (FrequencyInfo);
      }
    }

    CopyMem (&mFrequencyInfo, FrequencyInfo, sizeof (AMD_TSC_FREQUENCY_INFORMATION));
    return EFI_SUCCESS;
  }

  //
  // TSC frequency GUID system configuration table is not found, install it.
  //
  Status = gBS->AllocatePool (EfiBootServicesData, sizeof (AMD_TSC_FREQUENCY_INFORMATION), (VOID **)&FrequencyInfo);
  ASSERT_EFI_ERROR (Status);
  if (FrequencyInfo == NULL) {
    return EFI_SUCCESS;
  }

  InternalGetTscFrequencyInformation (FrequencyInfo);

  FrequencyInfoRt = NULL;
  if (mFrequencyInfo.ApmInfoEdx.Field.TscInvariant == 0) {
    Status = gBS->AllocatePool (EfiACPIMemoryNVS, sizeof (AMD_TSC_FREQUENCY_INFORMATION), (VOID **)&FrequencyInfoRt);
    ASSERT_EFI_ERROR (Status);
    if (FrequencyInfoRt == NULL) {
      gBS->FreePool (FrequencyInfo);
      return EFI_SUCCESS;
    }
  }

  CopyMem (&mFrequencyInfo, FrequencyInfo, sizeof (AMD_TSC_FREQUENCY_INFORMATION));
  if (FrequencyInfoRt == NULL) {
    gBS->InstallConfigurationTable (&gAmdTscFrequencyGuid, FrequencyInfo);
  } else {
    CopyMem (FrequencyInfoRt, FrequencyInfo, sizeof (AMD_TSC_FREQUENCY_INFORMATION));
    gBS->InstallConfigurationTable (&gAmdTscFrequencyGuid, FrequencyInfoRt);
    gBS->FreePool (FrequencyInfo);
  }

  return EFI_SUCCESS;
}

/**
  Get TSC frequency.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  EFI_STATUS                     Status;
  AMD_TSC_FREQUENCY_INFORMATION  *FrequencyInfo;

  if (mFrequencyInfo.ApmInfoEdx.Field.TscInvariant == 0) {
    if (AsmReadMsr64 ((UINT32)MSR_PSTATE_0 + mFrequencyInfo.CurrentPState) != mFrequencyInfo.CurrentPStateReg) {
      Status = EfiGetSystemConfigurationTable (&gAmdTscFrequencyGuid, (VOID **)&FrequencyInfo);
      ASSERT_EFI_ERROR (Status);
      InternalGetTscFrequencyInformation (FrequencyInfo);
      CopyMem (&mFrequencyInfo, FrequencyInfo, sizeof (AMD_TSC_FREQUENCY_INFORMATION));
    }
  }

  return mFrequencyInfo.TscFrequency;
}
