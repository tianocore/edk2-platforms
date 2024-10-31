/** @file
  DXE MMC set power off type driver

  Copyright (c) 2022, ADLink. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MmcLib.h>

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
STATIC
VOID
EFIAPI
MmcNotifyReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS    Status;
  STATIC UINT8  SettingCount = 0;

  if (SettingCount++ == 0) {
    Status = MmcSetPowerOffType (1);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  The Entry Point for MMC set power off type

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MmcSetPowerOffTypeDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  MmcNotifyReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
