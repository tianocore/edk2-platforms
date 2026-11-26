/** @file
  Copyright (c) 2025 Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/



#ifndef  __FSP_NVS_LOCATION_HOB_H__
#define  __FSP_NVS_LOCATION_HOB_H__

#include <Base.h>

#define FSP_NVS_LOCATION_GUID { 0xbc23b358, 0x6ad0, 0x11f0, {0xac,0xee,0xaf,0xf0,0x57,0x5f,0xe8,0x05} }


typedef struct {
  UINT64 CpuLinearAddress;
  UINT32 FlashLinearAddress;
  UINT32 Size;
} FSP_NVS_LOCATION;


#endif