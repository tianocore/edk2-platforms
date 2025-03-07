/** @file
     AMD Psp Base Lib
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_PSP_BASELIB_V2_H_
#define AMD_PSP_BASELIB_V2_H_

#include <AMD.h>
#include <AmdPspDirectory.h>

#define PSP_MAILBOX_BASE           0x70                         ///< Mailbox base offset on PCIe BAR
#define PSP_MAILBOX_STATUS_OFFSET  0x4                          ///< Staus Offset
#define IS_ADDRESS_MODE_1(a)  (((a) >> 62) == 1 ? TRUE : FALSE) // relative to BIOS image base 0
#define IS_ADDRESS_MODE_2(a)  (((a) >> 62) == 2 ? TRUE : FALSE) // relative to current directory header
#define IS_ADDRESS_MODE_3(a)  (((a) >> 62) == 3 ? TRUE : FALSE) // relative to active image slot address (as of now, active image slot address is equal to PSP L2 base address)
#define IS_SPI_OFFSET(a)      (((a) & 0xFF000000) != 0xFF000000 ? TRUE : FALSE)

#define MaxDirEntryNumber     64
#define MaxPspDirSize         sizeof(PSP_DIRECTORY_HEADER) + (sizeof(BIOS_DIRECTORY_ENTRY) * MaxDirEntryNumber)
#define MAX_IMAGE_SLOT_COUNT  32

#define ALIGNMENT_4K  BASE_4KB
#define ALIGN_CHECK(addr, alignment)  ((((UINTN)(addr)) & ((alignment) - 1)) == 0)
#define ALIGN_4K_CHECK(addr)          ALIGN_CHECK((addr), ALIGNMENT_4K)

#define IS_VALID_ADDR32(addr)  (((UINT32)(addr) != 0) && (UINT32)(addr) != 0xFFFFFFFF)
#define MaxImageSlotInfoSize  sizeof(IMAGE_SLOT_INFO)
//
// offset between Active Image Slot address and PSP L2 Directory
//
#define PSP_L2_DIR_OFFSET  0

#pragma pack (push, 1)

///
/// X86 to PSP Buffer which start mapping from C2PMSG_28
///
typedef volatile struct {
  UINT32    Status        : 16;             ///< Set by the target to indicate the execution status of last command
  UINT32    CommandId     : 8;              ///< Command ID set by host
  UINT32    Reserved      : 5;              ///< Reserved
  UINT32    ResetRequired : 1;              // < Set by the target to indicate that the host has to execute warm reset if corrupted detected in tOS
  UINT32    Recovery      : 1;              ///< Set by the target to indicate that the host has to execute FW recovery sequence
  UINT32    Ready         : 1;              ///< Set by the target to indicate the mailbox interface state.
} PSP_MBOX_V2_CMD_EXT;

typedef volatile union {
  IN  UINT32                 Value;               ///< Cmd register value
  IN  PSP_MBOX_V2_CMD_EXT    Field;               ///< Extended Cmd register with field definition
} PSP_MBOX_V2_CMD;

typedef volatile struct {
  PSP_MBOX_V2_CMD    Cmd;
  UINT64             Buffer;               ///< 64 bit Ponter to memory with additional parameter.
} PSP_MBOX_V2;

#define FIRMWARE_TABLE_SIGNATURE  0x55AA55AAul
/// Define the structure OEM signature table
typedef struct _FIRMWARE_ENTRY_TABLEV2 {
  UINT32    Signature;        ///< 0x00 Signature should be 0x55AA55AAul
  UINT32    ImcRomBase;       ///< 0x04 Base Address for Imc Firmware
  UINT32    GecRomBase;       ///< 0x08 Base Address for Gmc Firmware
  UINT32    XHCRomBase;       ///< 0x0C Base Address for XHCI Firmware
  UINT32    LegacyPspDirBase; ///< 0x10 Base Address of PSP directory for legacy program (ML, BP, CZ, BR, ST)
  UINT32    PspDirBase;       ///< 0x14 Base Address for PSP directory
  UINT32    ZpBiosDirBase;    ///< 0x18 Base Address for ZP BIOS directory
  UINT32    RvBiosDirBase;    ///< 0x1C Base Address for RV BIOS directory
  UINT32    SspBiosDirBase;   ///< 0x20 Base Address for RV BIOS directory
  UINT32    Config;           ///< 0x24 reserved for EFS configuration
  UINT32    NewBiosDirBase;   ///< 0x28 Generic Base address for all program start from RN
  UINT32    PspDirBackupBase; ///< 0x2C Backup PSP directory address for all programs starting from RMB
} FIRMWARE_ENTRY_TABLEV2;

/// Unified Boot BIOS Directory structure
enum _BIOS_DIRECTORY_ENTRY_TYPE {
  BIOS_PUBLIC_KEY       = 0x05,               ///< PSP entry points to BIOS public key stored in SPI space
  BIOS_RTM_SIGNATURE    = 0x07,               ///< PSP entry points to signed BIOS RTM hash stored  in SPI space
  MAN_OS                = 0x5C,               ///< PSP entry points to manageability OS binary
  MAN_IP_LIB            = 0x5D,               ///< PSP entry points to manageability proprietary IP library
  MAN_CONFIG            = 0x5E,               ///< PSP entry points to manageability configuration inforamtion
  BIOS_APCB_INFO        = 0x60,               ///< Agesa PSP Customization Block (APCB)
  BIOS_APOB_INFO        = 0x61,               ///< Agesa PSP Output Block (APOB) target location
  BIOS_FIRMWARE         = 0x62,               ///< BIOS Firmware volumes
  APOB_NV_COPY          = 0x63,               ///< APOB data copy on non-volatile storage which will used by ABL during S3 resume
  PMU_INSTRUCTION       = 0x64,               ///< Location field pointing to the instruction portion of PMU firmware
  PMU_DATA              = 0x65,               ///< Location field pointing to the data portion of PMU firmware
  UCODE_PATCH           = 0x66,               ///< Microcode patch
  CORE_MCEDATA          = 0x67,               ///< Core MCE data
  BIOS_APCB_INFO_BACKUP = 0x68,               ///< Backup Agesa PSP Customization Block (APCB)
  BIOS_DIR_LV2          = 0x70,               ///< BIOS entry points to Level 2 BIOS DIR
};

/// Directory type
typedef enum _DIRECTORY_TYPE {
  DIR_TYPE_PSP_LV2  = 0,                      ///< Level 2 PSP DIR
  DIR_TYPE_BIOS_LV2 = 1,                      ///< Level 2 BIOS DIR
} DIRECTORY_TYPE;

/// Type attribute for BIOS Directory entry
typedef struct {
  UINT32    Type           : 8; ///< [0:7], Type of BIOS entry
  UINT32    RegionType     : 8; ///< [8:15], 0 Normal memory, 1 TA1 memory, 2 TA2 memor
  UINT32    BiosResetImage : 1; ///< [16], Set for SEC or EL3 fw, which will be authenticate by PSP FW known as HVB
  UINT32    Copy           : 1; ///< [17], Copy: 1- copy BIOS image image from source to destination 0- Set region attribute based on <ReadOnly, Source, size> attributes
  UINT32    ReadOnly       : 1; ///< [18], 1: Set region to read-only (applicable for ARM- TA1/TA2) 0: Set region to read/write
  UINT32    Compressed     : 1; ///< [19], 1: Compresed
  UINT32    Instance       : 4; ///< [20:23], Specify the Instance of an entry
  UINT32    SubProgram     : 3; ///< [24:26], < Specify the SubProgram
  UINT32    RomId          : 2; ///< [27:28], Specify the RomId
  UINT32    Reserved       : 3; ///< [29:31], Reserve for future use
} TYPE_ATTRIB;

/// Structure for PSP Entry
typedef struct {
  TYPE_ATTRIB    TypeAttrib;                    ///< Type of PSP entry; 32 bit long
  UINT32         Size;                          ///< Size of PSP Entry in bytes
  UINT64         Location;                      ///< Location of PSP Entry (byte offset from start of SPI-ROM)
  UINT64         Destination;                   ///< Destination of PSP Entry copy to
} BIOS_DIRECTORY_ENTRY;

#define BIOS_DIRECTORY_HEADER_SIGNATURE      0x44484224ul ///< $BHD BIOS Directory Signature
#define BIOS_LV2_DIRECTORY_HEADER_SIGNATURE  0x324C4224ul ///< $BL2 BIOS Directory Lv2 Signature
/// Structure for BIOS directory
typedef struct {
  PSP_DIRECTORY_HEADER    Header;         ///< PSP directory header
  BIOS_DIRECTORY_ENTRY    BiosEntry[1];   ///< Array of PSP entries each pointing to a binary in SPI flash
                                          ///< The actual size of this array comes from the
                                          ///< header (PSP_DIRECTORY.Header.TotalEntries)
} BIOS_DIRECTORY;

/// Structure for PSP Combo directory
#define PSP_COMBO_DIRECTORY_COOKIE   0x50535032ul ///< 2PSP PSP Combo Directory Signature
#define BIOS_COMBO_DIRECTORY_COOKIE  0x44484232ul ///< "BHD2" BIOS Combo Directory Signature

typedef struct {
  UINT32    Cookie;       ///< "2PSP" or "2BHD"
  UINT32    Checksum;     ///< 32 bit CRC of header items below and the entire table
  UINT32    TotalEntries; ///< Number of PSP Entries
  UINT32    LookUpMode;   ///< 0 - Dynamic look up through all entries, 1 - PSP/chip ID match.
  UINT8     Reserved[16]; ///< Reserved
} COMBO_DIRECTORY_HEADER;

/// Structure for PSP Combo directory entry
typedef struct {
  UINT32    IdSelect;     ///< 0 - Compare PSP ID, 1 - Compare chip family ID
  UINT32    Id;           ///< 32-bit Chip/PSP ID
  UINT64    DirTableAddr; ///< Point to PSP directory table (level 2)
} COMBO_DIRECTORY_ENTRY;

/**
 * @brief PSP/BIOS entry region with start address and size
 *
 */
