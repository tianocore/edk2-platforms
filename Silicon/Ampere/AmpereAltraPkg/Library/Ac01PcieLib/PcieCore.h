/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AC01_PCIE_CORE_H_
#define AC01_PCIE_CORE_H_

#define BUS_SHIFT  20
#define DEV_SHIFT  15

#define GET_LOW_8_BITS(x)          ((x) & 0xFF)
#define GET_HIGH_8_BITS(x)         (((x) >> 8) & 0xFF)
#define GET_LOW_16_BITS(x)         ((x) & 0xFFFF)
#define GET_HIGH_16_BITS(x)        (((x) >> 16) & 0xFFFF)
#define GET_CAPABILITY_PTR(x)      (GET_LOW_16_BITS (x) >> 8)
#define GET_EXT_CAPABILITY_PTR(x)  (GET_HIGH_16_BITS (x) >> 4)

#define WORD_ALIGN_MASK  0x3

#define MAX_REINIT  3                           // Number of soft reset retry

#define SLOT_POWER_LIMIT_75W  75                // Watt

#define LINK_CHECK_SUCCESS          0
#define LINK_CHECK_FAILED           -1
#define LINK_CHECK_WRONG_PARAMETER  1

#define AMPERE_PCIE_VENDOR_ID           0x1DEF
#define AC01_HOST_BRIDGE_DEVICE_ID_RCA  0xE100
#define AC01_HOST_BRIDGE_DEVICE_ID_RCB  0xE110
#define AC01_PCIE_BRIDGE_DEVICE_ID_RCA  0xE101
#define AC01_PCIE_BRIDGE_DEVICE_ID_RCB  0xE111

#define MEMRDY_TIMEOUT            10                 // 10 us
#define PIPE_CLOCK_TIMEOUT        20000              // 20,000 us
#define LTSSM_TRANSITION_TIMEOUT  100000             // 100 ms in total
#define EP_LINKUP_TIMEOUT         (10 * 1000)        // 10ms
#define EP_LINKUP_EXTRA_TIMEOUT   (500 * 1000)       // 500ms
#define LINK_WAIT_INTERVAL_US     50

#define PFA_MODE_ENABLE  0
#define PFA_MODE_CLEAR   1
#define PFA_MODE_READ    2

#define BIFURCATION_X000  0
#define BIFURCATION_X0X0  1
#define BIFURCATION_X0XX  2
#define BIFURCATION_XXXX  3

//
// Host Bridge registers
//
#define AC01_HOST_BRIDGE_RCA_DEV_MAP_REG       0x0
#define AC01_HOST_BRIDGE_RCB_DEV_MAP_REG       0x4
#define AC01_HOST_BRIDGE_VENDOR_DEVICE_ID_REG  0x10

// AC01_HOST_BRIDGE_RCA_DEV_MAP_REG
#define RCA_DEV_MAP_SET(dst, src)  (((dst) & ~0x7) | (((UINT32) (src)) & 0x7))
#define RCA_DEV_MAP_GET(val)       ((val) & 0x7)

// AC01_HOST_BRIDGE_RCB_DEV_MAP_REG
#define RCB_DEV_MAP_LOW_SET(dst, src)  (((dst) & ~0x7) | (((UINT32) (src)) & 0x7))
#define RCB_DEV_MAP_LOW_GET(val)       ((val) & 0x7)

#define RCB_DEV_MAP_HIGH_SET(dst, src)  (((dst) & ~0x70) | (((UINT32) (src) << 4) & 0x70))
#define RCB_DEV_MAP_HIGH_GET(val)       (((val) & 0x7) >> 4)

// AC01_HOST_BRIDGE_VENDOR_DEVICE_ID_REG
#define VENDOR_ID_SET(dst, src)  (((dst) & ~0xFFFF) | (((UINT32) (src))  & 0xFFFF))
#define VENDOR_ID_GET(val)       ((val) & 0xFFFF)

#define DEVICE_ID_SET(dst, src)  (((dst) & ~0xFFFF0000) | (((UINT32) (src) << 16) & 0xFFFF0000))
#define DEVICE_ID_GET(val)       (((val) & 0xFFFF0000) >> 16)

