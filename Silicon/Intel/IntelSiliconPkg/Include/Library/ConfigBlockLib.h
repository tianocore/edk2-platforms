/** @file
  Header file for Config Block Lib implementation

  Copyright (c) 2019 - 2026 Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _CONFIG_BLOCK_LIB_H_
#define _CONFIG_BLOCK_LIB_H_

/**
  Create config block table.

  @param[in]     TotalSize                    - Max size to be allocated for the Config Block Table
  @param[out]    ConfigBlockTableAddress      - On return, points to a pointer to the beginning of Config Block Table Address

  @retval EFI_INVALID_PARAMETER - Invalid Parameter
  @retval EFI_OUT_OF_RESOURCES  - Out of resources
  @retval EFI_SUCCESS           - Successfully created Config Block Table at ConfigBlockTableAddress
**/
EFI_STATUS
EFIAPI
CreateConfigBlockTable (
  IN     UINT16    TotalSize,
  OUT    VOID      **ConfigBlockTableAddress
  );

/**
  Add config block into config block table structure.

  @param[in]     ConfigBlockTableAddress      - A pointer to the beginning of Config Block Table Address
  @param[out]    ConfigBlockAddress           - On return, points to a pointer to the beginning of Config Block Address

  @retval EFI_OUT_OF_RESOURCES - Config Block Table is full and cannot add new Config Block or
                                 Config Block Offset Table is full and cannot add new Config Block.
  @retval EFI_SUCCESS          - Successfully added Config Block
**/
EFI_STATUS
EFIAPI
AddConfigBlock (
  IN     VOID      *ConfigBlockTableAddress,
  OUT    VOID      **ConfigBlockAddress
  );

/**
  Retrieve a specific Config Block data by GUID.

  @param[in]      ConfigBlockTableAddress      - A pointer to the beginning of Config Block Table Address
  @param[in]      ConfigBlockGuid              - A pointer to the GUID uses to search specific Config Block
  @param[out]     ConfigBlockAddress           - On return, points to a pointer to the beginning of Config Block Address

  @retval EFI_NOT_FOUND         - Could not find the Config Block
  @retval EFI_SUCCESS           - Config Block found and return
**/
EFI_STATUS
EFIAPI
GetConfigBlock (
  IN     VOID      *ConfigBlockTableAddress,
  IN     EFI_GUID  *ConfigBlockGuid,
  OUT    VOID      **ConfigBlockAddress
  );

/**
  Retrieve a Config Block by type GUID and instance GUID.

  Walks the config block table matching GuidHob.Name against ConfigBlockGuid.
  Among matching blocks, returns the one whose Attributes has
  CONFIG_BLOCK_HEADER2_ATTRIBUTE (BIT0) set and whose Namespace matches
  the provided Namespace GUID. Config blocks that use the original
  CONFIG_BLOCK_HEADER format are never returned by this API, even if
  ConfigBlockGuid matches. Use GetConfigBlock() for those instead.

  @param[in]   ConfigBlockTableAddress  Pointer to the config block table.
  @param[in]   ConfigBlockGuid          IP type GUID (matches GuidHob.Name).
  @param[in]   Namespace                Scoping GUID (matches Header2.Namespace).
                                        Namespace can be a UUID v4 GUID to
                                        disambiguate between different logical
                                        partitions, or a UUID v5 GUID derived
                                        from a UUID v4 GUID and an instance
                                        index when more than one instance of a
                                        logical partition exists.
  @param[out]  ConfigBlockAddress       On success, pointer to the matching config block.

  @retval EFI_SUCCESS               Config block found.
  @retval EFI_NOT_FOUND             No matching block exists.
  @retval EFI_INVALID_PARAMETER     A required pointer argument is NULL.
**/
EFI_STATUS
EFIAPI
GetConfigBlockByInstance (
  IN     VOID      *ConfigBlockTableAddress,
  IN     EFI_GUID  *ConfigBlockGuid,
  IN     EFI_GUID  *Namespace,
  OUT    VOID      **ConfigBlockAddress
  );

/**
  Retrieve the next config block in a config block table.

  Starts the search after CurrentConfigBlock (or from the beginning if NULL).
  If ConfigBlockGuid is non-NULL, only blocks matching that GUID are returned.

  @param[in]           ConfigBlockTableAddress  Pointer to the config block table.
  @param[in, optional] ConfigBlockGuid          If non-NULL, filter by IP type GUID.
  @param[in, optional] CurrentConfigBlock       Starting point. NULL returns the first match.
  @param[out]          NextConfigBlock          On success, pointer to the next matching block.

  @retval EFI_SUCCESS               Next config block found.
  @retval EFI_NOT_FOUND             No further matching block exists.
  @retval EFI_INVALID_PARAMETER     ConfigBlockTableAddress or NextConfigBlock is NULL.
**/
EFI_STATUS
EFIAPI
GetNextConfigBlock (
  IN            VOID      *ConfigBlockTableAddress,
  IN  OPTIONAL  EFI_GUID  *ConfigBlockGuid,
  IN  OPTIONAL  VOID      *CurrentConfigBlock,
  OUT           VOID      **NextConfigBlock
  );

#endif // _CONFIG_BLOCK_LIB_H_
