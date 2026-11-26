/** @file
  SMBIOS Type 32 (System Boot Information) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE32  mSmbiosType32Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, sizeof (SMBIOS_TABLE_TYPE32), 0 },
  { 0,                                       0,                            0, 0, 0, 0}, // Reserved[6];
  BootInformationStatusNoError                                                  // BootStatus
};

/**
  Install the SMBIOS Type 32 (System Boot Information) record.

  Reports BootInformationStatusNoError.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context. Unused.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType32SystemBootInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType32Template,
           NULL,
           NULL
           );
}
