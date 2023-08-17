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

// AP QSPI flash device
#define MORELLO_AP_QSPI_AHB_BASE  0x1A000000
#define MORELLO_AP_QSPI_AHB_SZ    0x2000000

// Platform information structure base address
#define MORELLO_PLAT_INFO_STRUCT_BASE  MORELLO_NON_SECURE_SRAM_BASE

// ACPI OSC Status bits
#define OSC_STS_BIT0_RES           (1U << 0)
#define OSC_STS_FAILURE            (1U << 1)
#define OSC_STS_UNRECOGNIZED_UUID  (1U << 2)
#define OSC_STS_UNRECOGNIZED_REV   (1U << 3)
#define OSC_STS_CAPABILITY_MASKED  (1U << 4)
#define OSC_STS_MASK               (OSC_STS_BIT0_RES          |    \
                                       OSC_STS_FAILURE           | \
                                       OSC_STS_UNRECOGNIZED_UUID | \
                                       OSC_STS_UNRECOGNIZED_REV  | \
                                       OSC_STS_CAPABILITY_MASKED)

// ACPI OSC for Platform-Wide Capability
#define OSC_CAP_CPPC_SUPPORT          (1U << 5)
#define OSC_CAP_CPPC2_SUPPORT         (1U << 6)
#define OSC_CAP_PLAT_COORDINATED_LPI  (1U << 7)
#define OSC_CAP_OS_INITIATED_LPI      (1U << 8)

// morello silicon revision
#define MORELLO_SILICON_REVISION_R_MASK  0xFFFF0000
#define MORELLO_SILICON_REVISION_P_MASK  0x0000FFFF
#define MORELLO_SILICON_REVISION_R_POS   16
#define MORELLO_SILICON_REVISION_P_POS   0

// Morello firmware version masks
#define MORELLO_FW_REVISION_MAJOR_OFFSET   24
#define MORELLO_FW_REVISION_MINOR_OFFSET   16
#define MORELLO_FW_REVISION_PATCH_OFFSET   8
#define MORELLO_FW_REVISION_FLAGS_OFFSET   0
#define MORELLO_FW_REVISION_MASK           0xFF
#define MORELLO_FW_REVISION_MAINLINE_MASK  0x40
#define MORELLO_FW_REVISION_DIRTY_MASK     0x80

/*
 * Platform information structure stored in Non-secure SRAM. Platform
 * information are passed from the trusted firmware with the below structure
 * format. The elements of MORELLO_PLAT_INFO should be always in sync with
 * the lower level firmware.
 */
#pragma pack(1)

typedef struct {
  UINT64    LocalDdrSize;    ///< Local DDR memory size in Bytes
  UINT64    RemoteDdrSize;   ///< Remote DDR memory size in Bytes
  UINT8     RemoteChipCount; ///< Remote chip count in C2C mode
  UINT8     Mode;            ///< 0 - Single Chip, 1 - Chip to Chip (C2C)
  UINT32    SccConfig;       ///< Contains SCC configuration from BOOT_GPR1 register
} MORELLO_PLAT_INFO_SOC;

typedef struct {
  UINT64    LocalDdrSize; ///< Local DDR memory size in Bytes
} MORELLO_PLAT_INFO_FVP;

typedef struct {
  UINT32    ScpFwRevision; ///< Contains SCP Firwmare Revision
  UINT32    ScpFwCommit;   ///< Contains SCP Firmware commit ID
} MORELLO_FW_VERSION_SOC;

typedef struct {
  UINT32    ScpFwRevision; ///< Contains SCP Firwmare Revision
  UINT32    ScpFwCommit;   ///< Contains SCP Firmware commit ID
} MORELLO_FW_VERSION_FVP;

#pragma pack()

typedef struct {
  CONST VOID    *NtFwConfig;
  CONST VOID    *HwConfig;
} MORELLO_EL3_FW_HANDOFF_PARAM_PPI;
#endif //MORELLO_PLATFORM_H_
