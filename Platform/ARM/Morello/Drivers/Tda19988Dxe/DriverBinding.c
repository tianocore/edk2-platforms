/** @file
  NXP TDA19988 HDMI transmitter UEFI Driver binding implementation

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DriverBinding.h>

#include "Driver.h"

/// TDA19988_CONTEXT Signature
#define TDA19988_CONTEXT_SIGNATURE \
  SIGNATURE_64 ('T', 'D', 'A', '9', '9', 'I', '2', 'C')

/// The DXE singleton data
STATIC
struct {
  /// Number of UEFI Driver instanced with this driver
  UINTN                          Instances;

  /// Produced protocol
  EFI_DRIVER_BINDING_PROTOCOL    DriverBinding;
} mModule;

/**
  Extract base TDA19988_CONTEXT from a TDA19988_PROTOCOL pointer.

  @param[in] Tda19988  Pointer to extract context pointer from.

  @retval NULL    Signature check failed.
  @retval Others  The context pointer connected to I2cMaster pointer.

**/
STATIC
TDA19988_CONTEXT *
ContextFromTda19988 (
  IN TDA19988_PROTOCOL  *Tda19988
  )
{
  TDA19988_CONTEXT  *Base;

  Base = BASE_CR (
           Tda19988,
           TDA19988_CONTEXT,
           Tda19988
           );
  return (Base->Signature == TDA19988_CONTEXT_SIGNATURE) ? Base : NULL;
}

/**
  Extract the driver context pointer from a handle if possible.

  @param[in] Handle  EFI handle to extract context pointer from.

  @retval NULL    No context pointer connected with Handle.
  @retval Others  The context pointer connected to Handle.

**/
STATIC
TDA19988_CONTEXT *
ContextFromHandle (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  VOID        *Tda19988;

  Status = gBS->HandleProtocol (Handle, &gTda19988ProtocolGuid, &Tda19988);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ContextFromTda19988 (Tda19988);
}

