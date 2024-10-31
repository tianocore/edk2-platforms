/** @file
  PEIM MMC set power off type driver

  Copyright (c) 2022, ADLink. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/MmcLib.h>

/**
  The Entry Point to clear MMC set power off type

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MmcSetPowerOffTypePeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS    Status;
  STATIC UINT8  SettingCount = 0;

  if (SettingCount++ == 0) {
    Status = MmcSetPowerOffType (0);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
