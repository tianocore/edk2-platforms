/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

EFI_EVENT  mRuntimeAmpereCpuLibVirtualNotifyEvent = NULL;
VOID       *mPlatformInfoHob                      = NULL;

/**
  Get the platform HOB data.

  @return   PLATFORM_INFO_HOB   The pointer to the platform HOB data.

**/
PLATFORM_INFO_HOB *
GetPlatformHob (
  VOID
  )
{
  ASSERT (mPlatformInfoHob != NULL);
  return (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (mPlatformInfoHob);
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
RuntimeAmpereCpuLibVirtualNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Convert the platform HOB address to a virtual address.
  //
  Status = EfiConvertPointer (0, (VOID **)&mPlatformInfoHob);

  ASSERT_EFI_ERROR (Status);
}

/**
  Constructor of Runtime Ampere CPU Library Instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor completed successfully.
  @retval Others        The constructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
RuntimeAmpereCpuLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Hob;

  Hob = GetNextGuidHob (
          &gPlatformInfoHobGuid,
          (CONST VOID *)FixedPcdGet64 (PcdSystemMemoryBase)
          );
  if (Hob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get gPlatformInfoHobGuid!\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  mPlatformInfoHob = AllocateRuntimeCopyPool (sizeof (PLATFORM_INFO_HOB), Hob);
  ASSERT (mPlatformInfoHob != NULL);

  //
  // Register notify function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  RuntimeAmpereCpuLibVirtualNotify,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mRuntimeAmpereCpuLibVirtualNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Destructor of Runtime Ampere CPU Library Instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Others        The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
RuntimeAmpereCpuLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close the Set Virtual Address Map event
  //
  Status = gBS->CloseEvent (mRuntimeAmpereCpuLibVirtualNotifyEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
