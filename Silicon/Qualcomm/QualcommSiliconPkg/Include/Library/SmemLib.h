/** @file
  Public interface for the Qualcomm Shared Memory (SMEM) library.

  Provides functions for allocating and accessing shared memory regions
  used for inter-processor communication on Qualcomm platforms.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SMEM - Shared Memory
**/

#pragma once

#include <Base.h>
#include <SmemType.h>

#define SMEM_LIB_VERSION  0x00010001U

/** SMEM Library version on platforms without SMEM support */
#define SMEM_LIB_VERSION_NULL  0xFFFFFFFFU

/** Extract major version number from version */
#define SMEM_LIB_GET_MAJOR_VER(x)  (((UINT32)(x)) >> 16)

/** Major version number mask */
#define SMEM_LIB_MAJOR_VERSION_MASK  (0xFFFF0000U)

/** Minor version number mask */
#define SMEM_LIB_MINOR_VERSION_MASK  (0x0000FFFFU)

/** SMEM Status return codes */
#define SMEM_STATUS_SUCCESS           0
#define SMEM_STATUS_FAILURE           -1
#define SMEM_STATUS_INVALID_PARAM     -2
#define SMEM_STATUS_OUT_OF_RESOURCES  -3
#define SMEM_STATUS_NOT_FOUND         -4
#define SMEM_STATUS_DUPLICATE         -5
#define SMEM_STATUS_SIZE_MISMATCH     -6

/** Flags for use with SmemAllocEx() */
#define SMEM_ALLOC_FLAG_NONE  0           /**< Default behavior */

/**
  Get SMEM library version.

  This function returns the version number of the SMEM library implementation.
  The version number follows the format: Major.Minor (0xMMMMNNNN where MMMM is
  major version and NNNN is minor version).

  @retval SMEM_LIB_VERSION  Library version number in format 0xMMMMNNNN
                            (0x00010001 for version 1.1).

  @par Dependencies
       None.

  @par Side Effects
  None.
**/
UINT32
EFIAPI
SmemLibGetVersion (
  VOID
  );

/**
  Initializes the shared memory allocation structures.

  @par Dependencies
  Shared memory must have been cleared and initialized by the first system
  bootloader.
**/
VOID
EFIAPI
SmemInit (
  VOID
  );

/**
  Requests a pointer to a buffer in shared memory.

  @param[in] SmemType    Type of memory.
  @param[in] BufSize     Size of the buffer requested. For pre-allocated
                         buffers, a fatal error occurs if the buffer size
                         requested does not match the size of the existing
                         buffer. If the BufSize is not 64-bit aligned, this
                         function increases the size until it is 64-bit aligned.

  @retval NULL   SMEM is not initialized.
  @retval Other  Valid SMEM address to the requested buffer.

  @par Side Effects
  This function uses spinlocks for exclusive access to the shared memory heap.
**/
VOID *
EFIAPI
SmemAlloc (
  IN SMEM_MEM_TYPE  SmemType,
  IN UINT32         BufSize
  );

/**
  Requests a pointer to a buffer in shared memory.  Upon success, Params.Buffer
  points to the allocation in shared memory.

  @param[in, out] Params  See definition of SMEM_ALLOC_PARAMS_TYPE for details.

  @retval SMEM_STATUS_SUCCESS          Allocation was successful,
                                       or already exists with specified size.
                                       Pointer to SMEM buffer is saved in
                                       Params.Buffer.
                                       Params.Size contains the requested size rounded
                                       up to 8 bytes.
                                       Params.Flags contains the status of the cached
                                       flag, which may not match what was
                                       requested if the item has been allocated
                                       previously.
  @retval SMEM_STATUS_SIZE_MISMATCH    Allocation exists, but has different size.
                                       This is possible when another processor has
                                       already allocated an item with this SMEM ID and
                                       a different size.
                                       Pointer to SMEM buffer is saved in Params.Buffer.
                                       Params.Size contains the originally allocated
                                       size rounded up to 8 bytes.
                                       Params.Flags contains the status of the cached
                                       flag, which may not match what was
                                       requested if the item has been allocated
                                       previously.
  @retval SMEM_STATUS_INVALID_PARAM    Invalid parameter
  @retval SMEM_STATUS_OUT_OF_RESOURCES Not enough SMEM to allocate this buffer
  @retval SMEM_STATUS_FAILURE          General failure

  @par Side Effects
  This function uses spinlocks for exclusive access to the shared memory heap.
**/
INT32
EFIAPI
SmemAllocEx (
  IN OUT SMEM_ALLOC_PARAMS_TYPE  *Params
  );

