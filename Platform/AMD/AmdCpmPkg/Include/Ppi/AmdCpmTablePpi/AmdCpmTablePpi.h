/** @file
  PEI PPI published by AmdCpmPei to indicate the CPM table is available.

  Only the Revision field is defined; consumers only need to detect PPI
  presence (e.g. LocatePpi returning non-NULL), not the CPM table content.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

typedef struct {
  UINT32    Revision;
} AMD_CPM_TABLE_PPI;
