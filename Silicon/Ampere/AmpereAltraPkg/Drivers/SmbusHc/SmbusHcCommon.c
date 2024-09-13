/** @file
  SmbusHcCommon implement common api for SmbusHc

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/GpioLib.h>
#include <Library/I2cLib.h>
#include <Library/PcdLib.h>

#include "SmbusHcCommon.h"

/**
  Incremental calculate Pec base on previous Pec value and CRC8 of data array
  pointed to by Buffer

  @param  Pec        Previous Pec
  @param  Buffer     Pointer to data array
  @param  Length     Array count

  @retval Pec

**/
UINT8
CalculatePec (
  UINT8   Pec,
  UINT8   *Buffer,
  UINT32  Length
  )
{
  UINT8  Offset, Index;

  for (Offset = 0; Offset < Length; Offset++) {
    Pec ^= Buffer[Offset];
    for (Index = 0; Index < 8; Index++) {
      if ((Pec & 0x80) != 0) {
        Pec = (UINT8)((Pec << 1) ^ CRC8_POLYNOMINAL_KEY);
      } else {
        Pec <<= 1;
      }
    }
  }

  return Pec & 0xFF;
}

/**
  Executes an SMBus operation to an SMBus controller. Returns when either the command has been
  executed or an error is encountered in doing the operation.

  The Execute() function provides a standard way to execute an operation as defined in the System
  Management Bus (SMBus) Specification. The resulting transaction will be either that the SMBus
  slave devices accept this transaction or that this function returns with error.

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
SmbusHcCommonExecute (
  IN       EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN       EFI_SMBUS_DEVICE_COMMAND  Command,
  IN       EFI_SMBUS_OPERATION       Operation,
  IN       BOOLEAN                   PecCheck,
  IN OUT   UINTN                     *Length,
  IN OUT   VOID                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       DataLen, Idx;
  UINT8       ReadTemp[SMBUS_READ_TEMP_LENGTH];
  UINT8       WriteTemp[SMBUS_WRITE_TEMP_LENGTH];
  UINT8       CrcTemp[10];
  UINT8       Pec;

  if (  ((Operation != EfiSmbusQuickRead) && (Operation != EfiSmbusQuickWrite))
     && ((Length == NULL) || (Buffer == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Switch to correct I2C bus and speed
  //
  Status = I2cProbe (I2C_BUS_NUMBER, I2C_BUS_SPEED, TRUE, PecCheck);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Process Operation
  //
  switch (Operation) {
    case EfiSmbusWriteBlock:
      if (*Length > SMBUS_MAX_BLOCK_LENGTH) {
        return EFI_INVALID_PARAMETER;
      }

      WriteTemp[0] = Command;
      WriteTemp[1] = *Length;
      CopyMem (&WriteTemp[2], Buffer, *Length);
      DataLen = *Length + 2;

      //
      // PEC handling
      //
      if (PecCheck) {
        CrcTemp[0] = I2C_WRITE_ADDRESS (SlaveAddress.SmbusDeviceAddress);
        Pec        = CalculatePec (0, &CrcTemp[0], 1);
        Pec        = CalculatePec (Pec, WriteTemp, DataLen);
        DEBUG ((DEBUG_VERBOSE, "\nWriteBlock PEC = 0x%x \n", Pec));
        WriteTemp[DataLen] = Pec;
        DataLen           += 1;
      }

      DEBUG ((DEBUG_VERBOSE, "W %d: ", DataLen));
      for (Idx = 0; Idx < DataLen; Idx++) {
        DEBUG ((DEBUG_VERBOSE, "0x%x ", WriteTemp[Idx]));
      }

      DEBUG ((DEBUG_VERBOSE, "\n"));

      Status = I2cWrite (
                 I2C_BUS_NUMBER,
                 SlaveAddress.SmbusDeviceAddress,
                 WriteTemp,
                 (UINT32 *)&DataLen
                 );
      if (EFI_ERROR (Status)) {
        if (Status != EFI_TIMEOUT) {
          Status = EFI_DEVICE_ERROR;
        }
      }

      break;

    case EfiSmbusReadBlock:
      WriteTemp[0] = Command;
      DataLen      = *Length + 2; // +1 byte for Data Length +1 byte for PEC
      Status       = I2cRead (
                       I2C_BUS_NUMBER,
                       SlaveAddress.SmbusDeviceAddress,
                       WriteTemp,
                       1,
                       ReadTemp,
                       (UINT32 *)&DataLen
                       );
      if (EFI_ERROR (Status)) {
        if (Status != EFI_TIMEOUT) {
          Status = EFI_DEVICE_ERROR;
        }

        *Length = 0;
        break;
      }

      DEBUG ((DEBUG_VERBOSE, "R %d: ", DataLen));
      for (Idx = 0; Idx < DataLen; Idx++) {
        DEBUG ((DEBUG_VERBOSE, "0x%x ", ReadTemp[Idx]));
      }

      DEBUG ((DEBUG_VERBOSE, "\n"));

      DataLen = ReadTemp[0];

      //
      // PEC handling
      //
      if (PecCheck) {
        CrcTemp[0] = I2C_WRITE_ADDRESS (SlaveAddress.SmbusDeviceAddress);
        CrcTemp[1] = Command;
        CrcTemp[2] = I2C_READ_ADDRESS (SlaveAddress.SmbusDeviceAddress);

        Pec = CalculatePec (0, &CrcTemp[0], 3);
        Pec = CalculatePec (Pec, ReadTemp, DataLen + 1);

        if (Pec != ReadTemp[DataLen + 1]) {
          DEBUG ((DEBUG_ERROR, "ReadBlock PEC cal = 0x%x != 0x%x\n", Pec, ReadTemp[DataLen + 1]));
          return EFI_CRC_ERROR;
        } else {
          DEBUG ((DEBUG_VERBOSE, "ReadBlock PEC 0x%x\n", ReadTemp[DataLen + 1]));
        }
      }

      if ((DataLen == 0) || (DataLen > SMBUS_MAX_BLOCK_LENGTH)) {
        DEBUG ((DEBUG_ERROR, "%a: Invalid length = %d\n", __func__, DataLen));
        *Length = 0;
        Status  = EFI_INVALID_PARAMETER;
      } else if (DataLen > *Length) {
        DEBUG ((DEBUG_ERROR, "%a: Buffer too small\n", __func__));
        *Length = 0;
        Status  = EFI_BUFFER_TOO_SMALL;
      } else {
        *Length = DataLen;
        CopyMem (Buffer, &ReadTemp[1], DataLen);
      }

      break;

    case EfiSmbusQuickRead:
    case EfiSmbusQuickWrite:
    case EfiSmbusReceiveByte:
    case EfiSmbusSendByte:
    case EfiSmbusReadByte:
    case EfiSmbusWriteByte:
    case EfiSmbusReadWord:
    case EfiSmbusWriteWord:
    case EfiSmbusProcessCall:
    case EfiSmbusBWBRProcessCall:
      DEBUG ((DEBUG_ERROR, "%a: Unsupported command\n", __func__));
      Status = EFI_UNSUPPORTED;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}
