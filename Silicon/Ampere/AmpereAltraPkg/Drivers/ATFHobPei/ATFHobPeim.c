/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Uefi/UefiBaseType.h>

VOID
BuildPlatformInformationHob (
  VOID
  )
{
  VOID  *Hob;

  /* The ATF HOB handoff base is at PcdSystemMemoryBase */
  Hob = GetNextGuidHob (
          &gPlatformInfoHobGuid,
          (CONST VOID *)FixedPcdGet64 (PcdSystemMemoryBase)
          );
  if (Hob != NULL) {
    BuildGuidDataHob (
      &gPlatformInfoHobGuid,
      GET_GUID_HOB_DATA (Hob),
      GET_GUID_HOB_DATA_SIZE (Hob)
      );
  }
}

EFI_STATUS
EFIAPI
InitializeATFHobPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  BuildPlatformInformationHob ();

  return EFI_SUCCESS;
}
