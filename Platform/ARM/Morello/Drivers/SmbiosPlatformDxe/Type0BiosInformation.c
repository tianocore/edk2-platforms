/** @file
  SMBIOS Type 0 (BIOS information) table for ARM Morello System
  Development Platform.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.1
**/

#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#define TYPE0_STRINGS                                           \
  "ARM LTD\0"                           /* Vendor */            \
  "EDK II\0"                            /* BiosVersion */       \
  __DATE__"\0"                          /* BiosReleaseDate */   \
  "\0"

typedef enum {
  VendorName = 1,
  BiosVersion,
  BiosReleaseDate
} TYPE0_STRING_ELEMENTS;

/* SMBIOS Type0 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE0    Base;
  CHAR8                 Strings[sizeof (TYPE0_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE0;
#pragma pack()

/* BIOS information */
STATIC ARM_MORELLO_SMBIOS_TYPE0  mArmMorelloSmbiosType0 = {
  {
    {
      // SMBIOS header
      EFI_SMBIOS_TYPE_BIOS_INFORMATION, // Type 0
      sizeof (SMBIOS_TABLE_TYPE0),      // Length
      SMBIOS_HANDLE_PI_RESERVED,        // Assign an unused handle number
    },
    VendorName,       // String number of vendor name
    BiosVersion,      // String number of BiosVersion
    0,                // Bios starting address segment
    BiosReleaseDate,  // String number of BiosReleaseDate
    0x1F,             // Bios ROM size 2MB // 64KB*(n+1)
    {                 // MISC_BIOS_CHARACTERISTICS
      0,              // Reserved Bits 0-1
      0,              // Unknown
      0,              // BIOS Characteristics are not supported
      0,              // ISA not supported
      0,              // MCA not supported
      0,              // EISA not supported
      1,              // PCI supported
      0,              // PC card (PCMCIA) not supported
      0,              // Plug and Play is supported
      0,              // APM not supported
      1,              // BIOS upgradable(Flash)
      0,              // BIOS shadowing is not allowed
      0,              // VL-VESA not supported
      0,              // ESCD support is not available
      0,              // Boot from CD not supported
      1,              // selectable boot
    },
    {                 // BIOSCharacteristicsExtensionBytes
      (
        (1 << 0) |    // ACPI Supported
        (1 << 1)      // Legacy USB supported
      ),
      (1 << 3)      // UEFI spec supported
    },
    0,                // SMBIOS Major Release
    0,                // SMBIOS Minor Release
    0,                // Embedded Controller Firmware Major Release
    0,                // Embedded Controller Firmware Minor Release
    {                 // EXTENDED_BIOS_ROM_SIZE
      0,              // Size
      0               // Unit MB
    }
  },
  // Text strings (unformatted area)
  TYPE0_STRINGS
};

/**
  Install SMBIOS BIOS information Table.

  Install the SMBIOS BIOS information (type 0) table for Arm Morello System
  Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType0BiosInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType0)->Handle;

  /* Update firmware revision */
  mArmMorelloSmbiosType0.Base.SystemBiosMajorRelease =
    (PcdGet32 (PcdFirmwareRevision) >> 16) & 0xFF;
  mArmMorelloSmbiosType0.Base.SystemBiosMinorRelease =
    PcdGet32 (PcdFirmwareRevision) & 0xFF;

  /* Install type 0 table */
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType0
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type0 SMBIOS table.\n"
      ));
  }

  return Status;
}
