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

#define UNSUPPORTED_BELOW_VER  1

#define DEFAULT_MEM_REGION_ATTRIBUTE  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK

STATIC MEM_REGION_INFO  mRamPartitionTable[RAM_NUM_PART_ENTRIES];
STATIC MEM_REGION_INFO  mPreloadRamPartitionTable[RAM_NUM_PART_ENTRIES];
STATIC UINTN            mRamPartitionTableEntryCount        = 0;
STATIC UINTN            mPreloadRamPartitionTableEntryCount = 0;

/**
  Get the RAM partition table version.

  @param[in]      RamPartitionTable  Pointer to the RAM partition table.
  @param[in,out]  Version            Pointer to receive the version number.

  @retval  EFI_SUCCESS      Version retrieved successfully.
  @retval  EFI_NOT_FOUND    Invalid RAM partition table magic numbers.

**/
EFI_STATUS
EFIAPI
RamPartitionGetVersion (
  IN VOID        *RamPartitionTable,
  IN OUT UINT32  *Version
  )
{
  /* v0 and v1 have same header info */
  USABLE_RAM_PART_TABLE_TYPE  Table;

  Table = (USABLE_RAM_PART_TABLE_TYPE)RamPartitionTable;

  /* First, make sure the RAM partition table is valid */
  if ((Table->Magic1 == RAM_PART_MAGIC1) &&
      (Table->Magic2 == RAM_PART_MAGIC2))
  {
    *Version = Table->Version;
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Get pointer to the RAM partition table from SMEM.

  @param[in,out]  RamPartitionTable  Pointer to receive the RAM partition table address.
  @param[in,out]  Version            Pointer to receive the version number.

  @retval  EFI_SUCCESS       RAM partition table retrieved successfully.
  @retval  EFI_NOT_READY     SMEM not initialized.
  @retval  EFI_UNSUPPORTED   Deprecated RAM partition table version.
  @retval  Other             Error occurred during retrieval.

**/
STATIC
EFI_STATUS
RamPartitionGetRamPartitionTable (
  IN OUT VOID    **RamPartitionTable,
  IN OUT UINT32  *Version
  )
{
  EFI_STATUS  Status;
  UINT32      BufferSize;

  BufferSize = 0;

  /* Get the RAM partition table */
  *RamPartitionTable = SmemGetAddr (SmemUsableRamPartitionTable, (UINT32 *)&BufferSize);
  if (*RamPartitionTable == NULL) {
    DEBUG ((DEBUG_ERROR, "WARNING: Unable to read memory partition table from SMEM\n"));
    return EFI_NOT_READY;
  }

  Status = RamPartitionGetVersion (*RamPartitionTable, Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (*Version < UNSUPPORTED_BELOW_VER) {
    DEBUG ((DEBUG_WARN, "WARNING: deprecated RAM partition table !\n"));
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
EFIAPI
RamPartitionGetInstalledSdramMemory (
  UINTN  *MemoryCapacity
  )
{
  EFI_STATUS                  Status;
  VOID                        *RamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  Table;
  UINT32                      Index;
  UINT32                      Version;

  RamPartitionTable  = NULL;
  Version            = 0;

  if (MemoryCapacity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = RamPartitionGetRamPartitionTable (&RamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  Table = (USABLE_RAM_PART_TABLE_TYPE)RamPartitionTable;

  *MemoryCapacity = 0;
  for (Index = 0; Index < Table->NumPartitions; Index++) {
    if ((Table->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (Table->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      *MemoryCapacity += Table->RamPartEntry[Index].Length;
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
EFIAPI
RamPartitionGetInstalledPhysicalMemory (
  UINTN  *MemoryCapacity
  )
{
  EFI_STATUS                  Status;
  VOID                        *RamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  Table;
  UINT32                      Index;
  UINT32                      Version;
  
  RamPartitionTable = NULL;
  Version           = 0;

  if (MemoryCapacity == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  Status = RamPartitionGetRamPartitionTable (&RamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  Table = (USABLE_RAM_PART_TABLE_TYPE)RamPartitionTable;

  *MemoryCapacity = 0;
  for (Index = 0; Index < Table->NumPartitions; Index++) {
    *MemoryCapacity += Table->RamPartEntry[Index].Length;
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
EFIAPI
RamPartitionGetTotalPhysicalMemory (
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

  Status = RamPartitionGetRamPartitions (&EntryCount, EntryList);
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
EFIAPI
RamPartitionGetLowestPhysicalStartAddress (
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
  Status = RamPartitionGetRamPartitions (&EntryCount, EntryList);
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
RamPartitionGetPartitionsInRam (
  UINTN            *EntryCount,
  MEM_REGION_INFO  *EntryTable,
  BOOLEAN          Preloaded
  )
{
  UINTN            Index;
  UINTN            Count;
  MEM_REGION_INFO  *Table;

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

  for (Index = 0; Index < Count; Index++ ) {
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
EFIAPI
RamPartitionGetRamPartitions (
  UINTN            *EntryCount,
  MEM_REGION_INFO  *EntryTable
  )
{
  return RamPartitionGetPartitionsInRam (EntryCount, EntryTable, FALSE);
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
EFIAPI
RamPartitionGetPreLoadedRamPartitions (
  UINTN            *EntryCount,
  MEM_REGION_INFO  *EntryTable
  )
{
  return RamPartitionGetPartitionsInRam (EntryCount, EntryTable, TRUE);
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
EFIAPI
RamPartitionGetPartitionEntryByAddr (
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
EFIAPI
RamPartitionGetHighestBankBit (
  IN OUT UINT8  *HighBankBit
  )
{
  EFI_STATUS                  Status;
  VOID                        *RamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  Table;
  UINT32                      Index;
  UINT32                      Version;
  
  RamPartitionTable = NULL;
  Version           = 0;

  if (HighBankBit == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  Status = RamPartitionGetRamPartitionTable (&RamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  Table = (USABLE_RAM_PART_TABLE_TYPE)RamPartitionTable;
  for (Index = 0; Index < Table->NumPartitions; Index++) {
    if ((Table->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (Table->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      *HighBankBit = Table->RamPartEntry[Index].HighestBankBit;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Handle version 1 RAM partition table format.

  @param[in]  RamPartitionTable  Pointer to the RAM partition table.

  @retval  EFI_SUCCESS             Successfully processed version 1 partition table.
  @retval  EFI_INVALID_PARAMETER   RamPartitionTable is NULL.

**/
STATIC
EFI_STATUS
RamPartitionHandlePartitionVer1 (
  IN VOID  *RamPartitionTable
  )
{
  UINT32                         Index;
  USABLE_RAM_PART_TABLE_TYPE_V1  TableV1;
  MEM_REGION_INFO                *TableEntry;

  if (RamPartitionTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TableV1 = (USABLE_RAM_PART_TABLE_TYPE_V1)RamPartitionTable;

  /* Parse the DDR partition table, and fill in table for only DDR entries */
  for (Index = 0; Index < TableV1->NumPartitions; Index++ ) {
    RAM_PARTITION_CATEGORY  PartitionCategory;
    RAM_PARTITION_TYPE      PartitionType;
    UINT64                  StartAddr;
    UINT64                  Length;

    PartitionType     = TableV1->RamPartEntryV1[Index].PartitionType;
    PartitionCategory = TableV1->RamPartEntryV1[Index].PartitionCategory;
    StartAddr         = TableV1->RamPartEntryV1[Index].StartAddress;
    Length            = TableV1->RamPartEntryV1[Index].Length;

    if ((PartitionType == RamPartitionSysMemory) &&
        (PartitionCategory == RamPartitionSdram))
    {
      UINT64  EndAddr;

      if (mRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;
      }

      EndAddr = StartAddr + Length;

      /* Handle boundary condition for UINT32 rollover */
      if (EndAddr == 0x100000000ULL) {
        Length  = Length - 0x200000;
        EndAddr = StartAddr + Length;
      }

      TableEntry = &mRamPartitionTable[mRamPartitionTableEntryCount];
      AsciiStrCpyS (TableEntry->Name, MAX_MEM_LABEL_NAME, "RAM Partition");

      TableEntry->MemBase           = StartAddr;
      TableEntry->MemSize           = Length;
      TableEntry->BuildHobOption    = AddMem;
      TableEntry->ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
      TableEntry->ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
      TableEntry->MemoryType        = EfiConventionalMemory;
      TableEntry->CacheAttributes   = DEFAULT_MEM_REGION_ATTRIBUTE;
      TableEntry->PartitionType     = PartitionType;
      mRamPartitionTableEntryCount++;
    }
    /* Handle PreLoaded region */
    else if (((PartitionType ==  RamPartitionToolsFvMemory)    ||
              (PartitionType ==  RamPartitionQuantumFvMemory)  ||
              (PartitionType ==  RamPartitionQuestFvMemory)    ||
              (PartitionType ==  RamPartitionAppsMemory))      &&
             (PartitionCategory == RamPartitionSdram))
    {
      if (mPreloadRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mPreloadRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;  // Skip this entry but keep processing remaining partitions
      }

      TableEntry = &mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount];
      AsciiStrCpyS (TableEntry->Name, MAX_MEM_LABEL_NAME, "");

      TableEntry->MemBase           = StartAddr;
      TableEntry->MemSize           = Length;
      TableEntry->BuildHobOption    = NoBuildHob;
      TableEntry->ResourceType      = EFI_RESOURCE_MEMORY_RESERVED;
      TableEntry->ResourceAttribute = 0;                     /* No resource attribute */
      TableEntry->MemoryType        = EfiReservedMemoryType; /* Not used  */
      TableEntry->CacheAttributes   = 0;                     /* Not used  */
      TableEntry->PartitionType     = PartitionType;
      mPreloadRamPartitionTableEntryCount++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Initialize the RAM Partition Table Library.

  This function must be called before using any other library functions.
  It locates and parses the RAM partition table from SMEM.

  @retval  EFI_SUCCESS  Library initialized successfully.
  @retval  Other        Initialization failed.

**/
EFI_STATUS
EFIAPI
RamPartitionInitRamPartitionTableLib (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINT32                      Index;
  UINT32                      Version;
  VOID                        *RamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  Table;
  MEM_REGION_INFO             *TableEntry;

  mRamPartitionTableEntryCount        = 0;
  mPreloadRamPartitionTableEntryCount = 0;
  Version                             = 0;
  RamPartitionTable                   = NULL;

  SetMem (mRamPartitionTable, sizeof (mRamPartitionTable), 0);
  SetMem (mPreloadRamPartitionTable, sizeof (mPreloadRamPartitionTable), 0);

  Status = RamPartitionGetRamPartitionTable (&RamPartitionTable, &Version);
  if (Status == EFI_NOT_READY) {
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

  if (RamPartitionTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Version == 1) {
    Status = RamPartitionHandlePartitionVer1 (RamPartitionTable);
    return Status;
  }

  Table = (USABLE_RAM_PART_TABLE_TYPE)RamPartitionTable;
  if (Table == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  /* Parse the DDR partition table, and fill in table for only DDR entries */
  for ( Index = 0; Index < Table->NumPartitions; Index++ ) {
    RAM_PARTITION_CATEGORY  PartitionCategory;
    RAM_PARTITION_TYPE      PartitionType;
    UINT64                  StartAddr;

    PartitionCategory  = Table->RamPartEntry[Index].PartitionCategory;
    PartitionType      = Table->RamPartEntry[Index].PartitionType;
    StartAddr          = Table->RamPartEntry[Index].StartAddress;

    if ((PartitionType == RamPartitionSysMemory) &&
        (PartitionCategory == RamPartitionSdram))
    {
      UINT64  Length;
      UINT64  EndAddr;

      if (mRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;
      }

      Length = Table->RamPartEntry[Index].AvailableLength;

      EndAddr = StartAddr + Length;
      if (FixedPcdGet64 (PcdMaxMemory) != 0) {
        if (EndAddr > FixedPcdGet64 (PcdMaxMemory)) {
          continue;
        }
      }


      TableEntry = &mRamPartitionTable[mRamPartitionTableEntryCount];
      AsciiStrCpyS (TableEntry->Name, MAX_MEM_LABEL_NAME, "RAM Partition");

      TableEntry->MemBase           = StartAddr;
      TableEntry->MemSize           = Length;
      TableEntry->BuildHobOption    = AddMem;
      TableEntry->ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
      TableEntry->ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
      TableEntry->MemoryType        = EfiConventionalMemory;
      TableEntry->CacheAttributes   = DEFAULT_MEM_REGION_ATTRIBUTE;
      TableEntry->PartitionType     = PartitionType;
      mRamPartitionTableEntryCount++;

      ASSERT (mRamPartitionTableEntryCount <= RAM_NUM_PART_ENTRIES);
    }
    /* Handle PreLoaded region */
    else if (((PartitionType ==  RamPartitionToolsFvMemory)   ||
              (PartitionType ==  RamPartitionQuantumFvMemory) ||
              (PartitionType ==  RamPartitionQuestFvMemory)   ||
              (PartitionType ==  RamPartitionAblMemory)       ||
              (PartitionType ==  RamPartitionAppsMemory))     &&
             (PartitionCategory == RamPartitionSdram))

    {
      UINT64  Length;

      if (mPreloadRamPartitionTableEntryCount >= RAM_NUM_PART_ENTRIES) {
        ASSERT (mPreloadRamPartitionTableEntryCount < RAM_NUM_PART_ENTRIES);
        continue;
      }

      Length = Table->RamPartEntry[Index].Length;

      TableEntry = &mPreloadRamPartitionTable[mPreloadRamPartitionTableEntryCount];
      AsciiStrCpyS (TableEntry->Name, MAX_MEM_LABEL_NAME, "");

      TableEntry->MemBase           = StartAddr;
      TableEntry->MemSize           = Length;
      TableEntry->BuildHobOption    = NoBuildHob;
      TableEntry->ResourceType      = EFI_RESOURCE_MEMORY_RESERVED;
      TableEntry->ResourceAttribute = 0;                     /* No resource attribute */
      TableEntry->MemoryType        = EfiReservedMemoryType; /* Not used  */
      TableEntry->CacheAttributes   = 0;                     /* Not used  */
      TableEntry->PartitionType     = PartitionType;
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
EFIAPI
RamPartitionGetMinPasrSize (
  IN OUT UINT32  *MinPasrSize
  )
{
  EFI_STATUS                  Status;
  VOID                        *RamPartitionTable;
  USABLE_RAM_PART_TABLE_TYPE  Table;
  UINT32                      Index;
  UINT32                      Version;

  RamPartitionTable  = NULL;
  Version            = 0;

  if (MinPasrSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  Status = RamPartitionGetRamPartitionTable (&RamPartitionTable, &Version);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RamPartitionTable == NULL) {
    return EFI_NOT_FOUND;
  }

  Table = (USABLE_RAM_PART_TABLE_TYPE)RamPartitionTable;
  for (Index = 0; Index < Table->NumPartitions; Index++) {
    if ((Table->RamPartEntry[Index].PartitionType == RamPartitionSysMemory) &&
        (Table->RamPartEntry[Index].PartitionCategory == RamPartitionSdram))
    {
      *MinPasrSize = Table->RamPartEntry[Index].MinPasrSize;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
