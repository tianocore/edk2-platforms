/** @file
  SMBIOS Type 19 (Memory Array Mapped Address) table for ARM Morello
  System Development Platform.

  This file installs SMBIOS Type 19 (Memory Array Mapped Address) table for
  ARM Morello System Development Platform. It includes information about the
  address mapping for a Physical Memory Array.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.20
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "MorelloPlatform.h"
#include "SmbiosPlatformDxe.h"

#define TYPE19_STRINGS                                  \
  "\0"                          /* Null string */

/* SMBIOS Type19 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE19    Base;
  CHAR8                  Strings[sizeof (TYPE19_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE19;
#pragma pack()

/* Memory Array Mapped Address */
STATIC ARM_MORELLO_SMBIOS_TYPE19  mArmMorelloSmbiosType19 = {
  {
    {
      // SMBIOS header
      EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, // Type 19
      sizeof (SMBIOS_TABLE_TYPE19),                // Length
      SMBIOS_HANDLE_PI_RESERVED,                   // Assign an unused handle number
    },
    0,                                  // Starting address
    0,                                  // Ending address
    SMBIOS_HANDLE_PHYSICAL_MEMORY,      // Memory array handle
    2                                   // Partition width
  },
  // Text strings (unformatted area)
  TYPE19_STRINGS
};

/**
  Install SMBIOS memory array mapped address table

  Install the SMBIOS memory array mapped address (type 19) table for Arm
  Morello System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType19MemoryArrayMappedAddress (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  UINT64             DramBlock2Size = 0;

  Status = MorelloGetDramBlock2Size (&DramBlock2Size);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get Dram Block 2 Size.\n"
      ));
  }

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType19)->Handle;

  /* Physical address, in kilobytes, of a range of memory mapped
     to the specified Physical Memory Array DRAM0(~2GB)
  */
  mArmMorelloSmbiosType19.Base.StartingAddress =
    PcdGet64 (PcdSystemMemoryBase)/SIZE_1KB;
  mArmMorelloSmbiosType19.Base.EndingAddress =
    (PcdGet64 (PcdSystemMemoryBase) +
     PcdGet64 (PcdSystemMemorySize))/SIZE_1KB;

  /* Install type 19 table */
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType19
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type19 SMBIOS table.\n"
      ));
  }

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType19)->Handle;

  /* Physical address, in kilobytes, of a range of memory mapped
     to the specified Physical Memory Array DRAM1(Total Memory - DRAM0)
  */
  mArmMorelloSmbiosType19.Base.StartingAddress =
    PcdGet64 (PcdDramBlock2Base)/SIZE_1KB;
  mArmMorelloSmbiosType19.Base.EndingAddress =
    (PcdGet64 (PcdDramBlock2Base) +
     DramBlock2Size)/SIZE_1KB;

  /* Install type 19 table */
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType19
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type19 SMBIOS table.\n"
      ));
  }

  return Status;
}