//
// PCIe core registers
//
#define AC01_PCIE_CORE_LINK_CTRL_REG         0x0
#define AC01_PCIE_CORE_LINK_STAT_REG         0x4
#define AC01_PCIE_CORE_IRQ_SEL_REG           0xC
#define AC01_PCIE_CORE_HOT_PLUG_STAT_REG     0x28
#define AC01_PCIE_CORE_IRQ_ENABLE_REG        0x30
#define AC01_PCIE_CORE_IRQ_EVENT_STAT_REG    0x38
#define AC01_PCIE_CORE_BLOCK_EVENT_STAT_REG  0x3C
#define AC01_PCIE_CORE_BUS_CONTROL_REG       0x40
#define AC01_PCIE_CORE_RESET_REG             0xC000
#define AC01_PCIE_CORE_CLOCK_REG             0xC004
#define AC01_PCIE_CORE_MEM_READY_REG         0xC104
#define AC01_PCIE_CORE_RAM_SHUTDOWN_REG      0xC10C

// AC01_PCIE_CORE_LINK_CTRL_REG
#define LTSSMENB_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))
#define LTSSMENB_GET(dst)       ((dst) & (BIT0))
#define   HOLD_LINK_TRAINING   0
#define   START_LINK_TRAINING  1
#define DEVICETYPE_SET(dst, src)  (((dst) & ~0xF0) | (((UINT32) (src) << 4) & 0xF0))
#define DEVICETYPE_GET(val)       (((val) & 0xF0) >> 4)

// AC01_PCIE_CORE_LINK_STAT_REG
#define PHY_STATUS_MASK        (1 << 2)
#define SMLH_LTSSM_STATE_MASK  0x3F00
#define SMLH_LTSSM_STATE_GET(val)  ((val & SMLH_LTSSM_STATE_MASK) >> 8)
#define   LTSSM_STATE_L0  0x11
#define RDLH_SMLH_LINKUP_STATUS_GET(val)  (val & 0x3)
#define PHY_STATUS_MASK_BIT    0x04
#define SMLH_LINK_UP_MASK_BIT  0x02
#define RDLH_LINK_UP_MASK_BIT  0x01

// AC01_PCIE_CORE_IRQ_SEL_REG
#define AER_SET(dst, src)         (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))
#define PME_SET(dst, src)         (((dst) & ~0x2) | (((UINT32) (src) << 1) & 0x2))
#define LINKAUTOBW_SET(dst, src)  (((dst) & ~0x4) | (((UINT32) (src) << 2) & 0x4))
#define BWMGMT_SET(dst, src)      (((dst) & ~0x8) | (((UINT32) (src) << 3) & 0x8))
#define EQRQST_SET(dst, src)      (((dst) & ~0x10) | (((UINT32) (src) << 4) & 0x10))
#define INTPIN_SET(dst, src)      (((dst) & ~0xFF00) | (((UINT32) (src) << 8) & 0xFF00))
#define IRQ_INT_A  0x01

// AC01_PCIE_CORE_HOT_PLUG_STAT_REG
#define PWR_IND_SET(dst, src)    (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))
#define ATTEN_IND_SET(dst, src)  (((dst) & ~0x2) | (((UINT32) (src) << 1) & 0x2))
#define PWR_CTRL_SET(dst, src)   (((dst) & ~0x4) | (((UINT32) (src) << 2) & 0x4))
#define EML_CTRL_SET(dst, src)   (((dst) & ~0x8) | (((UINT32) (src) << 3) & 0x8))

// AC01_PCIE_CORE_BLOCK_EVENT_STAT_REG
#define LINKUP_MASK  0x1

// AC01_PCIE_CORE_BUS_CONTROL_REG
#define BUS_CTL_CFG_UR_MASK  0x8

// AC01_PCIE_CORE_RESET_REG
#define DWC_PCIE_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))
#define   RESET_MASK    0x1
#define   ASSERT_RESET  0x1

// AC01_PCIE_CORE_CLOCK_REG
#define AXIPIPE_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))

// AC01_PCIE_CORE_MEM_READY_REG
#define MEMORY_READY  0x1

// AC01_PCIE_CORE_RAM_SHUTDOWN_REG
#define SD_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))

