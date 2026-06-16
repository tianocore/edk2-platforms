/** @file
  Qualcomm RAM Partition Table Library Interface.

  RAM partition table interface for accessing and managing RAM partition
  information from Qualcomm Shared Memory (SMEM).

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - PASR - Partial Array Self Refresh

**/

#pragma once

#include <Uefi.h>

#include <MemRegionInfo.h>
#include <RamPartition.h>

/**
  Read RAM partition table and cache entries.

  @retval  EFI_SUCCESS  Successfully initialized the library.
  @retval  Other        Initialization failed.

**/
EFI_STATUS
InitRamPartitionTableLib (
  VOID
  );

/**
  Get Major version from RAM partition table.

  @param[in]      pRamPartitionTable  Pointer to RAM partition table.
  @param[in,out]  Version             Major version on output.

  @retval  EFI_SUCCESS             Successfully retrieved version.
  @retval  EFI_NOT_FOUND           RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   The RAM partition table is invalid.

**/
EFI_STATUS
GetRamPartitionVersion (
  IN VOID        *pRamPartitionTable,
  IN OUT UINT32  *Version
  );

/**
  Get partition entry by address.

  @param[in]      Address     The address to search for.
  @param[in,out]  TableEntry  Pointer to receive the partition entry.

  @retval  EFI_SUCCESS             Partition entry found.
  @retval  EFI_NOT_FOUND           No partition entry found for the address.
  @retval  EFI_INVALID_PARAMETER   TableEntry is NULL.

**/
EFI_STATUS
GetPartitionEntryByAddr (
  IN UINT64               Address,
  IN OUT MEM_REGION_INFO  *TableEntry
  );

/**
  Get RAM partition table entries.

  @param[in,out]  EntryCount  On input, number of available Table buffer entries.
                              On output, number of entries in the table.
  @param[in,out]  Table       Caller allocated, pointer to return table entries.

  @retval  EFI_SUCCESS             Successfully retrieved partition table entries.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   TableEntry is NULL.
  @retval  EFI_BUFFER_TOO_SMALL    Not enough buffer to return all available entries.

**/
EFI_STATUS
GetRamPartitions (
  IN OUT UINTN             *EntryCount,
  IN OUT  MEM_REGION_INFO  *Table
  );

/**
  Get Highest Bank bit from RAM partition table.

  @param[in,out]  HighBankBit  Return Highest Bank bit value.

  @retval  EFI_SUCCESS             Retrieved Highest Bank Bit from RAM partition.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
GetHighestBankBit (
  IN OUT UINT8  *HighBankBit
  );

/**
  Return total available DDR memory.

  @param[out]  MemoryCapacity  Return memory capacity.

  @retval  EFI_SUCCESS             Returned memory capacity.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
GetTotalPhysicalMemory (
  OUT UINTN  *MemoryCapacity
  );

/**
  Return total installed DDR memory.

  @param[out]  MemoryCapacity  Return memory capacity.

  @retval  EFI_SUCCESS             Returned memory capacity.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
GetInstalledPhysicalMemory (
  OUT UINTN  *MemoryCapacity
  );

/**
  Return total installed system DDR memory.

  @param[out]  MemoryCapacity  Return memory capacity.

  @retval  EFI_SUCCESS             Returned memory capacity.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
GetInstalledSDRAMMemory (
  OUT UINTN  *MemoryCapacity
  );

/**
  Return lowest physical start address of DDR memory.

  @param[out]  StartAddress  Return start address.

  @retval  EFI_SUCCESS             Returned start address.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
GetLowestPhysicalStartAddress (
  OUT UINTN  *StartAddress
  );

/**
  Get MinPasrSize from RAM partition table.

  @param[in,out]  MinPasrSize  Return MinPasrSize.

  @retval  EFI_SUCCESS             Retrieved MinPasrSize from RAM partition.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
GetMinPasrSize (
  IN OUT UINT32  *MinPasrSize
  );

/**
  Get Preloaded Ram partitions Information entries from Ram Partition table.

  @param[in,out]  EntryCount  On input, pointer to number(count) of available
                              preloaded ram partitions.
                              On output, the pointer(address it points to) will
                              be filled with number(count) of preloaded regions.
  @param[in,out]  Table       On input, Null pointer/Caller allocated of type
                              MEM_REGION_INFO.
                              On output, this table will be filled with values
                              of MEM_REGION_INFO members.

  @retval  EFI_SUCCESS             Successfully retrieved partition table entries.
  @retval  EFI_NOT_SUPPORTED       RAM partition table is not available.
  @retval  EFI_INVALID_PARAMETER   If EntryCount is NULL.
                                   If EntryCount(value at the address) is greater than zero and TableEntry is NULL.
  @retval  EFI_BUFFER_TOO_SMALL    Not enough buffer to return all available entries.

**/
EFI_STATUS
GetPreLoadedRamPartitions (
  IN OUT UINTN             *EntryCount,
  IN OUT  MEM_REGION_INFO  *Table
  );
