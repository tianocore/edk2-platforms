/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include <Guid/EventGroup.h>
#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/RealTimeClock.h>

#include "PCF8563.h"

#define TICKS_PER_SEC  (ArmGenericTimerGetTimerFreq ())

STATIC EFI_EVENT  mVirtualAddressChangeEvent = NULL;

STATIC UINT64  mLastSavedSystemCount = 0;
STATIC UINT64  mLastSavedTimeEpoch   = 0;

/**
 * Returns the current time and date information, and the time-keeping capabilities
 * of the hardware platform.
 *
 * @param  Time                  A pointer to storage to receive a snapshot of the current time.
 * @param  Capabilities          An optional pointer to a buffer to receive the real time clock
 *                               device's capabilities.
 *
 *
 * @retval EFI_SUCCESS           The operation completed successfully.
 * @retval EFI_INVALID_PARAMETER Time is NULL.
 * @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.
 */
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  EFI_STATUS  Status;
  UINT64      CurrentSystemCount;
  UINT64      TimeElapsed;
  UINTN       EpochSeconds;

  if ((mLastSavedTimeEpoch == 0) || EfiAtRuntime ()) {
    Status = PlatformGetTime (Time);
    if (EFI_ERROR (Status)) {
      // Failed to read platform RTC so create fake time
      Time->Second = 0;
      Time->Minute = 0;
      Time->Hour   = 10;
      Time->Day    = 1;
      Time->Month  = 1;
      Time->Year   = 2017;
    }

    EpochSeconds = EfiTimeToEpoch (Time);
    if (!EfiAtRuntime ()) {
      mLastSavedTimeEpoch   = EpochSeconds;
      mLastSavedSystemCount = ArmGenericTimerGetSystemCount ();
    }
  } else {
    CurrentSystemCount = ArmGenericTimerGetSystemCount ();
    if (CurrentSystemCount >= mLastSavedSystemCount) {
      TimeElapsed  = (CurrentSystemCount - mLastSavedSystemCount) / MultU64x32 (1, TICKS_PER_SEC);
      EpochSeconds = mLastSavedTimeEpoch + TimeElapsed;
    } else {
      // System counter overflow 64 bits
      // Call GetTime again to read the date from RTC HW, not using generic timer system counter
      mLastSavedTimeEpoch = 0;
      return LibGetTime (Time, Capabilities);
    }
  }

  // Adjust for the correct timezone
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds += Time->TimeZone * SEC_PER_MIN;
  }

  // Adjust for the correct period
  if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    // Convert to adjusted time, i.e. spring forwards one hour
    EpochSeconds += SEC_PER_HOUR;
  }

  EpochToEfiTime (EpochSeconds, Time);

  return EFI_SUCCESS;
}

/**
 * Sets the current local time and date information.
 *
 * @param  Time                  A pointer to the current time.
 *
 * @retval EFI_SUCCESS           The operation completed successfully.
 * @retval EFI_INVALID_PARAMETER A time field is out of range.
 * @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware error.
 */
EFI_STATUS
EFIAPI
LibSetTime (
  IN EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;
  UINTN       EpochSeconds;

  EpochSeconds = EfiTimeToEpoch (Time);

  // Adjust for the correct time zone, i.e. convert to UTC time zone
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds -= Time->TimeZone * SEC_PER_MIN;
  }

  // Adjust for the correct period, i.e. fall back one hour
  if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    EpochSeconds -= SEC_PER_HOUR;
  }

  EpochToEfiTime (EpochSeconds, Time);

  Status = PlatformSetTime (Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!EfiAtRuntime ()) {
    mLastSavedTimeEpoch   = EpochSeconds;
    mLastSavedSystemCount = ArmGenericTimerGetSystemCount ();
  }

  return EFI_SUCCESS;
}

/**
 * Returns the current wakeup alarm clock setting.
 *
 * @param  Enabled               Indicates if the alarm is currently enabled or disabled.
 * @param  Pending               Indicates if the alarm signal is pending and requires acknowledgement.
 * @param  Time                  The current alarm setting.
 *
 * @retval EFI_SUCCESS           The alarm settings were returned.
 * @retval EFI_INVALID_PARAMETER Any parameter is NULL.
 * @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.
 */
EFI_STATUS
EFIAPI
LibGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
 * Sets the system wakeup alarm clock time.
 *
 * @param  Enabled               Enable or disable the wakeup alarm.
 * @param  Time                  If Enable is TRUE, the time to set the wakeup alarm for.
 *
 * @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled. If
 *                               Enable is FALSE, then the wakeup alarm was disabled.
 * @retval EFI_INVALID_PARAMETER A time field is out of range.
 * @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
 * @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.
 */
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN BOOLEAN    Enabled,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
LibRtcVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Only needed if you are going to support the OS calling RTC functions in virtual mode.
  // You will need to call EfiConvertPointer (). To convert any stored physical addresses
  // to virtual address. After the OS transitions to calling in virtual mode, all future
  // runtime calls will be made in virtual mode.
  //
  PlatformVirtualAddressChangeEvent ();
}

/**
 * This is the declaration of an EFI image entry point. This can be the entry point to an application
 * written to this specification, an EFI boot service driver, or an EFI runtime driver.
 *
 * @param  ImageHandle           Handle that identifies the loaded image.
 * @param  SystemTable           System Table for this image.
 *
 * @retval EFI_SUCCESS           The operation completed successfully.
 */
EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = PlatformInitialize ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  LibRtcVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