//
// AC01 PCIe Type 1 configuration registers
//
#define TYPE1_DEV_ID_VEND_ID_REG                   0
#define TYPE1_CLASS_CODE_REV_ID_REG                0x8
#define TYPE1_CAP_PTR_REG                          0x34
#define SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG  0x18
#define BRIDGE_CTRL_INT_PIN_INT_LINE_REG           0x3C
#define PCIE_CAPABILITY_BASE                       0x70
#define EXT_CAPABILITY_START_BASE                  0x100
#define AER_CAPABILITY_BASE                        0x100

// TYPE1_DEV_ID_VEND_ID_REG
#define VENDOR_ID_SET(dst, src)  (((dst) & ~0xFFFF) | (((UINT32) (src)) & 0xFFFF))
#define DEVICE_ID_SET(dst, src)  (((dst) & ~0xFFFF0000) | (((UINT32) (src) << 16) & 0xFFFF0000))

// TYPE1_CLASS_CODE_REV_ID_REG
#define BASE_CLASS_CODE_SET(dst, src)  (((dst) & ~0xFF000000) | (((UINT32) (src) << 24) & 0xFF000000))
#define   DEFAULT_BASE_CLASS_CODE  6
#define SUB_CLASS_CODE_SET(dst, src)  (((dst) & ~0xFF0000) | (((UINT32) (src) << 16) & 0xFF0000))
#define   DEFAULT_SUB_CLASS_CODE  4
#define PROGRAM_INTERFACE_SET(dst, src)  (((dst) & ~0xFF00) | (((UINT32) (src) << 8) & 0xFF00))
#define REVISION_ID_SET(dst, src)        (((dst) & ~0xFF) | (((UINT32) (src)) & 0xFF))
#define   DEFAULT_REVISION_ID  4

// SEC_LAT_TIMER_SUB_BUS_SEC_BUS_PRI_BUS_REG
#define SUB_BUS_SET(dst, src)  (((dst) & ~0xFF0000) | (((UINT32) (src) << 16) & 0xFF0000))
#define   DEFAULT_SUB_BUS  0xFF
#define SEC_BUS_SET(dst, src)   (((dst) & ~0xFF00) | (((UINT32) (src) << 8) & 0xFF00))
#define PRIM_BUS_SET(dst, src)  (((dst) & ~0xFF) | (((UINT32) (src)) & 0xFF))
#define   DEFAULT_PRIM_BUS  0x00

// BRIDGE_CTRL_INT_PIN_INT_LINE_REG
#define INT_PIN_SET(dst, src)  (((dst) & ~0xFF00) | (((UINT32) (src) << 8) & 0xFF00))

//
// PCI Express Capability
//
#define PCIE_CAPABILITY_ID                  0x10
#define LINK_CAPABILITIES_REG               0xC
#define LINK_CONTROL_LINK_STATUS_REG        0x10
#define SLOT_CAPABILITIES_REG               0x14
#define DEVICE_CONTROL2_DEVICE_STATUS2_REG  0x28
#define LINK_CAPABILITIES2_REG              0x2C
#define LINK_CONTROL2_LINK_STATUS2_REG      0x30

// LINK_CAPABILITIES_REG
#define CAP_ACTIVE_STATE_LINK_PM_SUPPORT_SET(dst, src)  (((dst) & ~0xC00) | (((UINT32)(src) << 10) & 0xC00))
#define   NO_ASPM_SUPPORTED  0x0
#define   L0S_SUPPORTED      0x1
#define   L1_SUPPORTED       0x2
#define   L0S_L1_SUPPORTED   0x3
#define CAP_MAX_LINK_WIDTH_GET(val)       ((val & 0x3F0) >> 4)
#define CAP_MAX_LINK_WIDTH_SET(dst, src)  (((dst) & ~0x3F0) | (((UINT32) (src) << 4) & 0x3F0))
#define   CAP_MAX_LINK_WIDTH_X1   0x1
#define   CAP_MAX_LINK_WIDTH_X2   0x2
#define   CAP_MAX_LINK_WIDTH_X4   0x4
#define   CAP_MAX_LINK_WIDTH_X8   0x8
#define   CAP_MAX_LINK_WIDTH_X16  0x10
#define CAP_MAX_LINK_SPEED_GET(val)       ((val & 0xF))
#define CAP_MAX_LINK_SPEED_SET(dst, src)  (((dst) & ~0xF) | (((UINT32) (src)) & 0xF))
#define   MAX_LINK_SPEED_25   0x1
#define   MAX_LINK_SPEED_50   0x2
#define   MAX_LINK_SPEED_80   0x3
#define   MAX_LINK_SPEED_160  0x4
#define   MAX_LINK_SPEED_320  0x5

