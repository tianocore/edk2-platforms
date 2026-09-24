/** @file
  Parser for UEFI platform configuration data.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DtFrameworkLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RamPartitionTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>
#include <Library/SortLib.h>

#include <PlatformConfiguration.h>
#include <PlatformDeviceTree.h>

STATIC MEM_REGION_INFO  *mMemRegions        = NULL;
STATIC UINTN            mNumMemRegions      = 0;
STATIC UINTN            mNumMemoryMapRegion = 0;
STATIC UINT64           mMemMapLow          = 0xFFFFFFFFFFFFFFFFULL;
STATIC UINT64           mMemMapHigh         = 0;
STATIC BOOLEAN          mIsMemMapHighNoMap  = FALSE;

STATIC STRING_CONFIGURATION_PAIR   *mStrConfigTable          = NULL;
STATIC INTEGER_CONFIGURATION_PAIR  *mIntConfigTable          = NULL;
STATIC UINTN                       mStrConfigTableEntryCount = 0;
STATIC UINTN                       mIntConfigTableEntryCount = 0;

/**
  Allocates and zeros a buffer.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
STATIC VOID*
AllocateZeroPoolNoFree (
  IN UINTN  AllocationSize
  )
{
  VOID  *Memory;

  Memory = AllocatePages (EFI_SIZE_TO_PAGES (AllocationSize));
  if (Memory != NULL) {
    ZeroMem (Memory, AllocationSize);
  }

  return Memory;
}

/**
  Allocate memory without freeing to service multiple small allocation.
  This reduces number of HOBs.

  Memory is allocated from a static page-aligned pool and is never freed.

  @param[in]  Size  Number of bytes to allocate.

  @return  Pointer to allocated memory or NULL if allocation fails.

**/
STATIC VOID *
AllocateMemNoFree (
  UINTN  Size
  )
{
  STATIC UINT8  *FreeBufferPtr;
  STATIC UINT8  *EndPtr;
  UINT8         *AllocatedPtr;

  if (Size == 0) {
    return NULL;
  }

  if (Size >= EFI_PAGE_SIZE) {
    return AllocateZeroPoolNoFree (Size);
  }

  if (FreeBufferPtr == NULL) {
    if ((FreeBufferPtr = AllocateZeroPoolNoFree (EFI_PAGE_SIZE)) == NULL) {
      DEBUG ((DEBUG_WARN, "MemoryAlloc failed\n"));
      return NULL;
    }

    EndPtr = FreeBufferPtr + EFI_PAGE_SIZE;
  }

  Size = (Size + 7) & (~7);
  if (FreeBufferPtr + Size > EndPtr) {
    if ((FreeBufferPtr = AllocateZeroPoolNoFree (EFI_PAGE_SIZE)) == NULL) {
      DEBUG ((DEBUG_WARN, "MemoryAlloc failed\n"));
      ASSERT (FreeBufferPtr != NULL);
      return NULL;
    }

    EndPtr = FreeBufferPtr + EFI_PAGE_SIZE;
  }

  AllocatedPtr   = FreeBufferPtr;
  FreeBufferPtr += Size;
  return AllocatedPtr;
}

/**
  Read a UINT64 max-count property from a device tree node and use it to
  allocate and zero a configuration table.

  @param[in]   NodePath       Device tree path of the node holding the count property.
  @param[in]   CountPropName  Name of the UINT64 property giving the number of entries.
  @param[in]   PairSize       Size in bytes of a single table entry.
  @param[out]  ConfigTable    Receives the pointer to the newly allocated, zeroed table.
  @param[out]  MaxPairCount   Receives the validated maximum entry count.

  @retval  EFI_SUCCESS      Table allocated and zeroed successfully.
  @retval  EFI_LOAD_ERROR   Failed to read the count property, the count was out of
                            range, or allocation failed.

**/
STATIC EFI_STATUS
AllocateConfigTableFromDT (
  IN  CHAR8   *NodePath,
  IN  CHAR8   *CountPropName,
  IN  UINTN   PairSize,
  OUT VOID    **ConfigTable,
  OUT UINT64  *MaxPairCount
  )
{
  INT32           FdtStatus;
  DT_NODE_HANDLE  Node;
  UINTN           AllocSize;

  FdtStatus = FdtGetNodeHandle (&Node, NodePath);
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  FdtStatus = DtFrameworkGetUint64Prop (&Node, CountPropName, MaxPairCount);
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  if ((*MaxPairCount == 0) || (*MaxPairCount > 0x10000ULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "AllocateConfigTableFromDT: %a %lu out of range\n",
      CountPropName,
      *MaxPairCount
      ));
    return EFI_LOAD_ERROR;
  }

  if (*MaxPairCount > MAX_UINTN / PairSize) {
    DEBUG ((DEBUG_ERROR, "AllocateConfigTableFromDT: allocation size overflow\n"));
    return EFI_LOAD_ERROR;
  }

  AllocSize    = (UINTN)(*MaxPairCount) * PairSize;
  *ConfigTable = AllocateMemNoFree (AllocSize);
  if (*ConfigTable == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate enough memory for config table \n"));
    ASSERT (*ConfigTable != NULL);
    return EFI_LOAD_ERROR;
  }

  SetMem (*ConfigTable, AllocSize, 0);

  return EFI_SUCCESS;
}

/**
  Extract the next NUL-terminated property name from a packed property-name
  buffer into a newly allocated string, and advance past it.

  @param[in,out]  BuffWalk             On input, the current position in the buffer.
                                        On output, advanced past the extracted name.
  @param[in,out]  RemainingBufferSize  On input, bytes remaining in the buffer.
                                        On output, updated after consuming the name.
  @param[in]      CallerName           Name of the calling function, used for debug messages.
  @param[out]     Key                  Receives a newly allocated copy of the property name.

  @retval  EFI_SUCCESS      Property name extracted successfully.
  @retval  EFI_LOAD_ERROR   Malformed buffer or allocation failure.

**/
STATIC EFI_STATUS
GetNextPropNameFromBuff (
  IN OUT CHAR8   **BuffWalk,
  IN OUT UINT32  *RemainingBufferSize,
  IN     CHAR8   *CallerName,
  OUT    CHAR8   **Key
  )
{
  UINT32  WalkLength;

  WalkLength = AsciiStrLen (*BuffWalk);

  if (WalkLength >= MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "%a: WalkLength overflow\n", CallerName));
    return EFI_LOAD_ERROR;
  }

  if ((WalkLength + 1) > *RemainingBufferSize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: malformed PropNameBuff, WalkLength+1 exceeds remaining size\n",
      CallerName
      ));
    return EFI_LOAD_ERROR;
  }

  *Key = (CHAR8 *)AllocateMemNoFree (WalkLength + 1);
  if (*Key == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Key\n", CallerName));
    return EFI_LOAD_ERROR;
  }

  AsciiStrCpyS (*Key, WalkLength + 1, *BuffWalk);

  *BuffWalk             = *BuffWalk + WalkLength + 1;
  *RemainingBufferSize -= WalkLength + 1;

  return EFI_SUCCESS;
}

