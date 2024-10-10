/** @file
  Clock part of PlatformTpmLib to use TpmLib.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformTpmLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/TimerLib.h>
#include <Library/ArmGenericTimerCounterLib.h>

// Ticks per ms
#define CLOCK_VEXPRESS_NOMINAL  (ArmGenericTimerGetTimerFreq () / 1000U)

// 0.5% change in rate
#define CLOCK_VEXPRESS_ADJUST_CORSE  (CLOCK_VEXPRESS_NOMINAL / 5000)

// 0.05% change in rate
#define CLOCK_VEXPRESS_ADJUST_MEDIUM  (CLOCK_VEXPRESS_NOMINAL / 50000)

// A minimum change in rate
#define CLOCK_VEXPRESS_ADJUST_FINE  1

// The clock tolereance is +/- 10%
#define CLOCK_VEXPRESS_ADJUST_LIMIT  ((CLOCK_VEXPRESS_NOMINAL / 100) * 10)

STATIC UINT64   mLastSystemTimer;
STATIC UINT64   mTpmTimer;
STATIC UINT64   mEndTimer;
STATIC UINT64   mAdjustTimerRate;
STATIC BOOLEAN  mTimerWasReset;
STATIC BOOLEAN  mTimerWasStopped;

/**
  This function read current timer value and update several delta values
  which are used to calcluate realtime and tpm timer value.

  @return value   delta value since last timer has read.

**/
STATIC
UINT64
GetTimerDiff (
  VOID
  )
{
  UINT64  Diff;
  UINT64  TimerNow;

  if (mEndTimer == 0x00) {
    GetPerformanceCounterProperties (NULL, &mEndTimer);
  }

  TimerNow = GetPerformanceCounter ();
  if (TimerNow > mLastSystemTimer) {
    Diff = TimerNow - mLastSystemTimer;
  } else {
    Diff = (mEndTimer - mLastSystemTimer) + TimerNow;
  }

  mLastSystemTimer = TimerNow;

  return Diff;
}

/**
  _plat__TimerReset()

  This function sets current system clock time as t0 for counting TPM time.
  This function is called at a power on event to reset the clock. When the clock
  is reset, the indication that the clock was stopped is also set.

**/
VOID
EFIAPI
PlatformTpmLibTimerReset (
  VOID
  )
{
  mLastSystemTimer = GetPerformanceCounter ();
  mTpmTimer        = mLastSystemTimer;
  mAdjustTimerRate = CLOCK_VEXPRESS_NOMINAL;
  mTimerWasReset   = TRUE;
  mTimerWasStopped = TRUE;

  return;
}

/**
  _plat__TimerRestart()

  This function should be called in order to simulate the restart of the timer
  should it be stopped while power is still applied.

**/
VOID
EFIAPI
PlatformTpmLibTimerRestart (
  VOID
  )
{
  /*
  * This interface is for Simluator.
  * It shoduln't be called from TpmLib.
  */
  ASSERT (0);
}

/**
  _plat__RealTime()

  This is another, probably futile, attempt to define a portable function
  that will return a 64-bit clock value that has mSec resolution.

  @return   value   realtime

**/
UINT64
EFIAPI
PlatformTpmLibRealTime (
  VOID
  )
{
  /*
   * This interface is for Simluator.
   * It shoduln't be called from TpmLib.
   */
  ASSERT (0);

  return 0;
}

