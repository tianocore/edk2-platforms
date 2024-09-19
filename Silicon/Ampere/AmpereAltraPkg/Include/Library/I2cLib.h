/** @file
  Library implementation for the Designware I2C controller.

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef I2C_LIB_H_
#define I2C_LIB_H_

#include <Uefi/UefiBaseType.h>

/**
  Write to I2C bus.

  @param[in]     Bus          I2C bus Id.
  @param[in]     SlaveAddr    The address of slave device on the bus.
  @param[in,out] Buf          Buffer that holds data to write.
  @param[in,out] WriteLength  Pointer to length of buffer.

  @return EFI_SUCCESS            Write successfully.
  @return EFI_INVALID_PARAMETER  A parameter is invalid.
  @return EFI_UNSUPPORTED        The bus is not supported.
  @return EFI_NOT_READY          The device/bus is not ready.
  @return EFI_TIMEOUT            Timeout why transferring data.

**/
EFI_STATUS
EFIAPI
I2cWrite (
  IN     UINT32  Bus,
  IN     UINT32  SlaveAddr,
  IN OUT UINT8   *Buf,
  IN OUT UINT32  *WriteLength
  );

/**
  Read data from I2C bus.

  @param[in]     Bus          I2C bus Id.
  @param[in]     SlaveAddr    The address of slave device on the bus.
  @param[in]     BufCmd       Buffer where to send the command.
  @param[in]     CmdLength    Pointer to length of BufCmd.
  @param[in,out] Buf          Buffer where to put the read data to.
  @param[in,out] ReadLength   Pointer to length of buffer.

  @return EFI_SUCCESS            Read successfully.
  @return EFI_INVALID_PARAMETER  A parameter is invalid.
  @return EFI_UNSUPPORTED        The bus is not supported.
  @return EFI_NOT_READY          The device/bus is not ready.
  @return EFI_TIMEOUT            Timeout why transferring data.
  @return EFI_CRC_ERROR          There are errors on receiving data.

**/
EFI_STATUS
EFIAPI
I2cRead (
  IN     UINT32  Bus,
  IN     UINT32  SlaveAddr,
  IN     UINT8   *BufCmd,
  IN     UINT32  CmdLength,
  IN OUT UINT8   *Buf,
  IN OUT UINT32  *ReadLength
  );

/**
 Setup new transaction with I2C slave device.

  @param[in] Bus      I2C bus Id.
  @param[in] BusSpeed I2C bus speed in Hz.
  @param[in] IsSmbus  Flag to indicate if the bus is used to execute an SMBus operation.
  @param[in] PecCheck If Packet Error Code (PEC) checking is required for the SMBUS operation
                      and is ignored when present in other operations.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.

**/
EFI_STATUS
EFIAPI
I2cProbe (
  IN UINT32   Bus,
  IN UINTN    BusSpeed,
  IN BOOLEAN  IsSmbus,
  IN BOOLEAN  PecCheck
  );

/**
 Setup a bus that to be used in runtime service.

  @param[in] Bus I2C bus Id.

  @retval EFI_SUCCESS  Success.
  @retval Otherwise    Error code.

**/
EFI_STATUS
EFIAPI
I2cSetupRuntime (
  IN UINT32  Bus
  );

#endif /* I2C_LIB_H_ */
