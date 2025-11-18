/** @file
  The Morello SoC I2C HDMI bus

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/CadenceI2c.h>
#include <Protocol/I2cIo.h>
#include <Protocol/I2cMaster.h>
#include <Protocol/Tda19988.h>

#include "I2cBusHdmi.h"
#include "MorelloPlatform.h"

/// Unique ID of the GOP TDA19988 within the bus
#define MORELLO_GOP_TDA19988_DEVICE_INDEX  0x45

#define I2C_BUS_SIGNATURE  SIGNATURE_64 ('P', 'H', 'D', 'M', 'I', 'I', '2', 'C')
struct _I2C_BUS_HDMI {
  UINT64                                           Signature;

  /// I2cMaster handle
  EFI_HANDLE                                       Handle;

  /// Current configured bus frequency (0 means unknown)
  UINTN                                            BusFrequencyHertz;

  /// Protocol Notify to detect bus installed devices
  EFI_EVENT                                        DeviceEvent;
  VOID                                             *DeviceTracker;

  /// Produced protocols
  EFI_I2C_ENUMERATE_PROTOCOL                       I2cEnumerate;
  EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL    I2cBusConf;
};

STATIC_ASSERT (
  FixedPcdGet64 (PcdHdmiI2cBusCadanceControllerIoBase) != 0,
  "Invalid I2C controller IO base"
  );
STATIC_ASSERT (
  FixedPcdGet64 (PcdHdmiI2cBusDeviceAddrTda19988Cec) != 0,
  "Invalid TDA19988 CEC address"
  );
STATIC_ASSERT (
  FixedPcdGet64 (PcdHdmiI2cBusDeviceAddrTda19988Hdmi) != 0,
  "Invalid TDA19988 HDMI address"
  );
STATIC_ASSERT (
  FixedPcdGet64 (PcdHdmiI2cBusSpeed) != 0,
  "Invalid I2C bus speed"
  );

/// Addresses of the TDA19988
STATIC CONST UINT32  mTda19988Addr[] = {
  [TDA19988_CEC_INDEX]  = FixedPcdGet64 (PcdHdmiI2cBusDeviceAddrTda19988Cec),
  [TDA19988_HDMI_INDEX] = FixedPcdGet64 (PcdHdmiI2cBusDeviceAddrTda19988Hdmi),
};

/// The single device connected to this bus
STATIC CONST EFI_I2C_DEVICE  mBusHdmiSingleDevice = {
  .DeviceGuid        = &gTda19988Guid,
  .DeviceIndex       = MORELLO_GOP_TDA19988_DEVICE_INDEX,
  .SlaveAddressArray = mTda19988Addr,
  .SlaveAddressCount = ARRAY_SIZE (mTda19988Addr),
};

STATIC CADENCE_I2C_INSTALL_PROTOCOL  mNonDiscoverableCadenceI2cMaster = {
  .MmioBase     = FixedPcdGet64 (PcdHdmiI2cBusCadanceControllerIoBase),
  .InputClockHz = FixedPcdGet64 (PcdHdmiI2cBusCadanceControllerInputClk),
};

/**
  Enumerate the I2C devices

  This function enables the caller to traverse the set of I2C devices
  on an I2C bus.

  @param[in]  This              Pointer to an EFI_I2C_ENUMERATE_PROTOCOL
                                structure.
  @param[in, out] Device        Pointer to a buffer containing an
                                EFI_I2C_DEVICE structure.  Enumeration is
                                started by setting the initial EFI_I2C_DEVICE
                                structure pointer to NULL.  The buffer
                                receives an EFI_I2C_DEVICE structure pointer
                                to the next I2C device.

  @retval EFI_SUCCESS           The platform data for the next device on
                                the I2C bus was returned successfully.
  @retval EFI_INVALID_PARAMETER Device is NULL
  @retval EFI_NO_MAPPING        *Device does not point to a valid
                                EFI_I2C_DEVICE structure returned in a
                                previous call Enumerate().

**/
STATIC
EFI_STATUS
EFIAPI
I2cEnumerate (
  IN CONST EFI_I2C_ENUMERATE_PROTOCOL  *This,
  IN OUT CONST EFI_I2C_DEVICE          **Device
  )
{
  if (Device == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Device == NULL) {
    *Device = &mBusHdmiSingleDevice;
    return EFI_SUCCESS;
  }

  if (*Device == &mBusHdmiSingleDevice) {
    *Device = NULL;
    return EFI_SUCCESS;
  }

  return EFI_NO_MAPPING;
}

