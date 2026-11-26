/** @file
  Cadence SD6 (SDHCI) PHY and clock control.

  SPDX-License-Identifier: BSD-2-Clause-Patent
  Copyright (C) 2024-2026 Marvell
**/

#include <Library/CadenceSd6PhyLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define BIT(n)                     (1U << (n))
#define GENMASK(h, l)              (((~0U) << (l)) & (~0U >> (31 - (h))))
#define FIELD_GET(mask, reg)       (((reg) & (mask)) >> LowBitSet32 (mask))
#define DIV_ROUND_UP(n, d)         (((n) + (d) - 1) / (d))
#define DIV_ROUND_DOWN_ULL(ll, d)  ((UINT32)((ll) / (d)))

#define UDELAY(x)          gBS->Stall (x)
#define READL(addr)        MmioRead32 ((UINTN)(addr))
#define WRITEL(val, addr)  MmioWrite32 ((UINTN)(addr), (val))
#define PR_ERR(...)        DEBUG ((DEBUG_ERROR, __VA_ARGS__))
#define PR_WARN(...)       DEBUG ((DEBUG_WARN, __VA_ARGS__))
#define PR_DEBUG(...)      do { } while (0)
#define DEBUG_DRV(...)     do { } while (0)

//
// Cadence HRS register offsets and fields.
//
#define SDHCI_CDNS_SRS_BASE  0x200

#define SDHCI_CDNS_HRS00      0x00
#define SDHCI_CDNS_HRS00_SWR  BIT (0)

#define SDHCI_CDNS_HRS02  0x08
#define SDHCI_CDNS_HRS04  0x10
#define SDHCI_CDNS_HRS05  0x14

#define SDHCI_CDNS_HRS06               0x18
#define SDHCI_CDNS_HRS06_MODE          GENMASK (2, 0)
#define SDHCI_CDNS_HRS06_MODE_LEGACY   0x1
#define SDHCI_CDNS_HRS06_MODE_MMC_SDR  0x2

#define SDHCI_CDNS_HRS07                0x1C
#define SDHCI_CDNS_HRS07_RW_COMPENSATE  GENMASK (20, 16)
#define SDHCI_CDNS_HRS07_IDELAY_VAL     GENMASK (4, 0)

#define SDHCI_CDNS_HRS09                    0x24
#define SDHCI_CDNS_HRS09_RDDATA_EN          BIT (16)
#define SDHCI_CDNS_HRS09_RDCMD_EN           BIT (15)
#define SDHCI_CDNS_HRS09_EXTENDED_WR_MODE   BIT (3)
#define SDHCI_CDNS_HRS09_EXTENDED_RD_MODE   BIT (2)
#define SDHCI_CDNS_HRS09_PHY_INIT_COMPLETE  BIT (1)
#define SDHCI_CDNS_HRS09_PHY_SW_RESET       BIT (0)

#define SDHCI_CDNS_HRS10             0x28
#define SDHCI_CDNS_HRS10_HCSDCLKADJ  GENMASK (19, 16)

#define SDHCI_CDNS_HRS11             0x2c
#define SDHCI_CDNS_HRS11_EMMC_RESET  BIT (0)

#define SDHCI_CDNS_SRS10  0x28
#define SDHCI_CDNS_SRS11  0x2c

#define SDHCI_CDNS_HRS16                    0x40
#define SDHCI_CDNS_HRS16_WRDATA1_SDCLK_DLY  GENMASK (31, 28)
#define SDHCI_CDNS_HRS16_WRDATA0_SDCLK_DLY  GENMASK (27, 24)
#define SDHCI_CDNS_HRS16_WRCMD1_SDCLK_DLY   GENMASK (23, 20)
#define SDHCI_CDNS_HRS16_WRCMD0_SDCLK_DLY   GENMASK (19, 16)
#define SDHCI_CDNS_HRS16_WRDATA1_DLY        GENMASK (15, 12)
#define SDHCI_CDNS_HRS16_WRDATA0_DLY        GENMASK (11, 8)
#define SDHCI_CDNS_HRS16_WRCMD1_DLY         GENMASK (7, 4)
#define SDHCI_CDNS_HRS16_WRCMD0_DLY         GENMASK (3, 0)

//
// SD6 PHY register addresses and fields.
//
#define SDHCI_CDNS_SD6_PHY_DQ_TIMING                     0x2000
#define SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_ALWAYS_ON   BIT (31)
#define SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_END         GENMASK (29, 27)
#define SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_START       GENMASK (26, 24)
#define SDHCI_CDNS_SD6_PHY_DQ_TIMING_DATA_SELECT_OE_END  GENMASK (2, 0)

#define SDHCI_CDNS_SD6_PHY_DQS_TIMING                    0x2004
#define SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_EXT_LPBK_DQS   BIT (22)
#define SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_LPBK_DQS       BIT (21)
#define SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_PHONY_DQS      BIT (20)
#define SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_PHONY_DQS_CMD  BIT (19)

#define SDHCI_CDNS_SD6_PHY_GATE_LPBK                      0x2008
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_SYNC_METHOD          BIT (31)
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_SW_DQS_PHASE_BYPASS  BIT (30)
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_EN_SW_HALF_CYCLE     BIT (29)
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_SW_HALF_CYCLE_SHIFT  BIT (28)
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_RD_DEL_SEL           GENMASK (24, 19)
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_UNDERRUN_SUPPRESS    BIT (18)
#define SDHCI_CDNS_SD6_PHY_GATE_LPBK_GATE_CFG_ALWAYS_ON   BIT (6)

#define SDHCI_CDNS_SD6_PHY_DLL_MASTER                   0x200C
#define SDHCI_CDNS_SD6_PHY_DLL_MASTER_BYPASS_MODE       BIT (23)
#define SDHCI_CDNS_SD6_PHY_DLL_MASTER_PHASE_DETECT_SEL  GENMASK (22, 20)
#define SDHCI_CDNS_SD6_PHY_DLL_MASTER_DLL_START_POINT   GENMASK (7, 0)

#define SDHCI_CDNS_SD6_PHY_DLL_SLAVE                     0x2010
#define SDHCI_CDNS_SD6_PHY_DLL_SLAVE_READ_DQS_CMD_DELAY  GENMASK (31, 24)
#define SDHCI_CDNS_SD6_PHY_DLL_SLAVE_CLK_WRDQS_DELAY     GENMASK (23, 16)
#define SDHCI_CDNS_SD6_PHY_DLL_SLAVE_CLK_WR_DELAY        GENMASK (15, 8)
#define SDHCI_CDNS_SD6_PHY_DLL_SLAVE_READ_DQS_DELAY      GENMASK (7, 0)

