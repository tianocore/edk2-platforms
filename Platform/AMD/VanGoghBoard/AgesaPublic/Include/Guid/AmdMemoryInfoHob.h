/** @file
     AMD Memory Info Hob Definition
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_MEMORY_INFO_HOB_H_
#define AMD_MEMORY_INFO_HOB_H_

extern EFI_GUID  gAmdMemoryInfoHobGuid;

#pragma pack (push, 1)

/// Memory descriptor structure for each memory rang
typedef struct {
  UINT64    Base;                           ///< Base address of memory rang
  UINT64    Size;                           ///< Size of memory rang
  UINT32    Attribute;                      ///< Attribute of memory rang
  UINT32    Reserved;                       ///< For alignment purpose
} AMD_MEMORY_RANGE_DESCRIPTOR;

#define AMD_MEMORY_ATTRIBUTE_AVAILABLE             0x1
#define AMD_MEMORY_ATTRIBUTE_UMA                   0x2
#define AMD_MEMORY_ATTRIBUTE_MMIO                  0x3
#define AMD_MEMORY_ATTRIBUTE_RESERVED              0x4
#define AMD_MEMORY_ATTRIBUTE_GPUMEM                0x5
#define AMD_MEMORY_ATTRIBUTE_GPU_SP                0x6
#define AMD_MEMORY_ATTRIBUTE_GPU_RESERVED          0x7
#define AMD_MEMORY_ATTRIBUTE_GPU_RESERVED_TMR      0x8
#define AMD_MEMORY_ATTRIBUTE_Reserved_SmuFeatures  0x9

/// Memory info HOB structure
typedef struct  {
  UINT32                         Version;                 ///< Version of HOB structure
  BOOLEAN                        AmdMemoryVddioValid;     ///< This field determines if Vddio is valid
  UINT16                         AmdMemoryVddio;          ///< Vddio Voltage
  BOOLEAN                        AmdMemoryVddpVddrValid;  ///< This field determines if VddpVddr is valid
  UINT8                          AmdMemoryVddpVddr;       ///< VddpVddr voltage
  BOOLEAN                        AmdMemoryFrequencyValid; ///< Memory Frequency Valid
  UINT32                         AmdMemoryFrequency;      ///< Memory Frquency
  UINT32                         AmdMemoryDdrMaxRate;     ///< Memory DdrMaxRate
  UINT32                         NumberOfDescriptor;      ///< Number of memory range descriptor
  AMD_MEMORY_RANGE_DESCRIPTOR    Ranges[1];               ///< Memory ranges array
} AMD_MEMORY_INFO_HOB;

#pragma pack (pop)

#define AMD_MEMORY_INFO_HOB_VERISION  0x00000110ul        // Ver: 00.00.01.10

#endif // AMD_MEMORY_INFO_HOB_H_
