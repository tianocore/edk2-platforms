/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Csr    - Cpu Status Register
    - Calc   - Calculation
    - Freq   - frequency
**/

#ifndef STABLE_TIMER_H_
#define STABLE_TIMER_H_
#include "Library/Cpu.h"

/**
  Gets the timer count value.

  @param[] VOID

  @retval  timer count value.
**/
extern
UINTN
EFIAPI
LoongArchReadTime (
  VOID
  );

/**
  Calculate the timer frequency.

  @param[] VOID

  @retval  Timer frequency.
**/
UINT32
EFIAPI
CalcConstFreq (
  VOID
  );

/*
  Reads data from the specified CPUCFG register.

  @param[OUT]  Val   Pointer to the variable used to store the CPUCFG register value.
  @param[IN]  reg    Specifies the register number of the CPUCFG to read the data.

  @retval  none
 */
extern
VOID
LoongArchReadCpuCfg (
  UINT64   *Val,
  UINT64   reg
  );

#endif // STABLE_TIMER_H_
