/** @file
  Allwinner H616/H618 SoC Hardware Register Definitions.

  Copyright (c) 2026, Khai Do <dovankhai97@gmail.com>. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

//
// On-Chip SRAM Memory Regions
//
#define AW_H618_SRAM_A1_BASE            0x00020000ULL
#define AW_H618_SRAM_A1_SIZE            0x00008000ULL
#define AW_H618_SRAM_C_SIZE             0x00030000ULL
#define AW_H618_SRAM_A2_BASE            0x00100000ULL
#define AW_H618_SRAM_A2_SIZE            0x00018000ULL

//
// Security ID (SID) eFuse Controller
//
#define AW_H618_SID_BASE                0x03006000ULL
#define AW_H618_SID_EFUSE_SIZE          0x00000200U
#define AW_H618_SID_PRCTL_OFFSET        0x40U
#define AW_H618_SID_RDKEY_OFFSET        0x60U
#define AW_H618_SID_OP_LOCK             0xAC00U
#define AW_H618_SID_READ_TRIG           0x0002U

//
// ARM Generic Interrupt Controller (GICv2 PL400)
//
#define AW_H618_GIC_DIST_BASE           0x03021000ULL
#define AW_H618_GIC_CPU_BASE            0x03022000ULL

//
// Real Time Clock (RTC)
//
#define AW_H618_RTC_LOSC_CTRL_REG       0x0000U
#define AW_H618_RTC_DAY_REG             0x0010U
#define AW_H618_RTC_HH_MM_SS_REG        0x0014U
#define AW_H618_RTC_LOSC_DAY_ACCE       BIT7
#define AW_H618_RTC_LOSC_HMS_ACCE       BIT8
#define AW_H618_RTC_DAY_MASK            0x0000FFFFU
#define AW_H618_RTC_HMS_MASK            0x001F3F3FU
#define AW_H618_RTC_HOUR_MASK           0x0000001FU
#define AW_H618_RTC_MINUTE_MASK         0x0000003FU
#define AW_H618_RTC_SECOND_MASK         0x0000003FU
#define AW_H618_RTC_HOUR_SHIFT          16U
#define AW_H618_RTC_MINUTE_SHIFT        8U

//
// System Clocks
//
#define AW_H618_OSC24M_FREQ             24000000U