// LINK_CONTROL_LINK_STATUS_REG
#define CAP_DLL_ACTIVE_GET(val)             ((val & 0x20000000) >> 29)
#define CAP_SLOT_CLK_CONFIG_SET(dst, src)   (((dst) & ~0x10000000) | (((UINT32) (src) << 28) & 0x10000000))
#define CAP_NEGO_LINK_WIDTH_GET(val)        ((val & 0x3F00000) >> 20)
#define CAP_LINK_SPEED_GET(val)             ((val & 0xF0000) >> 16)
#define CAP_LINK_SPEED_SET(dst, src)        (((dst) & ~0xF0000) | (((UINT32) (src) << 16) & 0xF0000))
#define CAP_LINK_SPEED_TO_VECTOR(val)       (1 << ((val) - 1))
#define CAP_EN_CLK_POWER_MAN_GET(val)       ((val & 0x100) >> 8)
#define CAP_EN_CLK_POWER_MAN_SET(dst, src)  (((dst) & ~0x100) | (((UINT32) (src) << 8) & 0x100))
#define CAP_COMMON_CLK_SET(dst, src)        (((dst) & ~0x40) | (((UINT32) (src) << 6) & 0x40))
#define CAP_RETRAIN_LINK_SET(dst, src)      (((dst) & ~0x20) | (((UINT32) (src) << 5) & 0x20))
#define CAP_LINK_TRAINING_GET(val)          ((val & 0x8000000) >> 27)
#define CAP_LINK_DISABLE_SET(dst, src)      (((dst) & ~0x10) | (((UINT32)(src) << 4) & 0x10))

// SLOT_CAPABILITIES_REG
#define SLOT_HPC_SET(dst, src)  (((dst) & ~0x40) | (((UINT32) (src) << 6) & 0x40))
#define SLOT_CAP_SLOT_POWER_LIMIT_VALUE_SET(dst, src) \
                                                (((dst) & ~0x7F80) | (((UINT32)(src) << 7) & 0x7F80))

// DEVICE_CONTROL2_DEVICE_STATUS2_REG
#define CAP_CPL_TIMEOUT_VALUE_SET(dst, src)  (((dst) & ~0xF) | (((UINT32) (src)) & 0xF))

// LINK_CONTROL2_LINK_STATUS2_REG
#define CAP_TARGET_LINK_SPEED_SET(dst, src)  (((dst) & ~0xF) | (((UINT32) (src)) & 0xF))

//
// Advanced Error Reporting Capability
//
#define AER_CAPABILITY_ID      0x0001
#define UNCORR_ERR_STATUS_OFF  0x04
#define UNCORR_ERR_MASK_OFF    0x08

// UNCORR_ERR_MASK_OFF
#define CMPLT_TIMEOUT_ERR_MASK_SET(dst, src)  (((dst) & ~0x4000) | (((UINT32) (src) << 14) & 0x4000))
#define SDES_ERR_MASK_SET(dst, src)           (((dst) & ~0x20) | (((UINT32)(src) << 5) & 0x20))

//
// Vendor specific RAS D.E.S Capability
//
#define RAS_DES_CAPABILITY_ID      0x000B
#define EVENT_COUNTER_CONTROL_REG  0x08
#define EVENT_COUNTER_DATA_REG     0x0C

// EVENT_COUNTER_CONTROL_REG
#define ECCR_GROUP_EVENT_SEL_SET(dst, src)       (((dst) & ~0xFFF0000) | (((UINT32)(src) << 16) & 0xFFF0000))
#define ECCR_GROUP_SEL_SET(dst, src)             (((dst) & ~0xF000000) | (((UINT32)(src) << 24) & 0xF000000))
#define ECCR_EVENT_SEL_SET(dst, src)             (((dst) & ~0xFF0000) | (((UINT32)(src) << 16) & 0xFF0000))
#define ECCR_LANE_SEL_SET(dst, src)              (((dst) & ~0xF00) | (((UINT32)(src) << 8) & 0xF00))
#define ECCR_EVENT_COUNTER_ENABLE_SET(dst, src)  (((dst) & ~0x1C) | (((UINT32)(src) << 2) & 0x1C))
#define   EVENT_COUNTER_ENABLE_NO_CHANGE  0x00
#define   EVENT_COUNTER_ENABLE_ALL_ON     0x07
#define ECCR_EVENT_COUNTER_CLEAR_SET(dst, src)  (((dst) & ~0x3) | (((UINT32)(src)) & 0x3))
#define   EVENT_COUNTER_CLEAR_NO_CHANGE  0x00
#define   EVENT_COUNTER_CLEAR_ALL_CLEAR  0x03

