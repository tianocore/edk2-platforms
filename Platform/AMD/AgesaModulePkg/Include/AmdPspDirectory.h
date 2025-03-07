/** @file
  AMD Psp Directory header file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_PSP_DIR_H_
#define AMD_PSP_DIR_H_

#pragma pack (push, 1)
#define PSP_DIRECTORY_HEADER_SIGNATURE      0x50535024ul  ///< $PSP
#define PSP_LV2_DIRECTORY_HEADER_SIGNATURE  0x324C5024ul  ///< $PL2

/// Enum define various enum type for PSP entry type
enum _PSP_DIRECTORY_ENTRY_TYPE {
  PSP_REGION_A_DIR = 0x48,                                ///< PSP entry points to PSP DIR in Region A
  PSP_REGION_B_DIR = 0x4A,                                ///< PSP entry points to PSP DIR in Region B
};

/// Define structure for PSP directory
typedef struct {
  UINT32    Cookie;                                       ///< "$PSP"
  UINT32    Checksum;                                     ///< 32 bit CRC of header items below and the entire table
  UINT32    TotalEntries;                                 ///< Number of PSP Entries
  UINT32    Reserved;                                     ///< Unused
} PSP_DIRECTORY_HEADER;

/// Define structure for 32 bits Entry type field
typedef struct {
  UINT32    Type       : 8;                               ///< Type of PSP Directory entry
  UINT32    SubProgram : 8;                               ///< Specify the SubProgram
  UINT32    RomId      : 2;                               ///< Specify the ROM ID
  UINT32    Reserved   : 14;                              ///< Reserved
} PSP_DIRECTORY_ENTRY_TYPE_FIELD;

/// Define union type for 32 bits Entry type field
typedef union {
  PSP_DIRECTORY_ENTRY_TYPE_FIELD    Field;                ///< Definition of each filed
  UINT32                            Value;                ///< Group it as 32bits Int
} PSP_DIRECTORY_ENTRY_TYPE;

/// Structure for PSP Entry
typedef struct {
  PSP_DIRECTORY_ENTRY_TYPE    Type;                       ///< Type of PSP entry; 32 bit long
  UINT32                      Size;                       ///< Size of PSP Entry in bytes
  UINT64                      Location;                   ///< Location of PSP Entry (byte offset from start of SPI-ROM)
} PSP_DIRECTORY_ENTRY;

/// Structure for PSP directory
typedef struct {
  PSP_DIRECTORY_HEADER    Header;                         ///< PSP directory header
  PSP_DIRECTORY_ENTRY     PspEntry[1];                    ///< Array of PSP entries each pointing to a binary in SPI flash
                                                          ///< The actual size of this array comes from the
                                                          ///< header (PSP_DIRECTORY.Header.TotalEntries)
} PSP_DIRECTORY;

#pragma pack (pop)
#endif //AMD_PSP_DIR_H_
