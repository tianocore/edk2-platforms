/** @file
  Memory Region Information Definitions.

  Defines structures and types for managing memory region information,
  including HOB building options, resource attributes, and memory region
  descriptors used throughout the platform initialization.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <RamPartition.h>

/**
  HOB building options for memory regions.

  Defines various options for how memory regions should be handled during
  platform initialization, including HOB creation and memory mapping.
**/
typedef enum {
  AddMem,                           ///< Add memory as specified
  AddMemForNonDebugMode,            ///< Add memory if debug mode is not enabled
  AddMemForNonDebugOrNonCrashMode,  ///< Add memory if debug mode is not enabled or
                                    ///< if debug mode is enabled but crash did not happen
  ReserveMemForNonCrashMode,        ///< Reserve memory if crash did not happen
  NoBuildHob,                       ///< Do not build HOB for this region
  AddPeripheral,                    ///< Hob, Cache Settings
  HobOnlyNoCacheSetting,            ///< Hob, No Cache Settings
  CacheSettingCarveOutOnly,         ///< No Hob, Cache Setting in DEBUG only
  AllocOnly,                        ///< Allocation only
  NoMap,                            ///< No Hob, skip mapping in mmu and create a hole
  AddDynamicMem,                    ///< Add dynamic memory that are marked as reserved
  ErrorBuildHob = 99                ///< Return high invalid value on error
} BUILD_HOB_OPTION_TYPE;

/* Below flag is used for system memory, in platform config file */
#define SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS1                    \
                EFI_RESOURCE_ATTRIBUTE_PRESENT |                 \
                EFI_RESOURCE_ATTRIBUTE_INITIALIZED |             \
                EFI_RESOURCE_ATTRIBUTE_TESTED

#define SYSTEM_MEMORY_RESOURCE_ATTR_CAPABILITIES1                \
                EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |             \
                EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |       \
                EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
                EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |    \
                EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE |   \
                EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE |        \
                EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE

#define SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES        \
        (SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS1 |                 \
        SYSTEM_MEMORY_RESOURCE_ATTR_CAPABILITIES1)

#define MAX_MEM_LABEL_NAME  32

/**
  Memory region information structure.

  Contains all information needed to describe a memory region including
  its location, size, type, attributes, and HOB building options.
**/
typedef struct {
  CHAR8                           Name[MAX_MEM_LABEL_NAME]; ///< Region Name in ASCII
  UINT64                          MemBase;                  ///< Offset to DDR memory base
  UINT64                          MemSize;                  ///< Size (in bytes) of the memory region
  BUILD_HOB_OPTION_TYPE           BuildHobOption;           ///< Option to build HOB
  EFI_RESOURCE_TYPE               ResourceType;             ///< Resource type
  EFI_RESOURCE_ATTRIBUTE_TYPE     ResourceAttribute;        ///< Resource attribute
  EFI_MEMORY_TYPE                 MemoryType;               ///< Memory type, applicable for EFI_RESOURCE_SYSTEM_MEMORY only
  ARM_MEMORY_REGION_ATTRIBUTES    CacheAttributes;          ///< Cache attributes
  RAM_PARTITION_TYPE              PartitionType;            ///< RAM partition type
} MEM_REGION_INFO;