#define SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0                 0x201C
#define SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK        BIT (0)
#define SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK_MODE   GENMASK (2, 1)
#define SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK_VALUE  GENMASK (15, 8)

#define SDHCI_CDNS_SD6_PHY_CTRL                   0x2080
#define SDHCI_CDNS_SD6_PHY_CTRL_PHONY_DQS_TIMING  GENMASK (9, 4)

#define SDHCI_CDNS_SD6_PHY_GPIO_CTRL0              0x2088
#define SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_CLK_OVR_EN   BIT (7)
#define SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_DRV          GENMASK (6, 5)
#define SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_DRV_OVR_EN   BIT (4)
#define SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_SLEW         GENMASK (2, 1)
#define SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_SLEW_OVR_EN  BIT (0)

#define EMMC_SET_RD_DEL_SEL_         19
#define EMMC_SRS10_SD_BUS_VOLT_BASE  9
#define EMMC_SRS10_SD_BUS_VOLT_SIZE  7
#define EMMC_VOLTAGE_3_3             7
#define EMMC_SD_BUS_POWER            BIT (8)

#define SDHCI_CDNS_SD6_PHY_LOCK_MODE_FULL_CLK  0
#define SDHCI_CDNS_SD6_PHY_LOCK_MODE_HALF_CLK  2

enum {
  MMC_LEGACY = 0,
  MMC_HS_52,
};

//
// Timing constraints in picoseconds for the current bus mode.
//
typedef struct {
  UINT32    TCmdOutputMin;
  UINT32    TCmdOutputMax;
  UINT32    TDatOutputMin;
  UINT32    TDatOutputMax;
  UINT32    TCmdInputMin;
  UINT32    TCmdInputMax;
  UINT32    TDatInputMin;
  UINT32    TDatInputMax;
  UINT32    TSdclkMin;
  UINT32    TSdclkMax;
} SDHCI_CDNS_SD6_PHY_TIMINGS;

//
// Configured and measured PHY and IO cell delays, in picoseconds.
//
typedef struct {
  UINT32     PhySdclkDelay;
  UINT32     PhyCmdODelay;
  UINT32     PhyDatODelay;
  UINT32     IocellInputDelay;
  UINT32     IocellOutputDelay;
  UINT32     DelayElementOrg;
  UINT32     DelayElement;
  BOOLEAN    OverrideDelayElement;
} SDHCI_CDNS_SD6_PHY_DELAYS;

//
// Register values derived by CalcSettings() and programmed by PhyInit().
//
typedef struct {
  UINT32    CpReadDqsCmdDelay;
  UINT32    CpReadDqsDelay;
  UINT32    CpClkWrDelay;
  UINT32    CpClkWrdqsDelay;
  UINT32    CpDllBypassMode;
  UINT32    CpDllStartPoint;
  UINT32    CpGateCfgAlwaysOn;
  UINT32    CpSyncMethod;
  UINT32    CpSwDqsPhaseBypass;
  UINT32    CpEnSwHalfCycle;
  UINT32    CpRdDelSel;
  UINT32    CpSwHalfCycleShift;
  UINT32    CpUnderrunSuppress;
  UINT32    CpIoMaskAlwaysOn;
  UINT32    CpIoMaskEnd;
  UINT32    CpIoMaskStart;
  UINT32    CpDataSelectOeEnd;
  UINT32    CpUseExtLpbkDqs;
  UINT32    CpUseLpbkDqs;
  UINT8     CpUsePhonyDqs;
  UINT8     CpUsePhonyDqsCmd;
  UINT8     SdhcExtendedRdMode;
  UINT8     SdhcExtendedWrMode;
  UINT32    SdhcRdcmdEn;
  UINT32    SdhcRddataEn;
  UINT32    SdhcHcsdclkadj;
  UINT32    SdhcIdelayVal;
  UINT32    SdhcRwCompensate;
  UINT32    SdhcWrcmd0Dly;
  UINT32    SdhcWrcmd0SdclkDly;
  UINT32    SdhcWrcmd1Dly;
  UINT32    SdhcWrcmd1SdclkDly;
  UINT32    SdhcWrdata0Dly;
  UINT32    SdhcWrdata0SdclkDly;
  UINT32    SdhcWrdata1Dly;
  UINT32    SdhcWrdata1SdclkDly;
  UINT32    Drive;
  UINT32    Slew;
} SDHCI_CDNS_SD6_PHY_SETTINGS;

//
// Complete PHY state for one controller, including the timing model, the
// derived register settings, and the current bus mode and clock period.
//
typedef struct {
  SDHCI_CDNS_SD6_PHY_TIMINGS     Timings;
  SDHCI_CDNS_SD6_PHY_DELAYS      Delays;
  UINT32                         TSdmclk;
  SDHCI_CDNS_SD6_PHY_SETTINGS    Settings;
  INT32                          Mode;
  INT32                          TSdclk;
} SDHCI_CDNS_SD6_PHY;

//
// Controller instance: wrapper base, current bus clock, and PHY state.
//
typedef struct {
  UINT32                Clock;
  UINT32                BaseClock;
  VOID                  *HrsAddr;
  SDHCI_CDNS_SD6_PHY    *Priv;
} SDHCI_CDNS_PLAT;

STATIC SDHCI_CDNS_SD6_PHY  mPhy;
STATIC SDHCI_CDNS_PLAT     mPlat;

/**
  Read a PHY register through the wrapper indirect access registers.

  The register offset is written to HRS04 and the value is read back from
  HRS05.

  @param[in]  Address  PHY register offset.

  @return  The PHY register value.
**/
STATIC
UINT32
ReadPhyReg (
  IN UINT32  Address
  )
{
  WRITEL (Address, mPlat.HrsAddr + SDHCI_CDNS_HRS04);
  return READL (mPlat.HrsAddr + SDHCI_CDNS_HRS05);
}

/**
  Write a PHY register through the wrapper indirect access registers.

  The offset is written to HRS04 followed by the value to HRS05. The final
  read of HRS05 flushes the write before the function returns.

  @param[in]  Address  PHY register offset.
  @param[in]  Value    Value to write.
**/
STATIC
VOID
WritePhyReg (
  IN UINT32  Address,
  IN UINT32  Value
  )
{
  WRITEL (Address, mPlat.HrsAddr + SDHCI_CDNS_HRS04);
  WRITEL (Value, mPlat.HrsAddr + SDHCI_CDNS_HRS05);
  WRITEL (Address, mPlat.HrsAddr + SDHCI_CDNS_HRS04);
  (VOID)READL (mPlat.HrsAddr + SDHCI_CDNS_HRS05);
}

