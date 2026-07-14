/** @file
  Qualcomm Shared Memory (SMEM) library for Qualcomm platforms.

  This instance implements the subset of the SMEM interface that the UEFI
  boot flow relies on: locating SMEM items that the boot firmware (XBL)
  publishes in the SMEM region via the legacy allocation table (TOC), so
  consumers can read them.

  The read-write allocation helpers (SmemAlloc, SmemAllocEx, SmemFree) are not
  implemented because the UEFI stage never produces new SMEM items; it only
  consumes those the earlier boot loaders published. This library is a generic
  SMEM item accessor and has no knowledge of any particular item's payload
  layout -- interpreting an item's contents is the consumer's responsibility.

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

//
// SMEM legacy heap layout. The per-item allocation table (TOC) follows the
// proc-comm region (4 * 16 bytes), the version array (32 * 4 bytes) and the
// heap-info header (16 bytes), so it begins 0xD0 bytes into the SMEM region.
// Each TOC entry is 16 bytes and is indexed by SMEM item id.
//
#define SMEM_TOC_OFFSET             0xD0
#define SMEM_ALLOC_ENTRY_ALLOCATED  1

typedef struct {
  UINT32    Allocated;
  UINT32    Offset;
  UINT32    Size;
  UINT32    BaseExt;
} SMEM_ALLOC_ENTRY;

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

  Looks the item up in the SMEM legacy allocation table (TOC) by its item id
  and, if the entry is marked allocated, returns a pointer to the item's
  payload within the SMEM region. The caller is responsible for validating and
  interpreting the payload.

  @param[in]  SmemType   Type of memory (SMEM item id) to get a pointer for.
  @param[out] BufSize    Size of the buffer located in shared memory. May be
                         NULL if the size is not needed.

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
  UINT64            SmemBase;
  SMEM_ALLOC_ENTRY  *Entry;

  SmemBase = PcdGet64 (PcdSmemBaseAddress);

  Entry = (SMEM_ALLOC_ENTRY *)(UINTN)(SmemBase + SMEM_TOC_OFFSET +
            ((UINT64)SmemType * sizeof (SMEM_ALLOC_ENTRY)));
  if (Entry->Allocated != SMEM_ALLOC_ENTRY_ALLOCATED) {
    return NULL;
  }

  if (BufSize != NULL) {
    *BufSize = Entry->Size;
  }

  return (VOID *)(UINTN)(SmemBase + Entry->Offset);
}

/**
  Requests the address and size of an allocated buffer in shared memory.

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
