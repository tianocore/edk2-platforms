/** @file
  Platform specific TPM2 component for configuring the Platform Hierarchy.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (C) 2017 - 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/TpmPlatformHierarchyLib.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <TpmSgNvIndexLib.h>

/**
  This callback function will run at the SmmReadyToLock event.

  Configuration of the TPM's Platform Hierarchy Authorization Value (platformAuth)
  and Platform Hierarchy Authorization Policy (platformPolicy) can be defined through this function.

  @param[in]  Event   Pointer to this event
  @param[in]  Context Event handler private data
 **/
VOID
EFIAPI
SmmReadyToLockEventCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  //
  // Try to locate it because EfiCreateProtocolNotifyEvent will trigger it once when registration.
  // Just return if it is not found.
  //
  Status = gBS->LocateProtocol (
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  NULL,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  ConfigureTpmPlatformHierarchy ();

  gBS->CloseEvent (Event);
}

/**
  The driver's entry point. Will register a function for callback during SmmReadyToLock event to
  configure the TPM's platform authorization.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
Tcg2PlatformDxeEntryPoint (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID       *Registration;
  EFI_EVENT  Event;

  //
  // To create SystemGuard NV Index consumed by OS
  //
  DefineSgTpmNvIndexforOs ();

  Event = EfiCreateProtocolNotifyEvent (
            &gEfiDxeSmmReadyToLockProtocolGuid,
            TPL_CALLBACK,
            SmmReadyToLockEventCallBack,
            NULL,
            &Registration
            );

  ASSERT (Event != NULL);

  return EFI_SUCCESS;
}
