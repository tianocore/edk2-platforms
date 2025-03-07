/** @file
     Common AMD header file
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AGESA_H_
#define AGESA_H_

#include  "AMD.h"

///< CPU MSR Register definitions ------------------------------------------
#define SYS_CFG   0xC0010010ul
#define TOP_MEM   0xC001001Aul
#define TOP_MEM2  0xC001001Dul
#define HWCR      0xC0010015ul
#define NB_CFG    0xC001001Ful

// CPU Build Configuration structures and definitions

#define AMD_AP_MTRR_FIX64k_00000  0x00000250ul
#define AMD_AP_MTRR_FIX16k_80000  0x00000258ul
#define AMD_AP_MTRR_FIX16k_A0000  0x00000259ul
#define AMD_AP_MTRR_FIX4k_C0000   0x00000268ul
#define AMD_AP_MTRR_FIX4k_C8000   0x00000269ul
#define AMD_AP_MTRR_FIX4k_D0000   0x0000026Aul
#define AMD_AP_MTRR_FIX4k_D8000   0x0000026Bul
#define AMD_AP_MTRR_FIX4k_E0000   0x0000026Cul
#define AMD_AP_MTRR_FIX4k_E8000   0x0000026Dul
#define AMD_AP_MTRR_FIX4k_F0000   0x0000026Eul
#define AMD_AP_MTRR_FIX4k_F8000   0x0000026Ful
#define CPU_LIST_TERMINAL         0xFFFFFFFFul

#endif // AGESA_H_
