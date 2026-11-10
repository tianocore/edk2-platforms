/** @file
  Allwinner H618 Real Time Clock (RTC) Library.

  Copyright (c) 2026, Khai Do <dovankhai97@gmail.com>. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <AllwinnerH618.h>
#include <Guid/EventGroup.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#define RTC_POLL_TIMEOUT         10000U
#define RTC_READ_MAX_RETRIES     4U

STATIC EFI_EVENT             mRtcVirtualAddrChangeEvent;
STATIC EFI_PHYSICAL_ADDRESS  mRtcBase;
STATIC UINTN                 mSoftwareEpochBase  = BUILD_EPOCH;
STATIC UINT64     mSoftwareBaseTicks  = 0;
STATIC UINT64     mTimerFreqHz        = AW_H618_OSC24M_FREQ;
STATIC INT16      mRtcTimeZone        = EFI_UNSPECIFIED_TIMEZONE;
STATIC UINT8      mRtcDaylight        = 0;

/**
  Wait until the specified access busy bits in LOSC_CTRL_REG clear.

  @param[in] BusyMask  Bitmask of busy flags to wait for.

  @retval TRUE          Bits cleared before timeout.
  @retval FALSE         Timed out waiting for RTC register write completion.
**/
STATIC
BOOLEAN
H618RtcWaitReady (
  IN UINT32  BusyMask
  )
{
  UINT32  Timeout;

  for (Timeout = 0; Timeout < RTC_POLL_TIMEOUT; Timeout++) {
    if ((MmioRead32 (mRtcBase + AW_H618_RTC_LOSC_CTRL_REG) & BusyMask) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Compute elapsed seconds since mSoftwareBaseTicks using the ARM Generic Timer.

  @return Elapsed seconds.
**/
STATIC
UINTN
H618GetElapsedSeconds (
  VOID
  )
{
  UINT64  CurrentTicks;

  if (mTimerFreqHz == 0) {
    return 0;
  }

  CurrentTicks = GetPerformanceCounter ();
  if (CurrentTicks >= mSoftwareBaseTicks) {
    return (UINTN)((CurrentTicks - mSoftwareBaseTicks) / mTimerFreqHz);
  }

  return 0;
}

/**
  Returns the current time and date information, and the time-keeping
  capabilities of the hardware platform.

  @param[out] Time          A pointer to storage to receive a snapshot of the current time.
  @param[out] Capabilities  An optional pointer to a buffer to receive the real time clock
                            device's capabilities.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Time is NULL.
**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  UINT32   DayReg;
  UINT32   HmsReg;
  UINT32   DayRegCheck;
  UINT32   HmsRegCheck;
  UINT32   Retry;
  BOOLEAN  Stable;
  UINTN    EpochSeconds;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The RTC runs on the 32.768 kHz LOSC domain while CPU accesses run on
  // the high-speed bus clock. Clock domain crossing during tick transitions
  // can cause torn Day/HMS reads. Perform consecutive reads until stable,
  // returning EFI_DEVICE_ERROR if RTC_READ_MAX_RETRIES retries fail.
  //
  Stable = FALSE;
  for (Retry = 0; Retry < RTC_READ_MAX_RETRIES; Retry++) {
    DayReg      = MmioRead32 (mRtcBase + AW_H618_RTC_DAY_REG);
    HmsReg      = MmioRead32 (mRtcBase + AW_H618_RTC_HH_MM_SS_REG);
    DayRegCheck = MmioRead32 (mRtcBase + AW_H618_RTC_DAY_REG);
    HmsRegCheck = MmioRead32 (mRtcBase + AW_H618_RTC_HH_MM_SS_REG);
    if ((DayReg == DayRegCheck) && (HmsReg == HmsRegCheck)) {
      Stable = TRUE;
      break;
    }
  }

  if (!Stable) {
    return EFI_DEVICE_ERROR;
  }

  if (((DayReg & AW_H618_RTC_DAY_MASK) != 0) || ((HmsReg & AW_H618_RTC_HMS_MASK) != 0)) {
    EpochSeconds = ((UINTN)(DayReg & AW_H618_RTC_DAY_MASK) * SEC_PER_DAY) +
                   ((UINTN)((HmsReg >> AW_H618_RTC_HOUR_SHIFT) & AW_H618_RTC_HOUR_MASK) * SEC_PER_HOUR) +
                   ((UINTN)((HmsReg >> AW_H618_RTC_MINUTE_SHIFT) & AW_H618_RTC_MINUTE_MASK) * SEC_PER_MIN) +
                   ((UINTN)(HmsReg & AW_H618_RTC_SECOND_MASK));
  } else {
    EpochSeconds = mSoftwareEpochBase + H618GetElapsedSeconds ();
  }

  ZeroMem (Time, sizeof (EFI_TIME));
  EpochToEfiTime (EpochSeconds, Time);
  Time->TimeZone = mRtcTimeZone;
  Time->Daylight = mRtcDaylight;

  if (Capabilities != NULL) {
    Capabilities->Resolution = 1;
    Capabilities->Accuracy   = 0;
    Capabilities->SetsToZero = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  Sets the current local time and date information.

  @param[in] Time               A pointer to the current time.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  A time field is out of range.
**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN EFI_TIME  *Time
  )
{
  UINTN   EpochSeconds;
  UINT32  DayReg;
  UINT32  HmsReg;

  if ((Time == NULL) || !IsTimeValid (Time)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Time->Year < 1970) || (Time->Year >= 2106)) {
    return EFI_INVALID_PARAMETER;
  }

  EpochSeconds       = EfiTimeToEpoch (Time);
  mSoftwareEpochBase = EpochSeconds;
  mSoftwareBaseTicks = GetPerformanceCounter ();
  mRtcTimeZone       = Time->TimeZone;
  mRtcDaylight       = Time->Daylight;

  DayReg = (UINT32)(EpochSeconds / SEC_PER_DAY) & AW_H618_RTC_DAY_MASK;
  HmsReg = (((UINT32)Time->Hour   & AW_H618_RTC_HOUR_MASK) << AW_H618_RTC_HOUR_SHIFT) |
           (((UINT32)Time->Minute & AW_H618_RTC_MINUTE_MASK) << AW_H618_RTC_MINUTE_SHIFT) |
           ((UINT32)Time->Second  & AW_H618_RTC_SECOND_MASK);

  if (!H618RtcWaitReady (AW_H618_RTC_LOSC_DAY_ACCE | AW_H618_RTC_LOSC_HMS_ACCE)) {
    return EFI_DEVICE_ERROR;
  }

  MmioWrite32 (mRtcBase + AW_H618_RTC_HH_MM_SS_REG, HmsReg);
  if (!H618RtcWaitReady (AW_H618_RTC_LOSC_HMS_ACCE)) {
    return EFI_DEVICE_ERROR;
  }

  MmioWrite32 (mRtcBase + AW_H618_RTC_DAY_REG, DayReg);
  if (!H618RtcWaitReady (AW_H618_RTC_LOSC_DAY_ACCE)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Returns the current wakeup alarm clock setting.

  @param[out] Enabled  Indicates if the alarm is currently enabled or disabled.
  @param[out] Pending  Indicates if the alarm signal is pending.
  @param[out] Time     The current alarm setting.

  @retval EFI_UNSUPPORTED  Wakeup alarm is not supported in UEFI runtime.
**/
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
  Sets the system wakeup alarm clock time.

  @param[in]  Enabled  Enable or disable the wakeup alarm.
  @param[out] Time     If Enable is TRUE, the time to set the wakeup alarm for.

  @retval EFI_UNSUPPORTED  Wakeup alarm is not supported in UEFI runtime.
**/
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN  BOOLEAN   Enabled,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Fixup internal data so that EFI can be called in virtual mode.

  @param[in] Event    The Event that is being processed.
  @param[in] Context  Event Context.
**/
STATIC
VOID
EFIAPI
VirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mRtcBase);
}

/**
  Initialize the Allwinner H618 Real Time Clock runtime library.

  @param[in] ImageHandle  Handle that identifies the loaded image.
  @param[in] SystemTable  System Table for this image.

  @retval EFI_SUCCESS     The operation completed successfully.
**/
EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;
  UINT64                           StartValue;
  UINT64                           EndValue;

  mRtcBase           = FixedPcdGet64 (PcdAllwinnerRtcBaseAddress);
  mSoftwareBaseTicks = GetPerformanceCounter ();
  mTimerFreqHz       = GetPerformanceCounterProperties (&StartValue, &EndValue);
  if (mTimerFreqHz == 0) {
    mTimerFreqHz = AW_H618_OSC24M_FREQ;
  }

  Status = gDS->GetMemorySpaceDescriptor (mRtcBase, &Desc);
  if (EFI_ERROR (Status) || (Desc.GcdMemoryType == EfiGcdMemoryTypeNonExistent)) {
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    mRtcBase,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "AllwinnerRtcLib: AddMemorySpace failed: %r\n", Status));
    }
  }

  Status = gDS->SetMemorySpaceAttributes (
                  mRtcBase,
                  SIZE_4KB,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "AllwinnerRtcLib: SetMemorySpaceAttributes failed: %r\n", Status));
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mRtcVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
