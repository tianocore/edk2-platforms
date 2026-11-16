/** @file

  Header file for the CN10K configuration non-volatile data structures.

  Copyright (c) 2022-2026 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Guid/HiiPlatformSetupFormset.h>

//
// Configuration options stored in the non-volatile variable and mirrored in
// the corresponding PCDs.
//
typedef struct {
  // TRUE to disable the platform watchdog.
  BOOLEAN    DisableWatchdog;
  // TRUE to disable the PCI1 bus.
  BOOLEAN    DisablePci1Bus;
  // TRUE to disable the ATF timers.
  BOOLEAN    DisableAtfTimers;
} CN10K_CONFIG_NV_DATA;
