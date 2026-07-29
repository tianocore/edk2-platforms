/** @file
  Parser for Qualcomm UEFI platform configuration data.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Pi/PiHob.h>

#include <MemRegionInfo.h>
/** Maximum number of memory region entries in the page table map. **/
#define MAX_MEMORY_ENTRIES  128

/** Maximum number of memory regions in the platform memory map. **/
#define MAX_MEMORY_REGIONS  125

/**
  Internal sort structure used by CheckOverlap() to validate that
  memory regions do not overlap.
**/
typedef struct {
  UINT64    MemBase;                  ///< Offset to DDR memory base
  UINT64    MemSize;                  ///< Size (in bytes) of the memory region
  CHAR16    Name[MAX_MEM_LABEL_NAME]; ///< Region Name in ASCII
} SORT_MEM_REG_INFO;

typedef struct {
  CHAR8    *Key;
  CHAR8    *Value;
} STRING_CONFIGURATION_PAIR;

typedef struct {
  CHAR8     *Key;
  UINT64    Value;
} INTEGER_CONFIGURATION_PAIR;

typedef struct {
  STRING_CONFIGURATION_PAIR     *ConfigTable;
  UINTN                         ConfigTableEntryCount;
  INTEGER_CONFIGURATION_PAIR    *IntConfigTable;
  UINTN                         IntConfigTableEntryCount;
} META_CONFIG;

/**
  Load and parse platform configuration.

  Parses UEFI platform configuration data (i.e. UEFI memory map) and
  stores/installs the necessary items to be accessed by other UEFI modules.

  @retval  EFI_SUCCESS     Configuration loaded and parsed successfully.
  @retval  EFI_LOAD_ERROR  Error occurred during parsing.

**/
EFI_STATUS
EFIAPI
LoadAndParsePlatformCfg (
  VOID
  );

/**
  Update system memory regions.

  Adds all banks from RAM partition table, as well as remainder of
  bank containing FD region to the memory map table.

  @retval  EFI_SUCCESS  System memory regions updated successfully.
  @retval  Other        Error occurred during update.

**/
EFI_STATUS
EFIAPI
UpdateSystemMemoryRegions (
  VOID
  );

EFI_STATUS
EFIAPI
AddUpperMemoryFromRamPartitions (
  VOID
  );

/**
  Get memory region configuration information.

  Gets the Memory Map that was parsed from the platform cfg file.

  @param[out]  MemoryRegions     Pointer to receive memory regions array.
  @param[out]  NumMemoryRegions  Pointer to receive number of memory regions.

  @retval  EFI_SUCCESS             Memory region info retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
EFIAPI
GetMemRegionCfgInfo (
  MEM_REGION_INFO  **MemoryRegions,
  UINTN            *NumMemoryRegions
  );

/* Gets the configuration tables detail parsed from config file */
EFI_STATUS
EFIAPI
GetMetaConfigTable (
  META_CONFIG  *MetaCfgTable
  );

/**
  Validate parsed memory regions.

  Check if there is a region defined in uefiplat that overlaps with a
  hole in rampartition table.

  @retval  EFI_SUCCESS             Memory regions validated successfully.
  @retval  EFI_INVALID_PARAMETER   Overlapping regions found.

**/
EFI_STATUS
EFIAPI
ValidateParsedMemoryRegions (
  VOID
  );

/**
  Update reserved memory regions.

  Updates the region marked as Reserved to Conventional memory based on
  the Ram Partiton table info.

  @retval  EFI_SUCCESS  Reserved memory regions updated successfully.
  @retval  Other        Error occurred during update.

**/
EFI_STATUS
EFIAPI
UpdateReservedMemoryRegions (
  VOID
  );

/**
  Load static platform configuration.

  Loads a minimal static memory map from PCDs when device tree is not
  available.

  @retval  EFI_SUCCESS           Static configuration loaded successfully.
  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed.

**/
EFI_STATUS
EFIAPI
LoadStaticPlatformCfg (
  VOID
  );
