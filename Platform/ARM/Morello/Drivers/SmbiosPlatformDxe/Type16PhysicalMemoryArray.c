/** @file
  SMBIOS Type 16 (Physical Memory Array) table for ARM Morello System
  Development Platform.

  This file installs SMBIOS Type 16 (Physical Memory Array) table for
  ARM Morello System Development Platform. It describes a collection of
  memory devices that operate together to form a memory address. It includes
  information about number of devices, total memory installed, error correction
  mechanism used and other related information.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.17
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

#include "MorelloPlatform.h"

#define TYPE16_STRINGS                                  \
  "\0"                          /* Null string */

/* SMBIOS Type16 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE16    Base;
  CHAR8                  Strings[sizeof (TYPE16_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE16;
#pragma pack()

/* Physical Memory Array */
STATIC ARM_MORELLO_SMBIOS_TYPE16  mArmMorelloSmbiosType16 = {
  {
    {
      // SMBIOS header
      EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, // Type 16
      sizeof (SMBIOS_TABLE_TYPE16),          // Length
      SMBIOS_HANDLE_PHYSICAL_MEMORY
    },
    MemoryArrayLocationSystemBoard,    // Location
    MemoryArrayUseSystemMemory,        // Used as system memory
    MemoryErrorCorrectionSingleBitEcc, // Error correction
    0x0,                               // Maximum capacity in kilobytes, update dynamically
    0xFFFE,                            // Memory error info handle, does not provide this info
    2,                                 // Num of memory devices
    0                                  // Extended Maximum capacity.
  },
  // Text strings (unformatted area)
  TYPE16_STRINGS
};

/**
  Install SMBIOS physical memory array table.

  Install the SMBIOS physical memory array (type 16) table for
  Arm Morello System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType16PhysicalMemoryArray (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  UINT64             InstalledMemory;
  UINT64             DramBlock2Size = 0;

  Status = MorelloGetDramBlock2Size (&DramBlock2Size);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get Dram Block 2 Size.\n"
      ));
  }

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType16)->Handle;

  InstalledMemory = PcdGet64 (PcdSystemMemorySize);
  if (DramBlock2Size != 0) {
    InstalledMemory += DramBlock2Size;
  }

  mArmMorelloSmbiosType16.Base.MaximumCapacity = (UINT32)(InstalledMemory/SIZE_1KB);

  /* Install type 16 table */
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType16
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type16 SMBIOS table.\n"
      ));
  }

  return Status;
}
