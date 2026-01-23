/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_H__
#define PLATFORM_H__

#define ENABLE_MEM_MAPPED_TIMER

#ifdef ENABLE_MEM_MAPPED_TIMER
// REFCLK CNTControl
#define FVP_SYSTEM_TIMER_BASE_ADDRESS   0x2A430000
// REFCLK CNTRead
#define FVP_CNT_READ_BASE_ADDRESS       0x2A800000
#else
#define FVP_SYSTEM_TIMER_BASE_ADDRESS   0xFFFFFFFFFFFFFFFF
#define FVP_CNT_READ_BASE_ADDRESS       0xFFFFFFFFFFFFFFFF
#endif

// GT Block Timer
// AP_REFCLK CNTCTL
#define FVP_GT_BLOCK_CTL_BASE           0x2A810000
#define FVP_TIMER_FRAMES_COUNT          2

// GT Block Timer Frames
// AP_REFCLK_S CNTBase0
#define FVP_GT_BLOCK_FRAME0_CTL_BASE      0x2A820000
#define FVP_GT_BLOCK_FRAME0_CTL_EL0_BASE  0xFFFFFFFFFFFFFFFF
#define FVP_GT_BLOCK_FRAME0_GSIV          57
#define FVP_GT_BLOCK_FRAME0_GSIV_GICV5    (GICV5_INTERRUPT_TYPE_SPI | 0x19)                       // [CODE_FIRST] 11148

// AP_REFCLK_NS CNTBase1
#define FVP_GT_BLOCK_FRAME1_CTL_BASE      0x2A830000
#define FVP_GT_BLOCK_FRAME1_CTL_EL0_BASE  0xFFFFFFFFFFFFFFFF
#define FVP_GT_BLOCK_FRAME1_GSIV          58
#define FVP_GT_BLOCK_FRAME1_GSIV_GICV5    (GICV5_GSI_TYPE_IWB | 0x3)                              // [CODE_FIRST] 11148

#define GTDT_TIMER_EDGE_TRIGGERED   \
          EFI_ACPI_6_2_GTDT_TIMER_FLAG_TIMER_INTERRUPT_MODE
#define GTDT_TIMER_LEVEL_TRIGGERED  0
#define GTDT_TIMER_ACTIVE_LOW       \
          EFI_ACPI_6_2_GTDT_TIMER_FLAG_TIMER_INTERRUPT_POLARITY
#define GTDT_TIMER_ACTIVE_HIGH      0
#define GTDT_TIMER_SAVE_CONTEXT     \
          EFI_ACPI_6_2_GTDT_TIMER_FLAG_ALWAYS_ON_CAPABILITY
#define GTDT_TIMER_LOSE_CONTEXT     0

#define FVP_GTDT_GTIMER_FLAGS       (GTDT_TIMER_LOSE_CONTEXT   | \
                                       GTDT_TIMER_ACTIVE_LOW   | \
                                       GTDT_TIMER_LEVEL_TRIGGERED)

// GT Block Timer Flags
#define GTX_TIMER_EDGE_TRIGGERED    \
          EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_FLAG_TIMER_INTERRUPT_MODE
#define GTX_TIMER_LEVEL_TRIGGERED   0
#define GTX_TIMER_ACTIVE_LOW        \
          EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_FLAG_TIMER_INTERRUPT_POLARITY
#define GTX_TIMER_ACTIVE_HIGH       0

#define FVP_GTX_TIMER_FLAGS         (GTX_TIMER_ACTIVE_HIGH | \
                                       GTX_TIMER_LEVEL_TRIGGERED)

#define GTX_TIMER_SECURE            \
          EFI_ACPI_6_2_GTDT_GT_BLOCK_COMMON_FLAG_SECURE_TIMER
#define GTX_TIMER_NON_SECURE        0
#define GTX_TIMER_SAVE_CONTEXT      \
          EFI_ACPI_6_2_GTDT_GT_BLOCK_COMMON_FLAG_ALWAYS_ON_CAPABILITY
#define GTX_TIMER_LOSE_CONTEXT      0

#define FVP_GTX_COMMON_FLAGS_S      (GTX_TIMER_SAVE_CONTEXT      | \
                                       GTX_TIMER_SECURE)
#define FVP_GTX_COMMON_FLAGS_NS     (GTX_TIMER_SAVE_CONTEXT      | \
                                       GTX_TIMER_NON_SECURE)

// Watchdog
#define SBSA_WATCHDOG_EDGE_TRIGGERED   \
          EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_FLAG_TIMER_INTERRUPT_MODE
#define SBSA_WATCHDOG_LEVEL_TRIGGERED  0
#define SBSA_WATCHDOG_ACTIVE_LOW       \
          EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_FLAG_TIMER_INTERRUPT_POLARITY
