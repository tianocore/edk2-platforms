/** @file
  Cadence I2C controller UEFI Driver binding implementation

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DriverBinding.h>

#include "Driver.h"

/// CADENCE_I2C_CONTEXT Signature
#define CADENCE_I2C_SIGNATURE \
  SIGNATURE_64 ('C', 'A', 'D', 'E', 'N', 'I', '2', 'C')

/// The DXE singleton data
STATIC
struct {
  /// Number of UEFI Driver instanced with this driver
  UINTN                          Instances;

  /// Produced protocol
  EFI_DRIVER_BINDING_PROTOCOL    DriverBinding;
} mModule;

/**
  Extract base CADENCE_I2C_CONTEXT from a EFI_I2C_MASTER_PROTOCOL pointer.

  @param[in] I2cMaster  Pointer to extract context pointer from.

  @retval NULL    Signature check failed.
  @retval Others  The context pointer connected to I2cMaster pointer.

**/
STATIC
CADENCE_I2C_CONTEXT *
ContextFromI2cMaster (
  IN CONST EFI_I2C_MASTER_PROTOCOL  *I2cMaster
  )
{
  CADENCE_I2C_CONTEXT  *Base;

  Base = BASE_CR (
           I2cMaster,
           CADENCE_I2C_CONTEXT,
           I2cMaster
           );
  return Base->Signature == CADENCE_I2C_SIGNATURE ? Base : NULL;
}

/**
  Extract the driver context pointer from a handle if possible.

  @param[in] Handle  EFI handle to extract context pointer from.

  @retval NULL    No context pointer connected with Handle.
  @retval Others  The context pointer connected to Handle.

**/
STATIC
CADENCE_I2C_CONTEXT *
ContextFromHandle (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  VOID        *I2cMaster;

  Status = gBS->HandleProtocol (Handle, &gEfiI2cMasterProtocolGuid, &I2cMaster);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ContextFromI2cMaster (I2cMaster);
}

