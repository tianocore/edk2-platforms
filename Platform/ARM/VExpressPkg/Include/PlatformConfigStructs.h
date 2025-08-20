/** @file
*  Header defining Versatile Express platform configuration structure
*
*  Copyright (c) 2025, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _PLATFORMCONFIGSTRUCTS_H_
#define _PLATFORMCONFIGSTRUCTS_H_

#define CONFIGURATION_VARSTORE_ID  0x1234

#pragma pack(1)

#define MAX_CPUS  8

typedef struct {
  UINT8    CpuEnable[8];
} PLATFORM_CONFIG_DATA;

#pragma pack()

#endif
