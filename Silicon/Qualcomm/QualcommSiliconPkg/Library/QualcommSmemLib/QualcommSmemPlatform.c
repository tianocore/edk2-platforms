/** @file
  Qualcomm Shared Memory (SMEM) - UEFI platform integration layer.

  Implements the platform abstraction interface declared in QualcommSmemPlatform.h for
  UEFI environments.  SMEM is statically mapped as part of the QTI device
  region; physical and virtual addresses are identical (1:1 mapping).

  @par Glossary
    - SMEM - Shared Memory
    - Pa   - Physical Address
    - Va   - Virtual Address
    - Toc  - Table of Contents

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmLib.h>
#include <Library/QualcommSmemLib.h>
#include "QualcommSmemPlatform.h"

/**
  Map a region of SMEM into the virtual address space.

  SMEM is statically mapped as part of the QTI device region; no additional
  mapping is required.  Validates that Offset + Size does not exceed the SMEM
  region size.

  @param[in]  Offset  Byte offset from the SMEM physical base.
  @param[in]  Size    Number of bytes to map.
  @param[in]  Flags   QUALCOMM_SMEM_PLAT_MAP_RO or QUALCOMM_SMEM_PLAT_MAP_RW.

  @retval  EFI_SUCCESS            Success.
  @retval  EFI_INVALID_PARAMETER  Offset + Size exceeds the SMEM region.
**/
EFI_STATUS
EFIAPI
QualcommSmemPlatMap (
  IN  UINT32  Offset,
  IN  UINTN   Size,
  IN  UINT32  Flags
  )
{
  if ((UINT64)Offset + (UINT64)Size > (UINT64)QUALCOMM_SMEM_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Translate an SMEM offset to a virtual address.

  SMEM is statically mapped with a 1:1 physical-to-virtual mapping, so the
  virtual address is computed directly from the physical base and offset.

  @param[in]  Offset  Byte offset from the SMEM physical base.

  @return  Virtual address corresponding to Offset; NULL if Offset is invalid.
**/
VOID *
EFIAPI
QualcommSmemPlatGetAddr (
  IN  UINT32  Offset
  )
{
  if ((UINTN)Offset >= (UINTN)QUALCOMM_SMEM_SIZE) {
    return NULL;
  }

  return (VOID *)(UINTN)(QUALCOMM_SMEM_BASE + (UINT64)Offset);
}

/**
  Platform entry point for SMEM initialization.

  Discovers SMEM target parameters and writes them into PlatInfo.  The local
  host is set to the Trust-Zone processor identity; MaxItems and SmemSize are
  taken from the compile-time platform constants.

  @param[out]  PlatInfo  Caller-allocated struct to fill with parameters.

  @retval  EFI_SUCCESS            Success.
  @retval  EFI_INVALID_PARAMETER  PlatInfo is NULL.
  @retval  EFI_DEVICE_ERROR       Host identifier construction failed.
**/
EFI_STATUS
EFIAPI
QualcommSmemPlatInit (
  OUT  QUALCOMM_SMEM_PLAT_INFO  *PlatInfo
  )
{
  EFI_STATUS  Status;

  if (PlatInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = QualcommSmemHostId (
             QUALCOMM_SMEM_PROC_TZ,
             0U,
             0U,
             0U,
             &PlatInfo->LocalHost
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PlatInfo->MaxItems = 0xFFFFU;
  PlatInfo->SmemSize = QUALCOMM_SMEM_SIZE;

  return EFI_SUCCESS;
}

/**
  Full memory barrier.

  Issues a data memory barrier to ensure that all preceding memory accesses
  are globally visible before any subsequent accesses.
**/
VOID
EFIAPI
QualcommSmemPlatMemBarrier (
  VOID
  )
{
  ArmDataMemoryBarrier ();
}

/**
  Acquire the SMEM serialisation lock.

  In this stub implementation the lock is always immediately available.

  @retval  EFI_SUCCESS  Always.
**/
EFI_STATUS
EFIAPI
QualcommSmemPlatHwlockAcquire (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Release the SMEM serialisation lock.
**/
VOID
EFIAPI
QualcommSmemPlatHwlockRelease (
  VOID
  )
{
}

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
  )
{
}

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
  )
{
}
