/** @file
  Cadence I2C controller UEFI Driver

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MORELLO_CADENCEI2CDXE_DRIVER_H_
#define MORELLO_CADENCEI2CDXE_DRIVER_H_

#include <Protocol/CadenceI2c.h>
#include <Protocol/DevicePath.h>
#include <Protocol/I2cMaster.h>

#pragma pack(1)
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL    Header;
  EFI_GUID                    Guid;
  EFI_PHYSICAL_ADDRESS        MmioBase;
} CADENCE_I2C_DEVICE_PATH;

typedef struct {
  CADENCE_I2C_DEVICE_PATH     Vendor;
  EFI_DEVICE_PATH_PROTOCOL    End;
} CADENCE_I2C_DEVICE_PATH_PROTOCOL;
#pragma pack()

/// Device context struct for Cadence I2C controller
typedef struct {
  UINT64                                Signature;

  /// Probed RTL parameter cdnsi2c_p_fifo_depth
  UINT16                                FifoSize;

  /// Consumed protocols
  CONST CADENCE_I2C_INSTALL_PROTOCOL    *CadenceI2cInstall;

  /// Produced protocols
  CADENCE_I2C_DEVICE_PATH_PROTOCOL      DevicePath;
  EFI_I2C_MASTER_PROTOCOL               I2cMaster;
} CADENCE_I2C_CONTEXT;

/**
  Initialize a Cadence I2C controller and connect it to the device
  context instance Dev.

  @param[in,out] Dev  Device context.

  @retval  EFI_SUCCESS      Hardware probed and connected to the device context
                            Dev.
  @retval  EFI_UNSUPPORTED  The hardware is not compatible with this driver.
  @retval  *                Other errors are possible.

**/
EFI_STATUS
DriverStart (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  );

/**
  Disconnect the hardware from the driver instance.

  @param[in,out] Dev  Device context.
**/
VOID
DriverStop (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  );

/**
  Set the frequency for the I2C clock line.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_SET_BUS_FREQUENCY.

  @param[in,out] Dev            Device context.
  @param[in,out] BusClockHertz  Pointer to the requested I2C bus clock frequency
                                in Hertz.
                                Upon return this value contains the actual
                                frequency in use by the I2C controller.

  @retval EFI_SUCCESS     The bus frequency was set successfully.
  @retval EFI_UNSUPPORTED The controller does not support this frequency.

**/
EFI_STATUS
DriverSetBusFrequency (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  IN OUT UINTN                *BusClockHertz
  );

/**
  Reset the I2C controller and configure it for use.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_RESET.

  @param[in,out] Dev  Instance context.

  @retval EFI_SUCCESS            The reset completed successfully.
  @retval EFI_DEVICE_ERROR       The reset operation failed.

**/
EFI_STATUS
DriverReset (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  );

/**
  Start an I2C transaction on the host controller.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_START_REQUEST.

  @param[in,out] Dev              Instance context.
  @param[in]     SlaveAddress     Address of the device on the I2C bus.
  @param[in]     ExtendedAddress  TRUE if SlaveAddress is 10 bits.
  @param[in,out] RequestPacket    Pointer to an EFI_I2C_REQUEST_PACKET
                                  structure describing the I2C transaction.
  @param[in]     Event            Event to signal for asynchronous transactions,
                                  NULL for synchronous transactions.
  @param[out]    I2cStatus        Optional buffer to receive the I2C transaction
                                  completion status.

  @retval EFI_SUCCESS           The asynchronous transaction was successfully
                                started when Event is not NULL.
  @retval EFI_SUCCESS           The transaction completed successfully when
                                Event is NULL.
  @retval EFI_ALREADY_STARTED   The controller is busy with another transaction.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the
                                transaction.
  @retval EFI_NOT_FOUND         Reserved bit set in the SlaveAddress parameter
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the slave
                                address.  EFI_DEVICE_ERROR will be returned if
                                the controller cannot distinguish when the NACK
                                occurred.
  @retval EFI_UNSUPPORTED       The controller does not support the requested
                                transaction.

**/
EFI_STATUS
DriverStartRequest (
  IN OUT CADENCE_I2C_CONTEXT     *Dev,
  IN     UINTN                   SlaveAddress,
  IN     BOOLEAN                 ExtendedAddress,
  IN OUT EFI_I2C_REQUEST_PACKET  *RequestPacket,
  IN     EFI_EVENT               Event,
  OUT    EFI_STATUS              *I2cStatus
  );

#endif // MORELLO_CADENCEI2CDXE_DRIVER_H_
