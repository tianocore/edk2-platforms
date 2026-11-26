/** @file
  SMBIOS Type 3 (System Enclosure) table for the Marvell CN10K SoC.

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

STATIC SMBIOS_TABLE_TYPE3  mSmbiosType3Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, sizeof (SMBIOS_TABLE_TYPE3), 0 },
  1,                                                                    // Manufacturer String
  MiscChassisTypePizzaBox,                                              // Type;
  2,                                                                    // Version String
  3,                                                                    // SerialNumber String
  4,                                                                    // AssetTag String
  ChassisStateSafe,                                                     // BootupState;
  ChassisStateSafe,                                                     // PowerSupplyState;
  ChassisStateSafe,                                                     // ThermalState;
  ChassisSecurityStatusNone,                                            // SecurityStatus;
  { 0,                                0,                           0, 0}, // OemDefined[4];
  0,                                                                    // Height;
  1,                                                                    // NumberofPowerCords;
  0,                                                                    // ContainedElementCount;
  0,                                                                    // ContainedElementRecordLength;
  {
    { 0 }
  }            // ContainedElements[1];
};

STATIC CHAR8  *mSmbiosType3Strings[] = {
  "Marvell",
  "1.0",
  "Not specified",
  "Not specified",
  NULL
};

/**
  Install the SMBIOS Type 3 (System Enclosure) record.

  Sets the enclosure serial number and revision strings and stores the
  assigned handle in Context->ChassisHandle for later records that reference
  the chassis.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context with the board strings; receives
                           the chassis handle.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType3SystemEnclosure (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  SMBIOS_SET_STRING (mSmbiosType3Strings, &mSmbiosType3Template, SerialNumber, Context->SystemSerialNumber);
  SMBIOS_SET_STRING (mSmbiosType3Strings, &mSmbiosType3Template, Version, Context->SystemRevision);

  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType3Template,
           mSmbiosType3Strings,
           &Context->ChassisHandle
           );
}
