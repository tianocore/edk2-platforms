/** @file

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_DEVICE_PATHS_H_
#define SPI_DEVICE_PATHS_H_

#include <Base.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/DevicePath.h>

typedef struct {
  CONTROLLER_DEVICE_PATH      ControllerDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} SPI_CONTROLLER_DEVICE_PATH;

#define FCH_DEVICE_PATH  {                                  \
    {                                                       \
      {                                                     \
        HARDWARE_DEVICE_PATH,                               \
        HW_CONTROLLER_DP,                                   \
        {                                                   \
          (UINT8)(sizeof (CONTROLLER_DEVICE_PATH)),         \
          (UINT8)((sizeof (CONTROLLER_DEVICE_PATH)) >> 8)   \
        }                                                   \
      },                                                    \
      0                                                     \
    },                                                      \
    {                                                       \
      END_DEVICE_PATH_TYPE,                                 \
      END_ENTIRE_DEVICE_PATH_SUBTYPE,                       \
      { 0x4 }                                               \
    }                                                       \
  }

#endif // SPI_DEVICE_PATHS_H_
