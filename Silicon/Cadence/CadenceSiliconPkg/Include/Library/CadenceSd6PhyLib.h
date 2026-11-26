/** @file

  Cadence SD6 (SDHCI) PHY and clock control library.

  This is the subset of the Cadence SD6 support that is required for the
  generic SDHCI driver running in PIO mode at legacy/high-speed clock rates.
  HS200/HS400, DDR and tuning support are intentionally not included.

  Copyright (c) 2024-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

//
// Per-controller configuration.  WrapperBase and BaseClockHz are properties
// of the SoC integration; the remaining fields are board specific PHY tuning
// parameters, typically taken from the device tree.
//
typedef struct {
  //
  // Cadence SD6 host register set (wrapper) base.  The SDHCI register block
  // is located CN10K-style 0x200 bytes above it.
  //
  UINTN      WrapperBase;

  //
  // Fixed reference clock feeding the SD6 PHY, in Hz.
  //
  UINT32     BaseClockHz;

  //
  // "cdns,delay_element". Delay element size in picoseconds. When
  // OverrideDelayElement is TRUE this value is used as-is instead of being
  // measured by the DLL calibration at probe time.
  //
  UINT32     DelayElement;

  //
  // TRUE to use DelayElement directly and skip the DLL calibration.
  //
  BOOLEAN    OverrideDelayElement;

  //
  // "cdns,iocell_input_delay". Input IO cell delay in picoseconds.
  //
  UINT32     IocellInputDelay;

  //
  // "cdns,iocell_output_delay". Output IO cell delay in picoseconds.
  //
  UINT32     IocellOutputDelay;

  //
  // "cdns,host_drive". Output driver strength selection (0-3).
  //
  UINT32     HostDrive;

  //
  // "cdns,host_slew". Output slew rate selection (0-3).
  //
  UINT32     HostSlew;
} CADENCE_SD6_PHY_CONFIG;

/**
  Reset and initialize the Cadence SD6 PHY.

  @param[in]  Config  Controller and board specific PHY parameters.

  @retval EFI_SUCCESS            The PHY was initialized.
  @retval EFI_INVALID_PARAMETER  Config is NULL or incomplete.
  @retval EFI_DEVICE_ERROR       The PHY failed to initialize.
**/
EFI_STATUS
CadenceSd6PhyInit (
  IN CONST CADENCE_SD6_PHY_CONFIG  *Config
  );

/**
  Program the SD6 PHY for the divider currently written to the SDHCI Clock
  Control register.

  @param[in]  HwDivider  SDHCI clock divider field value (0 means 1).
**/
VOID
CadenceSd6PhySetClock (
  IN UINT32  HwDivider
  );