/**
  Get the requested I2C bus frequency for a specified bus configuration.

  This function returns the requested I2C bus clock frequency for the
  I2cBusConfiguration.  This routine is provided for diagnostic purposes
  and is meant to be called after calling Enumerate to get the
  I2cBusConfiguration value.

  @param[in] This                 Pointer to an EFI_I2C_ENUMERATE_PROTOCOL
                                  structure.
  @param[in] I2cBusConfiguration  I2C bus configuration to access the I2C
                                  device
  @param[out] *BusClockHertz      Pointer to a buffer to receive the I2C
                                  bus clock frequency in Hertz

  @retval EFI_SUCCESS           The I2C bus frequency was returned
                                successfully.
  @retval EFI_INVALID_PARAMETER This refers to a I2cBusConf not handled by this
                                driver.
  @retval EFI_INVALID_PARAMETER BusClockHertz was NULL
  @retval EFI_NO_MAPPING        Invalid I2cBusConfiguration value

**/
STATIC
EFI_STATUS
EFIAPI
I2cGetBusFrequency (
  IN CONST EFI_I2C_ENUMERATE_PROTOCOL  *This,
  IN UINTN                             I2cBusConfiguration,
  OUT UINTN                            *BusClockHertz
  )
{
  I2C_BUS_HDMI  *Bus;

  if (!BusClockHertz) {
    return EFI_INVALID_PARAMETER;
  }

  if (I2cBusConfiguration) {
    return EFI_NO_MAPPING;
  }

  Bus = BASE_CR (This, I2C_BUS_HDMI, I2cEnumerate);
  if (Bus->Signature != I2C_BUS_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  *BusClockHertz = Bus->BusFrequencyHertz;
  return EFI_SUCCESS;
}

/**
  Enable access to an I2C bus configuration.

  Only extensions implemented by this driver is documented here.
  For non-extension details see
  EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL_ENABLE_I2C_BUS_CONFIGURATION.

  The only valid I2cBusConfiguration is 0 which means set bus speed to
  PcdHdmiI2cBusSpeed.

  @param[in]  This  Pointer to an EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL
                    structure.
  @param[in]  I2cBusConfiguration  Index of an I2C bus configuration.  All
                                   values in the range of zero to N-1 are valid
                                   where N is the total number of I2C bus
                                   configurations for an I2C bus.
  @param[in]  Event      Event to signal when the transaction is complete.
  @param[out] I2cStatus  Buffer to receive the transaction status.

  @return  When Event is NULL, EnableI2cBusConfiguration operates synchrouously
  and returns the I2C completion status as its return value.  In this case it is
  recommended to use NULL for I2cStatus.  The values returned from
  EnableI2cBusConfiguration are:

  @retval EFI_SUCCESS            The asynchronous bus configuration request
                                 was successfully started when Event is not
                                 NULL.
  @retval EFI_SUCCESS            The bus configuration request completed
                                 successfully when Event is NULL.
  @retval EFI_INVALID_PARAMETER  This refers to a I2cBusConf not
                                 handled by this driver.
  @retval EFI_DEVICE_ERROR       The bus configuration failed.
  @retval EFI_NO_MAPPING         Invalid I2cBusConfiguration value

**/
STATIC
EFI_STATUS
EFIAPI
I2cEnableBusConf (
  IN CONST EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL  *This,
  IN UINTN                                                I2cBusConfiguration,
  IN EFI_EVENT                                            Event,
  IN EFI_STATUS                                           *I2cStatus
  )
{
  EFI_I2C_MASTER_PROTOCOL  *I2cMaster;
  EFI_STATUS               Status;
  I2C_BUS_HDMI             *Bus;
  UINTN                    BusClockHertz;

  if (I2cBusConfiguration > 0) {
    Status = EFI_NO_MAPPING;
    goto done;
  }

  Bus = BASE_CR (This, I2C_BUS_HDMI, I2cBusConf);
  if (Bus->Signature != I2C_BUS_SIGNATURE) {
    Status = EFI_INVALID_PARAMETER;
    goto done;
  }

  Status = gBS->HandleProtocol (
                  Bus->Handle,
                  &gEfiI2cMasterProtocolGuid,
                  (VOID **)&I2cMaster
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:%a]: gBS->HandleProtocol(EfiI2cMasterProtocolGuid) failed - %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    Status = EFI_INVALID_PARAMETER;
    goto done;
  }

  BusClockHertz = FixedPcdGet64 (PcdHdmiI2cBusSpeed);
  Status        = I2cMaster->SetBusFrequency (I2cMaster, &BusClockHertz);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:%a]: failed to set bus speed to %u - %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      BusClockHertz,
      Status
      ));
    goto done;
  }

  if (BusClockHertz != FixedPcdGet64 (PcdHdmiI2cBusSpeed)) {
    DEBUG ((
      DEBUG_WARN,
      "[%a:%a]: requested bus speed %u != configured bus speed %u\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      FixedPcdGet64 (PcdHdmiI2cBusSpeed),
      BusClockHertz
      ));
  }

  Bus->BusFrequencyHertz = BusClockHertz;
  Status                 = EFI_SUCCESS;

