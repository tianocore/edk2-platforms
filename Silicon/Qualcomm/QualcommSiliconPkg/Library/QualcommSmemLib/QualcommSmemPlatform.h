/** @file
  Qualcomm Shared Memory (SMEM) - Internal platform abstraction interface.

  Mapping contract
  ----------------
  The platform must reserve a contiguous virtual-address window for the
  entire SMEM physical range before QualcommSmemInit() is called.  The
  window is reserved but not backed by page-table entries.

  QualcommSmemPlatMap() is called by the SMEM core to install page-table
  entries for specific sub-ranges (BOOT info page, TOC page, individual
  partitions).  The virtual address returned by QualcommSmemPlatGetAddr()
  must satisfy:

    VA(Offset) == SmemReservedVaBase + Offset

  so that the core can compute partition VAs from offsets without any
  additional translation.

  Successful mappings are kept alive for the lifetime of the driver.
  There is no unmap path in the common core.

  @par Glossary
    - Smem - Shared Memory
    - Toc  - Table of Contents
    - Va   - Virtual Address
    - Pa   - Physical Address
    - Abi  - Application Binary Interface

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/BaseMemoryLib.h>
#include <Library/QualcommSmemLib.h>

// -------------------------------------------------------------------------
// Mapping attribute flags passed to QualcommSmemPlatMap().
// -------------------------------------------------------------------------

/// Install a read-only mapping.
#define QUALCOMM_SMEM_PLAT_MAP_RO  0x1U

/// Install a read-write mapping.
#define QUALCOMM_SMEM_PLAT_MAP_RW  0x2U

// -------------------------------------------------------------------------
// SMEM size and Base Address
// -------------------------------------------------------------------------
#define QUALCOMM_SMEM_SIZE  0x200000U
#define QUALCOMM_SMEM_BASE  0xFFE00000U

// -------------------------------------------------------------------------
// Platform target information
// -------------------------------------------------------------------------

//
// QUALCOMM_SMEM_PLAT_INFO - platform-supplied SMEM target parameters.
//
typedef struct {
  QUALCOMM_SMEM_HOST    LocalHost; ///< Local processor host ID.
  UINT16                MaxItems;  ///< Exclusive upper bound for item IDs.
  UINTN                 SmemSize;  ///< Total SMEM region size in bytes.
} QUALCOMM_SMEM_PLAT_INFO;

// -------------------------------------------------------------------------
// Internal init API (not exposed in the public header)
// -------------------------------------------------------------------------

/**
  Initialize the SMEM common core.

  Called by platform boot integration code after the platform has
  reserved the SMEM virtual address window.  Internally calls
  QualcommSmemPlatInit() to obtain platform target parameters, then
  maps and validates the BOOT info page, TOC, and relevant partitions.

  @retval  EFI_SUCCESS          Success.
  @retval  EFI_ALREADY_STARTED  Already initialized.
  @retval  other                EFI_STATUS error code on failure.
**/
EFI_STATUS
EFIAPI
QualcommSmemInit (
  VOID
  );

// -------------------------------------------------------------------------
// Platform operation interface
//
// Implemented once per platform in QualcommSmemPlatform.c.
// The common core calls these; it does not know PA/VA/MMU details.
// -------------------------------------------------------------------------

/**
  Platform entry point for SMEM initialization.

  Discovers SMEM target parameters (PA base, size, MaxItems, local host)
  from WONCE registers or device tree and writes them into PlatInfo.

  PA base and VA base are stored in platform-private state only.

  @param[out]  PlatInfo  Caller-allocated struct to fill with parameters.

  @retval  EFI_SUCCESS            Success.
  @retval  EFI_INVALID_PARAMETER  PlatInfo is NULL.
  @retval  EFI_DEVICE_ERROR       Target info is unavailable or corrupted.
**/
EFI_STATUS
EFIAPI
QualcommSmemPlatInit (
  OUT  QUALCOMM_SMEM_PLAT_INFO  *PlatInfo
  );

/**
  Map a region of SMEM into the virtual address space.

  The platform converts the offset to physical and virtual addresses:
    PA = SmemPaBase + Offset
    VA = SmemReservedVaBase + Offset

  Must be idempotent: if the range is already mapped with compatible
  permissions, return EFI_SUCCESS without error.

  Must validate that Offset + Size does not exceed SmemSize.

  @param[in]  Offset  Byte offset from the SMEM physical base.
  @param[in]  Size    Number of bytes to map.
  @param[in]  Flags   QUALCOMM_SMEM_PLAT_MAP_RO or QUALCOMM_SMEM_PLAT_MAP_RW.

  @retval  EFI_SUCCESS  Success.
  @retval  other        EFI_STATUS error code on failure.
**/
EFI_STATUS
EFIAPI
QualcommSmemPlatMap (
  IN  UINT32  Offset,
  IN  UINTN   Size,
  IN  UINT32  Flags
  );

/**
  Translate an SMEM offset to a virtual address.

  Returns the virtual address corresponding to the given offset within
  the pre-reserved SMEM virtual address window.  Must not perform any
  new mapping; the region must have been mapped by QualcommSmemPlatMap()
  before this function is called.

  @param[in]  Offset  Byte offset from the SMEM physical base.

  @return  Virtual address on success; NULL if the offset is invalid or
           the region has not been mapped.
**/
VOID *
EFIAPI
QualcommSmemPlatGetAddr (
  IN  UINT32  Offset
  );

/**
  Full memory barrier.

  Ensures that all preceding memory accesses are globally visible before
  any subsequent accesses.  Used before reading mutable shared-memory
  metadata that may have been written by a remote processor.
**/
VOID
EFIAPI
QualcommSmemPlatMemBarrier (
  VOID
  );

/**
  Acquire the SMEM serialisation lock.

  Must serialise concurrent calls to QualcommSmemLookup() on the local
  processor.  Production ports should also acquire the SMEM hardware
  spinlock to serialise against remote writers that update partition
  heap pointers when allocating new items.

  Do NOT hold this lock during MMU mapping operations.

  @retval  EFI_SUCCESS  Success.
  @retval  other        EFI_STATUS error code on failure.
**/
EFI_STATUS
EFIAPI
QualcommSmemPlatHwlockAcquire (
  VOID
  );

/**
  Release the SMEM serialisation lock.

  Must be called on every return path after a successful
  QualcommSmemPlatHwlockAcquire().
**/
VOID
EFIAPI
QualcommSmemPlatHwlockRelease (
  VOID
  );

/**
  Log an error-level message.

  @param[in]  Fmt  printf-style format string.
  @param[in]  ...  Variadic arguments.
**/
VOID
EFIAPI
QualcommSmemPlatLogErr (
  IN  CONST CHAR8  *Fmt,
  ...
  );

/**
  Log a debug-level message.

  @param[in]  Fmt  printf-style format string.
  @param[in]  ...  Variadic arguments.
**/
VOID
EFIAPI
QualcommSmemPlatLogDbg (
  IN  CONST CHAR8  *Fmt,
  ...
  );
