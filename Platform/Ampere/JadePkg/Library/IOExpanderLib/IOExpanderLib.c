/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IOExpanderLib.h>
#include <Library/I2cLib.h>

//
// Addresses of registers inside IO expander chip
//
#define TCA6424A_INPUT_REGISTER_BANK     {0x00, 0x01, 0x02}
#define TCA6424A_OUTPUT_REGISTER_BANK    {0x04, 0x05, 0x06}
#define TCA6424A_POLARITY_REGISTER_BANK  {0x08, 0x09, 0x0A}
#define TCA6424A_CONFIG_REGISTER_BANK    {0x0C, 0x0D, 0x0E}

#define TCA9534_INPUT_REGISTER_BANK     {0x00}
#define TCA9534_OUTPUT_REGISTER_BANK    {0x01}
#define TCA9534_POLARITY_REGISTER_BANK  {0x02}
#define TCA9534_CONFIG_REGISTER_BANK    {0x03}

CONST UINT8  gTca6424aRegister[4][3] = {
  TCA6424A_INPUT_REGISTER_BANK,
  TCA6424A_OUTPUT_REGISTER_BANK,
  TCA6424A_POLARITY_REGISTER_BANK,
  TCA6424A_CONFIG_REGISTER_BANK
};
CONST UINT8  gTca9534Register[4][1] = {
  TCA9534_INPUT_REGISTER_BANK,
  TCA9534_OUTPUT_REGISTER_BANK,
  TCA9534_POLARITY_REGISTER_BANK,
  TCA9534_CONFIG_REGISTER_BANK
};

//
// Boundary of Pin number
//
#define PINS_PER_REGISTER   8
#define REGISTERS_PER_BANK  3

#define INPUT_PORT_REGISTERS          0
#define OUTPUT_PROT_REGISTERS         1
#define POLARITY_INVERSION_REGISTERS  2
#define CONFIGURATION_REGISTERS       3

//
// Pin port/bin manipulation on TCA6424A and TCA9534
//
#define GET_REGISTER_ORDER(p)         (UINT8)(p / 10)
#define GET_BIT_OF_PIN(p)             (UINT8)(p % 10)
#define GET_BIT_VALUE(register, bit)  (UINT8)((register >> bit) & 0x01)
#define SET_BIT(register, bit)        (UINT8)(register | ~(0xFE << bit))
#define CLEAR_BIT(register, bit)      (UINT8)(register & (0xFE << bit))

#define GET_REGISTER_BANK(x)  ((x) ? 0 : 1)

#define IO_EXPANDER_I2C_BUS_SPEED  100000

