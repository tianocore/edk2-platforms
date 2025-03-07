/** @file
  AMD Psp Base Lib

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_PSP_BASELIB_V2_H_
#define AMD_PSP_BASELIB_V2_H_

#include <AmdPspDirectory.h>

#define MAX_IMAGE_SLOT_COUNT  32

#define ALIGNMENT_4K  BASE_4KB
#define ALIGN_CHECK(addr, alignment)  ((((UINTN)(addr)) & ((alignment) - 1)) == 0)
#define ALIGN_4K_CHECK(addr)          ALIGN_CHECK((addr), ALIGNMENT_4K)

#define IS_VALID_ADDR32(addr)  (((UINT32)(addr) != 0) && (UINT32)(addr) != 0xFFFFFFFF)

#pragma pack (push, 1)

#define FIRMWARE_TABLE_SIGNATURE  0x55AA55AAul

/// Define the structure OEM signature table
typedef struct _FIRMWARE_ENTRY_TABLEV2 {
  UINT32    Signature;          ///< 0x00 Signature should be 0x55AA55AAul
  UINT32    ImcRomBase;         ///< 0x04 Base Address for Imc Firmware
  UINT32    GecRomBase;         ///< 0x08 Base Address for Gmc Firmware
  UINT32    XHCRomBase;         ///< 0x0C Base Address for XHCI Firmware
  UINT32    LegacyPspDirBase;   ///< 0x10 Base Address of PSP directory
  UINT32    PspDirBase;         ///< 0x14 Base Address for PSP directory
  UINT32    Reserved1;          ///< 0x18 Base Address for Reserved BIOS directory
  UINT32    Reserved2;          ///< 0x1C Base Address for Reserved BIOS directory
  UINT32    Reserved3;          ///< 0x20 Base Address for Reserved BIOS directory
  UINT32    Config;             ///< 0x24 reserved for EFS Configuration
  UINT32    NewBiosDirBase;     ///< 0x28 Generic Base address for all program
  UINT32    PspDirBackupBase;   ///< 0x2C Backup PSP directory address for all programs
} FIRMWARE_ENTRY_TABLEV2;

/// It also used as structure to store ISH generic information across programs
typedef struct {
  UINT32    Priority;
  UINT32    UpdateRetries;
  UINT32    GlitchRetries;
  UINT32    ImageSlotAddr;
} IMAGE_SLOT_HEADER;

#pragma pack (pop)

#endif // AMD_LIB_H_
