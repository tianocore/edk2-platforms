/** @file
  SMBIOS Type 2 (Baseboard Information) table for the Marvell CN10K SoC.

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

STATIC SMBIOS_TABLE_TYPE2  mSmbiosType2Template = {
  { EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, sizeof (SMBIOS_TABLE_TYPE2), 0 },
  1,    // Manufacturer String
  2,    // ProductName String
  3,    // Version String
  4,    // SerialNumber String
  5,    // AssetTag String
  {     // FeatureFlag
    1,  //  Motherboard           :1;
    0,  //  RequiresDaughterCard  :1;
    0,  //  Removable             :1;
    0,  //  Replaceable           :1;
    0,  //  HotSwappable          :1;
    0,  //  Reserved              :3;
  },
  6,                        // LocationInChassis String
  0,                        // ChassisHandle;
  BaseBoardTypeMotherBoard, // BoardType;
  0,                        // NumberOfContainedObjectHandles;
  { 0 }                     // ContainedObjectHandles[1];
};

STATIC CHAR8  *mSmbiosType2Strings[] = {
  "Marvell",
  "Octeon 10 EBB",
  "1.0",
  "Not specified",
  "Not specified",
  "Not specified",
  NULL
};

/**
  Install the SMBIOS Type 2 (Baseboard Information) record.

  Sets the board product name, serial number, and revision strings from the
  driver context and links the board to the chassis handle produced by the
  Type 3 installer.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with the board strings and chassis
                           handle.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType2BaseboardInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  SMBIOS_SET_STRING (mSmbiosType2Strings, &mSmbiosType2Template, ProductName, Context->ProductName);
  SMBIOS_SET_STRING (mSmbiosType2Strings, &mSmbiosType2Template, SerialNumber, Context->SystemSerialNumber);
  SMBIOS_SET_STRING (mSmbiosType2Strings, &mSmbiosType2Template, Version, Context->SystemRevision);

  mSmbiosType2Template.ChassisHandle = Context->ChassisHandle;

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType2Template,
           mSmbiosType2Strings,
           NULL
           );
}
