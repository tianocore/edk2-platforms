/** @file
*  Header defining Versatile Express platform configuration structure
*
*  Copyright (c) 2025, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef PLATFORM_CONFIG_STRUCTS_H_
#define PLATFORM_CONFIG_STRUCTS_H_

#define CONFIGURATION_VARSTORE_ID  0x1234

#pragma pack(1)

#define MAX_CPUS  8

typedef struct {
  BOOLEAN    CpuEnable[MAX_CPUS];
} PLATFORM_CONFIG_DATA;

#pragma pack()

#endif // PLATFORM_CONFIG_STRUCTS_H_
