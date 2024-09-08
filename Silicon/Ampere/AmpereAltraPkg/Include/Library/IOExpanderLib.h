/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IO_EXPANDER_LIB_H_
#define IO_EXPANDER_LIB_H_

//
// Value to set config register.
//
#define CONFIG_IOEXPANDER_PIN_AS_INPUT   1
#define CONFIG_IOEXPANDER_PIN_AS_OUTPUT  0

typedef enum {
  IO_EXPANDER_TCA6424A = 0,
  IO_EXPANDER_TCA9534
} IO_EXPANDER_CHIP_ID;

typedef struct {
  IO_EXPANDER_CHIP_ID    ChipID;
  UINT32                 I2cBus;
  UINT32                 I2cAddress;
} IO_EXPANDER_CONTROLLER;

/**
  Return configuration for given IO expander pin.

  @param[in]  Controller          Object contain information of IO expander chip.
  @param[in]  Pin                 Pxx, pin of IO expanderchip.
  @param[out] PinValue            Contain data retrieve.

  @retval EFI_SUCCESS             Get successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         The bus is not supported.
  @retval EFI_NOT_READY           The device/bus is not ready.
  @retval EFI_TIMEOUT             Timeout why transferring data.
  @retval EFI_CRC_ERROR           There are errors on receiving data.
**/
EFI_STATUS
IOExpanderGetDir (
  IN  IO_EXPANDER_CONTROLLER  *Controller,
  IN  UINT8                   Pin,
  OUT UINT8                   *Value
  );

/**
  Set direction for given IO expander pin.

  @param[in] Controller           Object contain information of IO expander chip.
  @param[in] Pin                  Pxx, pin of IO expanderchip.
  @param[in] Value                1 for set as input and 0 for as output.

  @retval EFI_SUCCESS             Get successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         The bus is not supported.
  @retval EFI_NOT_READY           The device/bus is not ready.
  @retval EFI_TIMEOUT             Timeout why transferring data.
  @retval EFI_CRC_ERROR           There are errors on receiving data.
**/
EFI_STATUS
IOExpanderSetDir (
  IN IO_EXPANDER_CONTROLLER  *Controller,
  IN UINT8                   Pin,
  IN UINT8                   Value
  );

/**
  if given pin is configured in Input mode, this function will retrieve data from
  Input Register. Or it is in Output mode, data will be retrieved from Output
  Register.

  @param[in]  Controller          Object contain information of IO expander chip.
  @param[in]  Pin                 Pxx, pin of IO expanderchip.
  @param[out] Value               Contain data retrieved.

  @retval EFI_SUCCESS             Get successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         The bus is not supported.
  @retval EFI_NOT_READY           The device/bus is not ready.
  @retval EFI_TIMEOUT             Timeout why transferring data.
  @retval EFI_CRC_ERROR           There are errors on receiving data.
**/
EFI_STATUS
IOExpanderGetPin (
  IN  IO_EXPANDER_CONTROLLER  *Controller,
  IN  UINT8                   Pin,
  OUT UINT8                   *Value
  );

/**
  Set the Output value for the given Expander IO pin.

  @param[in] Controller           Object contain information of IO expander chip.
  @param[in] Pin                  Pxx, pin of IO expanderchip.
  @param[in] Value                Expected value of output.

  @retval EFI_SUCCESS             Get successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         The bus is not supported.
  @retval EFI_NOT_READY           The device/bus is not ready.
  @retval EFI_TIMEOUT             Timeout why transferring data.
  @retval EFI_CRC_ERROR           There are errors on receiving data.
**/
EFI_STATUS
IOExpanderSetPin (
  IN IO_EXPANDER_CONTROLLER  *Controller,
  IN UINT8                   Pin,
  IN UINT8                   Value
  );

#endif /* IO_EXPANDER_LIB_H_ */
