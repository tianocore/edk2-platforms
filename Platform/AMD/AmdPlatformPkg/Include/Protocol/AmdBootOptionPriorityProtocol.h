/** @file

  Boot option priority protocol definition.

  Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/SortLib.h>

///
/// Global ID for the boot option priority protocol.
///
#define AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL_GUID    \
  { 0x5806db97, 0x5303, 0x409f, { 0x8f, 0x09, 0xab, 0x29, 0xd8, 0x07, 0xa3, 0xf1 } }

///
/// This protocol is introduced so the platform can give certain boot options
/// a custom priority value. Useful in boot overrides, or when IPMI does not inherently
/// support a specific boot override needed by the platform.
///
typedef struct {
  UINT8           IpmiBootDeviceSelectorType;
  SORT_COMPARE    Compare;
} AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL;

extern EFI_GUID  gAmdBoardBdsBootOptionPriorityProtocolGuid;
