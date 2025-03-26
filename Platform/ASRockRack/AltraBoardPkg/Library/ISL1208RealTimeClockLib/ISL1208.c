/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/GpioLib.h>
#include <Library/I2cLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "ISL1208.h"

#define RTC_TIMEOUT_WAIT_ACCESS        100000 /* 100 miliseconds */
#define RTC_DEFAULT_MIN_YEAR           2000
#define RTC_DEFAULT_MAX_YEAR           2099

#define RTC_ADDR                       0
#define RTC_DATA_BUF_LEN               8

/**
 * ISL1208 register offsets
 */
#define ISL1208_OFFSET_SEC            0x0
#define ISL1208_OFFSET_MIN            0x1
#define ISL1208_OFFSET_HR             0x2
#define ISL1208_OFFSET_DAY            0x3
#define ISL1208_OFFSET_MON            0x4
#define ISL1208_OFFSET_YEA            0x5
#define ISL1208_OFFSET_WKD            0x6

/* Buffer pointers to convert Vir2Phys and Phy2Vir */
STATIC volatile UINT64 RtcBufVir;
STATIC volatile UINT64 RtcBufPhy;

STATIC
EFI_STATUS
RtcI2cWaitAccess (
  VOID
  )
{
  INTN Timeout;

  Timeout = RTC_TIMEOUT_WAIT_ACCESS;
  while ((GpioReadBit (I2C_RTC_ACCESS_GPIO_PIN) != 0) && (Timeout > 0)) {
    MicroSecondDelay (100);
    Timeout -= 100;
  }

  if (Timeout <= 0) {
    DEBUG ((DEBUG_ERROR, "%a: Timeout while waiting access RTC\n", __FUNCTION__));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
RtcI2cRead (
  IN     UINT8  Addr,
  IN OUT UINT64 Data,
  IN     UINT32 DataLen
  )
{
  EFI_STATUS Status;
  UINT32     TmpLen;

  if (EFI_ERROR (RtcI2cWaitAccess ())) {
    return EFI_DEVICE_ERROR;
  }

  Status = I2cProbe (I2C_RTC_BUS_ADDRESS, I2C_RTC_BUS_SPEED, FALSE, FALSE);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Send the slave address for read
  //
  TmpLen = 1;
  Status = I2cWrite (I2C_RTC_BUS_ADDRESS, I2C_RTC_CHIP_ADDRESS, (UINT8 *)&Addr, &TmpLen);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Read back the time
  //
  Status = I2cRead (I2C_RTC_BUS_ADDRESS, I2C_RTC_CHIP_ADDRESS, NULL, 0, (UINT8 *)Data, &DataLen);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RtcI2cWrite (
  IN UINT8  Addr,
  IN UINT64 Data,
  IN UINT32 DataLen
  )
{
  EFI_STATUS Status;
  UINT8      TmpBuf[RTC_DATA_BUF_LEN + 1];
  UINT32     TmpLen;

  if (EFI_ERROR (RtcI2cWaitAccess ())) {
    return EFI_DEVICE_ERROR;
  }

  if (DataLen > sizeof (TmpBuf) - 1) {
    return EFI_INVALID_PARAMETER;
  }

  Status = I2cProbe (I2C_RTC_BUS_ADDRESS, I2C_RTC_BUS_SPEED, FALSE, FALSE);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // The first byte is the address
  //
  TmpBuf[0] = Addr;
  TmpLen = DataLen + 1;
  CopyMem ((VOID *)(TmpBuf + 1), (VOID *)Data, DataLen);

  Status = I2cWrite (I2C_RTC_BUS_ADDRESS, I2C_RTC_CHIP_ADDRESS, TmpBuf, &TmpLen);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  OUT EFI_TIME *Time
  )
{
  EFI_STATUS Status;
  UINT8      *Data;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = RtcI2cRead (RTC_ADDR, RtcBufVir, RTC_DATA_BUF_LEN);

  Data = (UINT8 *)RtcBufVir;
  if (Status == EFI_SUCCESS) {
    Time->Second = BcdToDecimal8 (Data[ISL1208_OFFSET_SEC] & 0x7F);
    Time->Minute = BcdToDecimal8 (Data[ISL1208_OFFSET_MIN] & 0x7F);
    Time->Hour   = BcdToDecimal8 (Data[ISL1208_OFFSET_HR]  & 0x3F);
    Time->Day    = BcdToDecimal8 (Data[ISL1208_OFFSET_DAY] & 0x3F);
    Time->Month  = BcdToDecimal8 (Data[ISL1208_OFFSET_MON] & 0x1F);
    Time->Year   = BcdToDecimal8 (Data[ISL1208_OFFSET_YEA] & 0xFF);
    Time->Year  += RTC_DEFAULT_MIN_YEAR;
    if (Time->Year > RTC_DEFAULT_MAX_YEAR) {
      Time->Year = RTC_DEFAULT_MAX_YEAR;
    }
    if (Time->Year < RTC_DEFAULT_MIN_YEAR) {
      Time->Year = RTC_DEFAULT_MIN_YEAR;
    }
  }

  return Status;
}

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
  IN EFI_TIME *Time
  )
{
  UINT8 *Data;
  EFI_STATUS Status;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Time->Year < RTC_DEFAULT_MIN_YEAR ||
      Time->Year > RTC_DEFAULT_MAX_YEAR)
  {
    return EFI_INVALID_PARAMETER;
  }

  Data = (UINT8 *)RtcBufVir;
  Data[ISL1208_OFFSET_SEC] = DecimalToBcd8 (Time->Second);
  Data[ISL1208_OFFSET_MIN] = DecimalToBcd8 (Time->Minute);
  Data[ISL1208_OFFSET_HR]  = DecimalToBcd8 (Time->Hour);
  Data[ISL1208_OFFSET_DAY] = DecimalToBcd8 (Time->Day);
  Data[ISL1208_OFFSET_MON] = DecimalToBcd8 (Time->Month);
  Data[ISL1208_OFFSET_YEA] = DecimalToBcd8 (Time->Year - RTC_DEFAULT_MIN_YEAR);

  Status = RtcI2cWrite (RTC_ADDR, RtcBufVir, RTC_DATA_BUF_LEN);
  if (EFI_ERROR (Status)) {
	  DEBUG ((DEBUG_ERROR, "Failed to write RTC: %r\n", Status));
  }

  return Status;
}

/**
 * Callback function for hardware platform to convert data pointers to virtual address
 */
VOID
EFIAPI
PlatformVirtualAddressChangeEvent (
  VOID
  )
{
  EfiConvertPointer (0x0, (VOID **)&RtcBufVir);
}


typedef struct {
	UINT8 TotalPowerFailure : 1;
	UINT8 BatteryBackupMode : 1;
	UINT8 Alarm             : 1;
	UINT8 Reserved1         : 1;
	UINT8 Write             : 1;
	UINT8 Reserved2         : 1;
	UINT8 ExternalCrystal   : 1;
	UINT8 AutoReset         : 1;
} RTC_STATUS;

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
  )
{
  EFI_STATUS Status;

  /*
   * Allocate the buffer for RTC data
   * The buffer can be accessible after ExitBootServices
   */
  RtcBufVir = (UINT64)AllocateRuntimeZeroPool (RTC_DATA_BUF_LEN);
  ASSERT_EFI_ERROR (RtcBufVir);
  RtcBufPhy = (UINT64)RtcBufVir;

  Status = I2cSetupRuntime (I2C_RTC_BUS_ADDRESS);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d I2cSetupRuntime() failed - %r \n",
      __func__,
      __LINE__,
      Status
      ));
    return Status;
  }

  Status = GpioSetupRuntime (I2C_RTC_ACCESS_GPIO_PIN);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d GpioSetupRuntime() failed - %r \n",
      __func__,
      __LINE__,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}
