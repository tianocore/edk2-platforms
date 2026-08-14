/** @file
  Qualcomm Shared Memory (SMEM) library for Qualcomm platforms.

  This instance implements the subset of the SMEM interface that the UEFI
  boot flow relies on: locating the usable RAM partition table (SMEM item
  SmemUsableRamPartitionTable, legacy id 402) that the boot firmware (XBL)
  publishes in the SMEM region, so RamPartitionTableLib can size DRAM
  dynamically instead of relying on the statically described region.

  The table is located via the SMEM legacy allocation table (TOC); if that
  lookup does not validate, a bounded scan of the SMEM region for the table's
  magic values is used as a fallback. Both paths validate the result against
  the table magics before returning it. The read-only allocation helpers
  (SmemAlloc, SmemAllocEx, SmemFree) are not implemented because the UEFI
  stage never produces new SMEM items; it only consumes those the earlier
  boot loaders published.

  The physical base of the SMEM region is supplied by the platform through
  PcdSmemBaseAddress.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SMEM - Shared Memory
    - TOC  - Table Of Contents (the SMEM legacy per-item allocation table)
    - XBL  - eXtensible Boot Loader
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SmemLib.h>
#include <RamPartition.h>

//
// SMEM legacy heap layout. The per-item allocation table (TOC) follows the
// proc-comm region (4 * 16 bytes), the version array (32 * 4 bytes) and the
// heap-info header (16 bytes), so it begins 0xD0 bytes into the SMEM region.
// Each TOC entry is 16 bytes and is indexed by SMEM item id.
//
#define SMEM_TOC_OFFSET             0xD0
#define SMEM_ALLOC_ENTRY_ALLOCATED  1

//
// The usable RAM partition table (SmemUsableRamPartitionTable) format, its
// magic values (RAM_PART_MAGIC1/2) and the entry/table structures
// (RAM_PARTITION_ENTRY, USABLE_RAM_PARTITION_TABLE) are shared with the
// consumer (RamPartitionTableLib) via <RamPartition.h>.
//
typedef struct {
  UINT32    Allocated;
  UINT32    Offset;
  UINT32    Size;
  UINT32    BaseExt;
} SMEM_ALLOC_ENTRY;

/**
  Locate the usable RAM partition table within the SMEM region.

  Primary path: the legacy allocation-table entry for the usable RAM partition
  table item. Fallback: a bounded scan of the SMEM region for the table magics
  (robust to TOC layout differences). The returned pointer, if any, has
  validated magic values.

  @param[in]  SmemBase  Physical base address of the SMEM region.

  @retval NULL   The table was not found.
  @retval Other  Pointer to the validated table.
**/
STATIC
USABLE_RAM_PARTITION_TABLE *
LocatePartitionTable (
  IN UINT64  SmemBase
  )
{
  SMEM_ALLOC_ENTRY            *Entry;
  USABLE_RAM_PARTITION_TABLE  *Table;
  UINT8                       *Scan;
  UINT8                       *ScanEnd;

  //
  // Primary: legacy allocation-table lookup of the usable RAM partition table.
  //
  Entry = (SMEM_ALLOC_ENTRY *)(UINTN)(SmemBase + SMEM_TOC_OFFSET +
            ((UINT64)SmemUsableRamPartitionTable * sizeof (SMEM_ALLOC_ENTRY)));
  if (Entry->Allocated == SMEM_ALLOC_ENTRY_ALLOCATED) {
    Table = (USABLE_RAM_PARTITION_TABLE *)(UINTN)(SmemBase + Entry->Offset);
    if ((Table->Magic1 == RAM_PART_MAGIC1) && (Table->Magic2 == RAM_PART_MAGIC2)) {
      return Table;
    }
  }

  //
  // Fallback: scan the SMEM region for the table magics.
  //
  Scan    = (UINT8 *)(UINTN)SmemBase;
  ScanEnd = Scan + FixedPcdGet32 (PcdSmemSize) - sizeof (USABLE_RAM_PARTITION_TABLE);
  for ( ; Scan <= ScanEnd; Scan += sizeof (UINT32)) {
    Table = (USABLE_RAM_PARTITION_TABLE *)Scan;
    if ((Table->Magic1 == RAM_PART_MAGIC1) && (Table->Magic2 == RAM_PART_MAGIC2)) {
      return Table;
    }
  }

  return NULL;
}

/**
  Get SMEM library version.

  @retval SMEM_LIB_VERSION  Version number of this SMEM library instance.
**/
UINT32
EFIAPI
SmemLibGetVersion (
  VOID
  )
{
  return SMEM_LIB_VERSION;
}