/**
  Fill in the eMMC legacy timing constraints.

  Used for bus clocks up to 26 MHz. The window values are derived from the
  SD clock period so that the PHY delay calculations can convert them into
  delay elements.

  @param[out]  Timings  Timing values to initialize.
  @param[in]   TSdClk   SD clock period in picoseconds.
**/
STATIC
VOID
InitEmmcLegacy (
  OUT SDHCI_CDNS_SD6_PHY_TIMINGS  *Timings,
  IN  INT32                       TSdClk
  )
{
  *Timings = (SDHCI_CDNS_SD6_PHY_TIMINGS) {
    .TCmdOutputMin = 3000, .TCmdOutputMax = TSdClk - 3000,
    .TDatOutputMin = 3000, .TDatOutputMax = TSdClk - 3000,
    .TCmdInputMin  = 11700, .TCmdInputMax = TSdClk + 8300,
    .TDatInputMin  = 11700, .TDatInputMax = TSdClk + 8300,
    .TSdclkMin     = 1000000 / 25, .TSdclkMax = 1000000 / 0.4
  };
}

/**
  Fill in the eMMC high-speed SDR timing constraints.

  Used for bus clocks above 26 MHz. The window values are derived from the
  SD clock period so that the PHY delay calculations can convert them into
  delay elements.

  @param[out]  Timings  Timing values to initialize.
  @param[in]   TSdClk   SD clock period in picoseconds.
**/
STATIC
VOID
InitEmmcSdr (
  OUT SDHCI_CDNS_SD6_PHY_TIMINGS  *Timings,
  IN  INT32                       TSdClk
  )
{
  *Timings = (SDHCI_CDNS_SD6_PHY_TIMINGS) {
    .TCmdOutputMin = 3000, .TCmdOutputMax = TSdClk - 3000,
    .TDatOutputMin = 3000, .TDatOutputMax = TSdClk - 3000,
    .TCmdInputMin  = 13700, .TCmdInputMax = TSdClk + 2500,
    .TDatInputMin  = 13700, .TDatInputMax = TSdClk + 2500,
    .TSdclkMin     = 1000000 / 50, .TSdclkMax = 1000000 / 0.4
  };
}

/**
  Compute the output (write) delay for the command or data path.

  The computed delay is stored in the corresponding SDHCI write-delay
  settings. The clock and write-DQS delays are cleared because the delay is
  applied through the SDHCI delay elements.

  @param[in,out]  Phy        PHY state holding the timings and computed
                             settings.
  @param[in]      CmdNotDat  TRUE to configure the command path, FALSE to
                             configure the data path.
**/
STATIC
VOID
CalcOut (
  IN OUT SDHCI_CDNS_SD6_PHY  *Phy,
  IN     BOOLEAN             CmdNotDat
  )
{
  UINT32  Wr0Dly;
  UINT32  Wr1Dly;
  UINT32  OutputMin;
  UINT32  PhyODelay;
  INT32   Timings;

  Wr0Dly = 0;
  Wr1Dly = 0;

  if (CmdNotDat) {
    OutputMin = Phy->Timings.TCmdOutputMin;
    PhyODelay = Phy->Delays.PhyCmdODelay;
  } else {
    OutputMin = Phy->Timings.TDatOutputMin;
    PhyODelay = Phy->Delays.PhyDatODelay;
  }

  Timings = PhyODelay - Phy->Delays.PhySdclkDelay - OutputMin;
  if (Timings < 0) {
    UINT32  NHalfCycle;

    NHalfCycle = DIV_ROUND_UP (-Timings * 2, Phy->TSdmclk);
    Wr0Dly     = (NHalfCycle + 1) / 2;
    Wr1Dly     = (NHalfCycle + 1) % 2 + Wr0Dly - 1;
  }

  if (CmdNotDat) {
    Phy->Settings.SdhcWrcmd0Dly      = Wr0Dly;
    Phy->Settings.SdhcWrcmd1Dly      = Wr1Dly;
    Phy->Settings.CpClkWrdqsDelay    = 0;
    Phy->Settings.SdhcWrcmd0SdclkDly = 0;
    Phy->Settings.SdhcWrcmd1SdclkDly = 0;
  } else {
    Phy->Settings.SdhcWrdata0Dly      = Wr0Dly;
    Phy->Settings.SdhcWrdata1Dly      = Wr1Dly;
    Phy->Settings.CpClkWrDelay        = 0;
    Phy->Settings.SdhcWrdata0SdclkDly = 0;
    Phy->Settings.SdhcWrdata1SdclkDly = 0;
  }
}

/**
  Compute the command input sampling settings.

  The IO mask end is derived from the round-trip IO cell delay and clamped to
  the 3-bit field width. A phony DQS is selected for the command phase and the
  read DQS command delay is left at zero.

  @param[in,out]  Phy  PHY state to update.
**/
STATIC
VOID
CalcCmdIn (
  IN OUT SDHCI_CDNS_SD6_PHY  *Phy
  )
{
  Phy->Settings.CpIoMaskEnd =
    ((Phy->Delays.IocellOutputDelay + Phy->Delays.IocellInputDelay) * 2) / Phy->TSdmclk;

  if (Phy->Settings.CpIoMaskEnd >= 8) {
    Phy->Settings.CpIoMaskEnd = 7;
  }

  Phy->Settings.CpUsePhonyDqsCmd  = 1;
  Phy->Settings.CpReadDqsCmdDelay = 0;
}

