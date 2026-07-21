/** @file
  Qualcomm Shared Memory (SMEM) - Public API.

  SMEM is a fixed-size, physically contiguous shared memory region used as
  the primary inter-processor communication (IPC) substrate on Qualcomm SoCs.
  It is allocated at boot time by the boot firmware and remains accessible
  to all participating processors for the lifetime of the system.  Each
  processor (host) that participates in SMEM has a unique host identifier
  and may read from or write to the portions of SMEM that it is permitted
  to access.

  Memory Layout
  -------------
  The SMEM region is divided into three logical areas:
    - A boot/static metadata page at the start, containing version info.
    - A set of partitions in the middle, each owned by one or two hosts.
    - A Table of Contents (TOC) at the end, describing all partitions.

  Partition Model
  ---------------
  SMEM data is organised into partitions of two kinds:

    Common partition:  accessible to all hosts; holds globally shared items.
    Edge-pair partition:  shared between exactly two hosts; holds items
                          private to that host pair.

  Within each partition, items are stored in two heap regions that grow
  toward each other: an uncached/upward region and a cached/downward region.

  NOTE: This driver supports lookup of uncached/upward items only.
  Cached/downward item allocation and lookup are not implemented; items
  that exist only in the cached region are not visible to this driver.

  Item Model
  ----------
  An SMEM item is a data blob identified by a 16-bit item ID.

  Host Identifier Model
  ---------------------
  Each processor is assigned an opaque 16-bit host identifier that encodes
  the processor type, instance, protection domain, and chiplet.  Two
  special values are reserved: QUALCOMM_SMEM_HOST_COMMON (common partition
  access) and QUALCOMM_SMEM_HOST_INVALID (unset/error sentinel).  Host
  identifiers must be constructed with QualcommSmemHostId(), never directly.

  @par Glossary
    - Smem - Shared Memory
    - Proc - Processor
    - Pd   - Protection-domain
    - Ipc  - Inter-processor Communication
    - Toc  - Table of Contents

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/BaseLib.h>
#include <Uefi.h>

/**
  QUALCOMM_SMEM_HOST - SMEM host (processor) identifier.

  Opaque 16-bit host identifier.  Use QualcommSmemHostId() to construct
  a valid value from a processor ID, instance number, protection-domain
  number, and chiplet identifier.  Do not construct host values directly
  or rely on the internal bit layout.
**/
typedef UINT16 QUALCOMM_SMEM_HOST;

/**
  QUALCOMM_SMEM_HOST_COMMON - pseudo-host for the common SMEM partition.

  Items in the common partition are accessible to all hosts.
**/
#define QUALCOMM_SMEM_HOST_COMMON  ((QUALCOMM_SMEM_HOST)0xfffeU)

/**
  QUALCOMM_SMEM_HOST_INVALID - sentinel value for an invalid or unset host.
**/
#define QUALCOMM_SMEM_HOST_INVALID  ((QUALCOMM_SMEM_HOST)0xffffU)

// -------------------------------------------------------------------------
// Processor identifiers
//
// Pass one of these as the ProcId argument to QualcommSmemHostId().
// -------------------------------------------------------------------------
#define QUALCOMM_SMEM_PROC_APPS        0U
#define QUALCOMM_SMEM_PROC_MODEM       1U
#define QUALCOMM_SMEM_PROC_ADSP        2U
#define QUALCOMM_SMEM_PROC_SSC         3U
#define QUALCOMM_SMEM_PROC_WCN         4U
#define QUALCOMM_SMEM_PROC_CDSP        5U
#define QUALCOMM_SMEM_PROC_RPM         6U
#define QUALCOMM_SMEM_PROC_TZ          7U
#define QUALCOMM_SMEM_PROC_SPSS        8U
#define QUALCOMM_SMEM_PROC_HYP         9U
#define QUALCOMM_SMEM_PROC_BOOT        10U
#define QUALCOMM_SMEM_PROC_SPSS_SP     11U
#define QUALCOMM_SMEM_PROC_CDSP1       12U
#define QUALCOMM_SMEM_PROC_WPSS        13U
#define QUALCOMM_SMEM_PROC_TME         14U
#define QUALCOMM_SMEM_PROC_APPS_VM_LA  15U
#define QUALCOMM_SMEM_PROC_EXT_PM      16U
#define QUALCOMM_SMEM_PROC_GPDSP       17U
#define QUALCOMM_SMEM_PROC_GPDSP1      18U
#define QUALCOMM_SMEM_PROC_SOCCP       19U
#define QUALCOMM_SMEM_PROC_OOBSS       20U
#define QUALCOMM_SMEM_PROC_OOBNS       21U
#define QUALCOMM_SMEM_PROC_DCP         22U
#define QUALCOMM_SMEM_PROC_WM          23U
#define QUALCOMM_SMEM_PROC_AM          24U
#define QUALCOMM_SMEM_PROC_QECP        25U
#define QUALCOMM_SMEM_PROC_LMCU        26U

