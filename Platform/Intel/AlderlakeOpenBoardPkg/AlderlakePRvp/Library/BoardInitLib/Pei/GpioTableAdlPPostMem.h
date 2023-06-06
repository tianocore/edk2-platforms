/** @file
  GPIO definition table for AlderLake P

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _ALDERLAKE_P_GPIO_TABLE_H_
#define _ALDERLAKE_P_GPIO_TABLE_H_

///

/// !!! For those GPIO pins are designed as native function, BIOS CAN NOT configure the pins to Native function in GPIO init table!!!

/// BIOS has to leave the native pins programming to Silicon Code(based on BIOS policies setting) or soft strap(set by CSME in FITc).

/// Configuring pins to native function in GPIO table would cause BIOS perform multiple programming the pins and then related function might be abnormal.

///

#include <Pins/GpioPinsVer2Lp.h>
#include <Library/GpioLib.h>
#include <Library/GpioConfig.h>

#endif // _ALDERLAKE_P_GPIO_TABLE_H_
