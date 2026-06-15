/** @file
  NULL implementation of the Qualcomm Shared Memory (SMEM) library.

  All functions return safe default values suitable for platforms
  where SMEM is not available.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SMEM - Shared Memory
**/

#include <Base.h>
#include <Library/SmemLib.h>

/**
  Get SMEM library version.

  @retval SMEM_LIB_VERSION_NULL  Always returns the NULL instance version.
**/
UINT32
EFIAPI
SmemLibGetVersion (
  VOID
  )
{
  return SMEM_LIB_VERSION_NULL;
}

/**
  Initializes the shared memory allocation structures.

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

  @param[in] SmemType    Type of memory.
  @param[in] BufSize     Size of the buffer requested.

  @retval NULL  Always returns NULL in this NULL implementation.
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

  @param[in, out] Params  See definition of SMEM_ALLOC_PARAMS_TYPE for details.

  @retval SMEM_STATUS_FAILURE  Always returns failure in this NULL implementation.
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

  @param[in]  SmemType   Type of memory to get a pointer for.
  @param[out] BufSize    Size of buffer allocated in shared memory.

  @retval NULL  Always returns NULL in this NULL implementation.
**/
VOID *
EFIAPI
SmemGetAddr (
  IN SMEM_MEM_TYPE  SmemType,
  OUT UINT32        *BufSize
  )
{
  return NULL;
}

/**
  Requests the address and size of an allocated buffer in shared memory.

  @param[in, out] Params  See definition of SMEM_ALLOC_PARAMS_TYPE for details.

  @retval SMEM_STATUS_FAILURE  Always returns failure in this NULL implementation.
**/
INT32
EFIAPI
SmemGetAddrEx (
  IN OUT SMEM_ALLOC_PARAMS_TYPE  *Params
  )
{
  return SMEM_STATUS_FAILURE;
}

/**
  Frees a pointer in shared memory.

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

  @param[in] Type       Type of object being version checked.
  @param[in] Version    Local version number for this memory object.
  @param[in] Mask       Bitwise AND mask for version comparison.

  @retval TRUE   Always returns TRUE in this NULL implementation.
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
