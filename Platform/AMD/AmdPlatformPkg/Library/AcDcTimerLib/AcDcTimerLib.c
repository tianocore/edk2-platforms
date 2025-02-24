/** @file
  AcDcTimerLib library.

  Copyright (C) 2019 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/AcDcTimerLib.h>

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
  )
{
  UINT8  TempData8;

  DEBUG ((DEBUG_INFO, "SetAcDcTimer Enter %X %X %X %X\n", AcDcTimerSelect, WakeupSupport, AcTimer, DcTimer));

  // Init GPIO23 multi function to AC_PRESENT
  MmioWrite8 (ACPI_MMIO_BASE + IOMUX_BASE + FCH_GPIO_REG23, AC_PRESENT);

  // Clear GPIO23 wakeup & interrupt status (write 1 clear)
  TempData8 = MmioRead8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_3);
  MmioWrite8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_3, TempData8);

  // Init GPIO23 wake up support
  TempData8  = MmioRead8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_1);
  TempData8 &= (BIT2 + BIT1 + BIT0);            // Clear Bit[7:3]
  TempData8 |= 3 << 3;                          // Set Bit[4:3] to 3
  TempData8 |= (WakeupSupport & 0x07) << 5;     // Set Bit[7:5] to WakeupSupport
  MmioWrite8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_1, TempData8);

  // Clear AC Timer status, set AC Timer counter and enable it
  if (AcDcTimerSelect & AC_TIMER_EN) {
    MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG08, (DC_TIMER_EN + AC_TIMER_EN));
    if (AcTimer) {
      MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG00, AcTimer);
    }

    TempData8 = MmioRead8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21);
    MmioWrite8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21, (TempData8 | AC_TIMER_EN));
  }

  // Clear DC Timer status, set DC Timer counter and enable it
  if (AcDcTimerSelect & DC_TIMER_EN) {
    MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG18, (DC_TIMER_EN + AC_TIMER_EN));
    if (DcTimer) {
      MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG10, DcTimer);
    }

    TempData8 = MmioRead8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21);
    MmioWrite8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21, (TempData8 | DC_TIMER_EN));
  }

  DEBUG ((DEBUG_INFO, "SetAcDcTimer Exit\n"));
}

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
  )
{
  UINT8  TempData8;

  DEBUG ((DEBUG_INFO, "ClearAcDcTimer Enter %X\n", AcDcTimerSelect));

  // Clear AC Timer status (write 1 clear), AC Timer counter and disable it
  if (AcDcTimerSelect & AC_TIMER_EN) {
    MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG08, (DC_TIMER_EN + AC_TIMER_EN));
    MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG00, 0xFFFFFFFF);
    TempData8 = MmioRead8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21);
    MmioWrite8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21, (TempData8 & ~AC_TIMER_EN));
  }

  // Clear DC Timer status (write 1 clear), DC Timer counter and disable it
  if (AcDcTimerSelect & DC_TIMER_EN) {
    MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG18, (DC_TIMER_EN + AC_TIMER_EN));
    MmioWrite32 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG10, 0xFFFFFFFF);
    TempData8 = MmioRead8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21);
    MmioWrite8 (ACPI_MMIO_BASE + ACDC_BASE + FCH_ACDC_REG21, (TempData8 & ~DC_TIMER_EN));
  }

  // Init GPIO23 multi function to AC_PRESENT
  MmioWrite8 (ACPI_MMIO_BASE + IOMUX_BASE + FCH_GPIO_REG23, AC_PRESENT);

  // Clear GPIO23 wakeup & interrupt status (write 1 clear)
  TempData8 = MmioRead8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_3);
  MmioWrite8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_3, TempData8);

  // disable GPIO23 wake up support
  TempData8  = MmioRead8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_1);
  TempData8 &= (BIT2 + BIT1 + BIT0);            // Clear Bit[7:3]
  MmioWrite8 (FCH_GPIOX05C_AGPIO23_SGPIO_LOAD_MDIO1_SDA + BYTE_1, TempData8);

  DEBUG ((DEBUG_INFO, "ClearAcDcTimer Exit\n"));
}
