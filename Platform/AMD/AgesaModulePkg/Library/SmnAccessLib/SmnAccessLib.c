/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include  <Library/SmnAccessLib.h>

/*
  Routine to read all register spaces.

  @param[in]  SegmentNumber     Segment number of D0F0 of the target die
  @param[in]  BusNumber         Bus number of D0F0 of the target die
  @param[in]  Address           Register offset, but PortDevice
  @param[out] Value             Return value
  @retval     VOID
 */
VOID
EFIAPI
SmnRegisterReadS (
  IN       UINT32  SegmentNumber,
  IN       UINT32  BusNumber,
  IN       UINT32  Address,
  OUT   VOID       *Value
  )
{
  return;
}
