/** @file
  RAM Partition Table Definitions.

  This header file gives the external definition of the RAM partition table.
  The table holds attributes for each RAM region and is stored in shared memory.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - APPBL   - Application Processor Boot Loader
    - IMEM    - Internal Memory
    - PASR    - Partial Array Self Refresh
    - QUANTUM - Qualcomm Native UEFI Platform
    - QUEST   - Qualcomm UEFI based SoC test suite 
**/

#pragma once

/// RAM partition version major number.
#define RAM_PARTITION_H_MAJOR  0x3
/// RAM partition version minor number.
#define RAM_PARTITION_H_MINOR  0x0

/// Total length of zero filled name string.
#define RAM_PART_NAME_LENGTH  16

/// Number of RAM partition entries usable by APPS.
#define RAM_NUM_PART_ENTRIES  32

/// Magic number 1 for valid RAM partition table.
#define RAM_PART_MAGIC1  0x9DA5E0A8
/// Magic number 2 for valid RAM partition table.
#define RAM_PART_MAGIC2  0xAF9EC4E2

/// RAM partition version number.
#define RAM_PARTITION_VERSION  0x3

/// Value indicating partition can grow to fill rest of RAM.
#define RAM_PARTITION_GROW  0xFFFFFFFF

/**
  Enumeration of RAM partition API return types.
**/
typedef enum {
  RamPartSuccess = 0,             ///< Successful return from API.
  RamPartNullPtrErr,              ///< Partition table/entry null pointer.
  RamPartOutOfBoundPtrErr,        ///< Partition table pointer is not in SMEM.
  RamPartTableEmptyErr,           ///< Trying to delete entry from empty table.
  RamPartTableFullErr,            ///< Trying to add entry to full table.
  RamPartCategoryNotExistErr,     ///< Partition doesn't belong to any memory category.
  RamPartOtherErr,                ///< Unknown error.
  RamPartReturnMaxSize = 0x7FFFFFFF
} RAM_PARTITION_RETURN_TYPE;

/**
  Enumeration of RAM partition attributes.
**/
typedef enum {
  RamPartitionDefaultAttrb = ~0,       ///< No specific attribute definition.
  RamPartitionReadOnly     = 0,        ///< Read-only RAM partition.
  RamPartitionReadwrite,               ///< Read/write RAM partition.
  RamPartitionAttributeMaxSize = 0x7FFFFFFF
} RAM_PARTITION_ATTRIBUTE;

/**
  Enumeration of RAM partition categories.
**/
typedef enum {
  RamPartitionDefaultCategory = ~0,    ///< No specific category definition.
  RamPartitionIram            = 4,     ///< IRAM RAM partition.
  RamPartitionImem            = 5,     ///< IMEM RAM partition.
  RamPartitionSdram           = 14,    ///< SDRAM type without specific bus information.
  RamPartitionCategoryMaxSize = 0x7FFFFFFF
} RAM_PARTITION_CATEGORY;

/**
  Enumeration of RAM partition domains.
**/
typedef enum {
  RamPartitionDefaultDomain = 0,       ///< No specific domain definition.
  RamPartitionAppsDomain    = 1,       ///< APPS RAM partition.
  RamPartitionModemDomain   = 2,       ///< MODEM RAM partition.
  RamPartitionDomainMaxSize = 0x7FFFFFFF
} RAM_PARTITION_DOMAIN;

/**
  Enumeration of RAM partition types.
**/
typedef enum {
  RamPartitionSysMemory = 1,           ///< System memory.
  RamPartitionBootRegionMemory1,       ///< Boot loader memory 1.
  RamPartitionBootRegionMemory2,       ///< Boot loader memory 2, reserved.
  RamPartitionAppblMemory,             ///< Apps boot loader memory.
  RamPartitionAppsMemory,              ///< HLOS usage memory.
  RamPartitionToolsFvMemory,           ///< Tools usage memory.
  RamPartitionQuantumFvMemory,         ///< Quantum usage memory.
  RamPartitionQuestFvMemory,           ///< Quest usage memory.
  RamPartitionAblMemory,               ///< ABL usage memory.
  RamPartitionTypeMaxSize = 0x7FFFFFFF
} RAM_PARTITION_TYPE;