/**
  This function parses device tree for Int Configuration parameter entries
  into IntConfigTable Structure and the count of entries into IntConfigTableEntryCount.

  @retval EFI_SUCCESS           IntConfigTable Structure is updated with all Int ConfigParams.
  @retval EFI_LOAD_ERROR        Failed to update aall Int ConfigParams to IntConfigTable Struct.

**/
STATIC EFI_STATUS
ParseIntConfigEntriesFromDT (
  VOID
  )
{
  EFI_STATUS      Status;
  INT32           FdtStatus;
  DT_NODE_HANDLE  Node;
  UINT64          MaxIntConfigPairCount;
  UINT32          PropNameBufferSize;
  CHAR8           *Buff;
  CHAR8           *BuffWalk;
  UINT64          Index;

  MaxIntConfigPairCount = 0;
  PropNameBufferSize    = 0;
  Buff                  = NULL;
  BuffWalk              = NULL;
  Index                 = 0;

  // Check if table is empty and Allocate table based of the total count
  if (mIntConfigTable == NULL) {
    Status = AllocateConfigTableFromDT (
               "/sw/uefi/int_param",
               "MaxCount",
               sizeof (INTEGER_CONFIGURATION_PAIR),
               (VOID **)&mIntConfigTable,
               &MaxIntConfigPairCount
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // Update the table with Key and Value Pairs
  FdtStatus = FdtGetNodeHandle (&Node, "/sw/uefi/int_param");
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  FdtStatus = DtFrameworkGetPropNamesSizeOfNode (&Node, &PropNameBufferSize);
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  if (PropNameBufferSize == 0) {
    mIntConfigTableEntryCount = 0;
    return EFI_SUCCESS;
  }

  Buff = AllocateMemNoFree (PropNameBufferSize);
  if (Buff == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate PropNameBuff\n"));
    ASSERT (Buff != NULL);
    return EFI_LOAD_ERROR;
  }

  FdtStatus = DtFrameworkGetPropNamesOfNode (&Node, Buff, PropNameBufferSize);
  if (FdtStatus != DTB_ERR_NOERROR) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetPropNamesOfNode failed: %d\n", FdtStatus));
    return EFI_LOAD_ERROR;
  }

  // Walk through buffer and store property names in Table
  BuffWalk = (CHAR8 *)Buff;
  while (PropNameBufferSize != 0) {
    if (Index >= MaxIntConfigPairCount) {
      DEBUG ((DEBUG_ERROR, "ParseIntConfigEntriesFromDT: Index %lu exceeds MaxCount\n", Index));
      break;
    }

    Status = GetNextPropNameFromBuff (
               &BuffWalk,
               &PropNameBufferSize,
               "ParseIntConfigEntriesFromDT",
               &mIntConfigTable[Index].Key
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FdtStatus = DtFrameworkGetUint64Prop (&Node, mIntConfigTable[Index].Key, &mIntConfigTable[Index].Value);
    if (FdtStatus != DTB_ERR_NOERROR) {
      DEBUG ((DEBUG_WARN, "DtFrameworkGetUint64Prop failed for key %a: %d\n", mIntConfigTable[Index].Key, FdtStatus));
    }

    Index++;
  }

  mIntConfigTableEntryCount = Index;

  return EFI_SUCCESS;
}

/**
  This function parses device tree for Str Configuration parameter entries
  into StrConfigTable Structure and the count of entries into StrConfigTableEntryCount.

  @retval EFI_SUCCESS           StrConfigTable Structure is updated with all Str ConfigParams.
  @retval EFI_LOAD_ERROR        Failed to update aall Str ConfigParams to StrConfigTable Struct.

**/
STATIC EFI_STATUS
ParseStrConfigEntriesFromDT (
  VOID
  )
{
  EFI_STATUS      Status;
  INT32           FdtStatus;
  DT_NODE_HANDLE  Node;
  UINT64          MaxStrConfigPairCount;
  UINT32          PropNameBufferSize;
  CHAR8           *Buff;
  CHAR8           *BuffWalk;
  UINT64          Index;
  UINT32          ValueBufferSize;

  MaxStrConfigPairCount = 0;
  PropNameBufferSize    = 0;
  Buff                  = NULL;
  BuffWalk              = NULL;
  Index                 = 0;
  ValueBufferSize       = 0;

  // Check if table is empty and Allocate table based of the total count
  if (mStrConfigTable == NULL) {
    /*
     * NOTE: StrMaxCount is stored under /sw/uefi/int_param (not str_param)
     * by DTB convention - both integer and string config max-count properties
     * are co-located in the int_param node.  This is intentional; do not
     * change this to str_param.
     */
    Status = AllocateConfigTableFromDT (
               "/sw/uefi/int_param",
               "StrMaxCount",
               sizeof (STRING_CONFIGURATION_PAIR),
               (VOID **)&mStrConfigTable,
               &MaxStrConfigPairCount
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // Update the table with Key and Value Pairs
  FdtStatus = FdtGetNodeHandle (&Node, "/sw/uefi/str_param");
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  FdtStatus = DtFrameworkGetPropNamesSizeOfNode (&Node, &PropNameBufferSize);
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  if (PropNameBufferSize == 0) {
    return EFI_SUCCESS;
  }

  Buff = AllocateMemNoFree (PropNameBufferSize);
  if (Buff == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate PropNameBuff\n"));
    ASSERT (Buff != NULL);
    return EFI_LOAD_ERROR;
  }

  FdtStatus = DtFrameworkGetPropNamesOfNode (&Node, Buff, PropNameBufferSize);
  if (FdtStatus != DTB_ERR_NOERROR) {
    return EFI_LOAD_ERROR;
  }

  // Walk through buffer and store property names in Table
  BuffWalk = (CHAR8 *)Buff;
  while (PropNameBufferSize != 0) {
    if (Index >= MaxStrConfigPairCount) {
      DEBUG ((DEBUG_ERROR, "ParseStrConfigEntriesFromDT: Index %lu exceeds MaxCount\n", Index));
      break;
    }

    Status = GetNextPropNameFromBuff (
               &BuffWalk,
               &PropNameBufferSize,
               "ParseStrConfigEntriesFromDT",
               &mStrConfigTable[Index].Key
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FdtStatus = DtFrameworkGetPropSize (&Node, mStrConfigTable[Index].Key, &ValueBufferSize);
    if (FdtStatus != DTB_ERR_NOERROR) {
      return EFI_LOAD_ERROR;
    }

    mStrConfigTable[Index].Value = AllocateMemNoFree (ValueBufferSize);
    if (mStrConfigTable[Index].Value == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed to allocate StrConfigTable Value\n"));
      return EFI_LOAD_ERROR;
    }

    FdtStatus = DtFrameworkGetStringPropList (
                  &Node,
                  mStrConfigTable[Index].Key,
                  mStrConfigTable[Index].Value,
                  ValueBufferSize
                  );
    if (FdtStatus != DTB_ERR_NOERROR) {
      return EFI_LOAD_ERROR;
    }

    Index++;
  }

  mStrConfigTableEntryCount = Index;

  return EFI_SUCCESS;
}

/**
  Compare two memory region entries for sorting.

  Comparison function used by QuickSort to order memory regions by
  base address and size.

  @param[in]  Left   Pointer to left memory region entry.
  @param[in]  Right  Pointer to right memory region entry.

  @retval  <0   Left entry base address is less than right.
  @retval  0    Entries have equal base addresses.
  @retval  >0   Left entry base address is greater than right.

**/
STATIC INTN
MemEntryCompare (
  CONST VOID  *Left,
  CONST VOID  *Right
  )
{
  SORT_MEM_REG_INFO  *LeftEntry;
  SORT_MEM_REG_INFO  *RightEntry;

  LeftEntry  = (SORT_MEM_REG_INFO *)Left;
  RightEntry = (SORT_MEM_REG_INFO *)Right;

  if (LeftEntry->MemBase != RightEntry->MemBase) {
    return (INTN)(LeftEntry->MemBase - RightEntry->MemBase);
  } else {
    return (INTN)(LeftEntry->MemSize - RightEntry->MemSize);
  }
}

/**
  Validate memory region entries.

  Sorts memory regions and checks for overlapping regions in the
  configuration.

  @param[in]  Sort  Pointer to array of memory region entries to validate.

  @retval  EFI_SUCCESS            No overlapping memory regions found.
  @retval  EFI_INVALID_PARAMETER  Two or more memory regions overlap.

**/
STATIC EFI_STATUS
ValidateEntry (
  IN SORT_MEM_REG_INFO  *Sort
  )
{
  UINTN  Index;

  // Sort in increasing order
  PerformQuickSort (Sort, mNumMemRegions, sizeof (SORT_MEM_REG_INFO), (SORT_COMPARE)MemEntryCompare);

  // Check overlap
  for (Index = 1; Index < mNumMemRegions; Index++) {
    if (Sort[Index].MemBase < (Sort[Index-1].MemBase + Sort[Index-1].MemSize)) {
      DEBUG ((
        DEBUG_ERROR,
        "MemRegion \"%s\" (0x%x) and \"%s\" (0x%x) are overlapping in cfg\n",
        Sort[Index].Name,
        Sort[Index].MemBase,
        Sort[Index-1].Name,
        Sort[Index-1].MemBase
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Check for overlapping memory regions.

  Verifies that memory regions in the configuration do not overlap
  with each other.

  @retval  EFI_SUCCESS            No overlapping memory regions found.
  @retval  EFI_OUT_OF_RESOURCES   Failed to allocate the sort buffer.
  @retval  EFI_INVALID_PARAMETER  Two or more memory regions overlap.

**/
STATIC EFI_STATUS
CheckOverlap (
  VOID
  )
{
  UINTN              Index;
  SORT_MEM_REG_INFO  *SortedRegions;
  EFI_STATUS         Status;

  SortedRegions = (SORT_MEM_REG_INFO *)AllocatePool (mNumMemRegions * sizeof (SORT_MEM_REG_INFO));
  if (SortedRegions == NULL) {
    DEBUG ((DEBUG_ERROR, "MemoryAlloc failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mNumMemRegions; Index++) {
    SortedRegions[Index].MemBase = mMemRegions[Index].MemBase;
    SortedRegions[Index].MemSize = mMemRegions[Index].MemSize;
    AsciiStrToUnicodeStrS (mMemRegions[Index].Name, SortedRegions[Index].Name, MAX_MEM_LABEL_NAME);
  }

  Status = ValidateEntry (SortedRegions);

  FreePool (SortedRegions);

  return Status;
}

/**
  This function parses given memory map node and updates the MemRegion struct.

  @param  Node                  Pointer to FDT_NODE_HANDLE for Memory/Register Map Entry.
  @param  MemRegion             Pointer to mMemRegion Table Member to be updated.

  @retval EFI_SUCCESS           All the Entries of MemRegion is updated.
  @retval EFI_LOAD_ERROR        One of the query to DTB failed and MemRegion is not fully updated.

**/
STATIC EFI_STATUS
GetMemoryMapOfNode (
  IN OUT DT_NODE_HANDLE   *Node,
  OUT    MEM_REGION_INFO  *MemRegion
  )
{
  EFI_STATUS  Status;

  Status = DtFrameworkGetReg (Node, NULL, 0, 2, 2, &MemRegion->MemBase, &MemRegion->MemSize);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetReg: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = DtFrameworkGetStringPropList (Node, "mem-label", (CHAR8 *)&(MemRegion->Name), MAX_MEM_LABEL_NAME);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetStringPropList MemLabel: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = DtFrameworkGetUint8Prop (Node, "build-hob", (UINT8 *)&MemRegion->BuildHobOption);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetUint8Prop BuildHob: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = DtFrameworkGetUint8Prop (Node, "resource-type", (UINT8 *)&MemRegion->ResourceType);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetUint8Prop ResourceType: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = DtFrameworkGetUint8Prop (Node, "memory-type", (UINT8 *)&MemRegion->MemoryType);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetUint8Prop MemoryType: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = DtFrameworkGetUint8Prop (Node, "cache-attributes", (UINT8 *)&MemRegion->CacheAttributes);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetUint8Prop CacheAttributes: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = DtFrameworkGetUint32Prop (Node, "resource-attribute", (UINT32 *)&MemRegion->ResourceAttribute);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetUint32Prop ResourceAttribute: %d\n", Status));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function parses device tree and updates all the memory map entries
  into mMemRegion Structure and the count of entries into mNumMemRegions.

  @retval EFI_SUCCESS           mMemRegion Structure is updated with all the memory map entries from DT.
  @retval EFI_LOAD_ERROR        Failed to update all the memory map entries to mMemRegionStruct.

**/
STATIC EFI_STATUS
ParseMemoryMapEntriesFromDT (
  VOID
  )
{
  DT_NODE_HANDLE  Node;
  EFI_STATUS      Status;
  UINT32          MemEntryCount;
  DT_NODE_HANDLE  *CachedMmapNode;
  UINTN           MemIndex;

  MemEntryCount  = 0;
  CachedMmapNode = NULL;
  MemIndex       = 0;

  /*
   * Allocate the region table on first call.  On re-entry (e.g. a second
   * parse pass) the existing buffer is reused; always zero it so stale
   * entries from the previous call do not corrupt the new parse result.
   */
  if (mMemRegions == NULL) {
    mMemRegions = (MEM_REGION_INFO *)AllocateMemNoFree (sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS);
    if (mMemRegions == NULL) {
      DEBUG ((DEBUG_ERROR, "Unable to allocate memory for memory table!\n"));
      ASSERT (mMemRegions != NULL);
      CpuDeadLoop ();
      return EFI_LOAD_ERROR;
    }
  }

  SetMem (mMemRegions, sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS, 0);
  mNumMemRegions = 0;

  Status = FdtGetNodeHandle (&Node, "/soc/memorymap/");
  if (Status) {
    DEBUG ((DEBUG_ERROR, "FdtGetNodeHandle: %d\n", Status));
    goto ErrorExit;
  }

  Status = DtFrameworkGetCountOfSubnodes (&Node, &MemEntryCount);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetCountOfSubnodes: %d\n", Status));
    goto ErrorExit;
  }

  if (MemEntryCount == 0) {
    mNumMemRegions = 0;
    return EFI_SUCCESS;
  }

  ASSERT (mNumMemRegions == 0);

  if (MemEntryCount > MAX_MEMORY_REGIONS - mNumMemRegions) {
    DEBUG ((
      DEBUG_ERROR,
      "ParseMemoryMapEntriesFromDT: MemEntryCount %d would exceed MAX_MEMORY_REGIONS %d\n",
      MemEntryCount,
      MAX_MEMORY_REGIONS
      ));
    goto ErrorExit;
  }

  CachedMmapNode = (DT_NODE_HANDLE *)AllocateMemNoFree (sizeof (DT_NODE_HANDLE) * MemEntryCount);
  if (CachedMmapNode == NULL) {
    DEBUG ((DEBUG_ERROR, "AllocateMemNoFree for CachedMmapNode failed\n"));
    goto ErrorExit;
  }

  Status = DtFrameworkGetCacheOfSubnodes (&Node, CachedMmapNode, MemEntryCount);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetCacheOfSubnodes: %d\n", Status));
    goto ErrorExit;
  }

  // Fill the table by looping through the entries
  for (MemIndex = 0; MemIndex < MemEntryCount; MemIndex++) {
    Status = GetMemoryMapOfNode (&CachedMmapNode[MemIndex], &mMemRegions[mNumMemRegions]);
    if (Status) {
      DEBUG ((DEBUG_ERROR, "GetMemoryMapOfNode Failed\n"));
      goto ErrorExit;
    }

    mNumMemRegions++;
  }

  return EFI_SUCCESS;

ErrorExit:
  DEBUG ((DEBUG_ERROR, "Failed ParseMemoryMapEntriesFromDT\r\n"));
  return EFI_LOAD_ERROR;
}

/**
  This function parses device tree and updates all the register map entries
  into mMemRegion Structure and update mNumMemRegions count.

  @retval EFI_SUCCESS           mMemRegion Structure is updated with all the register map entries from DT.
  @retval EFI_LOAD_ERROR        Failed to update all the register map entries to mMemRegionStruct.

**/
STATIC EFI_STATUS
ParseRegisterMapEntriesFromDT (
  VOID
  )
{
  DT_NODE_HANDLE  Node;
  EFI_STATUS      Status;
  UINT32          RegisterEntryCount;
  DT_NODE_HANDLE  *CachedMmapNode;
  UINTN           RegIndex;

  RegisterEntryCount = 0;
  CachedMmapNode     = NULL;
  RegIndex           = 0;

  // Check if the mMemRegions is Empty and assign space as per number of mNumMemRegions
  if (mMemRegions == NULL) {
    mMemRegions = (MEM_REGION_INFO *)AllocateMemNoFree (sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS);
    if (mMemRegions == NULL) {
      DEBUG ((DEBUG_ERROR, "Unable to allocate memory for memory table!\n"));
      ASSERT (mMemRegions != NULL);
      CpuDeadLoop ();
      return EFI_LOAD_ERROR;
    }

    SetMem (mMemRegions, sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS, 0);
  }

  Status = FdtGetNodeHandle (&Node, "/soc/registermap/");
  if (Status) {
    DEBUG ((DEBUG_ERROR, "FdtGetNodeHandle: %d\n", Status));
    goto ErrorExit;
  }

  Status = DtFrameworkGetCountOfSubnodes (&Node, &RegisterEntryCount);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetCountOfSubnodes: %d\n", Status));
    goto ErrorExit;
  }

  if (RegisterEntryCount == 0) {
    return EFI_SUCCESS;
  }

  CachedMmapNode = (DT_NODE_HANDLE *)AllocateMemNoFree (sizeof (DT_NODE_HANDLE) * RegisterEntryCount);
  if (CachedMmapNode == NULL) {
    DEBUG ((DEBUG_ERROR, "AllocateMemNoFree for CachedMmapNode failed\n"));
    goto ErrorExit;
  }

  Status = DtFrameworkGetCacheOfSubnodes (&Node, CachedMmapNode, RegisterEntryCount);
  if (Status) {
    DEBUG ((DEBUG_ERROR, "DtFrameworkGetCacheOfSubnodes: %d\n", Status));
    goto ErrorExit;
  }

  // Fill the table by looping through the entries
  for (RegIndex = 0; RegIndex < RegisterEntryCount; RegIndex++) {
    if (mNumMemRegions >= MAX_MEMORY_REGIONS) {
      DEBUG ((DEBUG_ERROR, "ParseRegisterMapEntriesFromDT: exceeded MAX_MEMORY_REGIONS\n"));
      goto ErrorExit;
    }

    Status = GetMemoryMapOfNode (&CachedMmapNode[RegIndex], &mMemRegions[mNumMemRegions]);
    if (Status) {
      DEBUG ((DEBUG_ERROR, "GetMemoryMapOfNode: %d\n", Status));
      goto ErrorExit;
    }

    mNumMemRegions++;
  }

  return EFI_SUCCESS;

ErrorExit:
  DEBUG ((DEBUG_ERROR, "Failed ParseRegisterMapEntriesFromDT\r\n"));
  return EFI_LOAD_ERROR;
}

/**
  Get configuration memory map bounds.

  Calculates and stores the lowest and highest addresses from the
  configured memory map in global variables.

**/
STATIC VOID
GetConfigurationMemMapBounds (
  VOID
  )
{
  UINT64  Start;
  UINT64  End;
  UINTN   Index;

  for (Index = 0; Index < mNumMemRegions; Index++) {
    Start = mMemRegions[Index].MemBase;
    End   = Start + mMemRegions[Index].MemSize;
    if (Start < mMemMapLow) {
      mMemMapLow = Start;
    }

    if (End > mMemMapHigh) {
      mMemMapHigh        = End;
      mIsMemMapHighNoMap = FALSE;
      if (mMemRegions[Index].BuildHobOption == NoMap) {
        mIsMemMapHighNoMap = TRUE;
      }
    }
  }
}

/**
  Add non-FD memory regions.

  Adds memory regions from RAM partition table that are not part of
  the firmware device (FD) region to the memory map.

  @param[in]  EntryCount  Number of entries in the entry table.
  @param[in]  EntryTable  Pointer to array of memory region entries.

  @retval  EFI_SUCCESS  Non-FD regions added successfully.

**/
STATIC EFI_STATUS
AddNonFdRegions (
  UINTN            EntryCount,
  MEM_REGION_INFO  *EntryTable
  )
{
  UINT64  RegionEndAddr;
  UINT64  DDRMemSize;
  UINTN   EntryIndex;

  DDRMemSize    = 0;

  for (EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++) {
    DDRMemSize += EntryTable[EntryIndex].MemSize;

    RegionEndAddr = EntryTable[EntryIndex].MemBase +  EntryTable[EntryIndex].MemSize;
    if ((EntryTable[EntryIndex].MemBase < mMemMapHigh) && (RegionEndAddr >= mMemMapLow)) {
      continue;
    }

    // Update segment properties which are common to all cases.
    AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, EntryTable[EntryIndex].Name);
    mMemRegions[mNumMemRegions].ResourceType      = EntryTable[EntryIndex].ResourceType;
    mMemRegions[mNumMemRegions].ResourceAttribute = EntryTable[EntryIndex].ResourceAttribute;
    mMemRegions[mNumMemRegions].MemoryType        = EntryTable[EntryIndex].MemoryType;
    mMemRegions[mNumMemRegions].CacheAttributes   = EntryTable[EntryIndex].CacheAttributes;
    mMemRegions[mNumMemRegions].MemBase           = EntryTable[EntryIndex].MemBase;
    mMemRegions[mNumMemRegions].MemSize           = EntryTable[EntryIndex].MemSize;
    mMemRegions[mNumMemRegions].BuildHobOption    = EntryTable[EntryIndex].BuildHobOption;

    mNumMemRegions++;
  }

  return EFI_SUCCESS;
}

/**
  Add remainder of non-FD region.

  Adds the remaining portion of a memory bank that extends beyond
  the configured memory map high address.

  @retval  EFI_SUCCESS        Remainder added successfully.
  @retval  EFI_LOAD_ERROR     Error occurred during addition.
  @retval  EFI_NOT_FOUND      Region not found (may be acceptable).

**/
STATIC EFI_STATUS
AddNonFdRegionRemainder (
  VOID
  )
{
  MEM_REGION_INFO  FdRegion;
  MEM_REGION_INFO  NonFdRegion;
  UINT64           RegionEndAddr;
  EFI_STATUS       Status;
  UINT64           UefiFdBase;

  RegionEndAddr = 0;
  UefiFdBase    = FixedPcdGet64 (PcdFdBaseAddress);

  Status = RamPartitionGetPartitionEntryByAddr (UefiFdBase, &FdRegion);
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  Status = RamPartitionGetPartitionEntryByAddr(mMemMapHigh, &NonFdRegion);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) && mIsMemMapHighNoMap) {
      return EFI_SUCCESS;
    }

    return EFI_LOAD_ERROR;
  }

  if (FdRegion.MemBase == NonFdRegion.MemBase) {
    return EFI_SUCCESS;
  }

  RegionEndAddr = NonFdRegion.MemBase +  NonFdRegion.MemSize;

  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, NonFdRegion.Name);

  mMemRegions[mNumMemRegions].MemBase           = mMemMapHigh;
  mMemRegions[mNumMemRegions].MemSize           = (RegionEndAddr - mMemMapHigh);
  mMemRegions[mNumMemRegions].BuildHobOption    = NonFdRegion.BuildHobOption;
  mMemRegions[mNumMemRegions].ResourceType      = NonFdRegion.ResourceType;
  mMemRegions[mNumMemRegions].ResourceAttribute = NonFdRegion.ResourceAttribute;
  mMemRegions[mNumMemRegions].MemoryType        = NonFdRegion.MemoryType;
  mMemRegions[mNumMemRegions].CacheAttributes   = NonFdRegion.CacheAttributes;

  mNumMemRegions++;

  return EFI_SUCCESS;
}

/**
  Add remainder of FD region.

  Adds the portions of the memory bank containing the firmware device
  region that are outside the configured memory map bounds.

  @retval  EFI_SUCCESS     FD remainder added successfully.
  @retval  EFI_LOAD_ERROR  Error occurred during addition.

**/
STATIC EFI_STATUS
AddFdRegionRemainder (
  VOID
  )
{
  MEM_REGION_INFO  FdRegion;
  UINT64           RegionEndAddr;
  EFI_STATUS       Status;
  UINT64           UefiFdBase;

  UefiFdBase    = FixedPcdGet64 (PcdFdBaseAddress);

  Status = RamPartitionGetPartitionEntryByAddr (UefiFdBase, &FdRegion);
  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  RegionEndAddr = FdRegion.MemBase +  FdRegion.MemSize;

  if (mMemMapLow > FdRegion.MemBase) {
    AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, FdRegion.Name);

    mMemRegions[mNumMemRegions].MemBase           = FdRegion.MemBase;
    mMemRegions[mNumMemRegions].MemSize           = (mMemMapLow - FdRegion.MemBase);
    mMemRegions[mNumMemRegions].BuildHobOption    = FdRegion.BuildHobOption;
    mMemRegions[mNumMemRegions].ResourceType      = FdRegion.ResourceType;
    mMemRegions[mNumMemRegions].ResourceAttribute = FdRegion.ResourceAttribute;
    mMemRegions[mNumMemRegions].MemoryType        = FdRegion.MemoryType;
    mMemRegions[mNumMemRegions].CacheAttributes   = FdRegion.CacheAttributes;

    mNumMemRegions++;
  }

  if (mMemMapHigh < RegionEndAddr) {
    AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, FdRegion.Name);

    mMemRegions[mNumMemRegions].MemBase           = mMemMapHigh;
    mMemRegions[mNumMemRegions].MemSize           = (RegionEndAddr - mMemMapHigh);
    mMemRegions[mNumMemRegions].BuildHobOption    = FdRegion.BuildHobOption;
    mMemRegions[mNumMemRegions].ResourceType      = FdRegion.ResourceType;
    mMemRegions[mNumMemRegions].ResourceAttribute = FdRegion.ResourceAttribute;
    mMemRegions[mNumMemRegions].MemoryType        = FdRegion.MemoryType;
    mMemRegions[mNumMemRegions].CacheAttributes   = FdRegion.CacheAttributes;

    mNumMemRegions++;
  }

  return EFI_SUCCESS;
}

/**
  Update dynamic memory regions.

  Updates memory regions marked as AddDynamicMem based on RAM partition
  table information, converting them to conventional memory where appropriate.

  @retval  EFI_SUCCESS  Dynamic regions updated successfully.
  @retval  Other        Error occurred during update.

**/
STATIC EFI_STATUS
UpdateDynamicMemoryRegions (
  VOID
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  RamPartTableEntry[RAM_NUM_PART_ENTRIES];
  UINTN            RamPartTableEntryCount;
  UINT32           MemRgnCnt;

  RamPartTableEntryCount = RAM_NUM_PART_ENTRIES;
  Status = RamPartitionGetRamPartitions (&RamPartTableEntryCount, RamPartTableEntry);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  /* Parse through the memory region and check for the region with AddDynamicMem one */
  for (MemRgnCnt = 0; MemRgnCnt < mNumMemRegions; MemRgnCnt++) {
    BUILD_HOB_OPTION_TYPE  HobValue;

    HobValue = mMemRegions[MemRgnCnt].BuildHobOption;
    if (HobValue == AddDynamicMem) {
      /* Check if the RamPartitiontable hole is less than carved out */
      UINT32  RamPartIndex;

      for (RamPartIndex = 0; RamPartIndex < RamPartTableEntryCount; RamPartIndex++) {
        UINT64  RamPartitionEntryEndAddress;

        RamPartitionEntryEndAddress = RamPartTableEntry[RamPartIndex].MemBase +
                                      RamPartTableEntry[RamPartIndex].MemSize;
        if ((RamPartitionEntryEndAddress >= mMemRegions[MemRgnCnt].MemBase) &&
            (RamPartitionEntryEndAddress <= mMemRegions[MemRgnCnt].MemBase + mMemRegions[MemRgnCnt].MemSize))
        {
          /* Update if requred pMemRegion size and allocate rest of the reserved memory space as Conventional memory */
          UINT64  HoleSize;

          HoleSize = mMemRegions[MemRgnCnt].MemBase + mMemRegions[MemRgnCnt].MemSize - RamPartitionEntryEndAddress;
          if (HoleSize < mMemRegions[MemRgnCnt].MemSize) {
            /* Update MemRegion size and attributes */
            mMemRegions[MemRgnCnt].MemSize           = mMemRegions[MemRgnCnt].MemSize - HoleSize;
            mMemRegions[MemRgnCnt].BuildHobOption    = (BUILD_HOB_OPTION_TYPE)AddMem;
            mMemRegions[MemRgnCnt].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
            mMemRegions[MemRgnCnt].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
            mMemRegions[MemRgnCnt].MemoryType        = (EFI_MEMORY_TYPE)EfiConventionalMemory;
            mMemRegions[MemRgnCnt].CacheAttributes   =
              (ARM_MEMORY_REGION_ATTRIBUTES)ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
          }

          break;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Identify and report upper RAM partitions to the HOB list.

  Add the highest RAM aprtition table entry as avilable memory.

  @retval EFI_SUCCESS           Upper memory partitions were successfully
                                identified and reported.
  @retval EFI_NOT_FOUND         No RAM partitions were found in the
                                platform configuration.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources for HOB creation.

**/
EFI_STATUS
EFIAPI
AddUpperMemoryFromRamPartitions (
  VOID
  )
{
  MEM_REGION_INFO  EntryList[RAM_NUM_PART_ENTRIES];
  UINTN            EntryCount;
  UINT64           HighestBase;
  UINT64           HighestSize;
  UINTN            Index;
  UINTN            HighestIndex;
  EFI_STATUS       Status;

  EntryCount = RAM_NUM_PART_ENTRIES;

  Status = RamPartitionGetRamPartitions (&EntryCount, &EntryList[0]);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /* Find the uppermost RAM partition entry */
  HighestBase  = 0;
  HighestSize  = 0;
  HighestIndex = EntryCount; /* sentinel: no match */

  for (Index = 0; Index < EntryCount; Index++) {
    if (EntryList[Index].MemBase > HighestBase) {
      HighestBase  = EntryList[Index].MemBase;
      HighestSize  = EntryList[Index].MemSize;
      HighestIndex = Index;
    }
  }

  if (HighestIndex == EntryCount) {
    return EFI_NOT_FOUND;
  }

  if (mNumMemRegions >= MAX_MEMORY_REGIONS) {
    ASSERT (mNumMemRegions < MAX_MEMORY_REGIONS);
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, EntryList[HighestIndex].Name);
  mMemRegions[mNumMemRegions].MemBase           = HighestBase;
  mMemRegions[mNumMemRegions].MemSize           = HighestSize;
  mMemRegions[mNumMemRegions].BuildHobOption    = NoBuildHob;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
  mMemRegions[mNumMemRegions].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
  mMemRegions[mNumMemRegions].MemoryType        = (EFI_MEMORY_TYPE)EfiConventionalMemory;
  mMemRegions[mNumMemRegions].CacheAttributes   = (ARM_MEMORY_REGION_ATTRIBUTES)ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  mNumMemRegions++;

  /* Manually build the ResourceDescriptorHob so DXE discovers this memory,
 * without a MemoryAllocationHob that would expose it to the PEI allocator */
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES,
    HighestBase,
    HighestSize
    );

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  EntryList[RAM_NUM_PART_ENTRIES];
  UINTN            EntryCount;

  EntryCount = RAM_NUM_PART_ENTRIES;

  Status = RamPartitionGetRamPartitions (&EntryCount, &EntryList[0]);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = AddNonFdRegions (EntryCount, &EntryList[0]);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = AddFdRegionRemainder ();
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = AddNonFdRegionRemainder ();
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  Status = UpdateDynamicMemoryRegions ();
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  return Status;
}

/**
  Load and parse platform configuration.

  Parses UEFI platform configuration data (memory map, register map,
  and configuration parameters) from device tree and stores the
  information for access by other UEFI modules.

  @retval  EFI_SUCCESS     Configuration loaded and parsed successfully.
  @retval  EFI_LOAD_ERROR  Error occurred during parsing.

**/
EFI_STATUS
EFIAPI
LoadAndParsePlatformCfg (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = ParseMemoryMapEntriesFromDT ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ParseMemoryMapEntriesFromDT failed\r\n"));
    Status = EFI_LOAD_ERROR;
    goto ErrorExit;
  }

  mNumMemoryMapRegion = mNumMemRegions;

  GetConfigurationMemMapBounds ();

  Status = ParseRegisterMapEntriesFromDT ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ParseRegisterMapEntriesFromDT failed\r\n"));
    Status = EFI_LOAD_ERROR;
    goto ErrorExit;
  }

  Status = CheckOverlap ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "CheckOverlap failed\r\n"));
    Status = EFI_LOAD_ERROR;
    goto ErrorExit;
  }

  mIntConfigTable           = NULL;
  mIntConfigTableEntryCount = 0;
  Status                    = ParseIntConfigEntriesFromDT ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ParseIntConfigEntriesFromDT failed\r\n"));
    Status = EFI_LOAD_ERROR;
    goto ErrorExit;
  }

  mStrConfigTable           = NULL;
  mStrConfigTableEntryCount = 0;
  Status                    = ParseStrConfigEntriesFromDT ();
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ParseStrConfigEntriesFromDT failed\r\n"));
    Status = EFI_LOAD_ERROR;
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  DEBUG ((DEBUG_ERROR, "Failed LoadAndParsePlatformCfg\r\n"));
  return Status;
}

/**
  Load static platform configuration.

  Populates the memory region table with a minimal static memory map
  using PCD values when device tree is not available. The UEFI FD region
  is added as the sole memory-map entry (used for RAM partition validation
  and memory bounds calculation), followed by peripheral regions for SMEM,
  UART, and IMEM cookies.

  @retval  EFI_SUCCESS          Static configuration loaded successfully.
  @retval  EFI_OUT_OF_RESOURCES Memory allocation failed.

**/
EFI_STATUS
EFIAPI
LoadStaticPlatformCfg (
  VOID
  )
{
  mMemRegions = (MEM_REGION_INFO *)AllocateMemNoFree (sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS);
  if (mMemRegions == NULL) {
    DEBUG ((DEBUG_ERROR, "LoadStaticPlatformCfg: failed to allocate memory table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (mMemRegions, sizeof (MEM_REGION_INFO) * MAX_MEMORY_REGIONS, 0);
  mNumMemRegions = 0;

  /* ------------------------------------------------------------------- */
  /* Memory-map region: UEFI FD (system memory where firmware lives)     */
  /*                                                                     */
  /* ArmPlatformSetupDebugBuffer() has already built an                  */
  /* EFI_RESOURCE_MEMORY_RESERVED HOB for the T32 DDR buffer at          */
  /* [PcdTrace32DdrBase, PcdTrace32DdrBase + PcdTrace32DdrSize).         */
  /* That region falls within the UEFI FD range, so we must NOT emit a   */
  /* single system-memory descriptor covering the whole UEFI region.     */
  /* ------------------------------------------------------------------- */

  /* Lower portion: FD base up to (but not including) the T32 buffer */
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES,
    FixedPcdGet64 (PcdFdBaseAddress),
    FixedPcdGet64 (PcdTrace32DdrBase) - FixedPcdGet64 (PcdFdBaseAddress)
    );

  /* Upper portion: above the T32 buffer to the end of the UEFI region */
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES,
    FixedPcdGet64 (PcdTrace32DdrBase) + FixedPcdGet64 (PcdTrace32DdrSize),
    (FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet64 (PcdSystemMemorySize))
    - (FixedPcdGet64 (PcdTrace32DdrBase) + FixedPcdGet64 (PcdTrace32DdrSize))
    );

  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "UEFI FD");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdFdBaseAddress);
  mMemRegions[mNumMemRegions].MemSize           = FixedPcdGet64 (PcdSystemMemorySize);
  mMemRegions[mNumMemRegions].BuildHobOption    = NoBuildHob;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_SYSTEM_MEMORY;
  mMemRegions[mNumMemRegions].ResourceAttribute = SYSTEM_MEMORY_RESOURCE_ATTR_SETTINGS_CAPABILITIES;
  mMemRegions[mNumMemRegions].MemoryType        = EfiBootServicesData;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  mNumMemRegions++;

  /* mNumMemoryMapRegion tracks only memory-map entries (not register/peripheral
   * entries) so that ValidateParsedMemoryRegions() only checks RAM regions. */
  mNumMemoryMapRegion = mNumMemRegions;

  /* Calculate mMemMapLow / mMemMapHigh from the memory-map entries only.
   * This must be done before appending peripheral entries so that
   * UpdateSystemMemoryRegions() uses the correct FD region bounds. */
  GetConfigurationMemMapBounds ();

  /* ------------------------------------------------------------------ */
  /* Register/peripheral regions (not validated against RAM partitions) */
  /* ------------------------------------------------------------------ */

  /* SMEM - shared memory (uncached) */
  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "SMEM");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdSmemBaseAddress);
  mMemRegions[mNumMemRegions].MemSize           = FixedPcdGet64 (PcdSmemSize);
  mMemRegions[mNumMemRegions].BuildHobOption    = AddPeripheral;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_MEMORY_MAPPED_IO;
  mMemRegions[mNumMemRegions].ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
  mMemRegions[mNumMemRegions].MemoryType        = EfiMemoryMappedIO;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  mNumMemRegions++;

  /* Serial Port */
  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "UART");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdSerialRegisterBase);
  mMemRegions[mNumMemRegions].MemSize           = EFI_PAGE_SIZE;
  mMemRegions[mNumMemRegions].BuildHobOption    = AddPeripheral;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_MEMORY_MAPPED_IO;
  mMemRegions[mNumMemRegions].ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
  mMemRegions[mNumMemRegions].MemoryType        = EfiMemoryMappedIO;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  mNumMemRegions++;

  /* IMEM Cookies */
  AsciiStrCpyS (mMemRegions[mNumMemRegions].Name, MAX_MEM_LABEL_NAME, "IMEM");
  mMemRegions[mNumMemRegions].MemBase           = FixedPcdGet64 (PcdIMemCookiesBase);
  mMemRegions[mNumMemRegions].MemSize           = FixedPcdGet64 (PcdIMemCookiesSize);
  mMemRegions[mNumMemRegions].BuildHobOption    = AddPeripheral;
  mMemRegions[mNumMemRegions].ResourceType      = EFI_RESOURCE_MEMORY_MAPPED_IO;
  mMemRegions[mNumMemRegions].ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE;
  mMemRegions[mNumMemRegions].MemoryType        = EfiMemoryMappedIO;
  mMemRegions[mNumMemRegions].CacheAttributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  mNumMemRegions++;

  DEBUG ((
    DEBUG_INFO,
    "LoadStaticPlatformCfg: %d regions (FD base=0x%lx size=0x%lx)\n",
    mNumMemRegions,
    FixedPcdGet64 (PcdFdBaseAddress),
    (UINT64)PcdGet32 (PcdSystemMemoryUefiRegionSize)
    ));

  return EFI_SUCCESS;
}

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
  )
{
  if ((MemoryRegions == NULL) || (NumMemoryRegions == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *MemoryRegions    = mMemRegions;
  *NumMemoryRegions = mNumMemRegions;

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS       Status;
  MEM_REGION_INFO  RamPartTableEntry[RAM_NUM_PART_ENTRIES];
  UINTN            RamPartTableEntryCount;
  BOOLEAN          NoOverlap;
  UINT32           Index;
  UINT32           RamPartitionIndex;

  RamPartitionIndex = 0;

  RamPartTableEntryCount = RAM_NUM_PART_ENTRIES;
  Status = RamPartitionGetRamPartitions (&RamPartTableEntryCount, RamPartTableEntry);
  if (Status != EFI_SUCCESS) {
    ASSERT (Status == EFI_SUCCESS);
    return Status;
  }

  for (Index = 0; Index < mNumMemoryMapRegion; Index++) {
    NoOverlap = FALSE;

    /* This first cond added for the hole that is carved out in memmap.dtsi
     * for converting it to Conventional Memory and NoMap is not needed to
     * be checked for overlap in RamPartitionTable */
    if ((mMemRegions[Index].BuildHobOption == AddDynamicMem) ||
        (mMemRegions[Index].BuildHobOption == NoMap) ||
        (mMemRegions[Index].BuildHobOption == NoBuildHob))
    {
      continue;
    }

    for (RamPartitionIndex = 0; RamPartitionIndex < RamPartTableEntryCount; RamPartitionIndex++) {
      UINT64  RamPartitionEntryBase;
      UINT64  RamPartitionEntryEnd;
      UINT64  MemRegionEnd;

      RamPartitionEntryBase = RamPartTableEntry[RamPartitionIndex].MemBase;
      RamPartitionEntryEnd  = RamPartitionEntryBase + RamPartTableEntry[RamPartitionIndex].MemSize;
      MemRegionEnd          = mMemRegions[Index].MemBase + mMemRegions[Index].MemSize - 1;

      if ((mMemRegions[Index].MemBase >= RamPartitionEntryBase) &&
          (MemRegionEnd >= RamPartitionEntryBase) &&
          (mMemRegions[Index].MemBase < RamPartitionEntryEnd) &&
          (MemRegionEnd < RamPartitionEntryEnd)
          )
      {
        /* The region lies in the usable rampartition table range and there is
         * no overlap in the hole region defined by RamPartition table */
        NoOverlap = TRUE;
        break;
      }
    }

    if (!NoOverlap) {
      DEBUG ((
        DEBUG_ERROR,
        "Memory region %a carved out in a hole defined by RAM partition table !\n",
        mMemRegions[Index].Name
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}