/**
  Check if a sink is present by checking the hot-plug detection level.

  @param[in,out] This         Pointer to an TDA19988_PROTOCOL structure.
  @param[out]    SinkPresent  Write TRUE if sink is detected, otherwise FALSE.

  @retval EFI_SUCCESS           *SinkPresent updated and valid.
  @retval EFI_INVALID_PARAMETER This refers to a TDA19988_PROTOCOL not created
                                by this driver.
  @retval EFI_INVALID_PARAMETER SinkPresent is NULL.
  @retval *                     Other errors are possible.

**/
STATIC
EFI_STATUS
EFIAPI
GetSinkPresent (
  IN OUT TDA19988_PROTOCOL  *This,
  OUT    BOOLEAN            *SinkPresent
  )
{
  if (SinkPresent == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TDA19988_CONTEXT *CONST  Dev = ContextFromTda19988 (This);

  if (Dev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return DriverGetSinkPresent (Dev, SinkPresent);
}

/**
  Configure the specified mode.

  @param[in,out] This  Pointer to an TDA19988_PROTOCOL structure.
  @param[in]     Mode  Describe the mode to configure.

  @retval EFI_SUCCESS           Mode configured.
  @retval EFI_INVALID_PARAMETER This refers to a TDA19988_PROTOCOL not created
                                by this driver.
  @retval EFI_INVALID_PARAMETER Mode is NULL.
  @retval *                     Other errors are possible.

**/
STATIC
EFI_STATUS
EFIAPI
SetMode (
  IN OUT TDA19988_PROTOCOL        *This,
  IN     CONST TDA19988_MODEINFO  *Mode
  )
{
  if (Mode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TDA19988_CONTEXT *CONST  Dev = ContextFromTda19988 (This);

  if (Dev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return DriverSetMode (Dev, Mode);
}

/**
  Retrieve the EDID.

  Retrieve the EDID and copy it into an allocated buffer of type
  EfiBootServicesData.
  On success the EdidData is update to point to the allocated buffer and
  EdidSize is updated with the size of the size of the EDID.

  @param[in,out] This     Pointer to an TDA19988_PROTOCOL structure.
  @param[out]    EdidData Updated with pointer to an allocated buffer containing
                          the EDID.
  @param[out]    EdidSize Size of the EDID that EdidData is pointing to.

  @retval EFI_SUCCESS           *EdidData and *EdidSize updated and valid.
  @retval EFI_INVALID_PARAMETER This refers to a TDA19988_PROTOCOL not created
                                by this driver.
  @retval EFI_INVALID_PARAMETER EdidData is NULL.
  @retval EFI_INVALID_PARAMETER EdidSize is NULL.
  @retval *                     Other errors are possible.

**/
STATIC
EFI_STATUS
EFIAPI
GetEdid (
  IN OUT TDA19988_PROTOCOL  *This,
  OUT    VOID               **EdidData,
  OUT    UINTN              *EdidSize
  )
{
  if (EdidData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EdidSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TDA19988_CONTEXT *CONST  Dev = ContextFromTda19988 (This);

  if (Dev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return DriverGetEdid (Dev, EdidData, EdidSize);
}

/**
  Tests to see if this driver supports a given controller.

  @param[in] This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                  instance.
  @param[in] Controller           The handle of the controller to test.
  @param[in] RemainingDevicePath  The remaining device path.
                                  (Ignored - this is not a bus driver.)

  @retval EFI_SUCCESS             The driver supports this controller.
  @retval EFI_UNSUPPORTED         The device specified by Controller is not
                                  supported by this driver.

**/
STATIC
EFI_STATUS
EFIAPI
BindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_I2C_IO_PROTOCOL  *TmpI2cIo;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiI2cIoProtocolGuid,
                  (VOID **)&TmpI2cIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (!CompareGuid (TmpI2cIo->DeviceGuid, &gTda19988Guid)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Starts a device controller.

  @param[in] This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in] Controller            The handle of the device to start.
  @param[in] RemainingDevicePath   The remaining portion of the device path.
                                   (Ignored - this is not a bus driver.)

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a
                                   device error.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a
                                   lack of resources.
  @retval *                        Other errors are possible.

**/
STATIC
EFI_STATUS
EFIAPI
BindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS           Status;
  EFI_I2C_IO_PROTOCOL  *I2cIo;
  TDA19988_CONTEXT     *Dev;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (!Dev) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cIoProtocolGuid,
                  (VOID **)&I2cIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto fail_FreePool;
  }

  Dev->Signature               = TDA19988_CONTEXT_SIGNATURE;
  Dev->I2cIo                   = I2cIo;
  Dev->Tda19988.GetSinkPresent = GetSinkPresent;
  Dev->Tda19988.SetMode        = SetMode;
  Dev->Tda19988.GetEdid        = GetEdid;

  Status = DriverStart (Dev);
  if (EFI_ERROR (Status)) {
    goto fail_CloseProtocol;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gTda19988ProtocolGuid,
                  &Dev->Tda19988,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto fail_DriverStop;
  }

  mModule.Instances++;
  return EFI_SUCCESS;

fail_DriverStop:
  DriverStop (Dev);

fail_CloseProtocol:
  gBS->CloseProtocol (
         Controller,
         &gEfiI2cIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

fail_FreePool:
  FreePool (Dev);

  return Status;
}

/**
  Stops a device controlled by this driver.

  @param[in] This               A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                instance.
  @param[in] Controller         A handle to the device being stopped.
  @param[in] NumberOfChildren   Must be 0 for this driver.
  @param[in] ChildHandleBuffer  Not used by this driver.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device
                                error.
  @retval EFI_INVALID_PARAMETER NumberOfChildren was not 0.
  @retval *                     Other errors are possible.

**/
STATIC
EFI_STATUS
EFIAPI
BindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  if (NumberOfChildren != 0) {
    return EFI_INVALID_PARAMETER;
  }

  TDA19988_CONTEXT *CONST  Dev = ContextFromHandle (Controller);

  if (Dev != NULL) {
    EFI_STATUS  Status;
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gTda19988ProtocolGuid,
                    &Dev->Tda19988,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // no more pointers into the image
    mModule.Instances--;
    DriverStop (Dev);
    FreePool (Dev);
  }

  return gBS->CloseProtocol (
                Controller,
                &gEfiI2cIoProtocolGuid,
                This->DriverBindingHandle,
                Controller
                );
}

/**
  The UEFI Driver entry point.

  @param[in] ImageHandle  The image handle of the UEFI Driver.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The UEFI Driver installed.
  @retval *               Other errors are possible.

**/
EFI_STATUS
Load (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mModule.DriverBinding.Supported           = BindingSupported;
  mModule.DriverBinding.Start               = BindingStart;
  mModule.DriverBinding.Stop                = BindingStop;
  mModule.DriverBinding.Version             = 0x10;
  mModule.DriverBinding.ImageHandle         = ImageHandle;
  mModule.DriverBinding.DriverBindingHandle = ImageHandle;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &mModule.DriverBinding,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "[%a]: NXP TDA19988 HDMI transmitter UEFI Driver installed.\n",
      gEfiCallerBaseName
      ));
  }

  return Status;
}

/**
  The UEFI Driver unload point.

  @param[in] ImageHandle    The image handle of the UEFI Driver.

  @retval EFI_SUCCESS       The UEFI Driver unloaded.
  @retval EFI_ACCESS_DENIED The UEFI Driver is in-use.
  @retval *                 Other errors are possible.

**/
EFI_STATUS
Unload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mModule.Instances != 0) {
    return EFI_ACCESS_DENIED;
  }

  return gBS->UninstallMultipleProtocolInterfaces (
                ImageHandle,
                &gEfiDriverBindingProtocolGuid,
                &mModule.DriverBinding,
                NULL
                );
}
