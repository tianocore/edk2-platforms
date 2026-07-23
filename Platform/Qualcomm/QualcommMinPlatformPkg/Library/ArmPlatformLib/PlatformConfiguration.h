/** @file
  Parser for iqualcomm UEFI platform configuration data.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Pi/PiHob.h>

#include <MemRegionInfo.h>

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

/* Parses UEFI platform configuration data (i.e. UEFI memory map) and
   stores/installs the necessary items to be accessed by other UEFI modules
*/
EFI_STATUS
EFIAPI
LoadAndParsePlatformCfg (
  VOID
  );

/* Adds all banks from ram partition table, as well as remainder of bank
   containing FD region to the memory map table */
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

/* Initializes the cache with all the memory regions */
EFI_STATUS
EFIAPI
InitCacheWithMemoryRegions (
  VOID
  );

/* Gets the Memory Map that was parsed from the platform cfg file */
EFI_STATUS
EFIAPI
GetMemRegionCfgInfo (
  MEM_REGION_INFO  **MemoryRegions,
  UINTN            *NumMemoryRegions
  );

/* Gets the configuration tables detail parsed from config file */
EFI_STATUS EFIAPI
GetMetaConfigTable (
  META_CONFIG  *MetaCfgTable
  );

/* To Check if there is a region defined in uefiplat that overlaps with a hole in rampartition table */
EFI_STATUS
EFIAPI
ValidateParsedMemoryRegions (
  VOID
  );

/* Updates the region marked as Reserved to Conventional memory based on the Ram Partiton table info */
EFI_STATUS
EFIAPI
UpdateReservedMemoryRegions (
  VOID
  );

/* Loads a minimal static memory map from PCDs when device tree is not available */
EFI_STATUS
EFIAPI
LoadStaticPlatformCfg (
  VOID
  );
