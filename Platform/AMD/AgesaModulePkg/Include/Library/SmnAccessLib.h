/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMNACCESSLIB_H_
#define SMNACCESSLIB_H_

VOID
EFIAPI
SmnRegisterReadS (
  IN       UINT32  SegmentNumber,
  IN       UINT32  BusNumber,
  IN       UINT32  Address,
  OUT      VOID    *Value
  );

VOID
EFIAPI
SmnRegisterRMWS (
  IN       UINT32  SegmentNumber,
  IN       UINT32  BusNumber,
  IN       UINT32  Address,
  IN       UINT32  AndMask,
  IN       UINT32  OrValue,
  IN       UINT32  Flags
  );

#endif // SMNACCESSLIB_H_
