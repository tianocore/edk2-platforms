/** @file
  The header file for AcDcTimerLib library.

  Copyright (C) 2019 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AC_DC_TIMER_LIB_H_
#define AC_DC_TIMER_LIB_H_

#define ACPI_MMIO_BASE  0xFED80000ul

#define IOMUX_BASE      0xD00
#define FCH_GPIO_REG23  0x17

#define FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA  0xFED8155Cul
#define BYTE_0                                     0x00
#define BYTE_1                                     0x01
#define BYTE_2                                     0x02
#define BYTE_3                                     0x03

#define ACDC_BASE       0x1D00
#define FCH_ACDC_REG00  0x00
#define FCH_ACDC_REG08  0x08
#define FCH_ACDC_REG10  0x10
#define FCH_ACDC_REG18  0x18
#define FCH_ACDC_REG21  0x21

#define AC_PRESENT   0x00
#define AC_TIMER_EN  BIT0
#define DC_TIMER_EN  BIT1

#define SLP_TYPE_SUPPORT_S0I3  BIT0
#define SLP_TYPE_SUPPORT_S3    BIT1
#define SLP_TYPE_SUPPORT_S4S5  BIT2

/**
  Set AcDcTimer.

  @param[in] AcDcTimerSelect  Select AC or DC Timer.
                              Bit0 = 1 = Enable AC Timer.
                              Bit1 = 1 = Enable DC Timer.
  @param[in] WakeupSupport    Select ACDC Timer wake up support.
                              Bit0 = 1 = Enable wake in S0I3 state.
                              Bit1 = 1 = Enable wake in S3 state (SLP_TYP = 3, and !G0_State).
                              Bit2 = 1 = Enable wake in S4/S5 state (SLP_TYP = 4 or 5, and !G0_State).
  @param[in] AcTimer          The number of seconds for AC Timer.
  @param[in] DcTimer          The number of seconds for DC Timer.

**/
VOID
EFIAPI
SetAcDcTimer (
  IN UINT8   AcDcTimerSelect,
  IN UINT8   WakeupSupport,
  IN UINT32  AcTimer,
  IN UINT32  DcTimer
  );

/**
  Clear AcDcTimer.

  @param[in] AcDcTimerSelect  Select AC or DC Timer.
                              Bit0 = 1 = Clear AC Timer.
                              Bit1 = 1 = Clear DC Timer.

**/
VOID
EFIAPI
ClearAcDcTimer (
  IN UINT8  AcDcTimerSelect
  );

#endif // AC_DC_TIMER_LIB_H_
