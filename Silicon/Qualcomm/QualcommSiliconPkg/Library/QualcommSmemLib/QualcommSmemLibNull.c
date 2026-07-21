/** @file
  NULL implementation of the Qualcomm Shared Memory (SMEM) library.

  All functions return safe default values suitable for platforms
  where SMEM is not available.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SMEM - Shared Memory
**/

#include <Library/QualcommSmemLib.h>

/**
  Construct a SMEM host identifier.

  @param[in]   ProcId   Processor ID (use QUALCOMM_SMEM_PROC_* macros).
  @param[in]   ProcNum  Processor instance number.
  @param[in]   PdNum    Protection-domain number.
  @param[in]   Chiplet  Chiplet identifier.
  @param[out]  Host     Output host identifier (must not be NULL).

  @retval  EFI_UNSUPPORTED  Always returns unsupported in this NULL implementation.
**/
EFI_STATUS
EFIAPI
QualcommSmemHostId (
  IN   UINT16              ProcId,
  IN   UINT16              ProcNum,
  IN   UINT16              PdNum,
  IN   UINT16              Chiplet,
  OUT  QUALCOMM_SMEM_HOST  *Host
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Initialize the SMEM common core.

  @retval  EFI_SUCCESS  Always returns success in this NULL implementation.
**/
EFI_STATUS
EFIAPI
QualcommSmemInit (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Look up an existing SMEM item.

  @param[in]   RemoteHost  Remote host for a host-pair partition, or
                           QUALCOMM_SMEM_HOST_COMMON for the common partition.
  @param[in]   Item        SMEM item ID.
  @param[in]   Flags       Must be QUALCOMM_SMEM_FLAG_NONE.
  @param[out]  ItemPtr     Pointer to item payload (must not be NULL).
  @param[out]  ItemSize    Size of item payload in bytes (may be NULL).

  @retval  EFI_NOT_FOUND  Always returns not found in this NULL implementation.
**/
EFI_STATUS
EFIAPI
QualcommSmemLookup (
  IN      QUALCOMM_SMEM_HOST  RemoteHost,
  IN      UINT16              Item,
  IN      UINT32              Flags,
  OUT     VOID                **ItemPtr,
  OUT     UINTN               *ItemSize    OPTIONAL
  )
{
  return EFI_NOT_FOUND;
}