done:
  if (Event != NULL) {
    *I2cStatus = Status;
    gBS->SignalEvent (Event);
    return EFI_SUCCESS;
  }

  return Status;
}

/**
  Locate the TDA 19988 that is for the platform GOP.

  Check each installed gTda19988ProtocolGuid if it is the
  MORELLO_GOP_TDA19988_DEVICE_INDEX.
  If it is installed then the alias gArmMorelloPlatformGopTda19988Guid is used
  to mark it as the Morello platform GOP TDA 19988.

  @param[in]  Event    Event whose notification function is being invoked.
  @param[in]  Context  Pointer to a I2C_BUS_HDMI structure.

**/
STATIC
VOID
EFIAPI
InstalledDeviceProtocol (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS           Status;
  EFI_HANDLE           Handle;
  UINTN                HandleSize;
  I2C_BUS_HDMI         *Bus;
  EFI_I2C_IO_PROTOCOL  *I2cIo;
  VOID                 *Protocol;

  Bus = Context;
  while (1) {
    HandleSize = sizeof (Handle);
    Status     = gBS->LocateHandle (
                        ByRegisterNotify,
                        NULL,
                        Bus->DeviceTracker,
                        &HandleSize,
                        &Handle
                        );
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
        break;
      }

      DEBUG ((
        DEBUG_WARN,
        "[%a:%a]: LocateHandle() - %r\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        Status
        ));
      continue;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiI2cIoProtocolGuid,
                    (VOID **)&I2cIo
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a:%a]: OpenProtocol(EfiI2cIoProtocolGuid) - %r\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        Status
        ));
      continue;
    }

    if (I2cIo->DeviceIndex == MORELLO_GOP_TDA19988_DEVICE_INDEX) {
      Status = gBS->OpenProtocol (
                      Handle,
                      &gTda19988ProtocolGuid,
                      (VOID **)&Protocol,
                      gImageHandle,
                      gImageHandle,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "[%a:%a]: OpenProtocol(Tda19988ProtocolGuid) - %r!\n",
          gEfiCallerBaseName,
          __FUNCTION__,
          Status
          ));
        continue;
      }

      // install the alias GUID so that LcdGraphicsOutputDxe can start
      Status =  gBS->InstallProtocolInterface (
                       &Handle,
                       &gArmMorelloPlatformGopTda19988Guid,
                       EFI_NATIVE_INTERFACE,
                       Protocol
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "[%a:%a]: InstallProtocolInterface(ArmMorelloPlatformGopTda19988Guid)"
          " - %r!\n",
          gEfiCallerBaseName,
          __FUNCTION__,
          Status
          ));
        continue;
      }

      // close the event after fire the callback as this bus only have one
      // device
      Status = gBS->CloseEvent (Bus->DeviceEvent);
      ASSERT_EFI_ERROR (Status);
      Bus->DeviceEvent   = NULL;
      Bus->DeviceTracker = NULL;
    }
  }
}