// Lookup flags - currently only 0 is valid.
#define QUALCOMM_SMEM_FLAG_NONE  0U

// Ram Partition Location
#define SMEM_USABLE_RAM_PARTITION_TABLE  402U

// -------------------------------------------------------------------------
// Library version
//
// Encoded as a single UINT32:
//   bits [31:16]  major version
//   bits [15:8]   minor version
//   bits [7:0]    patch version
// -------------------------------------------------------------------------

#define QUALCOMM_SMEM_LIB_VERSION_MAJOR  1U
#define QUALCOMM_SMEM_LIB_VERSION_MINOR  0U
#define QUALCOMM_SMEM_LIB_VERSION_PATCH  0U

#define QUALCOMM_SMEM_LIB_VERSION                              \
  ((UINT32)(((QUALCOMM_SMEM_LIB_VERSION_MAJOR) << 16U) |      \
            ((QUALCOMM_SMEM_LIB_VERSION_MINOR) <<  8U) |      \
            ((QUALCOMM_SMEM_LIB_VERSION_PATCH)        )))

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

/**
  Construct a SMEM host identifier.

  Constructs a valid SMEM host identifier from the given parameters.
  The internal bit encoding is an implementation detail; callers must
  use this function rather than constructing host values directly.

  @param[in]   ProcId   Processor ID (use QUALCOMM_SMEM_PROC_* macros).
  @param[in]   ProcNum  Processor instance number.
  @param[in]   PdNum    Protection-domain number.
  @param[in]   Chiplet  Chiplet identifier.
  @param[out]  Host     Output host identifier (must not be NULL).

  @retval  EFI_SUCCESS            Host identifier constructed successfully.
  @retval  EFI_INVALID_PARAMETER  Host is NULL, any parameter is out of
                                  range, or the encoded value would collide
                                  with a reserved host identifier.
**/
EFI_STATUS
EFIAPI
QualcommSmemHostId (
  IN   UINT16              ProcId,
  IN   UINT16              ProcNum,
  IN   UINT16              PdNum,
  IN   UINT16              Chiplet,
  OUT  QUALCOMM_SMEM_HOST  *Host
  );

/**
  Look up an existing SMEM item.

  Searches the SMEM partition selected by RemoteHost for an item with the
  given item ID.

  @param[in]   RemoteHost  Remote host for a host-pair partition, or
                           QUALCOMM_SMEM_HOST_COMMON for the common partition.
  @param[in]   Item        SMEM item ID.
  @param[in]   Flags       Must be QUALCOMM_SMEM_FLAG_NONE.
  @param[out]  ItemPtr     Pointer to item payload (must not be NULL).
  @param[out]  ItemSize    Size of item payload in bytes (may be NULL).

  @retval  EFI_SUCCESS            ItemPtr points to the item payload.
  @retval  EFI_INVALID_PARAMETER  ItemPtr is NULL, RemoteHost is
                                  QUALCOMM_SMEM_HOST_INVALID, Flags is not
                                  QUALCOMM_SMEM_FLAG_NONE, or Item >= MaxItems.
  @retval  EFI_NOT_STARTED        Driver has not been initialized.
  @retval  EFI_NOT_FOUND          Item or partition not found.
  @retval  EFI_ACCESS_DENIED      Partition not mapped or access denied.
  @retval  EFI_DEVICE_ERROR       Corrupted shared-memory metadata.
**/
EFI_STATUS
EFIAPI
QualcommSmemLookup (
  IN      QUALCOMM_SMEM_HOST  RemoteHost,
  IN      UINT16              Item,
  IN      UINT32              Flags,
  OUT     VOID                **ItemPtr,
  OUT     UINTN               *ItemSize    OPTIONAL
  );

#ifdef __cplusplus
}
#endif
