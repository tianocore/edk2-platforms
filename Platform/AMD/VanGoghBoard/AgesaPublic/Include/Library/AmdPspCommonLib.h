/** @file
   AMD Psp Common Library header file
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_PSPCOMMONLIB_H_
#define AMD_PSPCOMMONLIB_H_

#include <AmdPspDirectory.h>

/*----------------------------------------------------------------------------------------
 *                   D E F I N I T I O N S    A N D    M A C R O S
 *----------------------------------------------------------------------------------------
 */

#define TCG_EVENT_BASE_AMD                   ((TCG_EVENTTYPE) 0x8000)
#define TCG_EVENT_BASE_AMD_BIOS              (TCG_EVENT_BASE_AMD + 0x400)
#define TCG_EVENT_AMD_BIOS_TSME_MEASUREMENT  (TCG_EVENT_BASE_AMD_BIOS + 1)

BOOLEAN
GetFtpmControlArea (
  IN OUT   VOID  **FtpmControlArea
  );

#define PSPLIB_WAIT_INFINITELY  0xFFFFFFFFL

#endif // AMD_PSPCOMMONLIB_H_
