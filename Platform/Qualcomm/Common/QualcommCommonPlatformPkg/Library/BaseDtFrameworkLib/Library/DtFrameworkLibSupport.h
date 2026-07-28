/** @file

  Library routine required for DTFramework.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Returns the current performance counter value in microseconds.

  Retrieves the performance counter frequency on the first call and caches
  it for subsequent calls. Validates that the frequency is within an
  acceptable range and that the counter is an up-counter before performing
  the conversion.

  @retval  0    The frequency is invalid or the counter is not supported.
  @retval  !0   The current performance counter value in microseconds.
**/
UINT64
GetTimerCountus (
  VOID
  );

//
// fdt_strrchr()/fdt_strtoul() - ISO C strrchr()/strtoul() equivalents
// required by the libfdt sources bundled with DTFramework, since libfdt
// is built with -nostdinc and has no C library to draw them from.
//
// Macros that directly map functions to BaseLib functions. fdt_strtoul()
// only supports Base 0 (auto-detect via a "0x"/"0X" prefix), 10, or 16 -
// the libfdt sources bundled with DTFramework only ever call this with
// Base 10 (to parse a decimal property offset in fdt_overlay.c).
//
#define fdt_strrchr(str, ch)             AsciiStrRChr (str, (CHAR8)(ch))
#define fdt_strtoul(nptr, endptr, base)  AsciiStrToUintn (nptr, endptr, (UINTN)(base))
