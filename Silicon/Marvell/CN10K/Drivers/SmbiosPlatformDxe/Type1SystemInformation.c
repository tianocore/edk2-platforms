/** @file
  SMBIOS Type 1 (System Information) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE1  mSmbiosType1Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, sizeof (SMBIOS_TABLE_TYPE1), 0    },
  1,    // Manufacturer String
  2,    // ProductName String
  3,    // Version String
  4,    // SerialNumber String
  // {ea04f2b5-0b7a-4ba9-9b46-13ea9f5f1b95}
  { 0xea04f2b5,                         0x0b7a,                      0x4ba9, { 0x9b, 0x46, 0x13, 0xea, 0x9f, 0x5f, 0x1b, 0x95}
  },
  SystemWakeupTypePowerSwitch,
  5,    // SKUNumber String
  6,    // Family String
};

STATIC CHAR8  *mSmbiosType1Strings[] = {
  "Marvell",
  "Octeon 10 Product",
  "1.0",
  "Not specified",
  "Not specified",
  "Octeon 10",
  NULL
};

/**
  Install the SMBIOS Type 1 (System Information) record.

  Sets the product name, system serial number, and system revision strings
  from the driver context.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with the board strings.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType1SystemInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  SMBIOS_SET_STRING (mSmbiosType1Strings, &mSmbiosType1Template, ProductName, Context->ProductName);
  SMBIOS_SET_STRING (mSmbiosType1Strings, &mSmbiosType1Template, SerialNumber, Context->SystemSerialNumber);
  SMBIOS_SET_STRING (mSmbiosType1Strings, &mSmbiosType1Template, Version, Context->SystemRevision);

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType1Template,
           mSmbiosType1Strings,
           NULL
           );
}
