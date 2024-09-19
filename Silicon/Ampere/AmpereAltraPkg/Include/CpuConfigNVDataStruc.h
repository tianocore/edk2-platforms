/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_CONFIG_NV_DATA_STRUC_H_
#define CPU_CONFIG_NV_DATA_STRUC_H_

#define CPU_SUBNUMA_MODE_MONO  0x00
#define CPU_SUBNUMA_MODE_HEMI  0x01
#define CPU_SUBNUMA_MODE_QUAD  0x02

#define CPU_SLC_AS_L3_ENABLE   0x00
#define CPU_SLC_AS_L3_DISABLE  0x01

#define CPU_SLC_AS_L3_PERMITTED_YES  0x00
#define CPU_SLC_AS_L3_PERMITTED_NO   0x01

#pragma pack(1)
typedef struct {
  UINT32    CpuSubNumaMode;
  UINT32    CpuSlcAsL3Permitted;
  UINT32    CpuSlcAsL3;
} CPU_VARSTORE_DATA;

#pragma pack()

#define CPU_CONFIG_VARIABLE_NAME  L"CpuConfigNVData"

#endif /* CPU_CONFIG_NV_DATA_STRUC_H_ */
