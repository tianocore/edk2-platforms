/** @file

  Copyright (C) 2018-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/PciSegmentInfoLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

/**
  Return an array of PCI_SEGMENT_INFO holding the segment information.

  Note: The returned array/buffer is owned by callee.

  @param  Count  Return the count of segments.

  @retval A callee owned array holding the segment information.
**/
PCI_SEGMENT_INFO *
EFIAPI
GetPciSegmentInfo (
  UINTN  *Count
  )
{
  return (PCI_SEGMENT_INFO *)NULL;
}
