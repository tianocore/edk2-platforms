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

#include "PCF8563.h"

#define RTC_TIMEOUT_WAIT_ACCESS  100000       /* 100 miliseconds */
#define RTC_DEFAULT_MIN_YEAR     2000
#define RTC_DEFAULT_MAX_YEAR     2099

#define RTC_ADDR          0x2
#define RTC_DATA_BUF_LEN  8

/**
 * PCF8563 register offsets
 */
#define PCF8563_OFFSET_SEC  0x0
#define PCF8563_OFFSET_MIN  0x1
#define PCF8563_OFFSET_HR   0x2
#define PCF8563_OFFSET_DAY  0x3
#define PCF8563_OFFSET_WKD  0x4
#define PCF8563_OFFSET_MON  0x5
#define PCF8563_OFFSET_YEA  0x6

/**
 * PCF8563 encoding macros
 */
#define PCF8563_SEC_ENC(s)  (((((s) / 10) & 0x7) << 4) | (((s) % 10) & 0xf))
#define PCF8563_MIN_ENC(m)  (((((m) / 10) & 0x7) << 4) | (((m) % 10) & 0xf))
#define PCF8563_HR_ENC(h)   (((((h) / 10) & 0x3) << 4) | (((h) % 10) & 0xf))
#define PCF8563_DAY_ENC(d)  (((((d) / 10) & 0x3) << 4) | (((d) % 10) & 0xf))
#define PCF8563_WKD_ENC(w)  ((w) & 0x7)
#define PCF8563_MON_ENC(m)  (((((m) / 10) & 0x1) << 4) | (((m) % 10) & 0xf))
#define PCF8563_YEA_ENC(y)  (((((y) / 10) & 0xf) << 4) | (((y) % 10) & 0xf))

/**
 * PCF8563 decoding macros
 */
#define PCF8563_SEC_DEC(s)  (((((s) & 0x70) >> 4) * 10) + ((s) & 0xf))
#define PCF8563_MIN_DEC(m)  (((((m) & 0x70) >> 4) * 10) + ((m) & 0xf))
#define PCF8563_HR_DEC(h)   (((((h) & 0x30) >> 4) * 10) + ((h) & 0xf))
#define PCF8563_DAY_DEC(d)  (((((d) & 0x30) >> 4)* 10) + ((d) & 0xf))
#define PCF8563_WKD_DEC(w)  ((w) & 0x7)
#define PCF8563_MON_DEC(m)  (((((m) & 0x10) >> 4) * 10) + ((m) & 0xf))
#define PCF8563_YEA_DEC(y)  (((((y) & 0xf0) >> 4) * 10) + ((y) & 0xf))

/* Buffer pointers to convert Vir2Phys and Phy2Vir */
STATIC volatile UINT64  RtcBufVir;
STATIC volatile UINT64  RtcBufPhy;

STATIC
EFI_STATUS
RtcI2cWaitAccess (
  VOID
  )
{
  INTN  Timeout;

  Timeout = RTC_TIMEOUT_WAIT_ACCESS;
  while ((GpioReadBit (I2C_RTC_ACCESS_GPIO_PIN) != 0) && (Timeout > 0)) {
    MicroSecondDelay (100);
    Timeout -= 100;
  }

  if (Timeout <= 0) {
    DEBUG ((DEBUG_ERROR, "%a: Timeout while waiting access RTC\n", __func__));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
RtcI2cRead (
  IN     UINT8   Addr,
  IN OUT UINT64  Data,
  IN     UINT32  DataLen
  )
{
  EFI_STATUS  Status;
  UINT32      TmpLen;

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
  IN UINT8   Addr,
  IN UINT64  Data,
  IN UINT32  DataLen
  )
{
  EFI_STATUS  Status;
  UINT8       TmpBuf[RTC_DATA_BUF_LEN + 1];
  UINT32      TmpLen;

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
  TmpLen    = DataLen + 1;
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
  OUT EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;
  UINT8       *Data;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = RtcI2cRead (RTC_ADDR, RtcBufVir, RTC_DATA_BUF_LEN);

  Data = (UINT8 *)RtcBufVir;
  if (Status == EFI_SUCCESS) {
    Time->Second = PCF8563_SEC_DEC (Data[PCF8563_OFFSET_SEC]);
    Time->Minute = PCF8563_MIN_DEC (Data[PCF8563_OFFSET_MIN]);
    Time->Hour   = PCF8563_HR_DEC (Data[PCF8563_OFFSET_HR]);
    Time->Day    = PCF8563_DAY_DEC (Data[PCF8563_OFFSET_DAY]);
    Time->Month  = PCF8563_MON_DEC (Data[PCF8563_OFFSET_MON]);
    Time->Year   = PCF8563_YEA_DEC (Data[PCF8563_OFFSET_YEA]);
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
  IN EFI_TIME  *Time
  )
{
  UINT8  *Data;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Time->Year < RTC_DEFAULT_MIN_YEAR) ||
      (Time->Year > RTC_DEFAULT_MAX_YEAR))
  {
    return EFI_INVALID_PARAMETER;
  }

  Data                     = (UINT8 *)RtcBufVir;
  Data[PCF8563_OFFSET_SEC] = PCF8563_SEC_ENC (Time->Second);
  Data[PCF8563_OFFSET_MIN] = PCF8563_MIN_ENC (Time->Minute);
  Data[PCF8563_OFFSET_HR]  = PCF8563_HR_ENC (Time->Hour);
  Data[PCF8563_OFFSET_DAY] = PCF8563_DAY_ENC (Time->Day);
  Data[PCF8563_OFFSET_MON] = PCF8563_MON_ENC (Time->Month);
  Data[PCF8563_OFFSET_YEA] = PCF8563_YEA_ENC (Time->Year - RTC_DEFAULT_MIN_YEAR);

  return RtcI2cWrite (RTC_ADDR, RtcBufVir, RTC_DATA_BUF_LEN);
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
  EFI_STATUS  Status;

  /*
   * Allocate the buffer for RTC data
   * The buffer can be accessible after ExitBootServices
   */
  RtcBufVir = (UINT64)AllocateRuntimeZeroPool (RTC_DATA_BUF_LEN);
  DEBUG ((DEBUG_INFO, "Allocated Runtime memory at %016llx\n", RtcBufVir));

  EFI_PHYSICAL_ADDRESS  AllocAddress;

  for (unsigned long long i = 0; i < 40; i++) {
    AllocAddress = 0x80000000 + (i*0x4000000);
    Status       = gBS->AllocatePages (AllocateAddress, EfiRuntimeServicesData, 0x20, &AllocAddress);
    AllocatePages (0x30);
  }

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
