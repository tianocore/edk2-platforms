/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

/**
  Hook to store the vector that all APs should jump to out of reset to a non-volatile,
  shared location.

  @param[in]     ApInitAddress     Address of the code that AP should jump to
  @param[in,out] ContentToRestore  The current value in the non-volatile storage

**/
VOID
SaveApInitVector (
  IN       UINT32  ApInitAddress,
  IN OUT   UINT32  *ContentToRestore
  )
{
}

/**
  Hook to restore the initial content of the non-volatile storage location.

  @param[in]     ContentToRestore  The value to restore

**/
VOID
RestoreContentVector (
  IN       UINT32  ContentToRestore
  )
{
}