/**
  Compute the data input sampling settings.

  Accumulates the delays along the loopback DQS path (IO cells, rebar, DLL and
  synchronizers), converts the result into SD clock cycles, and stores it in
  the HCSDCLKADJ field. A phony DQS is selected for the data phase and the read
  DQS delay is left at zero.

  @param[in,out]  Phy  PHY state to update.
**/
STATIC
VOID
CalcDatIn (
  IN OUT SDHCI_CDNS_SD6_PHY  *Phy
  )
{
  UINT32  HcsdClkAdj;
  UINT32  N;

  HcsdClkAdj = 0;

  Phy->Settings.CpUsePhonyDqs  = 1;
  Phy->Settings.CpReadDqsDelay = 0;

  /* rebar PHY delay */
  HcsdClkAdj += 2 * Phy->TSdmclk;
  /* rebar output via IO cell */
  HcsdClkAdj += Phy->Delays.IocellOutputDelay;
  /* dqs loopback input via IO cell */
  HcsdClkAdj += Phy->Delays.IocellInputDelay;
  /* dfi_dqs_in: mem_dqs -> clean_dqs_mod delay of hic_dll_dqs_nand2 */
  HcsdClkAdj += Phy->Delays.DelayElement / 2;
  /* dll: one delay element between SIGI_0 and SIGO_0 */
  HcsdClkAdj += Phy->Delays.DelayElement;
  /* dfi_dqs_in: mem_dqs_delayed -> clk_dqs delay of hic_dll_dqs_nand2 */
  HcsdClkAdj += Phy->Delays.DelayElement / 2;
  /* deskew DLL: clk_dqs -> clk_dqN: one delay element */
  HcsdClkAdj += Phy->Delays.DelayElement;

  if (Phy->TSdclk == Phy->TSdmclk) {
    N = (HcsdClkAdj - 2 * Phy->TSdmclk) / Phy->TSdclk;
  } else {
    N = HcsdClkAdj / Phy->TSdclk;
  }

  /* phase shift within one TSdclk clock cycle caused by rebar - lbk dqs delay */
  HcsdClkAdj = HcsdClkAdj % Phy->TSdclk;
  /* PHY FIFO write pointer */
  HcsdClkAdj += Phy->TSdclk / 2;
  /* 1st synchronizer */
  HcsdClkAdj += DIV_ROUND_UP (HcsdClkAdj, Phy->TSdmclk) * Phy->TSdmclk - HcsdClkAdj;
  /* 2nd synchronizer + PHY FIFO read pointer + PHY rddata + PHY rddata registered */
  HcsdClkAdj += 4 * Phy->TSdmclk;

  if ((Phy->TSdclk / Phy->TSdmclk) > 1) {
    UINT32  Tmp1;
    UINT32  Tmp2;

    Tmp1 = HcsdClkAdj;
    Tmp2 = (HcsdClkAdj / Phy->TSdclk) * Phy->TSdclk + Phy->TSdclk - Phy->TSdmclk;
    if (Tmp1 == Tmp2) {
      Tmp2 += Phy->TSdclk;
    }

    /* FIFO aligns to clock cycle before ciu_en */
    HcsdClkAdj += Tmp2 - Tmp1;
  }

  /* FIFO 1st ciu_en */
  HcsdClkAdj += Phy->TSdmclk;
  /* FIFO 2nd ciu_en */
  HcsdClkAdj += Phy->TSdclk;

  HcsdClkAdj /= Phy->TSdclk;

  HcsdClkAdj += N;

  if ((Phy->TSdclk / Phy->TSdmclk) >= 2) {
    HcsdClkAdj -= 1;
  } else if ((Phy->TSdclk / Phy->TSdmclk) == 1) {
    HcsdClkAdj += 2;
  }

  if (HcsdClkAdj > 15) {
    HcsdClkAdj = 15;
  }

  Phy->Settings.SdhcHcsdclkadj = HcsdClkAdj;
}

/**
  Compute the IO cell compensation settings.

  Derives the read/write compensation value and the input delay value from the
  IO cell delays, and computes the IO mask start when data is sampled at the
  same rate that it is written.

  @param[in,out]  Phy  PHY state to update.
**/
STATIC
VOID
CalcIo (
  IN OUT SDHCI_CDNS_SD6_PHY  *Phy
  )
{
  UINT32  RwCompensate;

  RwCompensate = (Phy->Delays.IocellInputDelay + Phy->Delays.IocellOutputDelay) /
                 Phy->TSdmclk + Phy->Settings.SdhcWrdata0Dly + 5 + 3;

  Phy->Settings.SdhcIdelayVal = (2 * Phy->Delays.IocellInputDelay) / Phy->TSdmclk;

  Phy->Settings.CpIoMaskStart = 0;
  if ((Phy->TSdclk == Phy->TSdmclk) && (RwCompensate > 10)) {
    Phy->Settings.CpIoMaskStart = 2 * (RwCompensate - 10);
  }

  Phy->Settings.SdhcRwCompensate = RwCompensate;
}

/**
  Run all PHY delay calculations in the required order.

  The command output delay must be computed before the command input settings,
  and the data output delay before the data input settings, because later
  calculations depend on the earlier results.

  @param[in,out]  Phy  PHY state to update.
**/
STATIC
VOID
CalcSettings (
  IN OUT SDHCI_CDNS_SD6_PHY  *Phy
  )
{
  CalcOut (Phy, TRUE);
  CalcCmdIn (Phy);
  CalcOut (Phy, FALSE);
  CalcDatIn (Phy);
  CalcIo (Phy);
}

/**
  Recompute PHY timings and delay settings for the current SD clock.

  Selects the legacy or high-speed timing set based on the mode, initializes
  the fixed PHY delay model, enables extended read/write mode (the DLL is not
  locked in this mode), and then recalculates all derived settings.

  @param[in,out]  Plat  Platform state containing the PHY instance.
**/
STATIC
VOID
UpdateTimings (
  IN OUT SDHCI_CDNS_PLAT  *Plat
  )
{
  SDHCI_CDNS_SD6_PHY  *Phy;
  INT32               TSdmClk;

  Phy     = Plat->Priv;
  TSdmClk = Phy->TSdmclk;

  if (Phy->Mode == MMC_LEGACY) {
    InitEmmcLegacy (&Phy->Timings, Phy->TSdclk);
  } else {
    InitEmmcSdr (&Phy->Timings, Phy->TSdclk);
  }

  Phy->Delays.PhySdclkDelay = 2 * TSdmClk;
  Phy->Delays.PhyCmdODelay  = 2 * TSdmClk + TSdmClk / 2;
  Phy->Delays.PhyDatODelay  = 2 * TSdmClk + TSdmClk / 2;

  /*
   * TSdclk is always larger than TSdmclk for the legacy/high-speed clock
   * rates supported here, so the extended read/write modes are always used.
   */
  Phy->Settings.SdhcExtendedWrMode = 1;
  Phy->Settings.SdhcExtendedRdMode = 1;

  Phy->Settings.CpGateCfgAlwaysOn  = 1;
  Phy->Settings.CpSwDqsPhaseBypass = 0;

  /* Extended mode means the DLL is not locked. */
  Phy->Settings.CpDllBypassMode = 1;

  CalcSettings (Phy);
}

