/** @file
  Library functions for Config Block management.

Copyright (c) 2017 - 2026, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <ConfigBlock.h>
#include <Library/ConfigBlockLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

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
  )
{
  CONFIG_BLOCK_TABLE_HEADER *ConfigBlkTblAddrPtr;
  UINT32                    ConfigBlkTblHdrSize;

  ConfigBlkTblHdrSize = (UINT32)(sizeof (CONFIG_BLOCK_TABLE_HEADER));

  if (TotalSize <= (ConfigBlkTblHdrSize + sizeof (CONFIG_BLOCK_HEADER))) {
    return EFI_INVALID_PARAMETER;
  }

  ConfigBlkTblAddrPtr = (CONFIG_BLOCK_TABLE_HEADER *)AllocateZeroPool (TotalSize);
  if (ConfigBlkTblAddrPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ConfigBlkTblAddrPtr->NumberOfBlocks = 0;
  ConfigBlkTblAddrPtr->Header.GuidHob.Header.HobLength = TotalSize;
  ConfigBlkTblAddrPtr->AvailableSize = TotalSize - ConfigBlkTblHdrSize;

  *ConfigBlockTableAddress = (VOID *)ConfigBlkTblAddrPtr;

  return EFI_SUCCESS;
}

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
  )
{
  CONFIG_BLOCK              *TempConfigBlk;
  CONFIG_BLOCK_TABLE_HEADER *ConfigBlkTblAddrPtr;
  CONFIG_BLOCK              *ConfigBlkAddrPtr;
  UINT16                    ConfigBlkSize;

  ConfigBlkTblAddrPtr = (CONFIG_BLOCK_TABLE_HEADER *)ConfigBlockTableAddress;
  ConfigBlkAddrPtr = (CONFIG_BLOCK *)(*ConfigBlockAddress);
  ConfigBlkSize = ConfigBlkAddrPtr->Header.GuidHob.Header.HobLength;

  if ((ConfigBlkSize % 4) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  if (ConfigBlkTblAddrPtr->AvailableSize < ConfigBlkSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  TempConfigBlk = (CONFIG_BLOCK *)((UINTN)ConfigBlkTblAddrPtr + (UINTN)(ConfigBlkTblAddrPtr->Header.GuidHob.Header.HobLength - ConfigBlkTblAddrPtr->AvailableSize));

  //
  // If the template carries CONFIG_BLOCK_HEADER2 format, copy the full version 2
  // header (including the Namespace GUID at offset 32); otherwise copy only the
  // base CONFIG_BLOCK_HEADER.
  //
  if ((ConfigBlkAddrPtr->Header.Attributes & CONFIG_BLOCK_HEADER2_ATTRIBUTE) != 0) {
    if (ConfigBlkSize < sizeof (CONFIG_BLOCK_HEADER2)) {
      return EFI_INVALID_PARAMETER;
    }
    CopyMem (&TempConfigBlk->Header, &ConfigBlkAddrPtr->Header, sizeof (CONFIG_BLOCK_HEADER2));
  } else {
    CopyMem (&TempConfigBlk->Header, &ConfigBlkAddrPtr->Header, sizeof (CONFIG_BLOCK_HEADER));
  }

  ConfigBlkTblAddrPtr->NumberOfBlocks++;
  ConfigBlkTblAddrPtr->AvailableSize = ConfigBlkTblAddrPtr->AvailableSize - ConfigBlkSize;

  *ConfigBlockAddress = (VOID *) TempConfigBlk;
  return EFI_SUCCESS;
}

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
  )
{
  UINT16                    OffsetIndex;
  CONFIG_BLOCK              *TempConfigBlk;
  CONFIG_BLOCK_TABLE_HEADER *ConfigBlkTblAddrPtr;
  UINTN                     ConfigBlkTblHdrSize;
  UINTN                     ConfigBlkOffset;
  UINT16                    NumOfBlocks;

  ConfigBlkTblHdrSize = (UINTN)(sizeof (CONFIG_BLOCK_TABLE_HEADER));
  ConfigBlkTblAddrPtr = (CONFIG_BLOCK_TABLE_HEADER *)ConfigBlockTableAddress;
  NumOfBlocks = ConfigBlkTblAddrPtr->NumberOfBlocks;

  ConfigBlkOffset = 0;
  for (OffsetIndex = 0; OffsetIndex < NumOfBlocks; OffsetIndex++) {
    if ((ConfigBlkTblHdrSize + ConfigBlkOffset) > (ConfigBlkTblAddrPtr->Header.GuidHob.Header.HobLength)) {
      break;
    }
    TempConfigBlk = (CONFIG_BLOCK *)((UINTN)ConfigBlkTblAddrPtr + (UINTN)ConfigBlkTblHdrSize + (UINTN)ConfigBlkOffset);
    if (CompareGuid (&(TempConfigBlk->Header.GuidHob.Name), ConfigBlockGuid)) {
      *ConfigBlockAddress = (VOID *)TempConfigBlk;
      return EFI_SUCCESS;
    }
    ConfigBlkOffset = ConfigBlkOffset + TempConfigBlk->Header.GuidHob.Header.HobLength;
  }

  return EFI_NOT_FOUND;
}

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
  )
{
  UINT16                     OffsetIndex;
  CONFIG_BLOCK              *TempConfigBlk;
  CONFIG_BLOCK2             *TempConfigBlk2;
  CONFIG_BLOCK_TABLE_HEADER *ConfigBlkTblAddrPtr;
  UINTN                      ConfigBlkTblHdrSize;
  UINTN                      ConfigBlkOffset;
  UINT16                     NumOfBlocks;

  if ((ConfigBlockTableAddress == NULL) || (ConfigBlockGuid == NULL) ||
      (Namespace == NULL)              || (ConfigBlockAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ConfigBlkTblHdrSize = (UINTN)(sizeof (CONFIG_BLOCK_TABLE_HEADER));
  ConfigBlkTblAddrPtr = (CONFIG_BLOCK_TABLE_HEADER *)ConfigBlockTableAddress;
  NumOfBlocks         = ConfigBlkTblAddrPtr->NumberOfBlocks;
  ConfigBlkOffset     = 0;

  for (OffsetIndex = 0; OffsetIndex < NumOfBlocks; OffsetIndex++) {
    if ((ConfigBlkTblHdrSize + ConfigBlkOffset) > ConfigBlkTblAddrPtr->Header.GuidHob.Header.HobLength) {
      break;
    }
    TempConfigBlk = (CONFIG_BLOCK *)((UINTN)ConfigBlkTblAddrPtr + ConfigBlkTblHdrSize + ConfigBlkOffset);
    if (CompareGuid (&TempConfigBlk->Header.GuidHob.Name, ConfigBlockGuid)) {
      if ((TempConfigBlk->Header.Attributes & CONFIG_BLOCK_HEADER2_ATTRIBUTE) != 0) {
        if (TempConfigBlk->Header.GuidHob.Header.HobLength >= sizeof (CONFIG_BLOCK_HEADER2)) {
          TempConfigBlk2 = (CONFIG_BLOCK2 *)TempConfigBlk;
          if (CompareGuid (&TempConfigBlk2->Header.Namespace, Namespace)) {
            *ConfigBlockAddress = (VOID *)TempConfigBlk;
            return EFI_SUCCESS;
          }
        }
      }
    }
    ConfigBlkOffset += TempConfigBlk->Header.GuidHob.Header.HobLength;
  }

  return EFI_NOT_FOUND;
}

/**
  Retrieve the next config block in a config block table.

  @param[in]           ConfigBlockTableAddress  Pointer to the config block table.
  @param[in, optional] ConfigBlockGuid          If non-NULL, filter by IP type GUID.
  @param[in, optional] CurrentConfigBlock       Starting point. NULL returns the first match.
                                                Must be a pointer previously returned by this function or NULL.
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
  )
{
  UINT16                     OffsetIndex;
  CONFIG_BLOCK              *TempConfigBlk;
  CONFIG_BLOCK_TABLE_HEADER *ConfigBlkTblAddrPtr;
  UINTN                      ConfigBlkTblHdrSize;
  UINTN                      ConfigBlkOffset;
  UINT16                     NumOfBlocks;
  BOOLEAN                    SearchStarted;

  if ((ConfigBlockTableAddress == NULL) || (NextConfigBlock == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ConfigBlkTblHdrSize = (UINTN)(sizeof (CONFIG_BLOCK_TABLE_HEADER));
  ConfigBlkTblAddrPtr = (CONFIG_BLOCK_TABLE_HEADER *)ConfigBlockTableAddress;
  NumOfBlocks         = ConfigBlkTblAddrPtr->NumberOfBlocks;
  ConfigBlkOffset     = 0;
  SearchStarted       = (CurrentConfigBlock == NULL);

  for (OffsetIndex = 0; OffsetIndex < NumOfBlocks; OffsetIndex++) {
    if ((ConfigBlkTblHdrSize + ConfigBlkOffset) > ConfigBlkTblAddrPtr->Header.GuidHob.Header.HobLength) {
      break;
    }
    TempConfigBlk = (CONFIG_BLOCK *)((UINTN)ConfigBlkTblAddrPtr + ConfigBlkTblHdrSize + ConfigBlkOffset);

    if (!SearchStarted) {
      if ((VOID *)TempConfigBlk == CurrentConfigBlock) {
        SearchStarted = TRUE;
      }
    } else {
      if ((ConfigBlockGuid == NULL) || CompareGuid (&TempConfigBlk->Header.GuidHob.Name, ConfigBlockGuid)) {
        *NextConfigBlock = (VOID *)TempConfigBlk;
        return EFI_SUCCESS;
      }
    }
    ConfigBlkOffset += TempConfigBlk->Header.GuidHob.Header.HobLength;
  }

  return EFI_NOT_FOUND;
}