//
// Secondary PCI Express Capability
//
#define SPCIE_CAPABILITY_ID   0x0019
#define SPCIE_CAP_OFF_0C_REG  0x0C

// SPCIE_CAP_OFF_0C_REG
#define DSP_TX_PRESET0_SET(dst, src)  (((dst) & ~0xF) | (((UINT32) (src)) & 0xF))
#define DSP_TX_PRESET1_SET(dst, src)  (((dst) & ~0xF0000) | (((UINT32) (src) << 16) & 0xF0000))
#define DEFAULT_GEN3_PRESET  0x05

//
//  Physical Layer 16.0 GT/s Extended Capability
//
#define PL16G_CAPABILITY_ID    0x0026
#define PL16G_STATUS_REG       0x0C
#define PL16G_CAP_OFF_20H_REG  0x20

// PL16G_STATUS_REG
#define PL16G_STATUS_EQ_CPL_GET(val)     (val & 0x1)
#define PL16G_STATUS_EQ_CPL_P1_GET(val)  ((val & 0x2) >> 1)
#define PL16G_STATUS_EQ_CPL_P2_GET(val)  ((val & 0x4) >> 2)
#define PL16G_STATUS_EQ_CPL_P3_GET(val)  ((val & 0x8) >> 3)

// PL16G_CAP_OFF_20H_REG
#define DSP_16G_TX_PRESET0_SET(dst, src)    (((dst) & ~0xF) | (((UINT32) (src)) & 0xF))
#define DSP_16G_TX_PRESET1_SET(dst, src)    (((dst) & ~0xF00) | (((UINT32) (src) << 8) & 0xF00))
#define DSP_16G_TX_PRESET2_SET(dst, src)    (((dst) & ~0xF0000) | (((UINT32) (src) << 16) & 0xF0000))
#define DSP_16G_TX_PRESET3_SET(dst, src)    (((dst) & ~0xF000000) | (((UINT32) (src) << 24) & 0xF000000))
#define DSP_16G_RXTX_PRESET0_SET(dst, src)  (((dst) & ~0xFF) | (((UINT32) (src)) & 0xFF))
#define DSP_16G_RXTX_PRESET1_SET(dst, src)  (((dst) & ~0xFF00) | (((UINT32) (src) << 8) & 0xFF00))
#define DSP_16G_RXTX_PRESET2_SET(dst, src)  (((dst) & ~0xFF0000) | (((UINT32) (src) << 16) & 0xFF0000))
#define DSP_16G_RXTX_PRESET3_SET(dst, src)  (((dst) & ~0xFF000000) | (((UINT32) (src) << 24) & 0xFF000000))
#define   DEFAULT_GEN4_PRESET  0x57

//
// Port Logic
//
#define PORT_LINK_CTRL_OFF               0x710
#define FILTER_MASK_2_OFF                0x720
#define GEN2_CTRL_OFF                    0x80C
#define GEN3_RELATED_OFF                 0x890
#define GEN3_EQ_CONTROL_OFF              0x8A8
#define MISC_CONTROL_1_OFF               0x8BC
#define AMBA_ERROR_RESPONSE_DEFAULT_OFF  0x8D0
#define AMBA_LINK_TIMEOUT_OFF            0x8D4
#define AMBA_ORDERING_CTRL_OFF           0x8D8
#define DTIM_CTRL0_OFF                   0xAB0
#define AUX_CLK_FREQ_OFF                 0xB40
#define CCIX_CTRL_OFF                    0xC20