/**
  Change the PHY DLL reset state.

  When DoReset is TRUE the PHY software reset bit is cleared. When DoReset is
  FALSE the bit is set and the function waits until the PHY reports
  SDHCI_CDNS_HRS09_PHY_INIT_COMPLETE.

  @param[in]  Plat     Platform state containing the wrapper base address.
  @param[in]  DoReset  TRUE to clear the reset bit, FALSE to set it and wait
                       for initialization to complete.

  @return  Always 0. The prototype is kept for symmetry with PhyInit().
**/
STATIC
INT32
DllReset (
  IN SDHCI_CDNS_PLAT  *Plat,
  IN BOOLEAN          DoReset
  )
{
  UINT32  Reg;

  Reg = READL (Plat->HrsAddr + SDHCI_CDNS_HRS09);
  if (DoReset) {
    Reg &= ~SDHCI_CDNS_HRS09_PHY_SW_RESET;
  } else {
    Reg |= SDHCI_CDNS_HRS09_PHY_SW_RESET;
  }

  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS09);

  if (!DoReset) {
    do {
      Reg = READL (Plat->HrsAddr + SDHCI_CDNS_HRS09);
    } while ((Reg & SDHCI_CDNS_HRS09_PHY_INIT_COMPLETE) == 0);
  }

  return 0;
}

