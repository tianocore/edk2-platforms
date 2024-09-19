/** @file
  Library implementation for the Designware GPIO controller.

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GPIO_LIB_H_
#define GPIO_LIB_H_

typedef enum {
  GpioConfigOutLow = 0,
  GpioConfigOutHigh,
  GpioConfigOutLowToHigh,
  GpioConfigOutHightToLow,
  GpioConfigIn,
  MaxGpioConfigMode
} GPIO_CONFIG_MODE;

/*
 *  GpioWriteBit: Use to Set/Clear GPIOs
 *  Input:
 *              Pin : Pin Identification
 *              Val : 1 to Set, 0 to Clear
 */
VOID
EFIAPI
GpioWriteBit (
  IN UINT32  Pin,
  IN UINT32  Val
  );

/*
 *   GpioReadBit:
 *   Input:
 *              Pin : Pin Identification
 *   Return:
 *              1 : On/High
 *              0 : Off/Low
 */
UINTN
EFIAPI
GpioReadBit (
  IN UINT32  Pin
  );

/*
 *  GpioModeConfig: Use to configure GPIOs as Input/Output
 *  Input:
 *              Pin : Pin Identification
 *              InOut : GPIO_OUT/1 as Output
 *                      GPIO_IN/0  as Input
 */
EFI_STATUS
EFIAPI
GpioModeConfig (
  UINT8             Pin,
  GPIO_CONFIG_MODE  Mode
  );

/*
 *  Setup a controller that to be used in runtime service.
 *  Input:
 *              Pin: Pin belongs to the controller.
 *  return:     0 for success.
 *              Otherwise, error code.
 */
EFI_STATUS
EFIAPI
GpioSetupRuntime (
  IN UINT32  Pin
  );

#endif /* GPIO_LIB_H_ */