/**
  _plat__TimerRead()

  This function provides access to the tick timer of the platform. The TPM code
  uses this value to drive the TPM Clock.

  The tick timer is supposed to run when power is applied to the device. This timer
  should not be reset by time events including _TPM_Init. It should only be reset
  when TPM power is re-applied.

  If the TPM is run in a protected environment, that environment may provide the
  tick time to the TPM as long as the time provided by the environment is not
  allowed to go backwards. If the time provided by the system can go backwards
  during a power discontinuity, then the _plat__Signal_PowerOn should call
  _plat__TimerReset().

  @return value timeval

**/
UINT64
EFIAPI
PlatformTpmLibTimerRead (
  VOID
  )
{
  UINT64  Diff;
  UINT64  AdjustedTimerDiff;

  if (mLastSystemTimer == 0x00) {
    mLastSystemTimer = GetPerformanceCounter ();
    mTpmTimer        = mLastSystemTimer;
  } else {
    Diff              = GetTimerDiff ();
    AdjustedTimerDiff = (Diff * CLOCK_VEXPRESS_NOMINAL) / mAdjustTimerRate;
    mTpmTimer        += AdjustedTimerDiff;
  }

  return mTpmTimer;
}

/**
  _plat__TimerWasReset()

  This function is used to interrogate the flag indicating if the tick timer has
  been reset.

  If the resetFlag parameter is SET, then the flag will be CLEAR before the
  function returns.

  @return TRUE    Timer was reset.
  @return FALSE   Timer wasn't reset.

**/
INT32
EFIAPI
PlatformTpmLibTimerWasReset (
  VOID
  )
{
  INT32  RetVal;

  RetVal         = mTimerWasReset;
  mTimerWasReset = FALSE;

  return (INT32)RetVal;
}

/**
  _plat__TimerWasStopped()

  This function is used to interrogate the flag indicating if the tick timer has
  been stopped. If so, this is typically a reason to roll the nonce.

  This function will CLEAR the s_timerStopped flag before returning. This provides
  functionality that is similar to status register that is cleared when read. This
  is the model used here because it is the one that has the most impact on the TPM
  code as the flag can only be accessed by one entity in the TPM. Any other
  implementation of the hardware can be made to look like a read-once register.

  @return TRUE    timer was stopped
  @return FALSE   timer wasn't stopped

**/
BOOLEAN
EFIAPI
PlatformTpmLibTimerWasStopped (
  VOID
  )
{
  BOOLEAN  RetVal;

  RetVal           = mTimerWasStopped;
  mTimerWasStopped = FALSE;

  return RetVal;
}

/**
  _plat__ClockAdjustRate()

  Adjust the clock rate

  @param [in] Adjust  The adjust number.

**/
VOID
EFIAPI
PlatformTpmLibClockAdjustRate (
  IN INT32  Adjust
  )
{
  switch (Adjust) {
    case TpmClockAdjustCoarseSlower:
      mAdjustTimerRate += CLOCK_VEXPRESS_ADJUST_CORSE;
      break;
    case TpmClockAdjustCoarseFaster:
      mAdjustTimerRate -= CLOCK_VEXPRESS_ADJUST_CORSE;
      break;
    case TpmClockAdjustMediumSlower:
      mAdjustTimerRate += CLOCK_VEXPRESS_ADJUST_MEDIUM;
      break;
    case TpmClockAdjustMediumFaster:
      mAdjustTimerRate -= CLOCK_VEXPRESS_ADJUST_MEDIUM;
      break;
    case TpmClockAdjustFineSlower:
      mAdjustTimerRate += CLOCK_VEXPRESS_ADJUST_FINE;
      break;
    case TpmClockAdjustFineFaster:
      mAdjustTimerRate -= CLOCK_VEXPRESS_ADJUST_FINE;
      break;
    default:
      // ignore any other values.
      break;
  }

  if (mAdjustTimerRate > (CLOCK_VEXPRESS_NOMINAL + CLOCK_VEXPRESS_ADJUST_LIMIT)) {
    mAdjustTimerRate = CLOCK_VEXPRESS_NOMINAL + CLOCK_VEXPRESS_ADJUST_LIMIT;
  }

  if (mAdjustTimerRate < (CLOCK_VEXPRESS_NOMINAL - CLOCK_VEXPRESS_ADJUST_LIMIT)) {
    mAdjustTimerRate = CLOCK_VEXPRESS_NOMINAL - CLOCK_VEXPRESS_ADJUST_LIMIT;
  }
}
