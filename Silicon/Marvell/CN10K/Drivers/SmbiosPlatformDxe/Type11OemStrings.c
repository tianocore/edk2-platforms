/** @file
  SMBIOS Type 11 (OEM Strings) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE11  mSmbiosType11Template = {
  { EFI_SMBIOS_TYPE_OEM_STRINGS, sizeof (SMBIOS_TABLE_TYPE11), 0 },
  1 // StringCount
};

STATIC CHAR8  *mSmbiosType11Strings[] = {
  "Marvell",
  NULL
};

/**
  Install the SMBIOS Type 11 (OEM Strings) record.

  Publishes a single OEM string identifying Marvell.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context. Unused.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType11OemStrings (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType11Template,
           mSmbiosType11Strings,
           NULL
           );
}
