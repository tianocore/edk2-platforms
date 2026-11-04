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

//
// ARM Generic Interrupt Controller (GICv2 PL400)
//
#define AW_H618_GIC_DIST_BASE           0x03021000ULL
#define AW_H618_GIC_CPU_BASE            0x03022000ULL

//
// System Clocks
//
#define AW_H618_OSC24M_FREQ             24000000U
