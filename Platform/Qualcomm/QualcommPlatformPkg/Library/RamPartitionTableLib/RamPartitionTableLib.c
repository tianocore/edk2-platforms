/** @file
  Qualcomm RAM Partition Table Library implementation.

  This library provides functions to access and manage RAM partition tables,
  including querying partition information, memory sizes, and partition entries.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include <Library/RamPartitionTableLib.h>
#include <Library/SmemLib.h>

#include <MemRegionInfo.h>
#include <RamPartition.h>

#define DEFAULT_MEM_REGION_ATTRIBUTE  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK

STATIC MEM_REGION_INFO  mRamPartitionTable[RAM_NUM_PART_ENTRIES];
STATIC MEM_REGION_INFO  mPreloadRamPartitionTable[RAM_NUM_PART_ENTRIES];
STATIC UINTN            mRamPartitionTableEntryCount        = 0;
STATIC UINTN            mPreloadRamPartitionTableEntryCount = 0;

/**
  Get the RAM partition table version.

  @param[in]      pRamPartitionTable  Pointer to the RAM partition table.
  @param[in,out]  Version             Pointer to receive the version number.

  @retval  EFI_SUCCESS      Version retrieved successfully.
  @retval  EFI_NOT_FOUND    Invalid RAM partition table magic numbers.

**/
EFI_STATUS
GetRamPartitionVersion (
  IN VOID        *pRamPartitionTable,
  IN OUT UINT32  *Version
  )
{
  /* v0 and v1 have same header info */
  USABLE_RAM_PART_TABLE_TYPE  pRamPartTable;

  pRamPartTable = (USABLE_RAM_PART_TABLE_TYPE)pRamPartitionTable;

  /* First, make sure the RAM partition table is valid */
  if ((pRamPartTable->Magic1 == RAM_PART_MAGIC1) &&
      (pRamPartTable->Magic2 == RAM_PART_MAGIC2))
  {
    *Version = pRamPartTable->Version;
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

#define UNSUPPORTED_BELOW_VER  1

/**
  Get pointer to the RAM partition table from SMEM.

  @param[in,out]  pRamPartitionTable  Pointer to receive the RAM partition table address.
  @param[in,out]  Version             Pointer to receive the version number.

  @retval  EFI_SUCCESS       RAM partition table retrieved successfully.
  @retval  EFI_NOT_READY     SMEM not initialized.
  @retval  EFI_UNSUPPORTED   Deprecated RAM partition table version.
  @retval  Other             Error occurred during retrieval.

**/
STATIC
EFI_STATUS
GetRamPartitionTable (
  IN OUT VOID    **pRamPartitionTable,
  IN OUT UINT32  *Version
  )
{
  EFI_STATUS  Status;
  UINT32      RamPartitionBuffSz;

  RamPartitionBuffSz = 0;

  /* Get the RAM partition table */
  *pRamPartitionTable = SmemGetAddr (SmemUsableRamPartitionTable, (UINT32 *)&RamPartitionBuffSz);
  if (*pRamPartitionTable == NULL) {
    /* NOTE: We should be here only if SMEM is not initialized */
    DEBUG ((DEBUG_ERROR, "WARNING: Unable to read memory partition table from SMEM\n"));
    return EFI_NOT_READY;
  }

  Status = GetRamPartitionVersion (*pRamPartitionTable, Version);
  if (Status != EFI_SUCCESS) {
    /* Invalid RAM Partition, return early */
    return Status;
  }

  if (*Version < UNSUPPORTED_BELOW_VER) {
    DEBUG ((DEBUG_WARN, "WARNING: Using deprecated RAM partition table !\n"));
    CpuDeadLoop ();
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Get the total installed SDRAM memory size.

  @param[out]  MemoryCapacity  Pointer to receive the total SDRAM memory size.

  @retval  EFI_SUCCESS             Memory capacity retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   MemoryCapacity is NULL.
  @retval  Other                   Error occurred during retrieval.

**/
EFI_STATUS
GetInstalledSDRAMMemory (
  UINTN  *MemoryCapacity
  )
{
  EFI_STATUS                  Status;
  VOID                        *pRamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  pRamPartTable;
  UINT32                      Index;
  UINT32                      Version;

  if (MemoryCapacity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  pRamPartitionTable = NULL;
  Version            = 0;

  Status = GetRamPartitionTable (&pRamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (pRamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  pRamPartTable = (USABLE_RAM_PART_TABLE_TYPE)pRamPartitionTable;

  *MemoryCapacity = 0;
  for (Index = 0; Index < pRamPartTable->NumPartitions; Index++) {
    if ((pRamPartTable->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (pRamPartTable->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      *MemoryCapacity += pRamPartTable->RamPartEntry[Index].Length;
    }
  }

  return EFI_SUCCESS;
}

/**
  Get the total installed physical memory size.

  @param[out]  MemoryCapacity  Pointer to receive the total physical memory size.

  @retval  EFI_SUCCESS             Memory capacity retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   MemoryCapacity is NULL.
  @retval  Other                   Error occurred during retrieval.

**/
EFI_STATUS
GetInstalledPhysicalMemory (
  UINTN  *MemoryCapacity
  )
{
  EFI_STATUS                  Status;
  VOID                        *pRamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  pRamPartTable;
  UINT32                      Index;
  UINT32                      Version;

  if (MemoryCapacity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  pRamPartitionTable = NULL;
  Version            = 0;

  Status = GetRamPartitionTable (&pRamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (pRamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  pRamPartTable = (USABLE_RAM_PART_TABLE_TYPE)pRamPartitionTable;

  *MemoryCapacity = 0;
  for (Index = 0; Index < pRamPartTable->NumPartitions; Index++) {
    *MemoryCapacity += pRamPartTable->RamPartEntry[Index].Length;
  }

  return EFI_SUCCESS;
}

/**
  Get the total physical memory size across all partitions.

  @param[out]  MemoryCapacity  Pointer to receive the total physical memory size.

  @retval  EFI_SUCCESS             Memory capacity retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   MemoryCapacity is NULL.
  @retval  Other                   Error occurred during retrieval.

**/
EFI_STATUS
GetTotalPhysicalMemory (
  UINTN  *MemoryCapacity
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  EntryList[RAM_NUM_PART_ENTRIES];
  UINTN            EntryCount;
  UINT32           Index;

  if (MemoryCapacity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SetMem (EntryList, sizeof (EntryList), 0);
  EntryCount = RAM_NUM_PART_ENTRIES;
  Index      = 0;

  Status = GetRamPartitions (&EntryCount, &EntryList[0]);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  *MemoryCapacity = 0;
  for (Index = 0; Index < EntryCount; Index++) {
    *MemoryCapacity += EntryList[Index].MemSize;
  }

  return EFI_SUCCESS;
}

/**
  Get the lowest physical start address from all RAM partitions.

  @param[out]  StartAddress  Pointer to receive the lowest physical start address.

  @retval  EFI_SUCCESS             Start address retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   StartAddress is NULL.
  @retval  Other                   Error occurred during retrieval.

**/
EFI_STATUS
GetLowestPhysicalStartAddress (
  UINTN  *StartAddress
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  EntryList[RAM_NUM_PART_ENTRIES];
  UINTN            EntryCount;
  UINT32           Index;

  if (StartAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SetMem (EntryList, sizeof (EntryList), 0);
  EntryCount = RAM_NUM_PART_ENTRIES;
  Index      = 0;

  Status = GetRamPartitions (&EntryCount, &EntryList[0]);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (EntryCount == 0) {
    return EFI_NOT_FOUND;
  }

  *StartAddress = MAX_UINTN;
  for (Index = 0; Index < EntryCount; Index++) {
    if (EntryList[Index].MemBase < *StartAddress) {
      *StartAddress = EntryList[Index].MemBase;
    }
  }

  return EFI_SUCCESS;
}

/**
  Get the number of partitions in RAM.

  This is an internal helper function.

  @param[in,out]  EntryCount  On input, size of EntryTable array. On output, number of entries returned.
  @param[out]     EntryTable  Pointer to array to receive partition entries.
  @param[in]      Preloaded   TRUE to get preloaded partitions, FALSE for regular partitions.

  @retval  EFI_SUCCESS             Partitions retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   Invalid parameters.
  @retval  EFI_BUFFER_TOO_SMALL    EntryTable too small, required size returned in EntryCount.

**/
STATIC
EFI_STATUS
GetPartitionsInRam (
  UINTN            *EntryCount,
  MEM_REGION_INFO  *EntryTable,
  BOOLEAN          Preloaded
  )
{
  UINTN            Index;
  UINTN            Count;
  MEM_REGION_INFO  *Table;

  Index = 0;
  Count = mRamPartitionTableEntryCount;
  Table = mRamPartitionTable;

  if (Preloaded) {
    Count = mPreloadRamPartitionTableEntryCount;
    Table = mPreloadRamPartitionTable;
  }

  if (EntryCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*EntryCount > 0) && (EntryTable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Count > *EntryCount) {
    *EntryCount = Count;
    return EFI_BUFFER_TOO_SMALL;
  }

  for ( Index = 0; Index < Count; Index++ ) {
    AsciiStrCpyS (EntryTable[Index].Name, MAX_MEM_LABEL_NAME, Table[Index].Name);

    EntryTable[Index].MemBase           = Table[Index].MemBase;
    EntryTable[Index].MemSize           = Table[Index].MemSize;
    EntryTable[Index].BuildHobOption    = Table[Index].BuildHobOption;
    EntryTable[Index].ResourceType      = Table[Index].ResourceType;
    EntryTable[Index].ResourceAttribute = Table[Index].ResourceAttribute;
    EntryTable[Index].MemoryType        = Table[Index].MemoryType;
    EntryTable[Index].CacheAttributes   = Table[Index].CacheAttributes;
    EntryTable[Index].PartitionType     = Table[Index].PartitionType;
  }

  *EntryCount = Count;

  return EFI_SUCCESS;
}

/**
  Get RAM partition entries.

  @param[in,out]  EntryCount  On input, size of EntryTable array. On output, number of entries returned.
  @param[out]     EntryTable  Pointer to array to receive partition entries.

  @retval  EFI_SUCCESS             Partitions retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   Invalid parameters.
  @retval  EFI_BUFFER_TOO_SMALL    EntryTable too small, required size returned in EntryCount.

**/
EFI_STATUS
GetRamPartitions (
  UINTN            *EntryCount,
  MEM_REGION_INFO  *EntryTable
  )
{
  return GetPartitionsInRam (EntryCount, EntryTable, FALSE);
}

/**
  Get pre-loaded RAM partition entries.

  @param[in,out]  EntryCount  On input, size of EntryTable array. On output, number of entries returned.
  @param[out]     EntryTable  Pointer to array to receive pre-loaded partition entries.

  @retval  EFI_SUCCESS             Partitions retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   Invalid parameters.
  @retval  EFI_BUFFER_TOO_SMALL    EntryTable too small, required size returned in EntryCount.

**/
EFI_STATUS
GetPreLoadedRamPartitions (
  UINTN            *EntryCount,
  MEM_REGION_INFO  *EntryTable
  )
{
  return GetPartitionsInRam (EntryCount, EntryTable, TRUE);
}

/**
  Get partition entry by address.

  @param[in]      Address    The address to search for.
  @param[in,out]  FirstBank  Pointer to receive the partition entry.

  @retval  EFI_SUCCESS             Partition entry found.
  @retval  EFI_NOT_FOUND           No partition entry found for the address.
  @retval  EFI_INVALID_PARAMETER   FirstBank pointer is NULL.

**/
EFI_STATUS
GetPartitionEntryByAddr (
  IN UINT64               Address,
  IN OUT MEM_REGION_INFO  *FirstBank
  )
{
  UINTN   Index;
  UINT64  MemEndAddr;

  if (FirstBank == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mRamPartitionTableEntryCount > RAM_NUM_PART_ENTRIES) {
    return EFI_DEVICE_ERROR;
  }

  Index      = 0;
  MemEndAddr = 0;

  for (Index = 0; Index < mRamPartitionTableEntryCount; Index++ ) {
    MemEndAddr = mRamPartitionTable[Index].MemBase + mRamPartitionTable[Index].MemSize;
    if ((Address >= mRamPartitionTable[Index].MemBase) && (Address <= MemEndAddr)) {
      *FirstBank = mRamPartitionTable[Index];
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get the highest bank bit from RAM partition table.

  @param[in,out]  HighBankBit  Pointer to receive the highest bank bit value.

  @retval  EFI_SUCCESS             Highest bank bit retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   HighBankBit is NULL.
  @retval  EFI_NOT_FOUND           No SDRAM partition found.
  @retval  Other                   Error occurred during retrieval.

**/
EFI_STATUS
GetHighestBankBit (
  IN OUT UINT8  *HighBankBit
  )
{
  EFI_STATUS                  Status;
  VOID                        *pRamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  pRamPartTable;
  UINT32                      Index;
  UINT32                      Version;

  if (HighBankBit == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  pRamPartitionTable = NULL;
  Version            = 0;

  Status = GetRamPartitionTable (&pRamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (pRamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  pRamPartTable = (USABLE_RAM_PART_TABLE_TYPE)pRamPartitionTable;
  for (Index = 0; Index < pRamPartTable->NumPartitions; Index++) {
    if ((pRamPartTable->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (pRamPartTable->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      *HighBankBit = pRamPartTable->RamPartEntry[Index].HighestBankBit;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Handle version 1 RAM partition table format.

  @param[in]  pRamPartitionTable  Pointer to the RAM partition table.

  @retval  EFI_SUCCESS             Successfully processed version 1 partition table.
  @retval  EFI_INVALID_PARAMETER   pRamPartitionTable is NULL.

**/
STATIC
EFI_STATUS
HandlePartitionVer1 (
  IN VOID  *pRamPartitionTable
  )
{
  EFI_STATUS                     Status;
  UINT32                         Index;
  USABLE_RAM_PART_TABLE_TYPE_V1  pRamPartTablev1;

  if (pRamPartitionTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status          = EFI_SUCCESS;
  Index           = 0;
  pRamPartTablev1 = (USABLE_RAM_PART_TABLE_TYPE_V1)pRamPartitionTable;

  /* Parse the DDR partition table, and fill in table for only DDR entries */
  for ( Index = 0; Index < pRamPartTablev1->NumPartitions; Index++ ) {
    RAM_PARTITION_TYPE  PartitionType;
    UINT64              StartAddr;
    UINT64              Length;

    PartitionType = pRamPartTablev1->RamPartEntryV1[Index].PartitionType;
    StartAddr     = pRamPartTablev1->RamPartEntryV1[Index].StartAddress;
    Length        = pRamPartTablev1->RamPartEntryV1[Index].Length;

    if ((pRamPartTablev1->RamPartEntryV1[Index].PartitionType == RamPartitionSysMemory) &&
        (pRamPartTablev1->RamPartEntryV1[Index].PartitionCategory == RamPartitionSdram))
    {
      UINT64  EndAddr;

      EndAddr = StartAddr + Length;

      /* Handle boundary condition for UINT32 rollover */
      if (EndAddr == 0x100000000ULL) {
        Length  = Length - 0x200000;
        EndAddr = StartAddr + Length;
      }

      if (mRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;
      }

      AsciiStrCpyS (mRamPartitionTable[mRamPartitionTableEntryCount].Name, MAX_MEM_LABEL_NAME, "RAM Partition");

      mRamPartitionTable[mRamPartitionTableEntryCount].MemBase           = StartAddr;
      mRamPartitionTable[mRamPartitionTableEntryCount].MemSize           = Length;
      mRamPartitionTable[mRamPartitionTableEntryCount].BuildHobOption    = AddMem;
      mRamPartitionTable[mRamPartitionTableEntryCount].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
      mRamPartitionTable[mRamPartitionTableEntryCount].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
      mRamPartitionTable[mRamPartitionTableEntryCount].MemoryType        = EfiConventionalMemory;
      mRamPartitionTable[mRamPartitionTableEntryCount].CacheAttributes   = DEFAULT_MEM_REGION_ATTRIBUTE;
      mRamPartitionTable[mRamPartitionTableEntryCount].PartitionType     = PartitionType;
      mRamPartitionTableEntryCount++;
    }
    /* Handle PreLoaded region */
    else if (((pRamPartTablev1->RamPartEntryV1[Index].PartitionType ==  RamPartitionToolsFvMemory)    ||
              (pRamPartTablev1->RamPartEntryV1[Index].PartitionType ==  RamPartitionQuantumFvMemory)  ||
              (pRamPartTablev1->RamPartEntryV1[Index].PartitionType ==  RamPartitionQuestFvMemory)    ||
              (pRamPartTablev1->RamPartEntryV1[Index].PartitionType ==  RamPartitionAppsMemory))      &&
             (pRamPartTablev1->RamPartEntryV1[Index].PartitionCategory == RamPartitionSdram))
    {
      if (mPreloadRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mPreloadRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;  // Skip this entry but keep processing remaining partitions
      }

      AsciiStrCpyS (mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].Name, MAX_MEM_LABEL_NAME, "");

      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].MemBase           = StartAddr;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].MemSize           = Length;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].BuildHobOption    = NoBuildHob;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].ResourceType      = EFI_RESOURCE_MEMORY_RESERVED;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].ResourceAttribute = 0;                     /* No resource attribute */
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].MemoryType        = EfiReservedMemoryType; /* Not used  */
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].CacheAttributes   = 0;                     /* Not used  */
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].PartitionType     = PartitionType;
      mPreloadRamPartitionTableEntryCount++;
    }
  }

  return Status;
}

/**
  Initialize the RAM Partition Table Library.

  This function must be called before using any other library functions.
  It locates and parses the RAM partition table from SMEM.

  @retval  EFI_SUCCESS  Library initialized successfully.
  @retval  Other        Initialization failed.

**/
EFI_STATUS
InitRamPartitionTableLib (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINT32                      Index;
  UINT32                      Version;
  VOID                        *pRamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  pRamPartTable;

  SetMem (mRamPartitionTable, sizeof (mRamPartitionTable), 0);
  SetMem (mPreloadRamPartitionTable, sizeof (mPreloadRamPartitionTable), 0);
  mRamPartitionTableEntryCount        = 0;
  mPreloadRamPartitionTableEntryCount = 0;

  Status             = EFI_SUCCESS;
  Index              = 0;
  Version            = 0;
  pRamPartitionTable = NULL;

  Status = GetRamPartitionTable (&pRamPartitionTable, &Version);
  if (Status == EFI_NOT_READY) {
    /* NOTE: Get here only on presilicon, if RAM partition is not available, and using NULL Libs */
    mRamPartitionTable[0].MemBase           = FixedPcdGet64 (PcdSystemMemoryBase);
    mRamPartitionTable[0].MemSize           = FixedPcdGet64 (PcdSystemMemorySize);
    mRamPartitionTable[0].BuildHobOption    = AddMem;
    mRamPartitionTable[0].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
    mRamPartitionTable[0].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
    mRamPartitionTable[0].MemoryType        = EfiConventionalMemory;
    mRamPartitionTable[0].CacheAttributes   = DEFAULT_MEM_REGION_ATTRIBUTE;
    mRamPartitionTableEntryCount++;
    return EFI_SUCCESS;
  }

  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (pRamPartitionTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Version == 1) {
    Status = HandlePartitionVer1 (pRamPartitionTable);
    return Status;
  }

  pRamPartTable = (USABLE_RAM_PART_TABLE_TYPE)pRamPartitionTable;
  if (pRamPartTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  /* Parse the DDR partition table, and fill in table for only DDR entries */
  for ( Index = 0; Index < pRamPartTable->NumPartitions; Index++ ) {
    RAM_PARTITION_TYPE  PartitionType;
    UINT64              StartAddr;

    PartitionType = pRamPartTable->RamPartEntry[Index].PartitionType;
    StartAddr     = pRamPartTable->RamPartEntry[Index].StartAddress;

    if ((pRamPartTable->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (pRamPartTable->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      UINT64  Length;
      UINT64  EndAddr;

      Length = pRamPartTable->RamPartEntry[Index].AvailableLength;

      EndAddr = StartAddr + Length;
      if (FixedPcdGet64 (PcdMaxMemory) != 0) {
        if (EndAddr > FixedPcdGet64 (PcdMaxMemory)) {
          continue;
        }
      }

      if (mRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;
      }

      AsciiStrCpyS (mRamPartitionTable[mRamPartitionTableEntryCount].Name, MAX_MEM_LABEL_NAME, "RAM Partition");

      mRamPartitionTable[mRamPartitionTableEntryCount].MemBase           = StartAddr;
      mRamPartitionTable[mRamPartitionTableEntryCount].MemSize           = Length;
      mRamPartitionTable[mRamPartitionTableEntryCount].BuildHobOption    = AddMem;
      mRamPartitionTable[mRamPartitionTableEntryCount].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
      mRamPartitionTable[mRamPartitionTableEntryCount].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
      mRamPartitionTable[mRamPartitionTableEntryCount].MemoryType        = EfiConventionalMemory;
      mRamPartitionTable[mRamPartitionTableEntryCount].CacheAttributes   = DEFAULT_MEM_REGION_ATTRIBUTE;
      mRamPartitionTable[mRamPartitionTableEntryCount].PartitionType     = PartitionType;
      mRamPartitionTableEntryCount++;

      ASSERT (mRamPartitionTableEntryCount <= RAM_NUM_PART_ENTRIES);
    }
    /* Handle PreLoaded region */
    else if (((pRamPartTable->RamPartEntry[Index].PartitionType ==  RamPartitionToolsFvMemory)   ||
              (pRamPartTable->RamPartEntry[Index].PartitionType ==  RamPartitionQuantumFvMemory) ||
              (pRamPartTable->RamPartEntry[Index].PartitionType ==  RamPartitionQuestFvMemory)   ||
              (pRamPartTable->RamPartEntry[Index].PartitionType ==  RamPartitionAblMemory)       ||
              (pRamPartTable->RamPartEntry[Index].PartitionType ==  RamPartitionAppsMemory))     &&
             (pRamPartTable->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))

    {
      UINT64  Length;

      Length = pRamPartTable->RamPartEntry[Index].Length;

      if (mPreloadRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mPreloadRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;
      }

      AsciiStrCpyS (mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].Name, MAX_MEM_LABEL_NAME, "");

      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].MemBase           = StartAddr;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].MemSize           = Length;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].BuildHobOption    = NoBuildHob;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].ResourceType      = EFI_RESOURCE_MEMORY_RESERVED;
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].ResourceAttribute = 0;                     /* No resource attribute */
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].MemoryType        = EfiReservedMemoryType; /* Not used  */
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].CacheAttributes   = 0;                     /* Not used  */
      mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount].PartitionType     = PartitionType;
      mPreloadRamPartitionTableEntryCount++;

      ASSERT (mPreloadRamPartitionTableEntryCount <= RAM_NUM_PART_ENTRIES);
    }
  }

  return EFI_SUCCESS;
}

/**
  Get the minimum PASR (Partial Array Self Refresh) size.

  @param[in,out]  MinPasrSize  Pointer to receive the minimum PASR size.

  @retval  EFI_SUCCESS             Minimum PASR size retrieved successfully.
  @retval  EFI_INVALID_PARAMETER   MinPasrSize is NULL.
  @retval  EFI_NOT_FOUND           No SDRAM partition found.
  @retval  Other                   Error occurred during retrieval.

**/
EFI_STATUS
GetMinPasrSize (
  IN OUT UINT32  *MinPasrSize
  )
{
  EFI_STATUS                  Status;
  VOID                        *pRamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  pRamPartTable;
  UINT32                      Index;
  UINT32                      Version;

  if (MinPasrSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  pRamPartitionTable = NULL;
  Version            = 0;

  Status = GetRamPartitionTable (&pRamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (pRamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  pRamPartTable = (USABLE_RAM_PART_TABLE_TYPE)pRamPartitionTable;
  for (Index = 0; Index < pRamPartTable->NumPartitions; Index++) {
    if ((pRamPartTable->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (pRamPartTable->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      *MinPasrSize = pRamPartTable->RamPartEntry[Index].MinPasrSize;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
