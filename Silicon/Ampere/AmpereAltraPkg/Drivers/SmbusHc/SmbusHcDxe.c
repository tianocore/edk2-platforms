/** SmbusHc protocol implementation follows SMBus 2.0 specification.

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/GpioLib.h>
#include <Library/I2cLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SmbusHc.h>

#include "SmbusHcCommon.h"

//
// Handle to install SMBus Host Controller protocol.
//
EFI_HANDLE  mSmbusHcHandle = NULL;

/**
  Executes an SMBus operation to an SMBus controller. Returns when either the command has been
  executed or an error is encountered in doing the operation.

  The Execute() function provides a standard way to execute an operation as defined in the System
  Management Bus (SMBus) Specification. The resulting transaction will be either that the SMBus
  slave devices accept this transaction or that this function returns with error.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  SlaveAddress            The SMBus slave address of the device with which to communicate.
  @param  Command                 This command is transmitted by the SMBus host controller to the
                                  SMBus slave device and the interpretation is SMBus slave device
                                  specific. It can mean the offset to a list of functions inside an
                                  SMBus slave device. Not all operations or slave devices support
                                  this command's registers.
  @param  Operation               Signifies which particular SMBus hardware protocol instance that
                                  it will use to execute the SMBus transactions. This SMBus
                                  hardware protocol is defined by the SMBus Specification and is
                                  not related to EFI.
  @param  PecCheck                Defines if Packet Error Code (PEC) checking is required for this
                                  operation.
  @param  Length                  Signifies the number of bytes that this operation will do. The
                                  maximum number of bytes can be revision specific and operation
                                  specific. This field will contain the actual number of bytes that
                                  are executed for this operation. Not all operations require this
                                  argument.
  @param  Buffer                  Contains the value of data to execute to the SMBus slave device.
                                  Not all operations require this argument. The length of this
                                  buffer is identified by Length.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_INVALID_PARAMETER   Operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER   Length/Buffer is NULL for operations except for EfiSmbusQuickRead
                                  and EfiSmbusQuickWrite. Length is outside the range of valid
                                  values.
  @retval EFI_UNSUPPORTED         The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL    Buffer is not sufficient for this operation.

**/
EFI_STATUS
EFIAPI
SmbusHcExecute (
  IN CONST EFI_SMBUS_HC_PROTOCOL     *This,
  IN       EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN       EFI_SMBUS_DEVICE_COMMAND  Command,
  IN       EFI_SMBUS_OPERATION       Operation,
  IN       BOOLEAN                   PecCheck,
  IN OUT   UINTN                     *Length,
  IN OUT   VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  ASSERT (This != NULL);

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  Status = SmbusHcCommonExecute (SlaveAddress, Command, Operation, PecCheck, Length, Buffer);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**

  The SmbusHcArpDevice() function provides a standard way for a device driver to
  enumerate the entire SMBus or specific devices on the bus.

  @param This           A pointer to the EFI_SMBUS_HC_PROTOCOL instance.

  @param ArpAll         A Boolean expression that indicates if the
                        host drivers need to enumerate all the devices
                        or enumerate only the device that is
                        identified by SmbusUdid. If ArpAll is TRUE,
                        SmbusUdid and SlaveAddress are optional. If
                        ArpAll is FALSE, ArpDevice will enumerate
                        SmbusUdid and the address will be at
                        SlaveAddress.

  @param SmbusUdid      The Unique Device Identifier (UDID) that is
                        associated with this device. Type
                        EFI_SMBUS_UDID is defined in
                        EFI_PEI_SMBUS_PPI.ArpDevice() in the
                        Platform Initialization SMBus PPI
                        Specification.

  @param SlaveAddress   The SMBus slave address that is
                        associated with an SMBus UDID.

  @retval EFI_SUCCESS           The last data that was returned from the
                                access matched the poll exit criteria.

  @retval EFI_CRC_ERROR         Checksum is not correct (PEC is
                                incorrect).

  @retval EFI_TIMEOUT           Timeout expired before the operation was
                                completed. Timeout is determined by the
                                SMBus host controller device.

  @retval EFI_OUT_OF_RESOURCES  The request could not be
                                completed due to a lack of
                                resources.

  @retval EFI_DEVICE_ERROR      The request was not completed
                                because a failure was reflected in
                                the Host Status Register bit. Device
                                Errors are a result of a transaction
                                collision, illegal command field,
                                unclaimed cycle (host initiated), or
                                bus errors (collisions).

  @retval EFI_UNSUPPORTED       ArpDevice, GetArpMap, and Notify are
                                not implemented by this driver.

**/
EFI_STATUS
EFIAPI
SmbusHcArpDevice (
  IN CONST EFI_SMBUS_HC_PROTOCOL *This,
  IN       BOOLEAN ArpAll,
  IN       EFI_SMBUS_UDID *SmbusUdid, OPTIONAL
  IN OUT   EFI_SMBUS_DEVICE_ADDRESS *SlaveAddress OPTIONAL
  )
{
  //
  // Not supported
  //
  return EFI_UNSUPPORTED;
}

/**
  The SmbusHcGetArpMap() function returns the mapping of all the SMBus devices
  that were enumerated by the SMBus host driver.

  @param This           A pointer to the EFI_SMBUS_HC_PROTOCOL instance.

  @param Length         Size of the buffer that contains the SMBus
                        device map.

  @param SmbusDeviceMap The pointer to the device map as
                        enumerated by the SMBus controller
                        driver.

  @retval EFI_SUCCESS       The SMBus returned the current device map.

  @retval EFI_UNSUPPORTED   ArpDevice, GetArpMap, and Notify are
                            not implemented by this driver.

**/
EFI_STATUS
EFIAPI
SmbusHcGetArpMap (
  IN CONST EFI_SMBUS_HC_PROTOCOL  *This,
  IN OUT   UINTN                  *Length,
  IN OUT   EFI_SMBUS_DEVICE_MAP   **SmbusDeviceMap
  )
{
  //
  // Not supported
  //
  return EFI_UNSUPPORTED;
}

/**

  The SmbusHcNotify() function registers all the callback functions to
  allow the bus driver to call these functions when the
  SlaveAddress/Data pair happens.

  @param  This            A pointer to the EFI_SMBUS_HC_PROTOCOL instance.

  @param  SlaveAddress    Address that the host controller detects
                          as sending a message and calls all the registered function.

  @param  Data            Data that the host controller detects as sending
                          message and calls all the registered function.


  @param  NotifyFunction  The function to call when the bus
                          driver detects the SlaveAddress and
                          Data pair.

  @retval EFI_SUCCESS       NotifyFunction was registered.

  @retval EFI_UNSUPPORTED   ArpDevice, GetArpMap, and Notify are
                            not implemented by this driver.

**/
EFI_STATUS
EFIAPI
SmbusHcNotify (
  IN CONST EFI_SMBUS_HC_PROTOCOL      *This,
  IN       EFI_SMBUS_DEVICE_ADDRESS   SlaveAddress,
  IN       UINTN                      Data,
  IN       EFI_SMBUS_NOTIFY_FUNCTION  NotifyFunction
  )
{
  //
  // Not supported
  //
  return EFI_UNSUPPORTED;
}

//
// Interface defintion of SMBUS Host Controller Protocol.
//
EFI_SMBUS_HC_PROTOCOL  mSmbusHcProtocol = {
  SmbusHcExecute,
  SmbusHcArpDevice,
  SmbusHcGetArpMap,
  SmbusHcNotify
};

/**
  SmbusHc driver entry point

  @param[in] ImageHandle          ImageHandle of this module
  @param[in] SystemTable          EFI System Table

  @retval EFI_SUCCESS             Driver initializes successfully
  @retval Other values            Some error occurred
**/
EFI_STATUS
InitializeSmbus (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install Smbus protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSmbusHcHandle,
                  &gEfiSmbusHcProtocolGuid,
                  &mSmbusHcProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
