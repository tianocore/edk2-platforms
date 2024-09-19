/** @file

  The non-volatile parameter layout in SPI-NOR is shown below. There is
  two copies. The master copy is changeable by the user. The Last Known
  copy is handled by the fail safe future. It is a last know bootable copy.

   ---------------------------
   | Master Copy             | 16KB
   | Pre-boot parameters     |
   ---------------------------
   | Master Copy             | 16KB
   | Pre-boot parameters     |
   | w/o failsafe support    |
   ---------------------------
   | Master Copy             |
   | Manufactory &           | 32KB
   | Users parameters        |
   ---------------------------
   | Last Known Copy         | 16KB
   | Pre-boot parameters     |
   ---------------------------
   |                         | 16KB
   ---------------------------
   | Last Known Copy         |
   | Manufactory &           | 32KB
   | Users parameters        |
   ---------------------------

  As each non-volatile parameter requires 8 bytes, there is a total of 8K
  parameters.

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NVPARAMDEF_H_
#define NVPARAMDEF_H_

//
// SoC validation pre-boot non-volatile setting
//
// These parameters will reset to default value on failsafe.
// They are not used in production life cycle.
//
#define NV_PREBOOT_PARAM_START                    (0x000000)
#define NV_SI_PCP_VDMC                            ((1 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_FAILSAFE_RETRY                      ((2 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PPR_EN                          ((3 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RESERVED0                       ((4 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RESERVED1                       ((5 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_LOG_LEVEL                       ((6 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RESERVED2                       ((7 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RD_DBI_EN                       ((8 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WR_DBI_EN                       ((9 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RETRY_EN                        ((10 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_BANK_HASH_EN                    ((11 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RESERVED3                       ((12 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RCD_PARITY_EN                   ((13 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WRPATH_CLK_GATE_EN              ((14 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_IOCAL_MARGIN                    ((15 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RTR_S_MARGIN                    ((16 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RTR_L_MARGIN                    ((17 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RTR_CS_MARGIN                   ((18 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WTW_S_MARGIN                    ((19 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WTW_L_MARGIN                    ((20 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WTW_CS_MARGIN                   ((21 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RTW_S_MARGIN                    ((22 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RTW_L_MARGIN                    ((23 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RTW_CS_MARGIN                   ((24 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WTR_S_MARGIN                    ((25 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WTR_L_MARGIN                    ((26 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WTR_CS_MARGIN                   ((27 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PARITY_EN                       ((28 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_SLC_DISABLE                         ((29 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_SLC_SIZE                            ((30 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_SLC_SCRUB                           ((31 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CCIX_DISABLE                        ((32 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_ESM_RESERVED                        ((33 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_CAL_MODE                    ((34 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_RAS_TEST_EN                         ((35 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_EYE_SCREEN_TEST_EN              ((36 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_EYE_MASK_RD_MARGIN              ((37 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_EYE_MASK_WR_MARGIN              ((38 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RDODT_ON_MARGIN                 ((39 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RDODT_OFF_MARGIN                ((40 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WRODT_ON_MARGIN                 ((41 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WRODT_OFF_MARGIN                ((42 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_SLC_OCM_EN                          ((43 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_ESM_WIDTH                           ((44 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_TM2_DISABLE                         ((45 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CPUPLL_FREQ_MHZ                     ((46 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_ERR_INJECT_MASK_SK0             ((47 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_ERR_INJECT_MASK_SK1             ((48 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CXG_DISABLE_EARLY_COMPACK           ((49 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CXG_ENABLE_SAME_ADDR_COMP_ORDER     ((50 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_TURNAROUND_CONTROL              ((51 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_HIT_TURNAROUND_CONTROL          ((52 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_QOS_CLASS_CONTROL               ((53 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_ESCALATION_CONTROL              ((54 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_QV_CONTROL_31_00                ((55 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_QV_CONTROL_63_32                ((56 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_CREDIT_CONTROL                  ((57 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WRITE_PRIORITY_CONTROL_31_00    ((58 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_WRITE_PRIORITY_CONTROL_63_32    ((59 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_QUEUE_THRESHOLD_CONTROL_31_00   ((60 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_QUEUE_THRESHOLD_CONTROL_63_32   ((61 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_ATF_FAILURE_FAILSAFE                ((62 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_UEFI_FAILURE_FAILSAFE               ((63 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_STRIPE_DECODE                   ((64 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_DEBUG_CTRL                      ((65 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CXG_RA_DEVNR_ORD_WFC_DIS            ((66 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_DLL_TRACK_UPD_THRESHOLD     ((67 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_DLL_TRACK_UPD_THRESHOLD_AC  ((68 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_INIT_UPDATE_CONFIG          ((69 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_UPDATE_CONTROL              ((70 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_PROFILE_EN                          ((71 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_PCIE_PHY_SETTING                    ((72 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_CAL_THRESHOLD               ((73 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_PHY_CAL_INTERVAL_CNT            ((74 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_RESERVED                            ((75 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_S0_RHS_RCA_EN                       ((76 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_S1_RHS_RCA_EN                       ((77 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_2P_DPLL                             ((78 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_2P_ALI_CFG                          ((79 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_2P_ALI_CFG_LINK_RETRAIN             ((80 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_2P_ALI_CFG_CRC                      ((81 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RT_CONTROL_31_00                ((82 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_RT_CONTROL_63_32                ((83 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_DDR_TIMEOUT_CONTROL                 ((84 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CPU_LPI_FREQ_DISABLE                ((85 * 8) + NV_PREBOOT_PARAM_START)
#define NV_SI_CPU_LPI_FREQ_ENERGY_THRSHLD         ((86 * 8) + NV_PREBOOT_PARAM_START)
#define NV_PMPRO_REGION1_LOAD_START               (NV_SI_SLC_DISABLE)
#define NV_PMPRO_REGION1_LOAD_END                 (NV_SI_CPU_LPI_FREQ_ENERGY_THRSHLD)
//
// NOTE: Add before NV_PREBOOT_PARAM_MAX and increase its value
//
#define NV_PREBOOT_PARAM_MAX  ((86 * 8) + NV_PREBOOT_PARAM_START)

//
// Manufactory non-volatile memory
//
// These parameters will reset to default value on failsafe.
//
#define NV_MANU_PARAM_START                      (0x004000)
#define NV_SI_DDR_VMARGIN                        ((0 * 8) + NV_MANU_PARAM_START)
#define NV_PMPRO_REGION2_LOAD_START              (NV_SI_DDR_VMARGIN)
#define NV_SI_SOC_VMARGIN                        ((1 * 8) + NV_MANU_PARAM_START)
#define NV_SI_AVS_VMARGIN                        ((2 * 8) + NV_MANU_PARAM_START)
#define NV_SI_TPC_TM1_MARGIN                     ((3 * 8) + NV_MANU_PARAM_START)
#define NV_SI_TPC_TM2_MARGIN                     ((4 * 8) + NV_MANU_PARAM_START)
#define NV_SI_TPC_FREQ_THROTTLE                  ((5 * 8) + NV_MANU_PARAM_START)
#define NV_SI_T_LTLM_EN                          ((6 * 8) + NV_MANU_PARAM_START)
#define NV_SI_T_LTLM_THRSHLD                     ((7 * 8) + NV_MANU_PARAM_START)
#define NV_SI_T_GTLM_THRSHLD                     ((8 * 8) + NV_MANU_PARAM_START)
#define NV_SI_P_LM_EN                            ((9 * 8) + NV_MANU_PARAM_START)
#define NV_SI_P_LM_THRSHLD                       ((10 * 8) + NV_MANU_PARAM_START)
#define NV_SI_TPC_OVERTEMP_ISR_DISABLE           ((11 * 8) + NV_MANU_PARAM_START)
#define NV_SI_VPP_VMARGIN                        ((12 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PMPRO_FAILURE_FAILSAFE             ((13 * 8) + NV_MANU_PARAM_START)
#define NV_SI_FAILSAFE_DISABLE                   ((14 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLIMIT_APM_DS_PERCENTAGE           ((15 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLIMIT_APM_EP_MS                   ((16 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLIMIT_APM_PM1_PERCENTAGE_TDP      ((17 * 8) + NV_MANU_PARAM_START)
#define NV_SI_CPU_LPI_RESERVED0                  ((18 * 8) + NV_MANU_PARAM_START)
#define NV_SI_CPU_LPI_RESERVED1                  ((19 * 8) + NV_MANU_PARAM_START)
#define NV_SI_CCIX_OPT_CONFIG                    ((20 * 8) + NV_MANU_PARAM_START)
#define NV_SI_MESH_FREQ_MARGIN                   ((21 * 8) + NV_MANU_PARAM_START)
#define NV_SI_MESH_TURBO_EN                      ((22 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PWR_HEADROOM_WATT                  ((23 * 8) + NV_MANU_PARAM_START)
#define NV_SI_EXTRA_PCP_VOLT_MV                  ((24 * 8) + NV_MANU_PARAM_START)
#define NV_SI_CPU_LPI_HYST_CNT                   ((25 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DVFS_VOLT_INC_STEP_MV              ((26 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DVFS_VOLT_DEC_STEP_MV              ((27 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLIMIT_APM_TEMP_THLD               ((28 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLIMIT_APM_EN                      ((29 * 8) + NV_MANU_PARAM_START)
#define NV_SI_VDM_EN                             ((30 * 8) + NV_MANU_PARAM_START)
#define NV_SI_VDM_VMARGIN_MV                     ((31 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_EN                             ((32 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_SOCKET                         ((33 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_MCU_MASK                       ((34 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_RANK_MASK                      ((35 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_SLICE_MASK                     ((36 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_BIT_MASK                       ((37 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_X_PARAM                        ((38 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_Y_PARAM                        ((39 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_X_LEFT                         ((40 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_X_RIGHT                        ((41 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_X_STEP                         ((42 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_Y_BOTTOM                       ((43 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_Y_TOP                          ((44 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_Y_STEP                         ((45 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_START_ADDR_LO                  ((46 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_START_ADDR_UP                  ((47 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_SIZE                           ((48 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_THREAD_CNT                     ((49 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_SCREEN                         ((50 * 8) + NV_MANU_PARAM_START)
#define NV_SI_PLT_RSVD                           ((51 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DVFS_VOLT_CHANGE_BY_STEP_EN        ((52 * 8) + NV_MANU_PARAM_START)
#define NS_SI_DVFS_TCAL_F_LIMIT                  ((53 * 8) + NV_MANU_PARAM_START)
#define NS_SI_DVFS_TCAL_T_LIMIT                  ((54 * 8) + NV_MANU_PARAM_START)
#define NV_SI_CCIX_DIAG_CTRL1                    ((55 * 8) + NV_MANU_PARAM_START)
#define NV_SI_CCIX_DIAG_CTRL2                    ((56 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_EN                        ((57 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_DIMM_LOW_TEMP_THRESHOLD   ((58 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_DIMM_HIGH_TEMP_THRESHOLD  ((59 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_MCU_LOW_TEMP_THRESHOLD    ((60 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_MCU_HIGH_TEMP_THRESHOLD   ((61 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_LOW_TEMP_VOLT_OFF_MV      ((62 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_PERIOD_SEC                ((63 * 8) + NV_MANU_PARAM_START)
#define NV_SI_DDR_TCAL_SOC_VOLT_CAP_MV           ((64 * 8) + NV_MANU_PARAM_START)
#define NV_SI_ALTRAMAX_ICCMAX_EN                 ((65 * 8) + NV_MANU_PARAM_START)
#define NV_SI_MESH_TURBO_ACTIVITY_THRESHOLD      ((66 * 8) + NV_MANU_PARAM_START)
#define NV_SI_ALTRAMAX_ICCMAX_OC_LIMIT_MARGIN    ((67 * 8) + NV_MANU_PARAM_START)
#define NV_SI_SLT_VREF_EXT_PROG_EN               ((68 * 8) + NV_MANU_PARAM_START)
#define NV_SI_SLT_FLAGS                          ((69 * 8) + NV_MANU_PARAM_START)
#define NV_PMPRO_REGION2_LOAD_END                (NV_SI_SLT_FLAGS)
//
// NOTE: Add before NV_MANU_PARAM_MAX and increase its value
//
#define NV_MANU_PARAM_MAX  ((66 * 8) + NV_MANU_PARAM_START)

//
// User non-volatile memory
//
// These parameters will reset to default value on failsafe.
//
#define NV_USER_PARAM_START                      (0x008000)
#define NV_SI_S0_PCP_ACTIVECPM_0_31              ((0 * 8) + NV_USER_PARAM_START)
#define NV_SI_S0_PCP_ACTIVECPM_32_63             ((1 * 8) + NV_USER_PARAM_START)
#define NV_SI_S1_PCP_ACTIVECPM_0_31              ((2 * 8) + NV_USER_PARAM_START)
#define NV_SI_S1_PCP_ACTIVECPM_32_63             ((3 * 8) + NV_USER_PARAM_START)
#define NV_SI_WDT_BIOS_EXP_MINS                  ((4 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_CE_RAS_THRESHOLD               ((5 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_CE_RAS_INTERVAL                ((6 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_SPEED                          ((7 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_SCRUB_EN                       ((8 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_ECC_MODE                       ((9 * 8) + NV_USER_PARAM_START)
#define NV_SI_S0_RCA_PCI_DEVMAP                  ((10 * 8) + NV_USER_PARAM_START)
#define NV_SI_S0_RCB_PCI_DEVMAP                  ((11 * 8) + NV_USER_PARAM_START)
#define NV_SI_S1_RCA_PCI_DEVMAP                  ((12 * 8) + NV_USER_PARAM_START)
#define NV_SI_S1_RCB_PCI_DEVMAP                  ((13 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_ERRCTRL                        ((14 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_REFRESH_GRANULARITY            ((15 * 8) + NV_USER_PARAM_START)
#define NV_SI_SUBNUMA_MODE                       ((16 * 8) + NV_USER_PARAM_START)
#define NV_SI_ERRATUM_1542419_WA                 ((17 * 8) + NV_USER_PARAM_START)
#define NV_SI_NEAR_ATOMIC_DISABLE                ((18 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_SLAVE_32BIT_MEM_EN             ((19 * 8) + NV_USER_PARAM_START)
#define NV_SI_CPUECTLR_EL1_0_31                  ((20 * 8) + NV_USER_PARAM_START)
#define NV_SI_CPUECTLR_EL1_32_63                 ((21 * 8) + NV_USER_PARAM_START)
#define NV_SI_HARDWARE_EINJ                      ((22 * 8) + NV_USER_PARAM_START)
#define NV_SI_2P_CE_RAS_THRESHOLD                ((23 * 8) + NV_USER_PARAM_START)
#define NV_SI_2P_CE_RAS_INTERVAL                 ((24 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_BERT_ENABLED                   ((25 * 8) + NV_USER_PARAM_START)
#define NV_SI_HNF_AUX_CTL_0_31                   ((26 * 8) + NV_USER_PARAM_START)
#define NV_SI_HNF_AUX_CTL_32_63                  ((27 * 8) + NV_USER_PARAM_START)
#define NV_SI_CPM_CE_RAS_THRESHOLD               ((28 * 8) + NV_USER_PARAM_START)
#define NV_SI_CPM_CE_RAS_INTERVAL                ((29 * 8) + NV_USER_PARAM_START)
#define NV_SI_HNF_AUX_CTL_0_31_WR_EN_MASK        ((30 * 8) + NV_USER_PARAM_START)
#define NV_SI_HNF_AUX_CTL_32_63_WR_EN_MASK       ((31 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_WR_BACK_EN                     ((32 * 8) + NV_USER_PARAM_START)
#define NV_SI_CPUECTLR_EL1_0_31_WR_EN_MASK       ((33 * 8) + NV_USER_PARAM_START)
#define NV_SI_CPUECTLR_EL1_32_63_WR_EN_MASK      ((34 * 8) + NV_USER_PARAM_START)
#define NV_SI_LINK_ERR_THRESHOLD                 ((35 * 8) + NV_USER_PARAM_START)
#define NV_SI_SEC_WDT_BIOS_EXP_MINS              ((36 * 8) + NV_USER_PARAM_START)
#define NV_SI_NVDIMM_MODE                        ((37 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_SDEI_ENABLED                   ((38 * 8) + NV_USER_PARAM_START)
#define NV_SI_NVDIMM_PROV_MASK_S0                ((39 * 8) + NV_USER_PARAM_START)
#define NV_SI_NVDIMM_PROV_MASK_S1                ((40 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_ZQCS_EN                        ((41 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_CRC_MODE                       ((42 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXG_RA_AUX_CTL_0_31                ((43 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXG_RA_AUX_CTL_32_63               ((44 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXG_RA_AUX_CTL_0_31_WR_EN_MASK     ((45 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXG_RA_AUX_CTL_32_63_WR_EN_MASK    ((46 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXLA_AUX_CTL_0_31                  ((47 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXLA_AUX_CTL_32_63                 ((48 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXLA_AUX_CTL_0_31_WR_EN_MASK       ((49 * 8) + NV_USER_PARAM_START)
#define NV_SI_CXLA_AUX_CTL_32_63_WR_EN_MASK      ((50 * 8) + NV_USER_PARAM_START)
#define NV_SI_DDR_LOW_POWER_CFG                  ((51 * 8) + NV_USER_PARAM_START)
#define NV_SI_ALERT_DIMM_SHUTDOWN_EN             ((52 * 8) + NV_USER_PARAM_START)
#define NV_SI_DFS_EN                             ((53 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_PCIE_AER_FW_FIRST              ((54 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_DRAM_EINJ_NOTRIGGER            ((55 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_AEST_PROC_EN                   ((56 * 8) + NV_USER_PARAM_START)
#define NV_SI_MESH_S0_CXG_RC_STRONG_ORDERING_EN  ((57 * 8) + NV_USER_PARAM_START)
#define NV_SI_MESH_S1_CXG_RC_STRONG_ORDERING_EN  ((58 * 8) + NV_USER_PARAM_START)
#define NV_SI_2P_RESERVED0                       ((59 * 8) + NV_USER_PARAM_START)
#define NV_SI_2P_RESERVED1                       ((60 * 8) + NV_USER_PARAM_START)
#define NV_SI_2P_RESERVED2                       ((61 * 8) + NV_USER_PARAM_START)
#define NV_SI_HCR_EL2_CTL_LOW                    ((62 * 8) + NV_USER_PARAM_START)
#define NV_SI_HCR_EL2_CTL_HIGH                   ((63 * 8) + NV_USER_PARAM_START)
#define NV_SI_ESM_SPEED                          ((64 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_DDR_CE_TH1                     ((65 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_DDR_CE_TH2                     ((66 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_DDR_CE_WINDOW1                 ((67 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_DDR_CE_WINDOW2                 ((68 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_2P_CE_FILTER_THRESHOLD         ((69 * 8) + NV_USER_PARAM_START)
#define NV_SI_RAS_2P_CE_FILTER_WINDOW            ((70 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC0_AER_CE_THRESHOLD       ((71 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC0_AER_CE_INTERVAL        ((72 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC1_AER_CE_THRESHOLD       ((73 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC1_AER_CE_INTERVAL        ((74 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC2_AER_CE_THRESHOLD       ((75 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC2_AER_CE_INTERVAL        ((76 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC3_AER_CE_THRESHOLD       ((77 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC3_AER_CE_INTERVAL        ((78 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC4_AER_CE_THRESHOLD       ((79 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC4_AER_CE_INTERVAL        ((80 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC5_AER_CE_THRESHOLD       ((81 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC5_AER_CE_INTERVAL        ((82 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC6_AER_CE_THRESHOLD       ((83 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC6_AER_CE_INTERVAL        ((84 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC7_AER_CE_THRESHOLD       ((85 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S0_RC7_AER_CE_INTERVAL        ((86 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC0_AER_CE_THRESHOLD       ((87 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC0_AER_CE_INTERVAL        ((88 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC1_AER_CE_THRESHOLD       ((89 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC1_AER_CE_INTERVAL        ((90 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC2_AER_CE_THRESHOLD       ((91 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC2_AER_CE_INTERVAL        ((92 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC3_AER_CE_THRESHOLD       ((93 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC3_AER_CE_INTERVAL        ((94 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC4_AER_CE_THRESHOLD       ((95 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC4_AER_CE_INTERVAL        ((96 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC5_AER_CE_THRESHOLD       ((97 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC5_AER_CE_INTERVAL        ((98 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC6_AER_CE_THRESHOLD       ((99 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC6_AER_CE_INTERVAL        ((100 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC7_AER_CE_THRESHOLD       ((101 * 8) + NV_USER_PARAM_START)
#define NV_SI_PCIE_S1_RC7_AER_CE_INTERVAL        ((102 * 8) + NV_USER_PARAM_START)
//
// NOTE: Add before NV_USER_PARAM_MAX and increase its value
//
#define NV_USER_PARAM_MAX            (NV_SI_PCIE_S1_RC7_AER_CE_INTERVAL)
#define NV_PMPRO_REGION3_LOAD_START  (NV_USER_PARAM_START)
#define NV_PMPRO_REGION3_LOAD_END    (NV_USER_PARAM_MAX)

//
// Non-volatile board read-only setting
//
// These parameters do not support failsafe and will always read
// from its location. Please note that the physical base address
// location for board setting is not the same as above region. This
// allows packaging these board setting along with the firmware
// image itself. See SPI-NOR flash layout design for more info.
//
// Please note that script will parse these and generate
// board setting. The keyword "Default: " is used to provide
// the default value.
//
#define NV_BOARD_PARAM_START                  (0x00C000)
#define NV_SI_RO_BOARD_VENDOR                 ((0 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x0000CD3A - Follow BMC FRU format */
#define NV_PMPRO_REGION4_LOAD_START           (NV_SI_RO_BOARD_VENDOR)
#define NV_SI_RO_BOARD_TYPE                   ((1 * 8) + NV_BOARD_PARAM_START)      /* Default: 0x00000000 - Follow BMC FRU format */
#define NV_SI_RO_BOARD_REV                    ((2 * 8) + NV_BOARD_PARAM_START)      /* Default: 0x00000000 Follow BMC FRU format */
#define NV_SI_RO_BOARD_CFG                    ((3 * 8) + NV_BOARD_PARAM_START)      /* Default: 0x00000000 Follow BMC FRU format */
#define NV_SI_RO_BOARD_S0_DIMM_AVAIL          ((4 * 8) + NV_BOARD_PARAM_START)      /* Default: 0x0000FFFF */
#define NV_SI_RO_BOARD_S1_DIMM_AVAIL          ((5 * 8) + NV_BOARD_PARAM_START)      /* Default: 0x0000FFFF */
#define NV_SI_RO_BOARD_SPI0CS0_FREQ_KHZ       ((6 * 8) + NV_BOARD_PARAM_START)      /* Default: 33000 */
#define NV_SI_RO_BOARD_SPI0CS1_FREQ_KHZ       ((7 * 8) + NV_BOARD_PARAM_START)      /* Default: 33000 */
#define NV_SI_RO_BOARD_SPI1CS0_FREQ_KHZ       ((8 * 8) + NV_BOARD_PARAM_START)      /* Default: 10000 */
#define NV_SI_RO_BOARD_SPI1CS1_FREQ_KHZ       ((9 * 8) + NV_BOARD_PARAM_START)      /* Default: 10000 */
#define NV_SI_RO_BOARD_TPM_LOC                ((10 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_I2C0_FREQ_KHZ          ((11 * 8) + NV_BOARD_PARAM_START)     /* Default: 400 */
#define NV_SI_RO_BOARD_I2C1_FREQ_KHZ          ((12 * 8) + NV_BOARD_PARAM_START)     /* Default: 400 */
#define NV_SI_RO_BOARD_I2C2_10_FREQ_KHZ       ((13 * 8) + NV_BOARD_PARAM_START)     /* Default: 400 */
#define NV_SI_RO_BOARD_I2C3_FREQ_KHZ          ((14 * 8) + NV_BOARD_PARAM_START)     /* Default: 400 */
#define NV_SI_RO_BOARD_I2C9_FREQ_KHZ          ((15 * 8) + NV_BOARD_PARAM_START)     /* Default: 400 */
#define NV_SI_RO_BOARD_2P_CFG                 ((16 * 8) + NV_BOARD_PARAM_START)     /* Default: 0xFFFFFF01 */
#define NV_SI_RO_BOARD_S0_RCA0_CFG            ((17 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA1_CFG            ((18 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA2_CFG            ((19 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000004 */
#define NV_SI_RO_BOARD_S0_RCA3_CFG            ((20 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000004 */
#define NV_SI_RO_BOARD_S0_RCB0_LO_CFG         ((21 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S0_RCB0_HI_CFG         ((22 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S0_RCB1_LO_CFG         ((23 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S0_RCB1_HI_CFG         ((24 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S0_RCB2_LO_CFG         ((25 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S0_RCB2_HI_CFG         ((26 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000003 */
#define NV_SI_RO_BOARD_S0_RCB3_LO_CFG         ((27 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000003 */
#define NV_SI_RO_BOARD_S0_RCB3_HI_CFG         ((28 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S1_RCA0_CFG            ((29 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA1_CFG            ((30 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA2_CFG            ((31 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S1_RCA3_CFG            ((32 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00030003 */
#define NV_SI_RO_BOARD_S1_RCB0_LO_CFG         ((33 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000003 */
#define NV_SI_RO_BOARD_S1_RCB0_HI_CFG         ((34 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S1_RCB1_LO_CFG         ((35 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S1_RCB1_HI_CFG         ((36 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000003 */
#define NV_SI_RO_BOARD_S1_RCB2_LO_CFG         ((37 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S1_RCB2_HI_CFG         ((38 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S1_RCB3_LO_CFG         ((39 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_S1_RCB3_HI_CFG         ((40 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00020002 */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_P0        ((41 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000001 */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_P1        ((42 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000002 */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_P2        ((43 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000003 */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_P3        ((44 * 8) + NV_BOARD_PARAM_START)     /* Default: 0x00000004 */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_M1        ((45 * 8) + NV_BOARD_PARAM_START)     /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_M2        ((46 * 8) + NV_BOARD_PARAM_START)     /* Default: 0xFFFFFFFE */
#define NV_SI_RO_BOARD_T_LTLM_DELTA_M3        ((47 * 8) + NV_BOARD_PARAM_START)     /* Default: 0xFFFFFFFD */
#define NV_SI_RO_BOARD_P_LM_PID_P             ((48 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_P_LM_PID_I             ((49 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_P_LM_PID_I_L_THOLD     ((50 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_P_LM_PID_I_H_THOLD     ((51 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_P_LM_PID_D             ((52 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_P_LM_EXP_SMOOTH_CONST  ((53 * 8) + NV_BOARD_PARAM_START)
//
// NV_SI_RO_BOARD_TPM_ALG_ID: 0=Default to SHA256, 1=SHA1, 2=SHA256
// Any other value will lead to default digest.
//
#define NV_SI_RO_BOARD_TPM_ALG_ID                         ((54 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000002 */
#define NV_SI_RO_BOARD_DDR_SPEED_GRADE                    ((55 * 8) + NV_BOARD_PARAM_START) /* Default: 3200 */
#define NV_SI_RO_BOARD_DDR_S0_RTT_WR                      ((56 * 8) + NV_BOARD_PARAM_START) /* Default: 0x20020000 */
#define NV_SI_RO_BOARD_DDR_S1_RTT_WR                      ((57 * 8) + NV_BOARD_PARAM_START) /* Default: 0x20020000 */
#define NV_SI_RO_BOARD_DDR_S0_RTT_NOM                     ((58 * 8) + NV_BOARD_PARAM_START) /* Default: 0x31060177 */
#define NV_SI_RO_BOARD_DDR_S1_RTT_NOM                     ((59 * 8) + NV_BOARD_PARAM_START) /* Default: 0x31060177 */
#define NV_SI_RO_BOARD_DDR_S0_RTT_PARK                    ((60 * 8) + NV_BOARD_PARAM_START) /* Default: 0x30060070 */
#define NV_SI_RO_BOARD_DDR_S1_RTT_PARK                    ((61 * 8) + NV_BOARD_PARAM_START) /* Default: 0x30060070 */
#define NV_SI_RO_BOARD_DDR_CS0_RDODT_MASK_1DPC            ((62 * 8) + NV_BOARD_PARAM_START) /* Default: 0x000000 */
#define NV_SI_RO_BOARD_DDR_CS1_RDODT_MASK_1DPC            ((63 * 8) + NV_BOARD_PARAM_START) /* Default: 0x000000 */
#define NV_SI_RO_BOARD_DDR_CS2_RDODT_MASK_1DPC            ((64 * 8) + NV_BOARD_PARAM_START) /* Default: 0x000000 */
#define NV_SI_RO_BOARD_DDR_CS3_RDODT_MASK_1DPC            ((65 * 8) + NV_BOARD_PARAM_START) /* Default: 0x000000 */
#define NV_SI_RO_BOARD_DDR_CS0_RDODT_MASK_2DPC            ((66 * 8) + NV_BOARD_PARAM_START) /* Default: 0x044C0CCC */
#define NV_SI_RO_BOARD_DDR_CS1_RDODT_MASK_2DPC            ((67 * 8) + NV_BOARD_PARAM_START) /* Default: 0x084C0CCC */
#define NV_SI_RO_BOARD_DDR_CS2_RDODT_MASK_2DPC            ((68 * 8) + NV_BOARD_PARAM_START) /* Default: 0x04130333 */
#define NV_SI_RO_BOARD_DDR_CS3_RDODT_MASK_2DPC            ((69 * 8) + NV_BOARD_PARAM_START) /* Default: 0x08130333 */
#define NV_SI_RO_BOARD_DDR_CS0_WRODT_MASK_1DPC            ((70 * 8) + NV_BOARD_PARAM_START) /* Default: 0x01130333 */
#define NV_SI_RO_BOARD_DDR_CS1_WRODT_MASK_1DPC            ((71 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02230333 */
#define NV_SI_RO_BOARD_DDR_CS2_WRODT_MASK_1DPC            ((72 * 8) + NV_BOARD_PARAM_START) /* Default: 0x01430333 */
#define NV_SI_RO_BOARD_DDR_CS3_WRODT_MASK_1DPC            ((73 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02830333 */
#define NV_SI_RO_BOARD_DDR_CS0_WRODT_MASK_2DPC            ((74 * 8) + NV_BOARD_PARAM_START) /* Default: 0x055EDEED */
#define NV_SI_RO_BOARD_DDR_CS1_WRODT_MASK_2DPC            ((75 * 8) + NV_BOARD_PARAM_START) /* Default: 0x0A5DEDDE */
#define NV_SI_RO_BOARD_DDR_CS2_WRODT_MASK_2DPC            ((76 * 8) + NV_BOARD_PARAM_START) /* Default: 0x055B7BB7 */
#define NV_SI_RO_BOARD_DDR_CS3_WRODT_MASK_2DPC            ((77 * 8) + NV_BOARD_PARAM_START) /* Default: 0x0A57B77B */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQ_CTRL_1DPC          ((78 * 8) + NV_BOARD_PARAM_START) /* Default: 0x5 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQ_VAL_1DPC           ((79 * 8) + NV_BOARD_PARAM_START) /* Default: 0x90DD90 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQS_CTRL_1DPC         ((80 * 8) + NV_BOARD_PARAM_START) /* Default: 0x5 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQS_VAL_1DPC          ((81 * 8) + NV_BOARD_PARAM_START) /* Default: 0x90DD90 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQ_CTRL_2DPC          ((82 * 8) + NV_BOARD_PARAM_START) /* Default: 0x5 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQ_VAL_2DPC           ((83 * 8) + NV_BOARD_PARAM_START) /* Default: 0x90DD90 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQS_CTRL_2DPC         ((84 * 8) + NV_BOARD_PARAM_START) /* Default: 0x5 */
#define NV_SI_RO_BOARD_DDR_PHY_TERM_DQS_VAL_2DPC          ((85 * 8) + NV_BOARD_PARAM_START) /* Default: 0x90DD90 */
#define NV_SI_RO_BOARD_DDR_PHY_VREFDQ_RANGE_VAL_1DPC      ((86 * 8) + NV_BOARD_PARAM_START) /* Default: 0x24 */
#define NV_SI_RO_BOARD_DDR_DRAM_VREFDQ_RANGE_VAL_1DPC     ((87 * 8) + NV_BOARD_PARAM_START) /* Default: 0x001A001A */
#define NV_SI_RO_BOARD_DDR_PHY_VREFDQ_RANGE_VAL_2DPC      ((88 * 8) + NV_BOARD_PARAM_START) /* Default: 0x50 */
#define NV_SI_RO_BOARD_DDR_DRAM_VREFDQ_RANGE_VAL_2DPC     ((89 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00240020 */
#define NV_SI_RO_BOARD_DDR_CLK_WRDQ_DLY_DEFAULT           ((90 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02800280 */
#define NV_SI_RO_BOARD_DDR_RDDQS_DQ_DLY_DEFAULT           ((91 * 8) + NV_BOARD_PARAM_START) /* Default: 0x90909090 */
#define NV_SI_RO_BOARD_DDR_WRDQS_SHIFT_DEFAULT            ((92 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_DDR_ADCMD_DLY_DEFAULT              ((93 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00C000C0 */
#define NV_SI_RO_BOARD_DDR_CLK_WRDQ_DLY_ADJ               ((94 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_DDR_RDDQS_DQ_DLY_ADJ               ((95 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_DDR_PHY_VREF_ADJ                   ((96 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_DDR_DRAM_VREF_ADJ                  ((97 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_DDR_WR_PREAMBLE_CYCLE              ((98 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02010201 */
#define NV_SI_RO_BOARD_DDR_ADCMD_2T_MODE                  ((99 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_I2C_VRD_CONFIG_INFO                ((100 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_DDR_PHY_FEATURE_CTRL               ((101 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_BMC_HANDSHAKE_SPI_ACCESS           ((102 * 8) + NV_BOARD_PARAM_START) /* Default: 0x01050106 */
#define NV_SI_RO_BOARD_DIMM_TEMP_THRESHOLD                ((103 * 8) + NV_BOARD_PARAM_START) /* Default: 0x5F4 */
#define NV_SI_RO_BOARD_DIMM_SPD_COMPARE_DISABLE           ((104 * 8) + NV_BOARD_PARAM_START) /* Default: 0x0 */
#define NV_SI_RO_BOARD_S0_PCIE_CLK_CFG                    ((105 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA4_CFG                        ((106 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S0_RCA5_CFG                        ((107 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S0_RCA6_CFG                        ((108 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S0_RCA7_CFG                        ((109 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020003 */
#define NV_SI_RO_BOARD_S0_RCA0_TXRX_G3PRESET              ((110 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA1_TXRX_G3PRESET              ((111 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA2_TXRX_G3PRESET              ((112 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA3_TXRX_G3PRESET              ((113 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB0A_TXRX_G3PRESET             ((114 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB0B_TXRX_G3PRESET             ((115 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB1A_TXRX_G3PRESET             ((116 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB1B_TXRX_G3PRESET             ((117 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB2A_TXRX_G3PRESET             ((118 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB2B_TXRX_G3PRESET             ((119 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB3A_TXRX_G3PRESET             ((120 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCB3B_TXRX_G3PRESET             ((121 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA4_TXRX_G3PRESET              ((122 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA5_TXRX_G3PRESET              ((123 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA6_TXRX_G3PRESET              ((124 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA7_TXRX_G3PRESET              ((125 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S0_RCA0_TXRX_G4PRESET              ((126 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA1_TXRX_G4PRESET              ((127 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA2_TXRX_G4PRESET              ((128 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA3_TXRX_G4PRESET              ((129 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB0A_TXRX_G4PRESET             ((130 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB0B_TXRX_G4PRESET             ((131 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB1A_TXRX_G4PRESET             ((132 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB1B_TXRX_G4PRESET             ((133 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB2A_TXRX_G4PRESET             ((134 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB2B_TXRX_G4PRESET             ((135 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB3A_TXRX_G4PRESET             ((136 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCB3B_TXRX_G4PRESET             ((137 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA4_TXRX_G4PRESET              ((138 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA5_TXRX_G4PRESET              ((139 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA6_TXRX_G4PRESET              ((140 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S0_RCA7_TXRX_G4PRESET              ((141 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_PCIE_CLK_CFG                    ((142 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA4_CFG                        ((143 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S1_RCA5_CFG                        ((144 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S1_RCA6_CFG                        ((145 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020202 */
#define NV_SI_RO_BOARD_S1_RCA7_CFG                        ((146 * 8) + NV_BOARD_PARAM_START) /* Default: 0x02020003 */
#define NV_SI_RO_BOARD_S1_RCA2_TXRX_G3PRESET              ((147 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA3_TXRX_G3PRESET              ((148 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB0A_TXRX_G3PRESET             ((149 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB0B_TXRX_G3PRESET             ((150 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB1A_TXRX_G3PRESET             ((151 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB1B_TXRX_G3PRESET             ((152 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB2A_TXRX_G3PRESET             ((153 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB2B_TXRX_G3PRESET             ((154 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB3A_TXRX_G3PRESET             ((155 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCB3B_TXRX_G3PRESET             ((156 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA4_TXRX_G3PRESET              ((157 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA5_TXRX_G3PRESET              ((158 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA6_TXRX_G3PRESET              ((159 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA7_TXRX_G3PRESET              ((160 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_S1_RCA2_TXRX_G4PRESET              ((161 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCA3_TXRX_G4PRESET              ((162 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB0A_TXRX_G4PRESET             ((163 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB0B_TXRX_G4PRESET             ((164 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB1A_TXRX_G4PRESET             ((165 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB1B_TXRX_G4PRESET             ((166 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB2A_TXRX_G4PRESET             ((167 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB2B_TXRX_G4PRESET             ((168 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB3A_TXRX_G4PRESET             ((169 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCB3B_TXRX_G4PRESET             ((170 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCA4_TXRX_G4PRESET              ((171 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCA5_TXRX_G4PRESET              ((172 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCA6_TXRX_G4PRESET              ((173 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_S1_RCA7_TXRX_G4PRESET              ((174 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_2P_CE_MASK_THRESHOLD               ((175 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000003 */
#define NV_SI_RO_BOARD_2P_CE_MASK_INTERVAL                ((176 * 8) + NV_BOARD_PARAM_START) /* Default: 0x000001A4 */
#define NV_SI_RO_BOARD_SX_PHY_CFG_SETTING                 ((177 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_DDR_PHY_DC_CLK                     ((178 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00018000 */
#define NV_SI_RO_BOARD_DDR_PHY_DC_DATA                    ((179 * 8) + NV_BOARD_PARAM_START) /* Default: 0x80018000 */
#define NV_SI_RO_BOARD_SX_RCA0_TXRX_20GPRESET             ((180 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA1_TXRX_20GPRESET             ((181 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA2_TXRX_20GPRESET             ((182 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA3_TXRX_20GPRESET             ((183 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA0_TXRX_25GPRESET             ((184 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA1_TXRX_25GPRESET             ((185 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA2_TXRX_25GPRESET             ((186 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_SX_RCA3_TXRX_25GPRESET             ((187 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_DDR_2X_REFRESH_TEMP_THRESHOLD      ((188 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00550055 */
#define NV_SI_RO_BOARD_PCP_VRD_VOUT_WAIT_US               ((189 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000064 */
#define NV_SI_RO_BOARD_PCP_VRD_VOUT_RESOLUTION_MV         ((190 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000005 */
#define NV_SI_RO_BOARD_DVFS_VOLT_READ_BACK_EN             ((191 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000001 */
#define NV_SI_RO_BOARD_DVFS_VOLT_READ_BACK_TIME           ((192 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000002 */
#define NV_SI_RO_BOARD_DVFS_VOUT_20MV_RAMP_TIME_US        ((193 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000005 */
#define NV_SI_RO_BOARD_PCIE_AER_FW_FIRST                  ((194 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_RTC_GPI_LOCK_BYPASS                ((195 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_TPM_DISABLE                        ((196 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_MESH_S0_CXG_RC_STRONG_ORDERING_EN  ((197 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_MESH_S1_CXG_RC_STRONG_ORDERING_EN  ((198 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_GPIO_SW_WATCHDOG_EN                ((199 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_PCIE_HP_DISABLE                    ((200 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_I2C_VRD_VOUT_FORMAT                ((201 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_I2C_VRD_SMBUS_CMD_FLAGS            ((202 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_CUST_SPM_LOCATION                  ((203 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_RAS_DDR_CE_WINDOW                  ((204 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_RAS_DDR_CE_TH1                     ((205 * 8) + NV_BOARD_PARAM_START) /* Default: 0x000001F4 */
#define NV_SI_RO_BOARD_RAS_DDR_CE_TH2                     ((206 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00001388 */
#define NV_SI_RO_BOARD_RAS_DDR_CE_THC                     ((207 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_MQ_SX_RCA0_TXRX_20GPRESET          ((208 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_MQ_SX_RCA1_TXRX_20GPRESET          ((209 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_MQ_SX_RCA0_TXRX_25GPRESET          ((210 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_MQ_SX_RCA1_TXRX_25GPRESET          ((211 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_MQ_S0_RCA0_TXRX_G3PRESET           ((212 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA1_TXRX_G3PRESET           ((213 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA2_TXRX_G3PRESET           ((214 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA3_TXRX_G3PRESET           ((215 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA4_TXRX_G3PRESET           ((216 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA5_TXRX_G3PRESET           ((217 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA6_TXRX_G3PRESET           ((218 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA7_TXRX_G3PRESET           ((219 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S1_RCA2_TXRX_G3PRESET           ((220 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S1_RCA3_TXRX_G3PRESET           ((221 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S1_RCA4_TXRX_G3PRESET           ((222 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S1_RCA5_TXRX_G3PRESET           ((223 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S1_RCA6_TXRX_G3PRESET           ((224 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S1_RCA7_TXRX_G3PRESET           ((225 * 8) + NV_BOARD_PARAM_START) /* Default: 0xFFFFFFFF */
#define NV_SI_RO_BOARD_MQ_S0_RCA0_TXRX_G4PRESET           ((226 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA1_TXRX_G4PRESET           ((227 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA2_TXRX_G4PRESET           ((228 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA3_TXRX_G4PRESET           ((229 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA4_TXRX_G4PRESET           ((230 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA5_TXRX_G4PRESET           ((231 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA6_TXRX_G4PRESET           ((232 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S0_RCA7_TXRX_G4PRESET           ((233 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S1_RCA2_TXRX_G4PRESET           ((234 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S1_RCA3_TXRX_G4PRESET           ((235 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S1_RCA4_TXRX_G4PRESET           ((236 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S1_RCA5_TXRX_G4PRESET           ((237 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S1_RCA6_TXRX_G4PRESET           ((238 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_MQ_S1_RCA7_TXRX_G4PRESET           ((239 * 8) + NV_BOARD_PARAM_START) /* Default: 0x57575757 */
#define NV_SI_RO_BOARD_RAS_FLAGS                          ((240 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_DDR_PROGRESS_LOG_CTRL              ((241 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_2P_ALI_CE_MASK_THRESHOLD           ((242 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000001 */
#define NV_SI_RO_BOARD_2P_ALI_CE_MASK_INTERVAL            ((243 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_RAS_2P_CE_FILTER                   ((244 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_PCIE_AER_CE_THRESHOLD_EN           ((245 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_BMC_HANDSHAKE_SPI_TO1              ((246 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_BMC_HANDSHAKE_SPI_TO2              ((247 * 8) + NV_BOARD_PARAM_START)
#define NV_SI_RO_BOARD_PCIE_AER_CE_THRESHOLD              ((248 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000001 */
#define NV_SI_RO_BOARD_PCIE_AER_CE_INTERVAL               ((249 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_I2C_RCA_VRD_VOUT_FORMAT            ((250 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_CCIX_MODE_OVERWRITE                ((251 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_DVFS_VOLT_READ_BACK_MARGIN_MV      ((252 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_2P_DPLL                            ((253 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_RC_DOMAIN_CTRL                     ((254 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_SI_RO_BOARD_PCIE_SRIS_MODE                     ((255 * 8) + NV_BOARD_PARAM_START) /* Default: 0x00000000 */
#define NV_PMPRO_REGION4_LOAD_END                         (NV_SI_RO_BOARD_PCIE_SRIS_MODE)
//
// NOTE: Add before NV_BOARD_PARAM_MAX and increase its value
//
#define NV_BOARD_PARAM_MAX  ((255 * 8) + NV_BOARD_PARAM_START)

typedef UINT32 NVPARAM;

#endif /* NVPARAMDEF_H_ */
