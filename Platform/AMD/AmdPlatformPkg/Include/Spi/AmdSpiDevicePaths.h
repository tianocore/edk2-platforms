/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef AMD_SPI_DEVICE_PATHS_H_
#define AMD_SPI_DEVICE_PATHS_H_

#include <Base.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/DevicePath.h>
#include <FchRegistersCommon.h>

typedef struct {
  CONTROLLER_DEVICE_PATH            ControllerDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          End;
} SPI_CONTROLLER_DEVICE_PATH;


SPI_CONTROLLER_DEVICE_PATH mFchDevicePath = {
  {
    {
    HARDWARE_DEVICE_PATH,
    HW_CONTROLLER_DP,
    {
      (UINT8)(sizeof (CONTROLLER_DEVICE_PATH)),
      (UINT8)((sizeof (CONTROLLER_DEVICE_PATH)) >> 8)
    }
  },
  FCH_LPC_BUS << 16 |  FCH_LPC_DEV << 8 | FCH_LPC_FUNC
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {0x4}
  }
};

#endif // AMD_SPI_DEVICE_PATHS_H_
