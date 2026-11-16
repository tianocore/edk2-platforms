/** @file
  SMBIOS Type 12 (System Configuration Options) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE12  mSmbiosType12Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS, sizeof (SMBIOS_TABLE_TYPE12), 0 },
  1 // StringCount
};

STATIC CHAR8  *mSmbiosType12Strings[] = {
  "No configuration options",
  NULL
};

/**
  Install the SMBIOS Type 12 (System Configuration Options) record.

  Publishes a placeholder string indicating that no configuration options are
  exposed through this record.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context. Unused.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType12SystemConfigurationOptions (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType12Template,
           mSmbiosType12Strings,
           NULL
           );
}
