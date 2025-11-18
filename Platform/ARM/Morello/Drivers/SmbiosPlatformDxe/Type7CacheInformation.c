/** @file
  SMBIOS Type 7 (Cache information) table for ARM Morello System
  Development Platform.

  This file installs SMBIOS Type 7 (Cache information) table for
  ARM Morello System Development Platform. It includes information
  about cache levels implemented, cache configuration, ways of
  associativity and other information related to cache memory installed.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.8
**/

#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

#define TYPE7_STRINGS                                   \
  "L1 Instruction\0"            /* L1I */               \
  "L1 Data\0"                   /* L1D */               \
  "L2\0"                        /* L2  */               \
  "L3\0"                        /* L3  */               \
  "SLC\0"                       /* L4  */              \
  "\0"

typedef enum {
  L1Instruction = 1,
  L1Data,
  L2,
  L3,
  Slc,
} TYPE7_STRING_ELEMENTS;

/* SMBIOS Type7 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE7    Base;
  CHAR8                 Strings[sizeof (TYPE7_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE7;
#pragma pack()

/* Cache information */
STATIC ARM_MORELLO_SMBIOS_TYPE7  mArmMorelloSmbiosType7[] = {
  {   // Entry 0, L1 instruction cache
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_CACHE_INFORMATION, // Type 7
        sizeof (SMBIOS_TABLE_TYPE7),       // Length
        SMBIOS_HANDLE_L1I_CACHE,           // Handle number
      },
      L1Instruction,
      (
        (1 << 8) | // Write-back
        (1 << 7) | // Cache enabled
        0x0        // Cache level 1
      ),
      64,                     // Uses Maximum cache size
      64,                     // Uses Installed cache size
      { 0, 1 },               // Supported SRAM type unknown
      { 0, 1 },               // Current SRAM type unknown
      0,                      // Cache Speed Unknown
      0x03,                   // Error correction type None
      0x03,                   // Instruction cache
      CacheAssociativity4Way, // Associativity
      0,                      // Maximum cache size 2
      0                       // Installed cache size 2
    },
    // Text strings (unformatted area)
    TYPE7_STRINGS
  },
  {   // Entry 1, L1 data cache
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_CACHE_INFORMATION, // Type 7
        sizeof (SMBIOS_TABLE_TYPE7),       // Length
        SMBIOS_HANDLE_L1D_CACHE,           // Handle number
      },
      L1Data,
      (
        (1 << 8) | // Write-back
        (1 << 7) | // Cache enabled
        (1 << 3) | // Cache socketed
        0x0        // Cache level 1
      ),
      64,                     // Uses Maximum cache size
      64,                     // Uses Installed cache size
      { 0, 1 },               // Supported SRAM type unknown
      { 0, 1 },               // Current SRAM type unknown
      0,                      // Cache Speed Unknown
      0x03,                   // Error correction type None
      0x04,                   // Data cache
      CacheAssociativity4Way, // Associativity
      0,                      // Maximum cache size
      0                       // Installed cache size
    },
    // Text strings (unformatted area)
    TYPE7_STRINGS
  },
  {   // Entry 2, L2 cache
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_CACHE_INFORMATION, // Type 7
        sizeof (SMBIOS_TABLE_TYPE7),       // Length
        SMBIOS_HANDLE_L2_CACHE,            // Handle number
      },
      L2,
      (
        (1 << 8) | // Write-back
        (1 << 7) | // Cache enabled
        0x1        // Cache level 2
      ),
      1024,                   // Uses Maximum cache size
      1024,                   // Uses Installed cache size
      { 0, 1 },               // Supported SRAM type unknown
      { 0, 1 },               // Current SRAM type unknown
      0,                      // Cache Speed Unknown
      0x05,                   // Error correction type Single-bit ECC
      0x05,                   // Unified cache
      CacheAssociativity8Way, // Associativity
      0,                      // Maximum cache size 2
      0                       // Installed cache size 2
    },
    // Text strings (unformatted area)
    TYPE7_STRINGS
  },
  {   // Entry 3, L3 cache
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_CACHE_INFORMATION, // Type 7
        sizeof (SMBIOS_TABLE_TYPE7),       // Length
        SMBIOS_HANDLE_L3_CACHE,            // Handle number
      },
      L3,
      (
        (1 << 8) | // Write-back
        (1 << 7) | // Cache enabled
        0x2        // Cache level 3
      ),
      1024,                   // Uses Maximum cache size
      1024,                   // Uses Installed cache size
      { 0, 1 },               // Supported SRAM type unknown
      { 0, 1 },               // Current SRAM type unknown
      0,                      // Cache Speed Unknown
      0x05,                   // Error correction type Single-bit ECC
      0x05,                   // Unified cache
      CacheAssociativity8Way, // Associativity
      0,                      // Maximum cache size 2
      0                       // Installed cache size 2
    },
    // Text strings (unformatted area)
    TYPE7_STRINGS
  },
  {   // Entry 4, SLC Cache
    {
      {
        // SMBIOS header
        EFI_SMBIOS_TYPE_CACHE_INFORMATION, // Type 7
        sizeof (SMBIOS_TABLE_TYPE7),       // Length
        SMBIOS_HANDLE_L4_CACHE,            // Handle number
      },
      Slc,
      (
        (1 << 8) | // Write-back
        (1 << 7) | // Cache enabled
        0x3        // Cache level 4
      ),
      4096,                    // Uses Maximum cache size
      4096,                    // Uses Installed cache size
      { 0, 1 },                // Supported SRAM type unknown
      { 0, 1 },                // Current SRAM type unknown
      0,                       // Cache Speed Unknown
      0x05,                    // Error correction type Single-bit ECC
      0x05,                    // Unified cache
      CacheAssociativity16Way, // Associativity
      0,                       // Maximum cache size 2
      0                        // Installed cache size 2
    },
    // Text strings (unformatted area)
    TYPE7_STRINGS
  }
};

/**
  Install SMBIOS Cache information Table

  Install the SMBIOS Cache information (type 7) table for Arm Morello
  System Development Platform

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType7CacheInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  UINT8              CacheIdx;

  /* Install valid cache information tables */
  for (CacheIdx = 0; CacheIdx < ARRAY_SIZE (mArmMorelloSmbiosType7); CacheIdx++) {
    SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType7[CacheIdx])->Handle;
    Status       = Smbios->Add (
                             Smbios,
                             NULL,
                             &SmbiosHandle,
                             (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType7[CacheIdx]
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "SMBIOS: Failed to install Type7 SMBIOS table.\n"
        ));
    }
  }

  return Status;
}
