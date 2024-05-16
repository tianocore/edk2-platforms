/** @file
  Defines AMD memory info hob.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_MEMORY_INFO_HOB_H_
#define AMD_MEMORY_INFO_HOB_H_

#pragma pack (push, 1)

/// Memory descriptor structure for each memory range
typedef struct {
  UINT64    Base;                           ///< Base address of memory rang
  UINT64    Size;                           ///< Size of memory rang
  UINT32    Attribute;                      ///< Attribute of memory rang
  UINT32    Reserved;                       ///< For alignment purpose
} AMD_MEMORY_RANGE_DESCRIPTOR;

/// Memory info HOB structure
typedef struct  {
  UINT32                         Version;               ///< Version of HOB structure
  BOOLEAN                        Reserved1;
  UINT16                         Reserved2;
  BOOLEAN                        Reserved3;
  UINT8                          Reserved4;
  BOOLEAN                        Reserved5;
  UINT32                         Reserved6;
  UINT32                         Reserved7;
  UINT32                         NumberOfDescriptor;    ///< Number of memory range descriptor
  AMD_MEMORY_RANGE_DESCRIPTOR    Ranges[1];             ///< Memory ranges array
} AMD_MEMORY_INFO_HOB;

#pragma pack (pop)

/// Memory attribute in the memory range descriptor = AVAILABLE
#define AMD_MEMORY_ATTRIBUTE_AVAILABLE  0x1

/// Memory attribute in the memory range descriptor = UMA
#define AMD_MEMORY_ATTRIBUTE_UMA  0x2

/// Memory attribute in the memory range descriptor = MMIO
#define AMD_MEMORY_ATTRIBUTE_MMIO  0x3

/// Memory attribute in the memory range descriptor = RESERVED
#define AMD_MEMORY_ATTRIBUTE_RESERVED  0x4

#endif
