/** @file
  Platform Library for Process Video OpRom.

  Copyright (C) 2022 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/PciIo.h>
#include <Protocol/Security2.h>
#include <Library/SecurityManagementLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AmdBeforeConsoleEvent.h>
#include "ProcessVideoOpromLib.h"

extern EFI_SECURITY2_ARCH_PROTOCOL  mSecurity2Stub;
EFI_SECURITY2_FILE_AUTHENTICATION   mOriginalFileAuthentication = NULL;

/**
  Use VID to check for trusted video controller

  @param[in]  File   File to be processed.

  @retval TRUE       Found the trusted video VID.
  @retval FALSE      Not found the trusted video VID.
**/
BOOLEAN
EFIAPI
CheckTrustedVideoHc (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *File
  )
{
  EFI_STATUS                Status;
  CHAR16                    *DevicePathStr;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_HANDLE                PciHandle;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                PciType;

  DevicePathStr = ConvertDevicePathToText (File, FALSE, FALSE);
  DEBUG ((DEBUG_INFO, "CheckTrustedVideoHc File Path = %s\n", DevicePathStr));
  if (DevicePathStr != NULL) {
    gBS->FreePool (DevicePathStr);
  }

  PciHandle           = NULL;
  TempDevicePath      = DuplicateDevicePath (File);
  RemainingDevicePath = TempDevicePath;
  Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &PciHandle);
  if (!EFI_ERROR (Status) && (PciHandle != NULL)) {
    Status = gBS->HandleProtocol (PciHandle, &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
    if (!EFI_ERROR (Status)) {
      PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0, sizeof (PCI_DEVICE_INDEPENDENT_REGION) >> 1, (VOID *)&PciType);
      if (((PciType.Hdr.VendorId == NVDA_VID) || (PciType.Hdr.VendorId == AMD_DP_VID)) && (IS_PCI_DISPLAY (&PciType))) {
        DEBUG ((DEBUG_INFO, "Found Trusted Video VID = %04X and skip defer 3rdParty image on it\n", PciType.Hdr.VendorId));
        gBS->FreePool (TempDevicePath);
        return TRUE;
      }
    }
  }

  gBS->FreePool (TempDevicePath);
  return FALSE;
}

/**
  Skip defer 3rd Party image on trusted video controller on PCI slot.

  @param[in]  This             The EFI_SECURITY2_ARCH_PROTOCOL instance.
  @param[in]  File             A pointer to the device path of the file that is
                               being dispatched. This will optionally be used for logging.
  @param[in]  FileBuffer       A pointer to the buffer with the UEFI file image.
  @param[in]  FileSize         The size of the file.
  @param[in]  BootPolicy       A boot policy that was used to call LoadImage() UEFI service. If
                               FileAuthentication() is invoked not from the LoadImage(),
                               BootPolicymust be set to FALSE.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is not NULL and the user has no permission to load
                                  drivers from the device path specified by DevicePath. The
                                  image has been added into the list of the deferred images.
  @retval EFI_ACCESS_DENIED       The file specified by File did not authenticate, and
                                  the platform policy dictates that the DXE
                                  Foundation may not use File.
  @retval EFI_INVALID_PARAMETER   File and FileBuffer are both NULL.
**/
EFI_STATUS
EFIAPI
ProcessVideoOprom (
  IN CONST EFI_SECURITY2_ARCH_PROTOCOL  *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL     *File  OPTIONAL,
  IN VOID                               *FileBuffer,
  IN UINTN                              FileSize,
  IN BOOLEAN                            BootPolicy
  )
{
  if (CheckTrustedVideoHc (File)) {
    return ExecuteSecurity2Handlers (
             EFI_AUTH_OPERATION_VERIFY_IMAGE |
             EFI_AUTH_OPERATION_DEFER_IMAGE_LOAD |
             EFI_AUTH_OPERATION_MEASURE_IMAGE |
             EFI_AUTH_OPERATION_CONNECT_POLICY,
             0,
             File,
             FileBuffer,
             FileSize,
             BootPolicy
             );
  } else {
    //
    // Use original EFI Security2 protocol.
    //
    return mOriginalFileAuthentication (
             This,
             File,
             FileBuffer,
             FileSize,
             BootPolicy
             );
  }
}

/**
  Callback function to replace FileAuthentication with ProcessVideoOprom.

  @retval EFI_SUCCESS             Success to assgin the callback for process video OpRom.
**/
EFI_STATUS
EFIAPI
StartProcessVideoOpromCallBack (
  VOID
  )
{
  mOriginalFileAuthentication       = mSecurity2Stub.FileAuthentication;
  mSecurity2Stub.FileAuthentication = ProcessVideoOprom;
  DEBUG ((DEBUG_INFO, " Replace Original FileAuthentication  ... \n"));
  return EFI_SUCCESS;
}

AMD_BEFORE_CONSOLE_EVENT_PROTOCOL  mAmdBeforeConsoleProtocol = {
  StartProcessVideoOpromCallBack,
  0x80
};

/**
  Notify function for gEfiPciEnumerationCompleteProtocolGuid.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
StopProcessVideoOpromCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if (mOriginalFileAuthentication != NULL) {
    mSecurity2Stub.FileAuthentication = mOriginalFileAuthentication;
  }

  DEBUG ((DEBUG_INFO, " Restore Original FileAuthentication  ... \n"));
  gBS->CloseEvent (Event);
}

/**
  Register handlers to process video controller's OPROM.

  @param[in]  ImageHandle   ImageHandle of the loaded driver.
  @param[in]  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
  @retval Other value   The constructor install or register protocol fails.
**/
EFI_STATUS
EFIAPI
ProcessVideoOpromLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gAmdBeforeConsoleEventProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mAmdBeforeConsoleProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  StopProcessVideoOpromCallBack,
                  NULL,
                  &Event
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->RegisterProtocolNotify (
                    &gEfiPciEnumerationCompleteProtocolGuid,
                    Event,
                    &Registration
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
