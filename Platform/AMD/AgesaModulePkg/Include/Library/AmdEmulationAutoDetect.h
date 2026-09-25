/** @file
  AMD Emulation environment auto detection.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_IDS_EMULATION_AUTO_DETEC_H
#define AMD_IDS_EMULATION_AUTO_DETEC_H

/**
 *      Detect the system is emulation or real platform.
 *
 *
 *  @retval       TRUE    The system is emulation
 *  @retval       FALSE   The system is real platform
 *
 **/
BOOLEAN
AmdIdsEmulationAutoDetect (
  VOID
  );

#endif // AMD_IDS_EMULATION_AUTO_DETEC_H
