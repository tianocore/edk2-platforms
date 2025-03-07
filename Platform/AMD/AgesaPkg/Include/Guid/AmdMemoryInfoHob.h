/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_MEMORY_INFO_HOB_H_
#define AMD_MEMORY_INFO_HOB_H_

/**
 * @brief 128 bit Buffer containing UID Unique Identifier value for Memory Info HOB.
 * EFI_GUID defined in UefiBaseType.h
 */
extern EFI_GUID  gAmdMemoryInfoHobGuid;

#pragma pack (push, 1)

/**
 * @brief Memory descriptor structure for each memory range
 */
typedef struct {
  UINT64    Base;                                         ///< Base address of memory rang
  UINT64    Size;                                         ///< Size of memory rang
  UINT32    Attribute;                                    ///< Attribute of memory rang
  UINT32    Reserved;                                     ///< For alignment purpose
} AMD_MEMORY_RANGE_DESCRIPTOR;

/**
 * @brief Memory attribute in the memory range descriptor = AVAILABLE
 */
#define AMD_MEMORY_ATTRIBUTE_AVAILABLE  0x1

/**
 * @brief Memory attribute in the memory range descriptor = UMA
 */
#define AMD_MEMORY_ATTRIBUTE_UMA  0x2

/**
 * @brief Memory attribute in the memory range descriptor = MMIO
 */
#define AMD_MEMORY_ATTRIBUTE_MMIO  0x3

/**
 * @brief Memory attribute in the memory range descriptor = RESERVED
 */
#define AMD_MEMORY_ATTRIBUTE_RESERVED  0x4

/**
 * @brief Memory attribute in the memory range descriptor = GPUMEM
 */
#define AMD_MEMORY_ATTRIBUTE_GPUMEM  0x5

/**
 * @brief Memory attribute in the memory range descriptor = GPU_SP
 */
#define AMD_MEMORY_ATTRIBUTE_GPU_SP  0x6

/**
 * @brief Memory attribute in the memory range descriptor = GPU_RESERVED
 */
#define AMD_MEMORY_ATTRIBUTE_GPU_RESERVED  0x7

/**
 * @brief Memory attribute in the memory range descriptor = GPU_RESERVED_TMR
 */
#define AMD_MEMORY_ATTRIBUTE_GPU_RESERVED_TMR  0x8

/**
 * @brief Memory attribute in the memory range descriptor = RESERVED_SMUFEATURES
 */
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

/**
 * @brief Macro that defines the Memory Info HOB version
 */
#define AMD_MEMORY_INFO_HOB_VERISION  0x00000110ul        // Ver: 00.00.01.10

#endif // AMD_MEMORY_INFO_HOB_H_