/**
  Program the PHY registers and wrapper timing registers.

  Applies the PHY settings computed by UpdateTimings() to the DQS timing, gate
  loopback, DLL master/slave, control, GPIO, and DQ timing registers, then
  programs the SDHCI extended modes, clock adjustment, write delays, and IO
  compensation in the wrapper.

  @param[in]  Plat  Platform state containing the PHY settings.

  @retval  0   The PHY was programmed successfully.
  @retval  -1  The DLL start point was out of range, or the DLL failed to
               initialize.
**/
STATIC
INT32
PhyInit (
  IN SDHCI_CDNS_PLAT  *Plat
  )
{
  SDHCI_CDNS_SD6_PHY  *Phy;
  UINT32              Reg;

  Phy = Plat->Priv;

  DllReset (Plat, TRUE);

  /* SDHCI_CDNS_SD6_PHY_DQS_TIMING */
  Reg  = ReadPhyReg (SDHCI_CDNS_SD6_PHY_DQS_TIMING);
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_EXT_LPBK_DQS;
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_LPBK_DQS;
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_PHONY_DQS;
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_PHONY_DQS_CMD;

  if (Phy->Settings.CpUseExtLpbkDqs) {
    Reg |= SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_EXT_LPBK_DQS;
  }

  if (Phy->Settings.CpUseLpbkDqs) {
    Reg |= SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_LPBK_DQS;
  }

  if (Phy->Settings.CpUsePhonyDqs) {
    Reg |= SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_PHONY_DQS;
  }

  if (Phy->Settings.CpUsePhonyDqsCmd) {
    Reg |= SDHCI_CDNS_SD6_PHY_DQS_TIMING_USE_PHONY_DQS_CMD;
  }

  WritePhyReg (SDHCI_CDNS_SD6_PHY_DQS_TIMING, Reg);

  /* SDHCI_CDNS_SD6_PHY_GATE_LPBK */
  Reg  = ReadPhyReg (SDHCI_CDNS_SD6_PHY_GATE_LPBK);
  Reg &= ~SDHCI_CDNS_SD6_PHY_GATE_LPBK_SYNC_METHOD;
  Reg &= ~SDHCI_CDNS_SD6_PHY_GATE_LPBK_SW_DQS_PHASE_BYPASS;
  Reg &= ~SDHCI_CDNS_SD6_PHY_GATE_LPBK_EN_SW_HALF_CYCLE;
  Reg &= ~SDHCI_CDNS_SD6_PHY_GATE_LPBK_SW_HALF_CYCLE_SHIFT;
  Reg &= ~SDHCI_CDNS_SD6_PHY_GATE_LPBK_RD_DEL_SEL;
  Reg &= ~SDHCI_CDNS_SD6_PHY_GATE_LPBK_GATE_CFG_ALWAYS_ON;

  if (Phy->Settings.CpSyncMethod) {
    Reg |= SDHCI_CDNS_SD6_PHY_GATE_LPBK_SYNC_METHOD;
  }

  if (Phy->Settings.CpSwDqsPhaseBypass) {
    Reg |= SDHCI_CDNS_SD6_PHY_GATE_LPBK_SW_DQS_PHASE_BYPASS;
  }

  if (Phy->Settings.CpEnSwHalfCycle) {
    Reg |= SDHCI_CDNS_SD6_PHY_GATE_LPBK_EN_SW_HALF_CYCLE;
  }

  if (Phy->Settings.CpSwHalfCycleShift) {
    Reg |= SDHCI_CDNS_SD6_PHY_GATE_LPBK_SW_HALF_CYCLE_SHIFT;
  }

  Reg |= ((Phy->Settings.CpRdDelSel << EMMC_SET_RD_DEL_SEL_) &
          SDHCI_CDNS_SD6_PHY_GATE_LPBK_RD_DEL_SEL);

  if (Phy->Settings.CpGateCfgAlwaysOn) {
    Reg |= SDHCI_CDNS_SD6_PHY_GATE_LPBK_GATE_CFG_ALWAYS_ON;
  }

  if (Phy->Settings.CpUnderrunSuppress) {
    Reg |= SDHCI_CDNS_SD6_PHY_GATE_LPBK_UNDERRUN_SUPPRESS;
  }

  WritePhyReg (SDHCI_CDNS_SD6_PHY_GATE_LPBK, Reg);

  /* SDHCI_CDNS_SD6_PHY_DLL_MASTER */
  Reg = 0;
  if (Phy->Settings.CpDllBypassMode) {
    Reg |= SDHCI_CDNS_SD6_PHY_DLL_MASTER_BYPASS_MODE;
  }

  Reg |= ((2 << 20) & SDHCI_CDNS_SD6_PHY_DLL_MASTER_PHASE_DETECT_SEL);
  Reg |= (Phy->Settings.CpDllStartPoint &
          SDHCI_CDNS_SD6_PHY_DLL_MASTER_DLL_START_POINT);

  if (Phy->Settings.CpDllStartPoint >= 256) {
    PR_ERR (
      "%a: CpDllStartPoint %u out of range\n",
      __func__,
      Phy->Settings.CpDllStartPoint
      );
    return -1;
  }

  WritePhyReg (SDHCI_CDNS_SD6_PHY_DLL_MASTER, Reg);

  /* SDHCI_CDNS_SD6_PHY_DLL_SLAVE */
  if ((Phy->Settings.CpReadDqsCmdDelay >= 256) ||
      (Phy->Settings.CpClkWrdqsDelay >= 256) ||
      (Phy->Settings.CpClkWrDelay >= 256) ||
      (Phy->Settings.CpReadDqsDelay >= 256))
  {
    PR_WARN ("%a: delay values out of range, clamping to 255\n", __func__);
  }

  Reg  = 0;
  Reg |= ((Phy->Settings.CpReadDqsDelay >= 256 ? 255 : Phy->Settings.CpReadDqsDelay) &
          SDHCI_CDNS_SD6_PHY_DLL_SLAVE_READ_DQS_DELAY);
  Reg |= (((Phy->Settings.CpClkWrDelay >= 256 ? 255 : Phy->Settings.CpClkWrDelay) << 8) &
          SDHCI_CDNS_SD6_PHY_DLL_SLAVE_CLK_WR_DELAY);
  Reg |= (((Phy->Settings.CpClkWrdqsDelay >= 256 ? 255 : Phy->Settings.CpClkWrdqsDelay) << 16) &
          SDHCI_CDNS_SD6_PHY_DLL_SLAVE_CLK_WRDQS_DELAY);
  Reg |= (((Phy->Settings.CpReadDqsCmdDelay >= 256 ? 255 : Phy->Settings.CpReadDqsCmdDelay) << 24) &
          SDHCI_CDNS_SD6_PHY_DLL_SLAVE_READ_DQS_CMD_DELAY);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_DLL_SLAVE, Reg);

  /* SDHCI_CDNS_SD6_PHY_CTRL */
  Reg  = ReadPhyReg (SDHCI_CDNS_SD6_PHY_CTRL);
  Reg &= ~SDHCI_CDNS_SD6_PHY_CTRL_PHONY_DQS_TIMING;
  WritePhyReg (SDHCI_CDNS_SD6_PHY_CTRL, Reg);

  Reg  = ReadPhyReg (SDHCI_CDNS_SD6_PHY_GPIO_CTRL0);
  Reg |= SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_CLK_OVR_EN;
  WritePhyReg (SDHCI_CDNS_SD6_PHY_GPIO_CTRL0, Reg);

  Reg  = ReadPhyReg (SDHCI_CDNS_SD6_PHY_GPIO_CTRL0);
  Reg &= ~(SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_SLEW_OVR_EN | SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_SLEW);
  Reg &= ~(SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_DRV_OVR_EN | SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_DRV);
  Reg |= SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_DRV_OVR_EN;
  Reg |= ((Phy->Settings.Drive << 5) & SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_DRV);
  Reg |= SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_SLEW_OVR_EN;
  Reg |= ((Phy->Settings.Slew << 1) & SDHCI_CDNS_SD6_PHY_GPIO_CTRL0_SLEW);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_GPIO_CTRL0, Reg);

  if (DllReset (Plat, FALSE) != 0) {
    return -1;
  }

  Reg  = ReadPhyReg (SDHCI_CDNS_SD6_PHY_DQ_TIMING);
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_ALWAYS_ON;
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_END;
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_START;
  Reg &= ~SDHCI_CDNS_SD6_PHY_DQ_TIMING_DATA_SELECT_OE_END;

  if (Phy->Settings.CpIoMaskAlwaysOn) {
    Reg |= SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_ALWAYS_ON;
  }

  Reg |= ((Phy->Settings.CpIoMaskEnd << 27) &
          SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_END);
  Reg |= ((Phy->Settings.CpIoMaskStart << 24) &
          SDHCI_CDNS_SD6_PHY_DQ_TIMING_IO_MASK_START);
  Reg |= ((Phy->Settings.CpDataSelectOeEnd << 0) &
          SDHCI_CDNS_SD6_PHY_DQ_TIMING_DATA_SELECT_OE_END);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_DQ_TIMING, Reg);

  Reg = READL (Plat->HrsAddr + SDHCI_CDNS_HRS09);
  if (Phy->Settings.SdhcExtendedWrMode) {
    Reg |= SDHCI_CDNS_HRS09_EXTENDED_WR_MODE;
  } else {
    Reg &= ~SDHCI_CDNS_HRS09_EXTENDED_WR_MODE;
  }

  if (Phy->Settings.SdhcExtendedRdMode) {
    Reg |= SDHCI_CDNS_HRS09_EXTENDED_RD_MODE;
  } else {
    Reg &= ~SDHCI_CDNS_HRS09_EXTENDED_RD_MODE;
  }

  if (Phy->Settings.SdhcRddataEn) {
    Reg |= SDHCI_CDNS_HRS09_RDDATA_EN;
  } else {
    Reg &= ~SDHCI_CDNS_HRS09_RDDATA_EN;
  }

  if (Phy->Settings.SdhcRdcmdEn) {
    Reg |= SDHCI_CDNS_HRS09_RDCMD_EN;
  } else {
    Reg &= ~SDHCI_CDNS_HRS09_RDCMD_EN;
  }

  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS09);

  WRITEL (0x30004, Plat->HrsAddr + SDHCI_CDNS_HRS02);

  Reg = ((Phy->Settings.SdhcHcsdclkadj << 16) & SDHCI_CDNS_HRS10_HCSDCLKADJ);
  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS10);

  if (Phy->Mode == MMC_HS_52) {
    Reg = 0x202;
  } else {
    Reg  = 0;
    Reg |= ((Phy->Settings.SdhcWrcmd0Dly << 0) & SDHCI_CDNS_HRS16_WRCMD0_DLY);
    Reg |= ((Phy->Settings.SdhcWrcmd1Dly << 4) & SDHCI_CDNS_HRS16_WRCMD1_DLY);
    Reg |= ((Phy->Settings.SdhcWrdata0Dly << 8) & SDHCI_CDNS_HRS16_WRDATA0_DLY);
    Reg |= ((Phy->Settings.SdhcWrdata1Dly << 12) & SDHCI_CDNS_HRS16_WRDATA1_DLY);
    Reg |= ((Phy->Settings.SdhcWrcmd0SdclkDly << 16) & SDHCI_CDNS_HRS16_WRCMD0_SDCLK_DLY);
    Reg |= ((Phy->Settings.SdhcWrcmd1SdclkDly << 20) & SDHCI_CDNS_HRS16_WRCMD1_SDCLK_DLY);
    Reg |= ((Phy->Settings.SdhcWrdata0SdclkDly << 24) & SDHCI_CDNS_HRS16_WRDATA0_SDCLK_DLY);
    Reg |= ((Phy->Settings.SdhcWrdata1SdclkDly << 28) & SDHCI_CDNS_HRS16_WRDATA1_SDCLK_DLY);
  }

  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS16);

  Reg  = 0;
  Reg |= ((Phy->Settings.SdhcIdelayVal << 0) & SDHCI_CDNS_HRS07_IDELAY_VAL);
  Reg |= ((Phy->Settings.SdhcRwCompensate << 16) & SDHCI_CDNS_HRS07_RW_COMPENSATE);
  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS07);

  return 0;
}