/**
  Holds information for an entry in the RAM partition table.
**/
typedef struct {
  CHAR8     Name[RAM_PART_NAME_LENGTH];  ///< Partition name.
  UINT64    StartAddress;                ///< Partition start address in RAM.
  UINT64    Length;                      ///< Partition length in RAM in bytes.
  UINT32    PartitionAttribute;          ///< Partition attribute.
  UINT32    PartitionCategory;           ///< Partition category.
  UINT32    PartitionDomain;             ///< Partition domain.
  UINT32    PartitionType;               ///< Partition type.
  UINT32    NumPartitions;               ///< Number of partitions on device.
  UINT32    HwInfo;                      ///< Hardware information such as type and frequency.
  UINT8     HighestBankBit;              ///< Highest bit corresponding to a bank.
  UINT8     Reserve0;                    ///< Reserved for future use.
  UINT8     Reserve1;                    ///< Reserved for future use.
  UINT8     Reserve2;                    ///< Reserved for future use.
  UINT32    MinPasrSize;                 ///< Minimum PASR size in MB.
  UINT64    AvailableLength;             ///< Available partition length in RAM in bytes.
} RAM_PARTITION_ENTRY;

typedef RAM_PARTITION_ENTRY  *RAM_PART_ENTRY;
typedef RAM_PARTITION_ENTRY  *RAM_PART_ENTRY_PTR_TYPE;

/**
  Defines the RAM partition table structure.
**/
typedef struct {
  UINT32                 Magic1;                                 ///< Magic number to identify valid RAM partition table.
  UINT32                 Magic2;                                 ///< Magic number to identify valid RAM partition table.
  UINT32                 Version;                                ///< Version number to track structure definition changes.
  UINT32                 Reserved1;                              ///< Reserved for future use.

  UINT32                 NumPartitions;                          ///< Number of RAM partition table entries.

  UINT32                 Reserved2;                              ///< Added for 8 bytes alignment of header.

  RAM_PARTITION_ENTRY    RamPartEntry[RAM_NUM_PART_ENTRIES];     ///< RAM partition table entries.
} USABLE_RAM_PARTITION_TABLE;

typedef USABLE_RAM_PARTITION_TABLE *USABLE_RAM_PART_TABLE_TYPE;

/**
  Holds used RAM information.
**/
typedef struct {
  UINTN    StartAddress;  ///< Partition start address.
  UINTN    Length;        ///< Partition length.
} USED_RAM_INFO_TYPE;

typedef USED_RAM_INFO_TYPE USED_RAM_INFO_TYPE_STRUCT;
typedef USED_RAM_INFO_TYPE *USED_RAM_INFO_PTR_TYPE;

// Version 1 structure 32 Bit

/**
  Holds information for an entry in the RAM partition table (version 1).
**/
typedef struct {
  CHAR8     Name[RAM_PART_NAME_LENGTH];  ///< Partition name.
  UINT64    StartAddress;                ///< Partition start address in RAM.
  UINT64    Length;                      ///< Partition length in RAM in bytes.
  UINT32    PartitionAttribute;          ///< Partition attribute.
  UINT32    PartitionCategory;           ///< Partition category.
  UINT32    PartitionDomain;             ///< Partition domain.
  UINT32    PartitionType;               ///< Partition type.
  UINT32    NumPartitions;               ///< Number of partitions on device.
  UINT32    HwInfo;                      ///< Hardware information such as type and frequency.
  UINT32    Reserved4;                   ///< Reserved for future use.
  UINT32    Reserved5;                   ///< Reserved for future use.
} RAM_PARTITION_ENTRY_V1;

typedef RAM_PARTITION_ENTRY_V1 *RAM_PART_ENTRY_V1;
typedef RAM_PARTITION_ENTRY_V1 *RAM_PART_ENTRY_PTR_TYPE_V1;

/**
  Defines the RAM partition table structure (version 1).
**/
typedef struct {
  UINT32                    Magic1;                                  ///< Magic number to identify valid RAM partition table.
  UINT32                    Magic2;                                  ///< Magic number to identify valid RAM partition table.
  UINT32                    Version;                                 ///< Version number to track structure definition changes.
  UINT32                    Reserved1;                               ///< Reserved for future use.

  UINT32                    NumPartitions;                           ///< Number of RAM partition table entries.

  UINT32                    Reserved2;                               ///< Added for 8 bytes alignment of header.

  RAM_PARTITION_ENTRY_V1    RamPartEntryV1[RAM_NUM_PART_ENTRIES];    ///< RAM partition table entries.
} USABLE_RAM_PARTITION_TABLE_V1;

typedef USABLE_RAM_PARTITION_TABLE_V1 *USABLE_RAM_PART_TABLE_TYPE_V1;
