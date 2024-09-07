/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_BDAT_H_
#define ACPI_BDAT_H_

#pragma pack (1)

#define EFI_BDAT_TABLE_SIGNATURE  SIGNATURE_32 ('B','D','A','T')
#define EFI_BDAT_TABLE_REVISION   0x1

#define BDAT_PRIMARY_VER    0x0004
#define BDAT_SECONDARY_VER  0x0000

/// Memory SPD Data Schema GUID
/// {1B19F809-1D91-4F00-A3F3-7A676606D3B1}
#define BDAT_MEM_SPD_GUID \
  { \
    0x1b19f809, 0x1d91, 0x4f00, { 0xa3, 0xf3, 0x7a, 0x67, 0x66, 0x6, 0xd3, 0xb1 } \
  }

/// Memory SPD data identification GUID
/// {46F60B90-9C94-43CA-A77C-09B848999348}
#define MEM_SPD_DATA_ID_GUID\
  { \
    0x46f60b90, 0x9c94, 0x43ca, { 0xa7, 0x7c, 0x9, 0xb8, 0x48, 0x99, 0x93, 0x48 } \
  };

typedef struct {
  UINT8     BiosDataSignature[8]; // "BDATHEAD"
  UINT32    BiosDataStructSize;   // sizeof BDAT_STRUCTURE + sizeof BDAT_MEMORY_DATA_STRUCTURE + sizeof BDAT_RMT_STRUCTURE
  UINT16    Crc16;                // 16-bit CRC of BDAT_STRUCTURE (calculated with 0 in this field)
  UINT16    Reserved;
  UINT16    PrimaryVersion;     // Primary version
  UINT16    SecondaryVersion;   // Secondary version
  UINT32    OemOffset;          // Optional offset to OEM-defined structure
  UINT32    Reserved1;
  UINT32    Reserved2;
} BDAT_HEADER_STRUCTURE;

typedef struct {
  UINT16    SchemaListLength;                    // Number of Schemas present
  UINT16    Reserved;
  UINT16    Year;
  UINT8     Month;
  UINT8     Day;
  UINT8     Hour;
  UINT8     Minute;
  UINT8     Second;
  UINT8     Reserved1;
  UINT32    SchemaOffsets[];
  //
  // This is a dynamic region, where Schema list address is filled out.
  // Each schema location is 32 bits long and complies with BDAT 4.0 version.
  //
} BDAT_SCHEMA_LIST_STRUCTURE;

// BDAT Header Struct which contains information of all existing BDAT Schemas
typedef struct {
  BDAT_HEADER_STRUCTURE         BdatHeader;
  BDAT_SCHEMA_LIST_STRUCTURE    BdatSchemas;
} BDAT_STRUCTURE;

#define MAX_SPD_BYTE  512

typedef struct BdatSchemaHeader {
  EFI_GUID    SchemaId;
  UINT32      DataSize;
  UINT16      Crc16;
} BDAT_SCHEMA_HEADER_STRUCTURE;

/// Memory SPD Data Header
typedef struct {
  EFI_GUID    MemSpdGuid; /// GUID that uniquely identifies the memory SPD data revision
  UINT32      Size;       /// Total size in bytes including the header and all SPD data
  UINT32      Crc;        /// 32-bit CRC generated over the whole size minus this crc field
                          /// Note: UEFI 32-bit CRC implementation (CalculateCrc32)
                          /// Consumers can ignore CRC check if not needed.

  UINT32      Reserved;  /// Reserved for future use, must be initialized to 0
} MEM_SPD_RAW_DATA_HEADER;

/// Memory SPD Raw Data
typedef struct {
  MEM_SPD_RAW_DATA_HEADER    Header;
  //
  // This is a dynamic region, where SPD data entries are filled out.
  //
} MEM_SPD_DATA_STRUCTURE;

typedef struct {
  BDAT_SCHEMA_HEADER_STRUCTURE    SchemaHeader;
  MEM_SPD_DATA_STRUCTURE          SpdData;
} BDAT_MEM_SPD_STRUCTURE;

/// List of all entry types supported by this revision of memory SPD data structure
typedef enum {
  MemSpdDataType0 = 0,
  MemTrainDataTypeMax,
  MemTrainDataTypeDelim = MAX_INT32
} MEM_SPD_DATA_TYPE;

/// Generic entry header for all memory SPD raw data entries
typedef struct {
  MEM_SPD_DATA_TYPE    Type;
  UINT16               Size;       /// Entries will be packed by byte in contiguous space.
                                   /// Size of the entry includes the header.
} MEM_SPD_DATA_ENTRY_HEADER;

/// Structure to specify SPD dimm memory location
typedef struct {
  UINT8    Socket;
  UINT8    Channel;
  UINT8    Dimm;
} MEM_SPD_DATA_ENTRY_MEMORY_LOCATION;

/// Type 0: SPD RDIMM/LRDIMM DDR4 or DDR5
/// The NumberOfBytes are 512 and 1024 for DDR4 and DDR5 respectively.
typedef struct {
  MEM_SPD_DATA_ENTRY_HEADER             Header;
  MEM_SPD_DATA_ENTRY_MEMORY_LOCATION    MemoryLocation;
  UINT16                                NumberOfBytes;
  //
  // This is a dynamic region, where SPD data are filled out.
  // The total number of bytes of the SPD data must match NumberOfBytes
  //
} MEM_SPD_ENTRY_TYPE0;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER               Header;
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE    BdatGas;
} EFI_BDAT_ACPI_DESCRIPTION_TABLE;

#pragma pack()

#endif // ACPI_BDAT_H_
