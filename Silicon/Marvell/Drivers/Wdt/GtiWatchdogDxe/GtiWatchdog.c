/** @file
*
* SPDX-License-Identifier: BSD-2-Clause-Patent
* https://spdx.org/licenses
*
* Copyright (C) 2022 Marvell
*
* Source file for Marvell Watchdog driver
*
**/


#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/FdtClient.h>
#include <Protocol/WatchdogTimer.h>

#define GTI_CWD_WDOG(Core)    (FixedPcdGet64(PcdGtiWatchdogBase64) + 0x40000 + Core * 0x8)
#define GTI_CWD_POKE(Core)    (FixedPcdGet64(PcdGtiWatchdogBase64) + 0x50000 + Core * 0x8)

typedef union _GTI_CWD_WDOG_UNION {
  UINT64          U64;
  struct {
    UINTN Mode   : 2;
    UINTN State  : 2;
    UINTN Len    : 16;
    UINTN Cnt    : 24;
    UINTN DStop  : 1;
    UINTN GStop  : 1;
    UINTN Rsvd   : 18;
  } PACKED S;
} GTI_CWD_WDOG_UNION;

#define CWD_WDOG_MODE_RST     (BIT1 | BIT0)

#define RST_BOOT_PNR_MUL(Val)  ((Val >> 33) & 0x1F)

EFI_EVENT mGtiExitBootServicesEvent = (EFI_EVENT)NULL;
UINT32  mSclk = 0;
BOOLEAN mHardwarePlatform = TRUE;

/**
  Stop the GTI watchdog timer from counting down by disabling interrupts.
**/
STATIC
VOID
GtiWdtStop (
  VOID
  )
{
  GTI_CWD_WDOG_UNION Wdog;

  MmioWrite64(GTI_CWD_POKE(0), 0);

  Wdog.U64 = MmioRead64(GTI_CWD_WDOG(0));

  // Disable WDT
  if (Wdog.S.Mode != 0) {
    Wdog.S.Len = 1;
    Wdog.S.Mode = 0;
    MmioWrite64 (GTI_CWD_WDOG(0), Wdog.U64);
  }
}

/**
  Starts the GTI WDT countdown by enabling interrupts.
  The count down will start from the value stored in the Load register,
  not from the value where it was previously stopped.
**/
STATIC
VOID
GtiWdtStart (
  VOID
  )
{
  GTI_CWD_WDOG_UNION Wdog;

  // Reset the WDT
  MmioWrite64 (GTI_CWD_POKE(0), 0);

  Wdog.U64 = MmioRead64 (GTI_CWD_WDOG(0));

  // Enable countdown
  if (Wdog.S.Mode == 0) {
    Wdog.S.Mode = CWD_WDOG_MODE_RST;
    MmioWrite64 (GTI_CWD_WDOG(0), Wdog.U64);
  }
}

/**
    On exiting boot services we must make sure the SP805 Watchdog Timer
    is stopped.
**/
VOID
EFIAPI
GtiExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  MmioWrite64 (GTI_CWD_POKE(0), 0);
  GtiWdtStop ();
}

/**
  This function registers the handler NotifyFunction so it is called every time
  the watchdog timer expires.  It also passes the amount of time since the last
  handler call to the NotifyFunction.
  If NotifyFunction is not NULL and a handler is not already registered,
  then the new handler is registered and EFI_SUCCESS is returned.
  If NotifyFunction is NULL, and a handler is already registered,
  then that handler is unregistered.
  If an attempt is made to register a handler when a handler is already registered,
  then EFI_ALREADY_STARTED is returned.
  If an attempt is made to unregister a handler when a handler is not registered,
  then EFI_INVALID_PARAMETER is returned.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  NotifyFunction   The function to call when a timer interrupt fires. This
                           function executes at TPL_HIGH_LEVEL. The DXE Core will
                           register a handler for the timer interrupt, so it can know
                           how much time has passed. This information is used to
                           signal timer based events. NULL will unregister the handler.

  @retval EFI_SUCCESS           The watchdog timer handler was registered.
  @retval EFI_ALREADY_STARTED   NotifyFunction is not NULL, and a handler is already
                                registered.
  @retval EFI_INVALID_PARAMETER NotifyFunction is NULL, and a handler was not
                                previously registered.
  @retval EFI_UNSUPPORTED       HW does not support this functionality.

**/
EFI_STATUS
EFIAPI
GtiWdtRegisterHandler (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  IN EFI_WATCHDOG_TIMER_NOTIFY                NotifyFunction
  )
{
  // UNSUPPORTED - The hardware watchdog will reset the board
  return EFI_UNSUPPORTED;
}

