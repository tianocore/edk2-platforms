/** @file

  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/RamDisk.h>

/// The DXE singleton data
STATIC
struct {
  EFI_EVENT    GopEvent;
  VOID         *GopNotifyRegistrationKey;
} mModule;

/**
  Called when the first GOP is installed and set a mode.

  @param[in] Event   Not used.
  @param[in] Context Not used.
**/
STATIC
VOID
OnGopNotifyCallback (
  IN EFI_EVENT  Event    OPTIONAL,
  IN VOID       *Context OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  Status = gBS->LocateProtocol (
                  &gEfiGraphicsOutputProtocolGuid,
                  mModule.GopNotifyRegistrationKey,
                  (VOID **)&Gop
                  );
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a:%a]: LocateProtocol(gEfiGraphicsOutputProtocolGuid) - %r\n",
        gEfiCallerBaseName,
        __func__,
        Status
        ));
    }

    return;
  }

  // Make sure that a mode have been set so we can hand the OS a running GOP
  Status = Gop->SetMode (Gop, 0);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a:%a]: SetMode() - %r\n",
        gEfiCallerBaseName,
        __func__,
        Status
        ));
    }

    return;
  }

  Status = gBS->CloseEvent (mModule.GopEvent);
  ASSERT_EFI_ERROR (Status);
  mModule.GopEvent                 = NULL;
  mModule.GopNotifyRegistrationKey = NULL;
}

/** Initialize the GOP notification callback event.

  @retval EFI_SUCCESS  The GOP notify callback was successfully installed.
  @retval *            Errors are possible.

**/
STATIC
EFI_STATUS
InitGopNotify (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OnGopNotifyCallback,
                  NULL,
                  &mModule.GopEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: Can not install the NotifyEvent handler -- %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiGraphicsOutputProtocolGuid,
                  mModule.GopEvent,
                  &mModule.GopNotifyRegistrationKey
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: RegisterProtocolNotify(gEfiGraphicsOutputProtocolGuid) -- %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return Status;
  }

  /** Explicitly signal the GopEvent to trigger the invocation of SetMode(),
      in case this module was loaded after the GOP was initialised
  **/
  Status = gBS->SignalEvent (mModule.GopEvent);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

VOID
InitVirtioDevices (
  VOID
  );

/**
  Entrypoint of Platform Dxe Driver

  @param  ImageHandle[in]       The firmware allocated handle for the EFI image.
  @param  SystemTable[in]       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval *                     Errors are possible.
**/
EFI_STATUS
EFIAPI
ArmMorelloEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_RAM_DISK_PROTOCOL     *RamDisk;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  // expose the Device Tree if it is available
  if (GetFirstGuidHob (&gFdtHobGuid) != NULL) {
    gBS->InstallProtocolInterface (
           &gImageHandle,
           &gEdkiiPlatformHasDeviceTreeGuid,
           EFI_NATIVE_INTERFACE,
           NULL
           );
  }

  Status = InitGopNotify ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: Failed to initialize GOP notification."
      " Can not gurantee that the runtime will have a GOP -- %r\n",
      gEfiCallerBaseName,
      Status
      ));
  }

  InitVirtioDevices ();

  if (FeaturePcdGet (PcdRamDiskSupported)) {
    Status = gBS->LocateProtocol (
                    &gEfiRamDiskProtocolGuid,
                    NULL,
                    (VOID **)&RamDisk
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Couldn't find the RAM Disk protocol %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = RamDisk->Register (
                        (UINTN)PcdGet32 (PcdRamDiskBase),
                        (UINTN)PcdGet32 (PcdRamDiskSize),
                        &gEfiVirtualCdGuid,
                        NULL,
                        &DevicePath
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to register RAM Disk - %r\n",
        __func__,
        Status
        ));
    }
  }

  return Status;
}
