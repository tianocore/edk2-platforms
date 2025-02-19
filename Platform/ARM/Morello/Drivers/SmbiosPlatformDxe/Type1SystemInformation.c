/** @file
  SMBIOS Type 1 (System information) table for Arm Morello System
  Development Platform.

  This file installs SMBIOS Type 1 (System information) table for
  Arm Morello System Development Platform. Type 1 table defines attributes
  of the overall system such as manufacturer, product name, UUID etc.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - SMBIOS Reference Specification 3.4.0, Chapter 7.2
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Protocol/Smbios.h>

#define TYPE1_STRINGS                                   \
  "ARM LTD\0"                             /* Manufacturer */      \
  "Version not set\0"                     /* Version */           \
  "Serial not set\0"                      /* Serial number */     \
  "V2M-MS1DP-0360A\0"                     /* SKU */               \
  "Development Boards\0"                  /* Family */            \
  "Morello System Development Platform\0" /* Product Names */ \
  "\0"

typedef enum {
  ManufacturerName = 1,
  Version,
  SerialNumber,
  Sku,
  Family,
  ProductNameBase
} TYPE1_STRING_ELEMENTS;

/* SMBIOS Type1 structure */
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE1    Base;
  CHAR8                 Strings[sizeof (TYPE1_STRINGS)];
} ARM_MORELLO_SMBIOS_TYPE1;
#pragma pack()

/* System information */
STATIC ARM_MORELLO_SMBIOS_TYPE1  mArmMorelloSmbiosType1 = {
  {
    {
      // SMBIOS header
      EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, // Type 1
      sizeof (SMBIOS_TABLE_TYPE1),        // Length
      SMBIOS_HANDLE_PI_RESERVED,          // Assign an unused handle number
    },
    ManufacturerName,                     // Manufacturer
    ProductNameBase,                      // Product Name
    Version,                              // Version
    SerialNumber,                         // Serial
    { 0xd90e12df, 0x90eb, 0x4691, { 0xbb, 0x26, 0xbe, 0x32, 0x5a, 0x9d, 0xef, 0xda }
    },                                    // UUID
    SystemWakeupTypePowerSwitch,          // Wakeup type: Power Switch
    Sku,                                  // SKU
    Family,                               // Family
  },
  // Text strings (unformatted)
  TYPE1_STRINGS
};

/**
  Install SMBIOS System information Table.

  Install the SMBIOS system information (type 1) table for
  Arm Morello System Development Platform.

  @param[in]  Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
InstallType1SystemInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;

  SmbiosHandle = ((EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType1)->Handle;

  /* Install type 1 table */
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)&mArmMorelloSmbiosType1
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "SMBIOS: Failed to install Type1 SMBIOS table.\n"
      ));
  }

  return Status;
}
