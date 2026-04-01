/** @file
  Header file for Config Block Lib implementation

  Copyright (c) 2019 - 2026 Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _CONFIG_BLOCK_H_
#define _CONFIG_BLOCK_H_

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#pragma pack (push,1)

///
/// Config Block Header
///
typedef struct _CONFIG_BLOCK_HEADER {
  EFI_HOB_GUID_TYPE GuidHob;                      ///< Offset 0-23  GUID extension HOB header
  UINT8             Revision;                     ///< Offset 24    Revision of this config block
  UINT8             Attributes;                   ///< Offset 25    The main revision for config block
  UINT8             Reserved[2];                  ///< Offset 26-27 Reserved for future use
} CONFIG_BLOCK_HEADER;

///
/// Config Block
///
typedef struct _CONFIG_BLOCK {
  CONFIG_BLOCK_HEADER            Header;          ///< Offset 0-27  Header of config block
  //
  // Config Block Data
  //
} CONFIG_BLOCK;

///
/// Attributes bit definition: set in CONFIG_BLOCK_HEADER.Attributes to indicate
/// that this config block uses CONFIG_BLOCK_HEADER2 and carries a Namespace GUID
/// at offset 32.
///
#define CONFIG_BLOCK_HEADER2_ATTRIBUTE  BIT0

///
/// Version 2 Config Block Header - supports multi-instance IP blocks.
/// The Namespace disambiguates multiple blocks sharing the same GuidHob.Name.
/// Namespace can be a UUID v4 GUID to disambiguate between different logical
/// partitions, or a UUID v5 GUID derived from a UUID v4 GUID and an instance
/// index when more than one instance of a logical partition exists.
///
/// This struct is layout-compatible with CONFIG_BLOCK_HEADER: the fields
/// GuidHob, Revision, Attributes, and Reserved are at identical offsets,
/// so a CONFIG_BLOCK_HEADER2 * can safely be cast to CONFIG_BLOCK_HEADER *.
///
typedef struct _CONFIG_BLOCK_HEADER2 {
  EFI_HOB_GUID_TYPE GuidHob;        ///< Offset 0-23  GUID extension HOB header
  UINT8             Revision;       ///< Offset 24    Revision of this config block
  UINT8             Attributes;     ///< Offset 25    Identifying properties; BIT0 = CONFIG_BLOCK_HEADER2_ATTRIBUTE
  UINT8             Reserved[2];    ///< Offset 26-27 Reserved for future use
  UINT8             Reserved2[4];   ///< Offset 28-31 Reserved for future use; naturally aligns Namespace
  EFI_GUID          Namespace;      ///< Offset 32-47 Disambiguates scope for identically structured blocks
} CONFIG_BLOCK_HEADER2;

///
/// Config Block using the version 2 header.
///
typedef struct _CONFIG_BLOCK2 {
  CONFIG_BLOCK_HEADER2           Header;   ///< Offset 0-47  Config Block Header
  //
  // Config Block Data
  //
} CONFIG_BLOCK2;

///
/// Compile-time assertions that enforce the layout-compatibility and alignment
/// guarantees for CONFIG_BLOCK_HEADER2.
///
#ifdef STATIC_ASSERT
STATIC_ASSERT (
  OFFSET_OF (CONFIG_BLOCK_HEADER2, Reserved2) == sizeof (CONFIG_BLOCK_HEADER),
  "CONFIG_BLOCK_HEADER2 fields before Reserved2 must exactly match CONFIG_BLOCK_HEADER's layout so a CONFIG_BLOCK_HEADER2 * can be safely cast to CONFIG_BLOCK_HEADER *"
  );
STATIC_ASSERT (
  (OFFSET_OF (CONFIG_BLOCK_HEADER2, Namespace) % 8) == 0,
  "CONFIG_BLOCK_HEADER2.Namespace must start on an 8-byte aligned offset"
  );
STATIC_ASSERT (
  (sizeof (CONFIG_BLOCK_HEADER2) % 8) == 0,
  "CONFIG_BLOCK_HEADER2 size must be a multiple of 8 bytes so the body of the config block is 8-byte aligned"
  );
#endif

///
/// Config Block Table Header
///
typedef struct _CONFIG_BLOCK_TABLE_STRUCT {
  CONFIG_BLOCK_HEADER            Header;          ///< Offset 0-27  GUID number for main entry of config block
  UINT8                          Rsvd0[2];        ///< Offset 28-29 Reserved for future use
  UINT16                         NumberOfBlocks;  ///< Offset 30-31 Number of config blocks (N)
  UINT32                         AvailableSize;   ///< Offset 32-35 Current config block table size
///
/// Individual Config Block Structures are added here in memory as part of AddConfigBlock()
///
} CONFIG_BLOCK_TABLE_HEADER;
#pragma pack (pop)

#endif // _CONFIG_BLOCK_H_
