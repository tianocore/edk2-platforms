/** @file
  Header file of AMD SMM SPI host controller state protocol

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_SPI_HC_CHIP_SELECT_PARAMETERS_H_
#define AMD_SPI_HC_CHIP_SELECT_PARAMETERS_H_

#include <Base.h>

#pragma pack (1)
typedef struct _CHIP_SELECT_PARAMETERS {
  UINT8    AndValue;
  UINT8    OrValue;
} CHIP_SELECT_PARAMETERS;
#pragma pack ()

#define CHIP_SELECT_1  { (UINT8)~((UINT8)0x03), 0x0 }
#define CHIP_SELECT_2  { (UINT8)~((UINT8)0x03), 0x1 }

#endif // AMD_SPI_HC_CHIP_SELECT_PARAMETERS_H_