/**

  This function adjusts the period of timer interrupts to the value specified
  by TimerPeriod.  If the timer period is updated, then the selected timer
  period is stored in EFI_TIMER.TimerPeriod, and EFI_SUCCESS is returned.  If
  the timer hardware is not programmable, then EFI_UNSUPPORTED is returned.
  If an error occurs while attempting to update the timer period, then the
  timer hardware will be put back in its state prior to this call, and
  EFI_DEVICE_ERROR is returned.  If TimerPeriod is 0, then the timer interrupt
  is disabled.  This is not the same as disabling the CPU's interrupts.
  Instead, it must either turn off the timer hardware, or it must adjust the
  interrupt controller so that a CPU interrupt is not generated when the timer
  interrupt fires.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      The rate to program the timer interrupt in 100 nS units. If
                           the timer hardware is not programmable, then EFI_UNSUPPORTED is
                           returned. If the timer is programmable, then the timer period
                           will be rounded up to the nearest timer period that is supported
                           by the timer hardware. If TimerPeriod is set to 0, then the
                           timer interrupts will be disabled.


  @retval EFI_SUCCESS           The timer period was changed.
  @retval EFI_UNSUPPORTED       The platform cannot change the period of the timer interrupt.
  @retval EFI_DEVICE_ERROR      The timer period could not be changed due to a device error.

**/
EFI_STATUS
EFIAPI
GtiWdtSetTimerPeriod (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  IN UINT64                                   TimerPeriod   // In 100ns units
  )
{
  UINT32             Clock;
  UINT64             CountDown;
  GTI_CWD_WDOG_UNION Wdog;

  if (TimerPeriod == 0) {

    // This is a watchdog stop request
    GtiWdtStop();

    return EFI_SUCCESS;
  } else {
    //
    // The system is reset only after the WDT expires for the 3rd time
    //

    Clock = mSclk / 1000000; //MHz
    CountDown = DivU64x32 (MultU64x32 (TimerPeriod, Clock), 30);

    // WDT counts in 1024 cycle steps
    // Only upper 16 bits can be used

    Wdog.U64   = 0;
    Wdog.S.Len = (CountDown + (0xFF << 10)) >> 18;
    MmioWrite64 (GTI_CWD_WDOG(0), Wdog.U64);

    // Start the watchdog
    if (mHardwarePlatform == TRUE) {
      GtiWdtStart();
    }
  }

  return EFI_SUCCESS;
}

/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      A pointer to the timer period to retrieve in 100 ns units. If
                           0 is returned, then the timer is currently disabled.


  @retval EFI_SUCCESS           The timer period was returned in TimerPeriod.
  @retval EFI_INVALID_PARAMETER TimerPeriod is NULL.

**/
EFI_STATUS
EFIAPI
GtiWdtGetTimerPeriod (
  IN CONST EFI_WATCHDOG_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                                  *TimerPeriod
  )
{
  UINT32             Clock;
  UINT64             CountDown;
  UINT64             ReturnValue;
  GTI_CWD_WDOG_UNION Wdog;

  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Wdog.U64 = MmioRead64 (GTI_CWD_WDOG(0));

  // Check if the watchdog is stopped
  if (Wdog.S.Mode == 0) {
    // It is stopped, so return zero.
    ReturnValue = 0;
  } else {
    // Convert the Watchdog ticks into TimerPeriod
    Clock = mSclk / 1000000; //MHz
    CountDown = Wdog.S.Len << 18;

    ReturnValue = MultU64x32(DivU64x32(3 * CountDown, Clock), 10); // usecs * 10
  }

  *TimerPeriod = ReturnValue;

  return EFI_SUCCESS;
}