/**
  Perform a full eMMC and controller soft reset.

  Asserts the eMMC reset in HRS11, triggers the controller soft reset in
  HRS00, and waits until the controller clears the reset bit.

  @param[in]  Plat  Platform state containing the wrapper base address.
**/
STATIC
VOID
FullSwReset (
  IN SDHCI_CDNS_PLAT  *Plat
  )
{
  UINT32  Reg;

  Reg  = READL (Plat->HrsAddr + SDHCI_CDNS_HRS11);
  Reg |= SDHCI_CDNS_HRS11_EMMC_RESET;
  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS11);

  Reg  = READL (Plat->HrsAddr + SDHCI_CDNS_HRS00);
  Reg |= SDHCI_CDNS_HRS00_SWR;
  WRITEL (Reg, Plat->HrsAddr + SDHCI_CDNS_HRS00);

  do {
    Reg = READL (Plat->HrsAddr + SDHCI_CDNS_HRS00);
  } while (Reg & SDHCI_CDNS_HRS00_SWR);
}

/**
  Configure the SD bus voltage and power in the controller.

  Selects 3.3 V signalling and enables SD bus power in SRS10.
**/
STATIC
VOID
SetControllerVoltage (
  VOID
  )
{
  UINT32  Reg;

  Reg  = READL (mPlat.HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS10);
  Reg &= ~(EMMC_SRS10_SD_BUS_VOLT_SIZE << EMMC_SRS10_SD_BUS_VOLT_BASE);
  Reg |= (EMMC_VOLTAGE_3_3 << EMMC_SRS10_SD_BUS_VOLT_BASE);
  Reg |= EMMC_SD_BUS_POWER;
  WRITEL (Reg, mPlat.HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS10);
}

/**
  Determine the PHY delay element value.

  If the delay element is overridden in the platform configuration, the
  configured value is returned directly. Otherwise the DLL is calibrated at
  250 MHz: the DLL lock value and lock mode are sampled, and the delay element
  is derived from the reference period. Falls back to 8 on timeout or an
  unknown lock mode.

  @param[in]  Plat  Platform state containing the PHY instance.

  @return  Delay element value.
**/
STATIC
UINT32
GetDelayElement (
  IN SDHCI_CDNS_PLAT  *Plat
  )
{
  INT32               TimeoutCounter;
  UINT32              DelayElement;
  UINT32              LockMode;
  UINT32              Locked;
  UINT32              RegSrs11;
  UINT32              Reg;
  UINT32              DllLockValue;
  UINT32              TmpLockValue;
  SDHCI_CDNS_SD6_PHY  *Phy;

  TimeoutCounter = 1000;
  DelayElement   = 0xFF;
  LockMode       = 0xFF;
  Locked         = 0xFF;
  RegSrs11       = 0;
  DllLockValue   = 0xFF;
  TmpLockValue   = 0;
  Phy            = Plat->Priv;

  Phy->Settings.CpDllBypassMode = 0;
  if (Phy->Delays.OverrideDelayElement) {
    return Phy->Delays.DelayElement;
  }

  /* Set up the registers for 250 MHz clock at HS400 and wait for DLL lock. */
  WritePhyReg (SDHCI_CDNS_SD6_PHY_DQS_TIMING, 0x780004);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_GATE_LPBK, 0x81a00040);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_DLL_MASTER, 0x200004);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_DLL_SLAVE, 0x404000);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_CTRL, 0x4000);
  WritePhyReg (SDHCI_CDNS_SD6_PHY_DQ_TIMING, 0x1);

  /* Turn on internal clock. */
  RegSrs11  = READL (Plat->HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS11);
  RegSrs11 |= 0x1;
  WRITEL (RegSrs11, Plat->HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS11);

  /* Wait for clock to be stable. */
  do {
    RegSrs11 = READL (Plat->HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS11);
  } while (!(RegSrs11 & BIT (1)));

  RegSrs11  = READL (Plat->HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS11);
  RegSrs11 &= ~0xFFC0;
  RegSrs11 |= 0x100;
  WRITEL (RegSrs11, Plat->HrsAddr + SDHCI_CDNS_SRS_BASE + SDHCI_CDNS_SRS11);

  /* 8-bit bursts, 4 transfers. */
  WRITEL (0x30004, Plat->HrsAddr + SDHCI_CDNS_HRS02);
  WRITEL (0x5, Plat->HrsAddr + SDHCI_CDNS_HRS06);

  /* Reset PHY. */
  WRITEL (0xF1C00003, Plat->HrsAddr + SDHCI_CDNS_HRS09);
  do {
    Reg = READL (Plat->HrsAddr + SDHCI_CDNS_HRS09);
  } while (!(Reg & BIT (1)));

  WRITEL (0x30004, Plat->HrsAddr + SDHCI_CDNS_HRS02);
  WRITEL (0x5, Plat->HrsAddr + SDHCI_CDNS_HRS06);

  UDELAY (1000);

  Reg          = ReadPhyReg (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0);
  Locked       = FIELD_GET (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK, Reg);
  TmpLockValue = FIELD_GET (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK_VALUE, Reg);

  while (DllLockValue != TmpLockValue) {
    Locked   = FIELD_GET (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK, Reg);
    LockMode = FIELD_GET (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK_MODE, Reg);

    if (TimeoutCounter-- <= 0) {
      return 8;
    }

    if (DllLockValue != TmpLockValue) {
      DllLockValue = TmpLockValue;
    } else {
      break;
    }

    UDELAY (1);
    Reg          = ReadPhyReg (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0);
    TmpLockValue = FIELD_GET (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK_VALUE, Reg);
  }

  DllLockValue = FIELD_GET (SDHCI_CDNS_SD6_PHY_DLL_OBS_REG0_DLL_LOCK_VALUE, Reg);

  if (LockMode == SDHCI_CDNS_SD6_PHY_LOCK_MODE_FULL_CLK) {
    DelayElement = DIV_ROUND_UP (4000, (DllLockValue + 1));
  } else if (LockMode == SDHCI_CDNS_SD6_PHY_LOCK_MODE_HALF_CLK) {
    DelayElement = DIV_ROUND_UP (2000, (DllLockValue + 1));
  } else {
    DelayElement = 8;
  }

  Phy->Settings.CpDllBypassMode = !Locked;
  return DelayElement;
}

