/** @file
  Provide TempRamInitParams data.

Copyright (c) 2017 - 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PcdLib.h>
#include <FspEas.h>
#include "FsptCoreUpd.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST FSPT_UPD FsptUpdDataPtr = {
  {
    FSPT_UPD_SIGNATURE,
    0x02,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00
    }
  },
  {
    0x01,
    {
      0x00, 0x00, 0x00
    },
    0x00000020,
    0x00000000,
    {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    FixedPcdGet32 (PcdFlashFvMicrocodeBase) + FixedPcdGet32 (PcdMicrocodeOffsetInFv),
    FixedPcdGet64 (PcdFlashFvMicrocodeSize) - FixedPcdGet32 (PcdMicrocodeOffsetInFv),
    0,          // Set CodeRegionBase as 0, so that caching will be 4GB-(CodeRegionSize > LLCSize ? LLCSize : CodeRegionSize) will be used.
    FixedPcdGet32 (PcdFlashCodeCacheSize),
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    FixedPcdGet8 (PcdSerialIoUartDebugEnable),
    FixedPcdGet8 (PcdSerialIoUartNumber),
    FixedPcdGet8 (PcdSerialIoUartMode),
    0,
    FixedPcdGet32 (PcdSerialIoUartBaudRate),
    FixedPcdGet64 (PcdPciExpressBaseAddress)
  },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  },
  0x55AA
};
