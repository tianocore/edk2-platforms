/** @file

  Copyright (C) 2012-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_CPM_TABLE_PROTOCOL_H_
#define AMD_CPM_TABLE_PROTOCOL_H_

#include <AmdCpmBase.h>

//
// GUID definition
//
#define AMD_CPM_TABLE_PROTOCOL_GUID \
  { 0x3724cf01, 0x00c2, 0x9762, 0x11, 0xb3, 0x0e, 0xa8, 0xaa, 0x89, 0x72, 0x00 }

#define AMD_CPM_TABLE_SMM_PROTOCOL_GUID \
  { 0xaf6efacf, 0x7a13, 0x45a3, 0xb1, 0xa5, 0xaa, 0xfc, 0x06, 0x1c, 0x4b, 0x79 }

extern EFI_GUID  gAmdCpmTableProtocolGuid;
extern EFI_GUID  gAmdCpmTableSmmProtocolGuid;

/// DXE Protocol Structure
typedef struct _AMD_CPM_TABLE_PROTOCOL {
  UINTN                          Revision;                        ///< Protocol Revision
  AMD_CPM_MAIN_TABLE             *MainTablePtr;                   ///< Pointer to CPM Main Table
  AMD_CPM_CHIP_ID                ChipId;                          ///< Id of SB Chip
  AMD_CPM_COMMON_FUNCTION        CommonFunction;                  ///< Private Common Functions
  AMD_CPM_DXE_PUBLIC_FUNCTION    DxePublicFunction;               ///< Public Function of Protocol
} AMD_CPM_TABLE_PROTOCOL;

// Current Protocol Revision
#define AMD_CPM_TABLE_PROTOCOL_REV  0x00

#endif // AMD_CPM_TABLE_PROTOCOL_H_
