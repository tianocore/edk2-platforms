/** @file

  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MORELLO_PLATFORM_H_
#define MORELLO_PLATFORM_H_

#define MORELLO_DRAM_BLOCK1_SIZE  SIZE_2GB

// ****************************************************************************
// Platform Memory Map
// ****************************************************************************

// SubSystem Peripherals - UART0
#define MORELLO_UART0_BASE  0x2A400000
#define MORELLO_UART0_SZ    SIZE_64KB

// SubSystem Peripherals - UART1
#define MORELLO_UART1_BASE  0x2A410000
#define MORELLO_UART1_SZ    SIZE_64KB

// SubSystem Peripherals - Generic Watchdog
#define MORELLO_GENERIC_WDOG_BASE  0x2A440000
#define MORELLO_GENERIC_WDOG_SZ    SIZE_128KB

// SubSystem Peripherals - REFCLK CNTRead
#define MORELLO_REFCLK_CNT_BASE  0x2A800000
#define MORELLO_REFCLK_CNT_SZ    SIZE_64KB

// SubSystem Peripherals - AP_REFCLK CNTCTL
#define MORELLO_AP_REFCLK_CNT_BASE  0x2A810000
#define MORELLO_AP_REFCLK_CNT_SZ    SIZE_64KB

// SubSystem Peripherals - AP_REFCLK_NS CNTCTL
#define MORELLO_AP_REFCLK_NS_CNT_BASE  0x2A830000
#define MORELLO_AP_REFCLK_NS_CNT_SZ    SIZE_64KB

// SubSystem Peripherals - GIC(600)
#define MORELLO_GIC_BASE     0x30000000
#define MORELLO_GICITS_BASE  0x30040000
#define MORELLO_GICR_BASE    0x300C0000
#define MORELLO_GIC_SZ       SIZE_256KB
#define MORELLO_GICITS_SZ    SIZE_512KB
#define MORELLO_GICR_SZ      SIZE_1MB

// SubSystem SMMU
#define MORELLO_SMMU_BASE  0x4F000000
#define MORELLO_SMMU_SZ    SIZE_16MB

// SubSystem non-secure SRAM
#define MORELLO_NON_SECURE_SRAM_BASE  0x06000000
#define MORELLO_NON_SECURE_SRAM_SZ    SIZE_64KB

// AXI Expansion peripherals
#define MORELLO_AXI_EXPANSION_PERIPHERAL_BASE  0x1C000000
#define MORELLO_AXI_EXPANSION_PERIPHERAL_SZ    0x1300000

// Platform information structure base address
#define MORELLO_PLAT_INFO_STRUCT_BASE  MORELLO_NON_SECURE_SRAM_BASE

/*
 * Platform information structure stored in Non-secure SRAM. Platform
 * information are passed from the trusted firmware with the below structure
 * format. The elements of MORELLO_PLAT_INFO should be always in sync with
 * the lower level firmware.
 */
#pragma pack(1)

typedef struct {
  UINT64    LocalDdrSize; ///< Local DDR memory size in Bytes
} MORELLO_PLAT_INFO_FVP;

#pragma pack()

typedef struct {
  CONST VOID    *NtFwConfig;
} MORELLO_EL3_FW_HANDOFF_PARAM_PPI;
#endif //MORELLO_PLATFORM_H_
