/** @file
  Qualcomm Shared Memory (SMEM) - Internal protocol definitions.

  Defines protocol constants, wire-format ABI structures, and the driver
  state type used exclusively by QualcommSmemLib.c.  This header is not part of
  the public API and must not be included by external code.

  @par Glossary
    - Smem - Shared Memory
    - Abi  - Application Binary Interface
    - Toc  - Table of Contents
    - Ipc  - Inter-processor Communication

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "QualcommSmemPlatform.h"

// -------------------------------------------------------------------------
// Protocol constants
// -------------------------------------------------------------------------

//
// BOOT info page: the first 4096 bytes of SMEM contain BOOT/static
// version metadata.  This page is mapped read-only during init.
//
// Layout of the first 4096 bytes:
//   [0,   64): ProcComm[16]  - legacy IPC mechanism (16 x UINT32)
//   [64, 192): Ver[32]       - version array (32 x UINT32)
//   [192, 4096): ...         - other static metadata
//
// The BOOT SMEM version is at Ver[QUALCOMM_SMEM_VERSION_BOOT_OFFSET].
//
#define QUALCOMM_SMEM_BOOT_INFO_SIZE  4096U

// TOC page size: the TOC occupies the last 4096 bytes of SMEM.
#define QUALCOMM_SMEM_TOC_SIZE  4096U

// Supported TOC version.
#define QUALCOMM_SMEM_TOC_VERSION  1U

// Maximum number of TOC entries processed (bounds the TOC walk).
#define QUALCOMM_SMEM_TOC_MAX_ENTRIES  40U

//
// TOC magic: "$TOC" stored as a little-endian UINT32.
//   '$' = 0x24, 'T' = 0x54, 'O' = 0x4F, 'C' = 0x43
//
#define QUALCOMM_SMEM_TOC_MAGIC  0x434F5424U

//
// Partition magic: "$PRT" stored as a little-endian UINT32.
//   '$' = 0x24, 'P' = 0x50, 'R' = 0x52, 'T' = 0x54
//
#define QUALCOMM_SMEM_PART_MAGIC  0x54525024U

// Canary value stored in every item header.
#define QUALCOMM_SMEM_ITEM_CANARY  0xa5a5U

//
// BOOT SMEM version constants.
//
// The BOOT SMEM version word is stored at index QUALCOMM_SMEM_VERSION_BOOT_OFFSET
// within the Ver[] array of QUALCOMM_SMEM_STATIC_HEADER.
//
// Version word format:
//   bits [31:16] = major version
//   bits [15:0]  = minor version
//
// This driver supports major version 0x000C (12).
// Minor version differences are accepted if the layout is compatible.
//
#define QUALCOMM_SMEM_VERSION_ID           0x000C0001U
#define QUALCOMM_SMEM_MAJOR_VERSION_MASK   0xffff0000U
#define QUALCOMM_SMEM_MINOR_VERSION_MASK   0x0000ffffU
#define QUALCOMM_SMEM_VERSION_BOOT_OFFSET  7U

//
// QUALCOMM_SMEM_HOST_INVALID sentinel - duplicated here as a UINT16 literal
// to avoid casting the public macro in internal comparisons.
//
#define QUALCOMM_SMEM_HOST_INVALID_U16  0xffffU

//
// Internal multi-host partition marker.
// A TOC entry with Host0 == Host1 == QUALCOMM_SMEM_HOST_MULTIHOST describes
// a multi-host partition whose membership is encoded in HostsBitmap.
// Not exposed publicly.
//
#define QUALCOMM_SMEM_HOST_MULTIHOST  0xfffcU

// -------------------------------------------------------------------------
// Host-ID encoding
//
// A regular host ID is a 16-bit value encoded as:
//
//   bits [5:0]   ProcId    (6 bits, values 0-63)
//   bits [9:6]   ProcNum   (4 bits, values 0-15)
//   bits [12:10] PdNum     (3 bits, values 0-7)
//   bits [15:13] Chiplet   (3 bits, values 0-7)
//
// Special values that must never be produced by QualcommSmemHostId():
//   QUALCOMM_SMEM_HOST_COMMON    = 0xfffe
//   QUALCOMM_SMEM_HOST_INVALID   = 0xffff
//   QUALCOMM_SMEM_HOST_MULTIHOST = 0xfffc
// -------------------------------------------------------------------------

#define HOST_PROC_ID_BITS   6U
#define HOST_PROC_NUM_BITS  4U
#define HOST_PD_NUM_BITS    3U
#define HOST_CHIPLET_BITS   3U

#define HOST_PROC_ID_SHIFT   0U
#define HOST_PROC_NUM_SHIFT  6U
#define HOST_PD_NUM_SHIFT    10U
#define HOST_CHIPLET_SHIFT   13U

#define HOST_PROC_ID_MASK   ((1U << HOST_PROC_ID_BITS)  - 1U)
#define HOST_PROC_NUM_MASK  ((1U << HOST_PROC_NUM_BITS) - 1U)
#define HOST_PD_NUM_MASK    ((1U << HOST_PD_NUM_BITS)   - 1U)
#define HOST_CHIPLET_MASK   ((1U << HOST_CHIPLET_BITS)  - 1U)

// -------------------------------------------------------------------------
// Shared-memory ABI structures
//
// These structures represent the fixed binary layout of the SMEM protocol.
// They are internal to the driver and must NOT be exposed in any public header.
// -------------------------------------------------------------------------

#pragma pack (1)

//
// QUALCOMM_SMEM_STATIC_HEADER - layout of the first 4096 bytes of SMEM.
//
// The Ver[] array at offset 64 contains version words for each SMEM
// subsystem.  Index QUALCOMM_SMEM_VERSION_BOOT_OFFSET (7) holds the BOOT
// SMEM version that this driver validates during init.
//
typedef struct {
  UINT32    ProcComm[16]; ///< Legacy IPC: 16 x UINT32 = 64 bytes.
  UINT32    Ver[32];      ///< Version array: 32 x UINT32 = 128 bytes.
} QUALCOMM_SMEM_STATIC_HEADER;

//
// QUALCOMM_SMEM_TOC_HEADER - SMEM partition table (TOC) header.
//
// Located at: SmemBase + SmemSize - QUALCOMM_SMEM_TOC_SIZE.
// Immediately followed by an array of QUALCOMM_SMEM_TOC_ENTRY records.
//
typedef struct {
  UINT32    Magic;        ///< Must equal QUALCOMM_SMEM_TOC_MAGIC.
  UINT32    Version;      ///< Must equal QUALCOMM_SMEM_TOC_VERSION.
  UINT32    NumEntries;   ///< Number of valid QUALCOMM_SMEM_TOC_ENTRY records.
  UINT32    MinorVersion; ///< Minor version; informational only.
  UINT32    Reserved[4];  ///< Reserved; not validated.
} QUALCOMM_SMEM_TOC_HEADER;

//
// QUALCOMM_SMEM_TOC_ENTRY - one entry in the SMEM partition table.
//
// Entries follow the QUALCOMM_SMEM_TOC_HEADER immediately in memory.
// Host0 and Host1 use UINT16 to match the SMEM wire format.
// Do NOT use QUALCOMM_SMEM_HOST here; the ABI layout must remain stable.
//
typedef struct {
  UINT32    Offset;            ///< Byte offset of partition from SMEM base.
  UINT32    Size;              ///< Partition size in bytes.
  UINT32    Flags;             ///< Reserved flags.
  UINT16    Host0;             ///< First host identifier (wire UINT16).
  UINT16    Host1;             ///< Second host identifier (wire UINT16).
  UINT32    SizeCacheline;     ///< Cached-item alignment (0 = default).
  UINT32    Reserved[3];       ///< Reserved; not validated.
  UINT32    ExclusionSizes[4]; ///< Per-host exclusion region sizes.
} QUALCOMM_SMEM_TOC_ENTRY;

//
// QUALCOMM_SMEM_PARTITION_HEADER - header at the start of each partition.
//
// Host0 and Host1 use UINT16 to match the SMEM wire format.
// Do NOT use QUALCOMM_SMEM_HOST here; the ABI layout must remain stable.
//
// OffsetFreeUncached: end of the allocated uncached (upward) region.
//   Grows upward from sizeof (QUALCOMM_SMEM_PARTITION_HEADER).
// OffsetFreeCached: start of the allocated cached (downward) region.
//   Grows downward from partition size.
//
typedef struct {
  UINT32    Magic;              ///< Must equal QUALCOMM_SMEM_PART_MAGIC.
  UINT16    Host0;              ///< First host identifier (wire UINT16).
  UINT16    Host1;              ///< Second host identifier (wire UINT16).
  UINT32    Size;               ///< Total partition size in bytes.
  UINT32    OffsetFreeUncached; ///< End of allocated uncached region.
  UINT32    OffsetFreeCached;   ///< Start of allocated cached region.
  UINT32    Reserved[3];        ///< Reserved; not validated.
} QUALCOMM_SMEM_PARTITION_HEADER;

//
// QUALCOMM_SMEM_ITEM_HEADER - header preceding each allocated SMEM item.
//
// Uncached item layout (growing upward from partition header):
//   [QUALCOMM_SMEM_ITEM_HEADER][PaddingHeader bytes][data][PaddingData bytes]
//
// Data address:  Ptr + sizeof (QUALCOMM_SMEM_ITEM_HEADER) + PaddingHeader
// Data size:     Size - PaddingData
// Next header:   Ptr + sizeof (QUALCOMM_SMEM_ITEM_HEADER) + PaddingHeader + Size
//
typedef struct {
  UINT16    Canary;        ///< Must equal QUALCOMM_SMEM_ITEM_CANARY (0xa5a5).
  UINT16    Item;          ///< SMEM item identifier.
  UINT32    Size;          ///< Total rounded size including PaddingData.
  UINT16    PaddingData;   ///< Unused bytes at end of data region.
  UINT16    PaddingHeader; ///< Alignment gap between header and data.
  UINT32    Reserved;      ///< Reserved; not validated.
} QUALCOMM_SMEM_ITEM_HEADER;

#pragma pack ()

// Compile-time size assertions - catch layout regressions immediately.
STATIC_ASSERT (
  sizeof (QUALCOMM_SMEM_STATIC_HEADER)    == 192U,
  "QUALCOMM_SMEM_STATIC_HEADER size mismatch"
  );
STATIC_ASSERT (
  sizeof (QUALCOMM_SMEM_TOC_HEADER)       == 32U,
  "QUALCOMM_SMEM_TOC_HEADER size mismatch"
  );
STATIC_ASSERT (
  sizeof (QUALCOMM_SMEM_TOC_ENTRY)        == 48U,
  "QUALCOMM_SMEM_TOC_ENTRY size mismatch"
  );
STATIC_ASSERT (
  sizeof (QUALCOMM_SMEM_PARTITION_HEADER) == 32U,
  "QUALCOMM_SMEM_PARTITION_HEADER size mismatch"
  );
STATIC_ASSERT (
  sizeof (QUALCOMM_SMEM_ITEM_HEADER)      == 16U,
  "QUALCOMM_SMEM_ITEM_HEADER size mismatch"
  );

// -------------------------------------------------------------------------
// Driver state
// -------------------------------------------------------------------------

//
// QUALCOMM_SMEM_INFO - driver state.
//
typedef struct {
  BOOLEAN               Initialized;      ///< FALSE = uninitialized, TRUE = initialized.
  QUALCOMM_SMEM_HOST    LocalHost;        ///< Local host ID.
  UINT16                MaxItems;         ///< Maximum item ID (exclusive).
  UINT32                SmemSize;         ///< Total SMEM size in bytes.
  UINT32                TocOffset;        ///< Byte offset of TOC from SMEM base.
  UINT32                NumTocEntries;    ///< Validated TOC entry count.
  UINT32                CommonPartOffset; ///< Common partition offset.
  UINT32                CommonPartSize;   ///< Common partition size.
} QUALCOMM_SMEM_INFO;