/**
  Select the controller transfer mode based on the bus clock.

  Clocks up to 26 MHz use the legacy mode; higher clocks use the MMC SDR mode.
  The mode is recorded in the PHY state and programmed into HRS06.

  @param[in]  Plat  Platform state containing the clock and PHY instance.
**/
STATIC
VOID
SetControlReg (
  IN SDHCI_CDNS_PLAT  *Plat
  )
{
  SDHCI_CDNS_SD6_PHY  *Phy;
  UINT32              Mode;
  UINT32              Tmp;

  Phy = Plat->Priv;

  if (Plat->Clock <= 26000000) {
    Phy->Mode = MMC_LEGACY;
    Mode      = SDHCI_CDNS_HRS06_MODE_LEGACY;
  } else {
    Phy->Mode = MMC_HS_52;
    Mode      = SDHCI_CDNS_HRS06_MODE_MMC_SDR;
  }

  Tmp  = READL (Plat->HrsAddr + SDHCI_CDNS_HRS06);
  Tmp &= ~SDHCI_CDNS_HRS06_MODE;
  Tmp |= (Mode & SDHCI_CDNS_HRS06_MODE);
  WRITEL (Tmp, Plat->HrsAddr + SDHCI_CDNS_HRS06);
}

/**
  Apply a clock divider to the PHY and wrapper.

  Computes the resulting SD clock and its period, selects the controller mode,
  recalculates the PHY timings, and reprograms the PHY. A divider of zero is
  treated as one.

  @param[in]  Plat  Platform state containing the base clock and PHY instance.
  @param[in]  Div   Clock divider applied to the base clock.
**/
STATIC
VOID
SetClock (
  IN SDHCI_CDNS_PLAT  *Plat,
  IN UINT32           Div
  )
{
  SDHCI_CDNS_SD6_PHY  *Phy;
  UINT32              Clock;

  Phy = Plat->Priv;

  if (Div == 0) {
    Div = 1;
  }

  Clock = DIV_ROUND_DOWN_ULL (Plat->BaseClock, Div);

  Plat->Clock = Clock;
  Phy->TSdclk = DIV_ROUND_DOWN_ULL (1000000000000ULL, Clock);

  SetControlReg (Plat);
  UpdateTimings (Plat);
  PhyInit (Plat);
}

/**
  Initialize the controller and PHY to their default state.

  Performs a full reset, sets the bus voltage, initializes the PHY delay model
  and register settings, and computes the timing values for the current clock.

  @param[in]  Plat  Platform state to initialize.
**/
STATIC
VOID
PlatInit (
  IN SDHCI_CDNS_PLAT  *Plat
  )
{
  SDHCI_CDNS_SD6_PHY  *Phy;

  Phy = Plat->Priv;

  FullSwReset (Plat);
  SetControllerVoltage ();

  Phy->TSdmclk = 5000;

  Phy->Delays.DelayElementOrg     = Phy->Delays.DelayElement;
  Phy->Settings.CpSyncMethod      = 1;
  Phy->Settings.CpRdDelSel        = 52;
  Phy->Settings.CpUseExtLpbkDqs   = 1;
  Phy->Settings.CpUseLpbkDqs      = 1;
  Phy->Settings.CpDataSelectOeEnd = 1;
  Phy->Settings.CpDllStartPoint   = 4;

  Phy->Settings.CpUsePhonyDqs    = 1;
  Phy->Settings.CpUsePhonyDqsCmd = 1;
  Phy->Settings.CpDllBypassMode  = 1;

  Phy->Settings.CpReadDqsCmdDelay  = 0;
  Phy->Settings.CpClkWrdqsDelay    = 0;
  Phy->Settings.CpClkWrDelay       = 0;
  Phy->Settings.CpReadDqsDelay     = 0;
  Phy->Settings.CpIoMaskEnd        = 5;
  Phy->Settings.CpIoMaskStart      = 0;
  Phy->Settings.SdhcExtendedRdMode = 1;
  Phy->Settings.SdhcExtendedWrMode = 1;
  Phy->Settings.SdhcHcsdclkadj     = 6;
  Phy->Settings.SdhcRwCompensate   = 10;
  Phy->Settings.SdhcIdelayVal      = 0;
  Phy->Settings.SdhcRddataEn       = 1;
  Phy->Settings.SdhcRdcmdEn        = 1;

  UpdateTimings (Plat);
}

/**
  Reset and initialize the Cadence SD6 PHY.

  Copies the platform and board specific parameters, resets the controller and
  PHY, programs the initial timings, and finally calibrates the DLL to
  determine the delay element value.

  @param[in]  Config  Controller and board specific PHY parameters.

  @retval  EFI_SUCCESS            The PHY was initialized.
  @retval  EFI_INVALID_PARAMETER  Config is NULL, or WrapperBase or
                                  BaseClockHz is zero.
  @retval  EFI_DEVICE_ERROR       The PHY failed to initialize.
**/
EFI_STATUS
CadenceSd6PhyInit (
  IN CONST CADENCE_SD6_PHY_CONFIG  *Config
  )
{
  if ((Config == NULL) || (Config->WrapperBase == 0) || (Config->BaseClockHz == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&mPhy, sizeof (mPhy));
  ZeroMem (&mPlat, sizeof (mPlat));

  mPlat.HrsAddr   = (VOID *)Config->WrapperBase;
  mPlat.BaseClock = Config->BaseClockHz;
  mPlat.Priv      = &mPhy;

  mPhy.Delays.DelayElement         = Config->DelayElement;
  mPhy.Delays.OverrideDelayElement = Config->OverrideDelayElement;
  mPhy.Delays.IocellInputDelay     = Config->IocellInputDelay;
  mPhy.Delays.IocellOutputDelay    = Config->IocellOutputDelay;
  mPhy.Settings.Drive              = Config->HostDrive;
  mPhy.Settings.Slew               = Config->HostSlew;

  PlatInit (&mPlat);
  if (PhyInit (&mPlat) != 0) {
    return EFI_DEVICE_ERROR;
  }

  mPhy.Delays.DelayElement = GetDelayElement (&mPlat);
  return EFI_SUCCESS;
}

/**
  Program the SD6 PHY for the divider currently written to the SDHCI Clock
  Control register.

  The SDHCI divider is doubled before being applied because the wrapper and
  PHY divide the base clock by two. A divider of zero is treated as one.

  @param[in]  HwDivider  SDHCI clock divider field value.
**/
VOID
CadenceSd6PhySetClock (
  IN UINT32  HwDivider
  )
{
  if (HwDivider == 0) {
    HwDivider = 1;
  }

  SetClock (&mPlat, HwDivider * 2);
}