STATIC
EFI_STATUS
IOExpanderI2cRead (
  IN  UINT32  I2cBus,
  IN  UINT32  I2cAddress,
  IN  UINT8   *RegisterAddress,
  IN  UINT32  *NumberOfRegister,
  OUT UINT8   *Data
  )
{
  EFI_STATUS  Status;
  UINT32      NumberOfRegisterAddress;

  Status = I2cProbe (I2cBus, IO_EXPANDER_I2C_BUS_SPEED, FALSE, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NumberOfRegisterAddress = 1;
  Status                  = I2cRead (I2cBus, I2cAddress, RegisterAddress, NumberOfRegisterAddress, Data, NumberOfRegister);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
IOExpanderI2cWrite (
  IN UINT32        I2cBus,
  IN UINT32        I2cAddress,
  IN CONST UINT8   *RegisterAddress,
  IN CONST UINT32  *NumberOfRegister,
  IN UINT8         *Data
  )
{
  EFI_STATUS  Status;
  UINT8       Buffer[*NumberOfRegister + 1];
  UINT32      WriteSize;

  Status = I2cProbe (I2cBus, IO_EXPANDER_I2C_BUS_SPEED, FALSE, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteSize = sizeof (Buffer) - 1;
  *Buffer   = *RegisterAddress;
  CopyMem ((VOID *)(Buffer + 1), (VOID *)Data, WriteSize);
  Status = I2cWrite (I2cBus, I2cAddress, Buffer, &WriteSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

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
  IN OUT IO_EXPANDER_CONTROLLER  *Controller,
  IN     UINT8                   Pin,
  OUT    UINT8                   *Value
  )
{
  EFI_STATUS  Status;
  UINT32      NumberOfRegister;
  UINT8       RegisterAddress;
  UINT8       RegisterData;

  NumberOfRegister = 1;
  switch (Controller->ChipID) {
    case IO_EXPANDER_TCA6424A:
      RegisterAddress = gTca6424aRegister[CONFIGURATION_REGISTERS][GET_REGISTER_ORDER (Pin)];
      break;

    case IO_EXPANDER_TCA9534:
      RegisterAddress = gTca9534Register[CONFIGURATION_REGISTERS][GET_REGISTER_ORDER (Pin)];
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IOExpanderI2cRead (
             Controller->I2cBus,
             Controller->I2cAddress,
             &RegisterAddress,
             &NumberOfRegister,
             &RegisterData
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Value = GET_BIT_VALUE (RegisterData, GET_BIT_OF_PIN (Pin));

  return EFI_SUCCESS;
}

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
EFIAPI
IOExpanderSetDir (
  IN IO_EXPANDER_CONTROLLER  *Controller,
  IN UINT8                   Pin,
  IN UINT8                   Value
  )
{
  EFI_STATUS  Status;
  UINT8       PinOldValue;
  UINT32      NumberOfRegister;
  UINT8       RegisterAddress;
  UINT8       RegisterData;

  NumberOfRegister = 1;
  switch (Controller->ChipID) {
    case IO_EXPANDER_TCA6424A:
      RegisterAddress = gTca6424aRegister[CONFIGURATION_REGISTERS][GET_REGISTER_ORDER (Pin)];
      break;

    case IO_EXPANDER_TCA9534:
      RegisterAddress = gTca9534Register[CONFIGURATION_REGISTERS][GET_REGISTER_ORDER (Pin)];
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IOExpanderI2cRead (
             Controller->I2cBus,
             Controller->I2cAddress,
             &RegisterAddress,
             &NumberOfRegister,
             &RegisterData
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PinOldValue = GET_BIT_VALUE (RegisterData, GET_BIT_OF_PIN (Pin));

  if (Value == PinOldValue) {
    return EFI_SUCCESS;
  }

  if (Value) {
    RegisterData = SET_BIT (RegisterData, GET_BIT_OF_PIN (Pin));
  } else {
    RegisterData = CLEAR_BIT (RegisterData, GET_BIT_OF_PIN (Pin));
  }

  NumberOfRegister = 1;
  Status           = IOExpanderI2cWrite (
                       Controller->I2cBus,
                       Controller->I2cAddress,
                       &RegisterAddress,
                       &NumberOfRegister,
                       &RegisterData
                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      NumberOfRegister;
  UINT8       ConfigValue;
  UINT8       RegisterAddress;
  UINT8       RegisterData;

  NumberOfRegister = 1;
  Status           = IOExpanderGetDir (
                       Controller,
                       Pin,
                       &ConfigValue
                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (Controller->ChipID) {
    case IO_EXPANDER_TCA6424A:
      RegisterAddress = gTca6424aRegister[GET_REGISTER_BANK (ConfigValue)][GET_REGISTER_ORDER (Pin)];
      break;

    case IO_EXPANDER_TCA9534:
      RegisterAddress = gTca9534Register[GET_REGISTER_BANK (ConfigValue)][GET_REGISTER_ORDER (Pin)];
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IOExpanderI2cRead (
             Controller->I2cBus,
             Controller->I2cAddress,
             &RegisterAddress,
             &NumberOfRegister,
             &RegisterData
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Value = GET_BIT_VALUE (RegisterData, GET_BIT_OF_PIN (Pin));

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      NumberOfRegister;
  UINT8       RegisterAddress;
  UINT8       RegisterData;

  switch (Controller->ChipID) {
    case IO_EXPANDER_TCA6424A:
      RegisterAddress = gTca6424aRegister[OUTPUT_PROT_REGISTERS][GET_REGISTER_ORDER (Pin)];
      break;

    case IO_EXPANDER_TCA9534:
      RegisterAddress = gTca9534Register[OUTPUT_PROT_REGISTERS][GET_REGISTER_ORDER (Pin)];
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  NumberOfRegister = 1;
  Status           = IOExpanderI2cRead (
                       Controller->I2cBus,
                       Controller->I2cAddress,
                       &RegisterAddress,
                       &NumberOfRegister,
                       &RegisterData
                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Value == GET_BIT_VALUE (RegisterData, GET_BIT_OF_PIN (Pin))) {
    return EFI_SUCCESS;
  }

  if (Value) {
    RegisterData = SET_BIT (RegisterData, GET_BIT_OF_PIN (Pin));
  } else {
    RegisterData = CLEAR_BIT (RegisterData, GET_BIT_OF_PIN (Pin));
  }

  Status = IOExpanderI2cWrite (
             Controller->I2cBus,
             Controller->I2cAddress,
             &RegisterAddress,
             &NumberOfRegister,
             &RegisterData
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
