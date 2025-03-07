/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SOC_LOGICAL_ID_H_
#define SOC_LOGICAL_ID_H_
#pragma pack (push, 1)

///
/// SOC logical ID structure
///
typedef struct _SOC_LOGICAL_ID {
  IN OUT   UINT32    Family;          ///< Indicates logical ID Family
  IN OUT   UINT16    Revision;        ///< Indicates logical ID Revision
} SOC_LOGICAL_ID;

#pragma pack (pop)
#endif // SOC_LOGICAL_ID_H_
