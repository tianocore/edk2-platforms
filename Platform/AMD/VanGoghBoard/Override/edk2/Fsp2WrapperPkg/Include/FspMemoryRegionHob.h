/** @file
  Implements FspMemoryRegionHob.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#pragma pack(push,1)
typedef struct {
  EFI_PHYSICAL_ADDRESS    BeginAddress;
  EFI_PHYSICAL_ADDRESS    Length;
} FSP_MEMORY_REGION_HOB;
#pragma pack(pop)
