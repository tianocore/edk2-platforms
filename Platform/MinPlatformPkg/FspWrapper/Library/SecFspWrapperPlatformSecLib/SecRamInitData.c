/** @file
  Provide TempRamInitParams data.

Copyright (c) 2017 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PcdLib.h>
#include <FspEas.h>
#include "FsptCoreUpd.h"

#if defined (MDE_CPU_IA32) && FixedPcdGetBool (PcdFspWrapperResetVectorInFsp) == 1
#error "PcdFspWrapperResetVectorInFsp == TRUE only supported for X64 builds"
#endif

typedef struct {
  FSP_UPD_HEADER    FspUpdHeader;
#if FixedPcdGet8 (PcdFsptArchUpdRevision) == 1
  FSPT_ARCH_UPD     FsptArchUpd;
#elif FixedPcdGet8 (PcdFsptArchUpdRevision) == 2
  FSPT_ARCH2_UPD    FsptArchUpd;
#endif
  FSPT_CORE_UPD     FsptCoreUpd;
  UINT16            UpdTerminator;
} FSPT_UPD_DATA;

GLOBAL_REMOVE_IF_UNREFERENCED CONST FSPT_UPD_DATA FsptUpdDataPtr = {
  {
    0x4450555F54505346,                                           // FSP-T UPD Header Signature - FSPT_UPD
    FixedPcdGet8 (PcdFsptUpdHeaderRevision),                      // FSP-T UPD Header Revision
    {                                                             // Reserved[23]
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00
    }
  },
#if FixedPcdGet8 (PcdFsptArchUpdRevision) == 1
  {
    0x01,                                                         // FSP-T ARCH UPD Revision
    {                                                             // Reserved[3]
      0x00, 0x00, 0x00
    },
    0x00000020,                                                   // Length of FSP-T ARCH UPD
    0,                                                            // FspDebugHandler
    {                                                             // Reserved1[20]
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
#elif FixedPcdGet8 (PcdFsptArchUpdRevision) == 2
  {
    0x02,                                                         // FSP-T ARCH2 UPD Revision
    {                                                             // Reserved[3]
      0x00, 0x00, 0x00
    },
    0x00000020,                                                   // Length of FSP-T ARCH2 UPD
    0,                                                            // FspDebugHandler
    {                                                             // Reserved1[16]
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
#endif
#if FixedPcdGet8 (PcdFsptArchUpdRevision) <= 1
  {
    FixedPcdGet32 (PcdFlashFvMicrocodeBase) + FixedPcdGet32 (PcdMicrocodeOffsetInFv), // MicrocodeRegionBase
    FixedPcdGet32 (PcdFlashFvMicrocodeSize) - FixedPcdGet32 (PcdMicrocodeOffsetInFv), // MicrocodeRegionSize
    0, // Set CodeRegionBase as 0, so that caching will be 4GB-(CodeRegionSize > LLCSize ? LLCSize : CodeRegionSize) will be used.
    FixedPcdGet32 (PcdFlashCodeCacheSize),                                            // CodeRegionSize
    {                                                                                 // Reserved[16]
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
#else
  {
    FixedPcdGet32 (PcdFlashFvMicrocodeBase) + FixedPcdGet32 (PcdMicrocodeOffsetInFv), // MicrocodeRegionBase
    FixedPcdGet32 (PcdFlashFvMicrocodeSize) - FixedPcdGet32 (PcdMicrocodeOffsetInFv), // MicrocodeRegionSize
    0, // Set CodeRegionBase as 0, so that caching will be 4GB-(CodeRegionSize > LLCSize ? LLCSize : CodeRegionSize) will be used.
    FixedPcdGet32 (PcdFlashCodeCacheSize)                                             // CodeRegionSize
  },
#endif
  0x55AA
};