/**
  Allocate and install UEFI driver bindings for the controller and devices for
  the Morello SoC HDMI I2C bus.

  @param[out] I2cBusHdmi  Updated with a pointer to the allocated I2C_BUS_HDMI
                          on success.

  @retval EFI_SUCCESS  Bus successfully allocated and installed with I2cBusHdmi
                       updated and valid.
  @retval *            Other errors are possible.

**/
EFI_STATUS
I2cBusHdmiStart (
  OUT I2C_BUS_HDMI  **I2cBusHdmi
  )
{
  EFI_STATUS    Status;
  I2C_BUS_HDMI  *Bus;

  Bus = AllocateZeroPool (sizeof (*Bus));
  if (!Bus) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  &InstalledDeviceProtocol,
                  Bus,
                  &Bus->DeviceEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:%a]: CreateEvent() - %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    goto fail_FreePool;
  }

  // we only have one device on this bus so we can hard-code the GUID
  Status = gBS->RegisterProtocolNotify (
                  &gTda19988ProtocolGuid,
                  Bus->DeviceEvent,
                  &Bus->DeviceTracker
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:%a]: RegisterProtocolNotify(%g) - %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      &gTda19988ProtocolGuid,
      Status
      ));
    goto fail_CloseEvent;
  }

  Bus->Signature                            = I2C_BUS_SIGNATURE;
  Bus->I2cEnumerate.Enumerate               = I2cEnumerate;
  Bus->I2cEnumerate.GetBusFrequency         = I2cGetBusFrequency;
  Bus->I2cBusConf.EnableI2cBusConfiguration = I2cEnableBusConf;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Bus->Handle,
                  &gCadenceI2cGuid,
                  &mNonDiscoverableCadenceI2cMaster,
                  &gEfiI2cEnumerateProtocolGuid,
                  &Bus->I2cEnumerate,
                  &gEfiI2cBusConfigurationManagementProtocolGuid,
                  &Bus->I2cBusConf,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto fail_CloseEvent;
  }

  *I2cBusHdmi = Bus;
  return EFI_SUCCESS;

fail_CloseEvent:
  {
    EFI_STATUS  CleanupStatus = gBS->CloseEvent (Bus->DeviceEvent);
    if (EFI_ERROR (CleanupStatus)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a:%a]: CloseEvent() - %r\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        CleanupStatus
        ));
    }
  }

fail_FreePool:
  FreePool (Bus);

  return Status;
}

/**
  Free and uninstall UEFI driver bindings for the controller and devices for
  the Morello SoC HDMI I2C bus.

  @param[in,out] I2cBusHdmi  Updated with a pointer to NULL on success.

  @retval EFI_SUCCESS  Bus successfully deallocated and uninstalled with
                       I2cBusHdmi updated and valid.
  @retval EFI_SUCCESS  *I2cBusHdmi is NULL already.
  @retval *            Other errors are possible.

**/
EFI_STATUS
I2cBusHdmiStop (
  IN OUT I2C_BUS_HDMI  **I2cBusHdmi
  )
{
  EFI_STATUS    Status;
  I2C_BUS_HDMI  *Bus = *I2cBusHdmi;

  if (Bus == NULL) {
    return EFI_SUCCESS;
  }

  if (Bus->DeviceEvent) {
    Status = gBS->CloseEvent (Bus->DeviceEvent);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a:%a]: CloseEvent(DeviceEvent) - %r\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        Status
        ));
      return Status;
    }

    Bus->DeviceEvent   = NULL;
    Bus->DeviceTracker = NULL;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Bus->Handle,
                  &gCadenceI2cGuid,
                  &mNonDiscoverableCadenceI2cMaster,
                  &gEfiI2cEnumerateProtocolGuid,
                  &Bus->I2cEnumerate,
                  &gEfiI2cBusConfigurationManagementProtocolGuid,
                  &Bus->I2cBusConf,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:%a]: UninstallMultipleProtocolInterfaces - %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  Bus->Handle = NULL;

  FreePool (Bus);
  I2cBusHdmi = NULL;
  return EFI_SUCCESS;
}