/**
  Initializes the shared memory allocation structures.

  The SMEM region is populated by the earlier boot loaders (XBL); the UEFI
  stage only reads it, so no initialization work is required here.

  @par Dependencies
  Shared memory must have been cleared and initialized by the first system
  bootloader before calling this function.
**/
VOID
EFIAPI
SmemInit (
  VOID
  )
{
}

/**
  Requests a pointer to a buffer in shared memory.

  Allocation is not supported at the UEFI stage, which only consumes SMEM
  items published by the earlier boot loaders.

  @param[in] SmemType    Type of memory.
  @param[in] BufSize     Size of the buffer requested.

  @retval NULL  Allocation is not supported.
**/
VOID *
EFIAPI
SmemAlloc (
  IN SMEM_MEM_TYPE  SmemType,
  IN UINT32         BufSize
  )
{
  return NULL;
}

/**
  Requests a pointer to a buffer in shared memory.

  Allocation is not supported at the UEFI stage.

  @param[in, out] Params  See definition of SMEM_ALLOC_PARAMS_TYPE for details.

  @retval SMEM_STATUS_FAILURE  Allocation is not supported.
**/
INT32
EFIAPI
SmemAllocEx (
  IN OUT SMEM_ALLOC_PARAMS_TYPE  *Params
  )
{
  return SMEM_STATUS_FAILURE;
}

/**
  Requests the address of an allocated buffer in shared memory.

  Only the usable RAM partition table item is served; other item ids are not
  consumed by the UEFI boot flow and return NULL.

  @param[in]  SmemType   Type of memory to get a pointer for.
  @param[out] BufSize    Size of the buffer located in shared memory.

  @retval NULL   The item is not available.
  @retval Other  Pointer to the located item in shared memory.
**/
VOID *
EFIAPI
SmemGetAddr (
  IN SMEM_MEM_TYPE  SmemType,
  OUT UINT32        *BufSize
  )
{
  USABLE_RAM_PARTITION_TABLE  *Table;

  if (SmemType != SmemUsableRamPartitionTable) {
    return NULL;
  }

  Table = LocatePartitionTable (PcdGet64 (PcdSmemBaseAddress));
  if (Table == NULL) {
    DEBUG ((
      DEBUG_WARN,
      "%a: RAM partition table not found in SMEM @ 0x%lx\n",
      __func__,
      PcdGet64 (PcdSmemBaseAddress)
      ));
    return NULL;
  }

  if (BufSize != NULL) {
    *BufSize = (UINT32)sizeof (USABLE_RAM_PARTITION_TABLE);
  }

  return Table;
}

/**
  Requests the address and size of an allocated buffer in shared memory.

  Only the default-partition lookup via SmemGetAddr is supported.

  @param[in, out] Params  See definition of SMEM_ALLOC_PARAMS_TYPE for details.

  @retval SMEM_STATUS_SUCCESS   The item was located; Params.Buffer and
                                Params.Size are updated.
  @retval SMEM_STATUS_NOT_FOUND The item was not located.
  @retval SMEM_STATUS_INVALID_PARAM  Params is NULL.
**/
INT32
EFIAPI
SmemGetAddrEx (
  IN OUT SMEM_ALLOC_PARAMS_TYPE  *Params
  )
{
  VOID    *Buffer;
  UINT32  BufSize;

  if (Params == NULL) {
    return SMEM_STATUS_INVALID_PARAM;
  }

  BufSize = 0;
  Buffer  = SmemGetAddr (Params->SmemType, &BufSize);
  if (Buffer == NULL) {
    return SMEM_STATUS_NOT_FOUND;
  }

  Params->Buffer = Buffer;
  Params->Size   = BufSize;
  return SMEM_STATUS_SUCCESS;
}

/**
  Frees a pointer in shared memory.

  Freeing is not supported at the UEFI stage.

  @param[in] Addr    Pointer to the shared memory block to be freed.
**/
VOID
EFIAPI
SmemFree (
  IN VOID  *Addr
  )
{
}

/**
  Sets the version number for this processor and a given object.

  Version negotiation is handled by the earlier boot loaders; the UEFI stage
  reports a match so consumers proceed with the published items.

  @param[in] Type       Type of object being version checked.
  @param[in] Version    Local version number for this memory object.
  @param[in] Mask       Bitwise AND mask for version comparison.

  @retval TRUE  The version is accepted.
**/
BOOLEAN
EFIAPI
SmemVersionSet (
  IN SMEM_MEM_TYPE  Type,
  IN UINT32         Version,
  IN UINT32         Mask
  )
{
  return TRUE;
}
