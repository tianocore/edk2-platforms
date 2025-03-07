/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TscTimerLibInternal.h"

/**  Calculate TSC frequency.

  The TSC counting frequency is determined by comparing how far it counts
  during a 1ms period as determined by the ACPI timer. The ACPI timer is
  used because it counts at a known frequency.
  If ACPI I/O space not enabled, this function will enable it. Then the
  TSC is sampled, followed by waiting for 3579 clocks of the ACPI timer, or 1ms.
  The TSC is then sampled again. The difference multiplied by 1000 is the TSC
  frequency. There will be a small error because of the overhead of reading
  the ACPI timer. An attempt is made to determine and compensate for this error.

  @return The number of TSC counts per second.

**/
UINT64
InternalCalculateTscFrequency (
  VOID
  )
{
  UINT64  StartTSC;
  UINT64  EndTSC;
  UINT16  TimerAddr;
  UINT32  Ticks;
  UINT64  TscFrequency;

  TimerAddr = MmioRead16 (ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REG64);
  if (TimerAddr == 0) {
    TimerAddr = PcdGet16 (PcdAmdFchCfgAcpiPmTmrBlkAddr);
    MmioWrite16 (ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REG64, TimerAddr);
  }

  //
  // ACPI I/O space should be enabled now, locate the ACPI Timer.
  // ACPI I/O base address maybe have be initialized by other driver with different value,
  // So get it from PCI space directly.
  //
  Ticks    = IoRead32 (TimerAddr) + (3579);   // Set Ticks to 1ms in the future
  StartTSC = AsmReadTsc ();                   // Get base value for the TSC
  //
  // Wait until the ACPI timer has counted 1ms.
  // Timer wrap-arounds are handled correctly by this function.
  // When the current ACPI timer value is greater than 'Ticks', the while loop will exit.
  //
  while (((Ticks - IoRead32 (TimerAddr)) & BIT23) == 0) {
    CpuPause ();
  }

  EndTSC = AsmReadTsc ();    // TSC value 1ms later

  TscFrequency =   MultU64x32 (
                     (EndTSC - StartTSC),     // Number of TSC counts in 1ms
                     1000                     // Number of ms in a second
                     );

  return TscFrequency;
}

/**  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param[in]  Delay     A period of time to delay in ticks.

**/
VOID
InternalX86Delay (
  IN      UINT64  Delay
  )
{
  UINT64  Ticks;

  //
  // The target timer count is calculated here
  //
  Ticks = AsmReadTsc () + Delay;

  //
  // Wait until time out
  // Timer wrap-arounds are NOT handled correctly by this function.
  // Thus, this function must be called within 10 years of reset since
  // Intel ensures a minimum of 10 years before the TSC wraps.
  //
  while (AsmReadTsc () <= Ticks) {
    CpuPause ();
  }
}

/**  Stalls the CPU for at least the specified number of MicroSeconds.

  @param[in]  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds input.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN  MicroSeconds
  )
{
  InternalX86Delay (
    DivU64x32 (
      MultU64x64 (
        InternalGetTscFrequency (),
        MicroSeconds
        ),
      1000000u
      )
    );
  return MicroSeconds;
}

/**  Stalls the CPU for at least the specified number of NanoSeconds.

  @param[in]  NanoSeconds The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds input.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN  NanoSeconds
  )
{
  InternalX86Delay (
    DivU64x32 (
      MultU64x32 (
        InternalGetTscFrequency (),
        (UINT32)NanoSeconds
        ),
      1000000000u
      )
    );
  return NanoSeconds;
}

/**  Retrieves the current value of the 64-bit free running Time-Stamp counter.

  The time-stamp counter (as implemented in the P6 family, Pentium, Pentium M,
  Pentium 4, Intel Xeon, Intel Core Solo and Intel Core Duo processors and
  later processors) is a 64-bit counter that is set to 0 following a RESET of
  the processor.  Following a RESET, the counter increments even when the
  processor is halted by the HLT instruction or the external STPCLK# pin. Note
  that the assertion of the external DPSLP# pin may cause the time-stamp
  counter to stop.

  The properties of the counter can be retrieved by the
  GetPerformanceCounterProperties() function.

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return AsmReadTsc ();
}

/**  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with, 0x0, is returned in StartValue. If EndValue is not NULL, then the value
  that the performance counter end with, 0xFFFFFFFFFFFFFFFF, is returned in
  EndValue.

  The 64-bit frequency of the performance counter, in Hz, is always returned.
  To determine average processor clock frequency, Intel recommends the use of
  EMON logic to count processor core clocks over the period of time for which
  the average is required.


  @param[out]   StartValue  Pointer to where the performance counter's starting value is saved, or NULL.
  @param[out]   EndValue    Pointer to where the performance counter's ending value is saved, or NULL.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64 *StartValue, OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xFFFFFFFFFFFFFFFFull;
  }

  return InternalGetTscFrequency ();
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64  Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift        = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder    = RShiftU64 (Remainder, (UINTN)Shift);
  Frequency    = RShiftU64 (Frequency, (UINTN)Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