typedef struct {
  UINT64    Address;
  UINT32    Size;
} ENTRY_REGION;

/// RECOVERY_REASON_VERSION
typedef enum {
  RECOVERY_REASON_VERSION_IGNORE = 0xFFFFFFFFul, // before RN
  RECOVERY_REASON_VERSION_1      = 1,            // RN, CZN
  RECOVERY_REASON_VERSION_2      = 2,            // Starting from VN
} RECOVERY_REASON_VERSION;

/// PSP Recovery Reason V1
typedef struct {
  UINT32    EntryType       : 16; ///< [0:15], Entry type ID of the binary in PSP/BIOS entry whose corruption caused recovery
  UINT32    DirectoryLevel  : 2;  ///< [16:17],b'01--The entry is from PSP directory L1
                                  ///          b'10--The entry is from PSP directory L2
                                  ///          b'11--The entry is from BIOS directory L2
  UINT32    Instance        : 4;  ///< [18:21],the instance number of the corrupted entry
  UINT32    PartitionNumber : 3;  ///< [22:24],Which partition this log is from
  UINT32    Reserved        : 7;  ///< [25:31] Reserve for future use
} RECOVERY_REASON_V1;

/// PSP Recovery Reason V2
typedef struct {
  UINT32    EntryType       : 8; ///< [0:7],  Entry type ID of the binary in PSP/BIOS entry whose corruption caused recovery
  UINT32    Instance        : 4; ///< [8:11],the instance number of the corrupted entry
  UINT32    SubProgram      : 4; ///< [12:15], SubProgram
  UINT32    DirectoryLevel  : 4; ///< [16:19],b'01--The entry is from PSP directory L1
                                 ///          b'10--The entry is from PSP directory L2
                                 ///          b'11--The entry is from BIOS directory L2
                                 ///          b'100--PSP L1 directory header
                                 ///          b'101--PSP L2 directory header
                                 ///          b'110--BIOS directory L2 header
                                 ///          b'111--Image Slot Header
  UINT32    Reserved        : 2; ///< [20:21], Reserved
  UINT32    PartitionNumber : 3; ///< [22:24],Which partition this log is from
  UINT32    Reserved2       : 7; ///< [25:31] Reserve for future use
} RECOVERY_REASON_V2;