// PORT_LINK_CTRL_OFF
#define LINK_CAPABLE_SET(dst, src)  (((dst) & ~0x3F0000) | (((UINT32) (src) << 16) & 0x3F0000))
#define   LINK_CAPABLE_X1   0x1
#define   LINK_CAPABLE_X2   0x3
#define   LINK_CAPABLE_X4   0x7
#define   LINK_CAPABLE_X8   0xF
#define   LINK_CAPABLE_X16  0x1F
#define   LINK_CAPABLE_X32  0x3F
#define FAST_LINK_MODE_SET(dst, src)  (((dst) & ~0x80) | (((UINT32) (src) << 7) & 0x80))

// FILTER_MASK_2_OFF
#define CX_FLT_MASK_VENMSG0_DROP_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))
#define CX_FLT_MASK_VENMSG1_DROP_SET(dst, src)  (((dst) & ~0x2) | (((UINT32) (src) << 1) & 0x2))
#define CX_FLT_MASK_DABORT_4UCPL_SET(dst, src)  (((dst) & ~0x4) | (((UINT32) (src) << 2) & 0x4))

// GEN2_CTRL_OFF
#define NUM_OF_LANES_SET(dst, src)  (((dst) & ~0x1F00) | (((UINT32) (src) << 8) & 0x1F00))
#define   NUM_OF_LANES_X2   0x2
#define   NUM_OF_LANES_X4   0x4
#define   NUM_OF_LANES_X8   0x8
#define   NUM_OF_LANES_X16  0x10

// GEN3_RELATED_OFF
#define RATE_SHADOW_SEL_SET(dst, src)  (((dst) & ~0x3000000) | (((UINT32) (src) << 24) & 0x3000000))
#define   GEN3_DATA_RATE  0x00
#define   GEN4_DATA_RATE  0x01
#define EQ_PHASE_2_3_SET(dst, src)  (((dst) & ~0x200) | (((UINT32) (src) << 9) & 0x200))
#define   ENABLE_EQ_PHASE_2_3   0x00
#define   DISABLE_EQ_PHASE_2_3  0x01
#define RXEQ_REGRDLESS_SET(dst, src)  (((dst) & ~0x2000) | (((UINT32) (src) << 13) & 0x2000))
#define   ASSERT_RXEQ  0x01

// GEN3_EQ_CONTROL_OFF
#define GEN3_EQ_FB_MODE(dst, src)  (((dst) & ~0xF) | ((UINT32) (src) & 0xF))
#define   FOM_METHOD  0x01
#define GEN3_EQ_PRESET_VEC(dst, src)  (((dst) & 0xFF0000FF) | (((UINT32) (src) << 8) & 0xFFFF00))
#define   EQ_DEFAULT_PRESET_VECTOR  0x370
#define GEN3_EQ_INIT_EVAL(dst, src)  (((dst) & ~0x1000000) | (((UINT32) (src) << 24) & 0x1000000))
#define   INCLUDE_INIT_FOM  0x01

// MISC_CONTROL_1_OFF
#define DBI_RO_WR_EN_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))
#define   ENABLE_WR   0x01
#define   DISABLE_WR  0x00

// AMBA_ERROR_RESPONSE_DEFAULT_OFF
#define AMBA_ERROR_RESPONSE_CRS_SET(dst, src)     (((dst) & ~0x18) | (((UINT32) (src) << 3) & 0x18))
#define AMBA_ERROR_RESPONSE_GLOBAL_SET(dst, src)  (((dst) & ~0x1) | (((UINT32) (src)) & 0x1))

// AMBA_LINK_TIMEOUT_OFF
#define LINK_TIMEOUT_PERIOD_DEFAULT_SET(dst, src)  (((dst) & ~0xFF) | (((UINT32) (src)) & 0xFF))

// AMBA_ORDERING_CTRL_OFF
#define AX_MSTR_ZEROLREAD_FW_SET(dst, src)  (((dst) & ~0x80) | (((UINT32) (src) << 7) & 0x80))

// DTIM_CTRL0_OFF
#define DTIM_CTRL0_ROOT_PORT_ID_SET(dst, src)  (((dst) & ~0xFFFF) | (((UINT32) (src)) & 0xFFFF))

// AUX_CLK_FREQ_OFF
#define AUX_CLK_FREQ_SET(dst, src)  (((dst) & ~0x1FF) | (((UINT32) (src)) & 0x1FF))
#define   AUX_CLK_500MHZ  500

#endif /* AC01_PCIE_CORE_H_ */
