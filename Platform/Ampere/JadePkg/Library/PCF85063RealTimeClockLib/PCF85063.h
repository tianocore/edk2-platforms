/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCF85063_H_
#define PCF85063_H_

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/RealTimeClockLib.h>

//
// I2C bus address that RTC connected to
//
#define I2C_RTC_BUS_ADDRESS  (FixedPcdGet8 (PcdRtcBusNumber))

//
// I2C RTC bus speed
//
#define I2C_RTC_BUS_SPEED  100000

//
// I2C chip address that RTC connected to
//
#define I2C_RTC_CHIP_ADDRESS  0x51

//
// The GPI PIN that tell if RTC can be access
//
#define I2C_RTC_ACCESS_GPIO_PIN  28

/**
 * Returns the current time and date information of the hardware platform.
 *
 * @param  Time                  A pointer to storage to receive a snapshot of the current time.
 *
 *
 * @retval EFI_SUCCESS           The operation completed successfully.
 * @retval EFI_INVALID_PARAMETER Time is NULL.
 * @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.
 */
EFI_STATUS
EFIAPI
PlatformGetTime (
  OUT EFI_TIME  *Time
  );

/**
 * Set the time and date information to the hardware platform.
 *
 * @param  Time                  A pointer to storage to set the current time to hardware platform.
 *
 *
 * @retval EFI_SUCCESS           The operation completed successfully.
 * @retval EFI_INVALID_PARAMETER Time is NULL.
 * @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware error.
 **/
EFI_STATUS
EFIAPI
PlatformSetTime (
  IN EFI_TIME  *Time
  );

/**
 * Callback function for hardware platform to convert data pointers to virtual address
 */
VOID
EFIAPI
PlatformVirtualAddressChangeEvent (
  VOID
  );

/**
 * Callback function for hardware platform to initialize private data
 *
 *
 * @retval EFI_SUCCESS           The operation completed successfully.
 * @retval Others                The error status indicates the error
 */
EFI_STATUS
EFIAPI
PlatformInitialize (
  VOID
  );

#endif /* PCF85063_H_ */