/**
  Requests the address and size of an allocated buffer in shared memory. Newly
  allocated shared memory buffers, which have never been allocated on any
  processor, are guaranteed to be zero-initialized.

  @param[in]  SmemType    Type of memory for which to get a pointer.
  @param[out] BufSize     Size of the buffer allocated in shared memory.

  @retval Pointer  Pointer to the requested buffer.
  @retval NULL     Buffer has not been allocated.

  @par Dependencies
  The buffer must already have been allocated.

  @par Side Effects
  This function uses spinlocks for exclusive access to the shared memory heap.
**/
VOID *
EFIAPI
SmemGetAddr (
  IN SMEM_MEM_TYPE  SmemType,
  OUT UINT32        *BufSize
  );

/**
  Requests the address and size of an allocated buffer in shared memory.
  If found, sets the buffer and size fields of the Params struct.

  @param[in, out] Params  Params.SmemType must be set to the ID to search for.
                          Params.RemoteHost must be set to the ID of the remote
                          host of the partition.
                          If Params.Flags has the SMEM_ALLOC_FLAG_PARTITION_ONLY
                          flag set, then this function will not search the
                          default partition for the item.  This is useful when
                          SMEM item X is possible in both the default partition
                          and an edge-pair partition, and the caller wants to
                          find out if the item exists in the edge-pair
                          partition only.

  @retval SMEM_STATUS_SUCCESS Allocated buffer found
                              Params.Buffer contains a pointer to the allocation.
                              Params.Size contains the size of the allocation
                              as originally requested, rounded up to 8 bytes.
                              Params.Flags contains the status of the cached
                              flag.
  @retval SMEM_STATUS_NOT_FOUND       Allocated buffer not found
  @retval SMEM_STATUS_INVALID_PARAM   Invalid parameter
  @retval SMEM_STATUS_FAILURE         Failure occurred

  @par Side Effects
  This function uses spinlocks for exclusive access to the shared memory heap.
  This function updates the SizeRemaining field for the partition.
**/
INT32
EFIAPI
SmemGetAddrEx (
  IN OUT SMEM_ALLOC_PARAMS_TYPE  *Params
  );

/**
  Frees a pointer in shared memory.

  @note This function is not actually implemented, but if it were, it would
  allow other functions to reuse the memory previously allocated in shared
  memory.

  @param[in] Addr    Pointer to the shared memory block to be freed.

  @par Dependencies
  SmemInit() must have been called on this processor.
  The pointer must have been allocated using SmemAlloc().
**/
VOID
EFIAPI
SmemFree (
  IN VOID  *Addr
  );

/**
  Sets the version number for this processor and a given object.

  The version number is compared to all previously set version numbers for this
  object. The last processor to register checks against all other processors.

  @param[in] Type       Type of object being version checked. It must be
                        between SmemVersionFirst and SmemVersionLast
                        in the SMEM_MEM_TYPE enum, unless it is of type
                        SmemVersionInfo (for internal use only).
  @param[in] Version    Local version number for this memory object.
  @param[in] Mask       Bitwise AND mask used to specify either SMEM or PROC_COMM
                        version when comparing against other processor version
                        numbers. To compare both (all 32 version bits), this
                        argument must be 0xFFFFFFFF.

  @retval TRUE  Version number of the local processor matches all previously
                registered versions for this object type.
  @retval FALSE There is a mismatch.

  @par Dependencies
  SmemInit() must have been called on this processor.

  @par Side Effects
  This function uses spinlocks for exclusive access to the shared memory heap.
**/
BOOLEAN
EFIAPI
SmemVersionSet (
  IN SMEM_MEM_TYPE  Type,
  IN UINT32         Version,
  IN UINT32         Mask
  );