#define VN_PSP_CHIP_ID  0xBC0B0800   ///< VN Chip ID in combo structure

typedef struct {
  COMBO_DIRECTORY_HEADER    Header;        ///< PSP Combo directory header
  COMBO_DIRECTORY_ENTRY     ComboEntry[1]; ///<  Array of PSP combo entries each pointing to level 2 PSP Direcotry header
} COMBO_DIRECTORY;

#define IMAGE_SLOT_PRIORITY_UNBOOTABLE  0
/// Structure for image slot entry, only used in family VN & MR
//  It also used as structure to store ISH generic information accross programs
typedef struct {
  UINT32    Priority;
  UINT32    UpdateRetries;
  UINT32    GlitchRetries;
  UINT32    ImageSlotAddr;
} IMAGE_SLOT_HEADER;

// Structure for image slot entry, start use from RMB
// Major changes:
// 1. Add CRC checksum
// 2. Add PSPID to support combo, w/o combo directory
// 3. Increased max entry number, 8 -> 32 (support up to 16 SOCs)
// 4. Increased L1 as well as pointer in EFS (support multiple SOC image flash programming)
typedef struct {
  UINT32    CheckSum;      // [0x0000]
  UINT32    Priority;      // [0x0004]
  UINT32    UpdateRetries; // [0x0008]
  UINT8     GlitchRetries; // [0x000C]
  UINT8     Reserved[3];   // [0x000D]
  UINT32    ImageSlotAddr; // [0x0010]
  UINT32    PspId;         // [0x0014]
  UINT32    SlotMaxSize;   // [0x0018]
  UINT32    Reserved_1;    // [0x001C]
} IMAGE_SLOT_HEADER_V2;          // [0x0020]

typedef struct {
  UINT32               SlotCount;                                 // the slot count in the system
  UINT8                BootableSlotCount;                         // the bootable slot count in the system
  UINT8                BootableSlotArray[MAX_IMAGE_SLOT_COUNT];   // bootable slot index array
  UINT8                UnbootableSlotCount;                       // the unbootable slot count in the system
  UINT8                UnbootableSlotArray[MAX_IMAGE_SLOT_COUNT]; // unbootable slot index array
  UINT8                SlotAIndex;                                // index of slot with highest priority
  IMAGE_SLOT_HEADER    SlotAHeader;                               // slot header with highest priority
  UINT8                SlotBIndex;                                // index of slot with second highest priority
  IMAGE_SLOT_HEADER    SlotBHeader;                               // slot header with second highest priority
} IMAGE_SLOT_INFO;

#pragma pack (pop)

#define INSTANCE_IGNORED    0xFF
#define SUBPROGRAM_IGNORED  0xFF
#endif // AMD_LIB_H_
