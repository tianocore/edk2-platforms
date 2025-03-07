/** @file
  Implements AmdIdsHookExtLibNull.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>

typedef enum {
  IDS_HOOK_UNSUPPORTED = 1
} IDS_HOOK_STATUS;

IDS_HOOK_STATUS
IdsHookExtEntry (
  UINT32  HookId,
  VOID    *Handle,
  VOID    *Data
  )
{
  return IDS_HOOK_UNSUPPORTED;
}

IDS_HOOK_STATUS
GetIdsNvTable (
  IN OUT   VOID    *IdsNvTable,
  IN OUT   UINT32  *IdsNvTableSize
  )
{
  return IDS_HOOK_UNSUPPORTED;
}