/**
  Interface structure for the Watchdog Architectural Protocol.

  @par Protocol Description:
  This protocol provides a service to set the amount of time to wait
  before firing the watchdog timer, and it also provides a service to
  register a handler that is invoked when the watchdog timer fires.

  @par When the watchdog timer fires, control will be passed to a handler
  if one has been registered.  If no handler has been registered,
  or the registered handler returns, then the system will be
  reset by calling the Runtime Service ResetSystem().

  @param RegisterHandler
  Registers a handler that will be called each time the
  watchdogtimer interrupt fires.  TimerPeriod defines the minimum
  time between timer interrupts, so TimerPeriod will also
  be the minimum time between calls to the registered
  handler.
  NOTE: If the watchdog resets the system in hardware, then
        this function will not have any chance of executing.

  @param SetTimerPeriod
  Sets the period of the timer interrupt in 100 nS units.
  This function is optional, and may return EFI_UNSUPPORTED.
  If this function is supported, then the timer period will
  be rounded up to the nearest supported timer period.

  @param GetTimerPeriod
  Retrieves the period of the timer interrupt in 100 nS units.

**/
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL    gWatchdogTimer = {
  (EFI_WATCHDOG_TIMER_REGISTER_HANDLER) GtiWdtRegisterHandler,
  (EFI_WATCHDOG_TIMER_SET_TIMER_PERIOD) GtiWdtSetTimerPeriod,
  (EFI_WATCHDOG_TIMER_GET_TIMER_PERIOD) GtiWdtGetTimerPeriod
};

/**
  Initialize the state information for the Watchdog Timer Architectural Protocol.

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
GtiWdtInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle = NULL;
  FDT_HANDLE  SclkHandle = 0;
  FDT_HANDLE  RootHandle = 0;
  CONST UINT32        *SclkFreq = NULL;
  MRVL_FDT_CLIENT_PROTOCOL *FdtClient = NULL;
  CONST CHAR8 *Platform;

  DEBUG ((DEBUG_INFO, "GtiWdtInitialize: Start\n"));
  // Stop the watchdog from triggering unexpectedly
  GtiWdtStop ();

  //
  // Make sure the Watchdog Timer Architectural Protocol has not been installed in the system yet.
  // This will avoid conflicts with the universal watchdog
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiWatchdogTimerArchProtocolGuid);

  Status = gBS->LocateProtocol (&gMrvlFdtClientProtocolGuid,
                                NULL,
                                (VOID **)&FdtClient);

  if (EFI_ERROR (Status) || (FdtClient == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: cannot locate: gMrvlFdtClientProtocolGuid\n", __func__));
    return EFI_ABORTED;
  }

  Status = FdtClient->GetNode (FdtClient, "/soc@0/sclk", &SclkHandle);
  if (EFI_ERROR (Status) || !SclkHandle) {
    DEBUG ((DEBUG_ERROR, "%a: %s node not found!\n", __func__, L"/soc@0/sclk"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "%a: Found: %s\n", __func__, L"/soc@0/sclk"));
  Status = FdtClient->GetNodeProperty (FdtClient,
                                        SclkHandle,
                                        "clock-frequency",
                                        (CONST VOID **)&SclkFreq,
                                        NULL);
  if (EFI_ERROR (Status) || NULL == SclkFreq) {
    DEBUG ((DEBUG_ERROR, "%a: %s property not found!\n", __func__, L"\"clock-frequency\""));
    return EFI_NO_MAPPING;
  }

  mSclk = FdtToCpu32(*SclkFreq);
  DEBUG ((DEBUG_INFO, "%a: DT sclk = %d Mhz (0x%x)\n", __func__, mSclk/1000000, mSclk));

  Status = FdtClient->GetNode (FdtClient, "/soc@0", &RootHandle);
  if (!EFI_ERROR (Status) && RootHandle) {
    Status = FdtClient->GetNodeProperty (FdtClient,
                                        RootHandle,
                                        "runplatform",
                                        (CONST VOID **)&Platform,
                                        NULL);
    if (!EFI_ERROR (Status)) {
      if (AsciiStrCmp (Platform, "HW_PLATFORM")) {
        mHardwarePlatform = FALSE;
        DEBUG ((DEBUG_INFO, "%a: Not a hardware platform\n", __func__));
      }
    }
  }

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES,
                             TPL_NOTIFY,
                             GtiExitBootServices,
                             NULL,
                             &mGtiExitBootServicesEvent);
  ASSERT_EFI_ERROR(Status);

  // Install the Timer Architectural Protocol onto a new handle
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &Handle,
                  &gEfiWatchdogTimerArchProtocolGuid, &gWatchdogTimer,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "GtiWdtInitialize: Exit\n"));

  return EFI_SUCCESS;
}