#define SBSA_WATCHDOG_ACTIVE_HIGH      0
#define SBSA_WATCHDOG_SECURE           \
          EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_FLAG_SECURE_TIMER
#define SBSA_WATCHDOG_NON_SECURE       0

#define FVP_SBSA_WATCHDOG_FLAGS        (SBSA_WATCHDOG_NON_SECURE       | \
                                          SBSA_WATCHDOG_ACTIVE_HIGH    | \
                                          SBSA_WATCHDOG_LEVEL_TRIGGERED)

// GICv5                                                                                          // [CODE_FIRST] 11148
#define GICV5_INTERRUPT_TYPE_SHIFT            (29)                                                // [CODE_FIRST] 11148
#define GICV5_INTERRUPT_TYPE_PPI              (1 << GICV5_INTERRUPT_TYPE_SHIFT)                   // [CODE_FIRST] 11148
#define GICV5_INTERRUPT_TYPE_LPI              (2 << GICV5_INTERRUPT_TYPE_SHIFT)                   // [CODE_FIRST] 11148
#define GICV5_INTERRUPT_TYPE_SPI              (3 << GICV5_INTERRUPT_TYPE_SHIFT)                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define GICV5_GSI_TYPE_IWB                    (7 << GICV5_INTERRUPT_TYPE_SHIFT)                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_ITS_CONF_FRAME_BASE         (0x2f120000)                                        // [CODE_FIRST] 11148
#define FVP_GICV5_ITS_TRANS_FRAME_BASE        (0x2f130000)                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_IRS_CONF_FRAME_BASE         (0x2f1a0000)                                        // [CODE_FIRST] 11148
#define FVP_GICV5_IRS_SETLPI_FRAME_BASE       (0x2f1b0000)                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_IWB_CONF_FRAME_BASE         (0x2f000000)                                        // [CODE_FIRST] 11148
#define FVP_GICV5_IWB_GSIV_BASE               (GICV5_GSI_TYPE_IWB)                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_PMU_IRQ                     (GICV5_INTERRUPT_TYPE_PPI | 0x17)                   // [CODE_FIRST] 11148
#define FVP_GICV5_VGIC_IRQ                    (GICV5_INTERRUPT_TYPE_PPI | 0x19)                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_ARM_ARCH_S_TIMER_IRQ        (GICV5_INTERRUPT_TYPE_PPI | 0x1d)                   // [CODE_FIRST] 11148
#define FVP_GICV5_ARM_ARCH_NS_TIMER_IRQ       (GICV5_INTERRUPT_TYPE_PPI | 0x1e)                   // [CODE_FIRST] 11148
#define FVP_GICV5_ARM_ARCH_VIRT_TIMER_IRQ     (GICV5_INTERRUPT_TYPE_PPI | 0x1b)                   // [CODE_FIRST] 11148
#define FVP_GICV5_ARM_ARCH_HYP_TIMER_IRQ      (GICV5_INTERRUPT_TYPE_PPI | 0x1a)                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_SPCR_IRQ                    (GICV5_INTERRUPT_TYPE_SPI | 0x05)                   // [CODE_FIRST] 11148
#define FVP_GICV5_DBG_IRQ                     (GICV5_INTERRUPT_TYPE_SPI | 0x06)                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_SMMU3_EVENT_IRQ             (GICV5_INTERRUPT_TYPE_SPI | 0x4a)                   // [CODE_FIRST] 11148
#define FVP_GICV5_SMMU3_PRI_IRQ               (GICV5_INTERRUPT_TYPE_SPI | 0x4b)                   // [CODE_FIRST] 11148
#define FVP_GICV5_SMMU3_GERR_IRQ              (GICV5_INTERRUPT_TYPE_SPI | 0x4f)                   // [CODE_FIRST] 11148
#define FVP_GICV5_SMMU3_SYNC_IRQ              (GICV5_INTERRUPT_TYPE_SPI | 0x4d)                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define FVP_GICV5_PCIE_PRT0_IRQ               (FVP_GICV5_IWB_GSIV_BASE | 0xa8)                    // [CODE_FIRST] 11148
#define FVP_GICV5_PCIE_PRT1_IRQ               (FVP_GICV5_IWB_GSIV_BASE | 0xa9)                    // [CODE_FIRST] 11148
#define FVP_GICV5_PCIE_PRT2_IRQ               (FVP_GICV5_IWB_GSIV_BASE | 0xaa)                    // [CODE_FIRST] 11148
#define FVP_GICV5_PCIE_PRT3_IRQ               (FVP_GICV5_IWB_GSIV_BASE | 0xab)                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#endif // PLATFORM_H__
