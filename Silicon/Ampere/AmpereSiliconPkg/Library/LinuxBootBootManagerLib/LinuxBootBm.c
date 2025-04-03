/** @file
  Implementation for PlatformBootManagerLib library class interfaces.

  Copyright (C) 2015-2016, Red Hat, Inc.
  Copyright (c) 2014 - 2019, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2020 - 2025, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/FirmwareVolume2.h>
#include <Guid/EventGroup.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PlatformBootManager.h>

/**
  Register a boot option using a file GUID in the FV.

  @param  FileGuid     The file GUID name in the FV.
  @param  Description  The description of the boot option.
  @param  Attributes   The attributes of the boot option.

**/
VOID
PlatformRegisterFvBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINT32    Attributes
  )
{
  EFI_STATUS                         Status;
  EFI_HANDLE                         *HandleBuffer;
  UINTN                              HandleCount;
  UINTN                              IndexFv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL      *Fv;
  CHAR16                             *UiSection;
  UINTN                              UiSectionLength;
  UINT32                             AuthenticationStatus;
  EFI_HANDLE                         FvHandle;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  EFI_BOOT_MANAGER_LOAD_OPTION       *BootOptions;
  UINTN                              BootOptionCount;
  UINTN                              OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION       NewOption;

  //
  // Locate all available FVs.
  //
  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiFirmwareVolume2ProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Go through FVs one by one to find the required FFS file
  //
  for (IndexFv = 0, FvHandle = NULL; IndexFv < HandleCount && FvHandle == NULL; IndexFv++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[IndexFv],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&Fv
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Attempt to read a EFI_SECTION_USER_INTERFACE section from the required FFS file
    //
    UiSection = NULL;
    Status    = Fv->ReadSection (
                      Fv,
                      FileGuid,
                      EFI_SECTION_USER_INTERFACE,
                      0,
                      (VOID **)&UiSection,
                      &UiSectionLength,
                      &AuthenticationStatus
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    FreePool (UiSection);

    //
    // Save the handle of the FV where the FFS file was found
    //
    FvHandle = HandleBuffer[IndexFv];
  }

  //
  // Free the buffer of FV handles
  //
  FreePool (HandleBuffer);

  //
  // If the FFS file was not found, then return
  //
  if (FvHandle == NULL) {
    return;
  }

  //
  // Create a device path for the FFS file that was found
  //
  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = AppendDevicePathNode (
                 DevicePathFromHandle (FvHandle),
                 (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                 );

  //
  // Create and add a new load option for the FFS file that was found
  //
  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             NULL,
             0
             );
  if (!EFI_ERROR (Status)) {
    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

    OptionIndex = EfiBootManagerFindLoadOption (&NewOption, BootOptions, BootOptionCount);

    if (OptionIndex == -1) {
      Status = EfiBootManagerAddLoadOptionVariable (&NewOption, (UINTN)-1);
      ASSERT_EFI_ERROR (Status);
    }

    EfiBootManagerFreeLoadOption (&NewOption);
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  }
}

/**
  Do the platform specific action before the console is connected.

  Such as:
    Update console variable;
    Register new Driver#### or Boot####;
    Signal ReadyToLock event.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  //
  // Signal EndOfDxe PI Event
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);
}

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch additional option roms.
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  EFI_GUID  LinuxBootFileGuid;

  CopyGuid (&LinuxBootFileGuid, PcdGetPtr (PcdLinuxBootFileGuid));

  if (!CompareGuid (&LinuxBootFileGuid, &gZeroGuid)) {
    //
    // Register LinuxBoot
    //
    PlatformRegisterFvBootOption (
      &LinuxBootFileGuid,
      L"LinuxBoot",
      LOAD_OPTION_ACTIVE
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a: PcdLinuxBootFileGuid was not set!\n", __func__));
  }
}

/**
  This function is called each second during the boot manager waits the
  timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  TimeoutRemain
  )
{
  return;
}

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
**/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
  VOID
  )
{
  return;
}