/**
  Set the frequency for the I2C clock line.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_SET_BUS_FREQUENCY.

  If the requested frequency is lower then was it possible then the function
  will return EFI_UNSUPPORTED and BusClockHertz is updated with the lowest
  possible frequency.

  @param[in]     This           Pointer to an EFI_I2C_MASTER_PROTOCOL structure
                                created by this driver.
  @param[in,out] BusClockHertz  Pointer to the requested I2C bus clock frequency
                                in Hertz.
                                Upon return this value contains the actual
                                frequency in use by the I2C controller.

  @retval EFI_SUCCESS           The bus frequency was set successfully.
  @retval EFI_ALREADY_STARTED   The controller is busy with another transaction.
  @retval EFI_INVALID_PARAMETER BusClockHertz is NULL
  @retval EFI_INVALID_PARAMETER This refers to a I2cMaster not handled by this
                                driver.
  @retval EFI_UNSUPPORTED       The controller does not support this frequency.

**/
STATIC
EFI_STATUS
EFIAPI
SetBusFrequency (
  IN     CONST EFI_I2C_MASTER_PROTOCOL  *This,
  IN OUT       UINTN                    *BusClockHertz
  )
{
  CADENCE_I2C_CONTEXT  *Dev;

  if (BusClockHertz == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = ContextFromI2cMaster (This);
  if (Dev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return DriverSetBusFrequency (Dev, BusClockHertz);
}

/**
  Reset the I2C controller and configure it for use.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_RESET.

  @param[in] This  Pointer to an EFI_I2C_MASTER_PROTOCOL structure created by
                   this driver.

  @retval EFI_SUCCESS            The reset completed successfully.
  @retval EFI_INVALID_PARAMETER  This refers to a I2cMaster not handled by this
                                 driver.
  @retval EFI_DEVICE_ERROR       The reset operation failed.

**/
STATIC
EFI_STATUS
EFIAPI
Reset (
  IN CONST EFI_I2C_MASTER_PROTOCOL  *This
  )
{
  CADENCE_I2C_CONTEXT  *Dev;

  Dev = ContextFromI2cMaster (This);
  if (Dev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return DriverReset (Dev);
}

/**
  Start an I2C transaction on the host controller.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_START_REQUEST.

  @param[in]  This              Pointer to an EFI_I2C_MASTER_PROTOCOL structure
                                created by this driver.
  @param[in]  SlaveAddress      Address of the device on the I2C bus.
  @param[in]  RequestPacket     Pointer to an EFI_I2C_REQUEST_PACKET
                                structure describing the I2C transaction.
  @param[in]  Event             Event to signal for asynchronous transactions,
                                NULL for asynchronous transactions.
  @param[out] I2cStatus         Optional buffer to receive the I2C transaction
                                completion status.

  @retval EFI_SUCCESS           The asynchronous transaction was successfully
                                started when Event is not NULL.
  @retval EFI_SUCCESS           The transaction completed successfully when
                                Event is NULL.
  @retval EFI_ALREADY_STARTED   The controller is busy with another transaction.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the
                                transaction.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL.
  @retval EFI_INVALID_PARAMETER This refers to a I2cMaster not handled by this
                                driver.
  @retval EFI_NOT_FOUND         Reserved bit set in the SlaveAddress parameter
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the slave
                                address.  EFI_DEVICE_ERROR will be returned if
                                the controller cannot distinguish when the NACK
                                occurred.
  @retval EFI_UNSUPPORTED       The controller does not support the requested
                                transaction.

**/
STATIC
EFI_STATUS
EFIAPI
StartRequest (
  IN  CONST EFI_I2C_MASTER_PROTOCOL  *This,
  IN        UINTN                    SlaveAddress,
  IN        EFI_I2C_REQUEST_PACKET   *RequestPacket,
  IN        EFI_EVENT                Event,
  OUT       EFI_STATUS               *I2cStatus
  )
{
  CADENCE_I2C_CONTEXT *CONST  Dev = ContextFromI2cMaster (This);
  BOOLEAN                     ExtendedAddress;

  if (Dev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (RequestPacket == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BOOLEAN)(SlaveAddress & I2C_ADDRESSING_10_BIT)) {
    SlaveAddress &= ~I2C_ADDRESSING_10_BIT;
    if ((BOOLEAN)(SlaveAddress & ~0x3FFu)) {
      return EFI_NOT_FOUND;
    }

    ExtendedAddress = TRUE;
  } else {
    if ((BOOLEAN)(SlaveAddress & ~0x7Fu)) {
      return EFI_NOT_FOUND;
    }

    ExtendedAddress = FALSE;
  }

  return DriverStartRequest (
           Dev,
           SlaveAddress,
           ExtendedAddress,
           RequestPacket,
           Event,
           I2cStatus
           );
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
  IN EFI_DRIVER_BINDING_PROTOCOL  *This OPTIONAL,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;
  VOID        *Dummy;

  Status = gBS->HandleProtocol (Controller, &gCadenceI2cGuid, &Dummy);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Fills in all the fields of the device path.

  @param[in] DevicePath  Device path to initialize.
  @param[in] MmioBase    Memory-mapped base address of the Cadence I2C
                         Controller.

**/
STATIC
VOID
DevicePathInit (
  IN CADENCE_I2C_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT64                            MmioBase
  )
{
  DevicePath->Vendor.Header.Type      = HARDWARE_DEVICE_PATH;
  DevicePath->Vendor.Header.SubType   = HW_VENDOR_DP;
  DevicePath->Vendor.Header.Length[0] = sizeof (DevicePath->Vendor);
  DevicePath->Vendor.Header.Length[1] = sizeof (DevicePath->Vendor) >> 8;
  DevicePath->Vendor.Guid             = gCadenceI2cGuid;
  DevicePath->Vendor.MmioBase         = SwapBytes64 (MmioBase);
  SetDevicePathEndNode (&DevicePath->End);
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
  CADENCE_I2C_CONTEXT  *Dev;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (!Dev) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gCadenceI2cGuid,
                  (VOID **)&Dev->CadenceI2cInstall,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto fail_FreePool;
  }

  Dev->Signature = CADENCE_I2C_SIGNATURE;
  DevicePathInit (&Dev->DevicePath, Dev->CadenceI2cInstall->MmioBase);
  Dev->I2cMaster.SetBusFrequency = SetBusFrequency;
  Dev->I2cMaster.Reset           = Reset;
  Dev->I2cMaster.StartRequest    = StartRequest;

  Status = DriverStart (Dev);
  if (EFI_ERROR (Status)) {
    goto fail_CloseProtocol;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiDevicePathProtocolGuid,
                  &Dev->DevicePath,
                  &gEfiI2cMasterProtocolGuid,
                  &Dev->I2cMaster,
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
         &gCadenceI2cGuid,
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
  @retval EFI_INVALID_PARAMETER NumberOfChildren was not 0;
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

  CADENCE_I2C_CONTEXT *CONST  Dev = ContextFromHandle (Controller);

  if (Dev != NULL) {
    EFI_STATUS  Status;
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    &Dev->DevicePath,
                    &gEfiI2cMasterProtocolGuid,
                    &Dev->I2cMaster,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DriverStop (Dev);
    mModule.Instances--;
    FreePool (Dev);
  }

  return gBS->CloseProtocol (
                Controller,
                &gCadenceI2cGuid,
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable OPTIONAL
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
                  &gImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &mModule.DriverBinding,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "[%a]: Cadence I2C controller UEFI Driver installed.\n",
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
