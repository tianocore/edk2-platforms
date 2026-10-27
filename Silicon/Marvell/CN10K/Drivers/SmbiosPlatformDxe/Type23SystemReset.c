/** @file
  SMBIOS Type 23 (System Reset) table for the Marvell CN10K SoC.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE23  mSmbiosType23Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_RESET, sizeof (SMBIOS_TABLE_TYPE23), 0 },
  // bit    5: System contains a watchdog timer.
  // bits 4:3: Boot Option on Limit -> Do not reboot
  // bits 2:1: Boot Option -> Operating system
  // bit    0: System reset is enabled by the user.
  // NOTE: used same values as for Octeon TX2
  0x3b,    // Capabilities;
  0xffff,  // ResetCount;
  0xffff,  // ResetLimit;
  0xffff,  // TimerInterval;
  0xffff   // Timeout;
};

/**
  Install the SMBIOS Type 23 (System Reset) record.

  Publishes the fixed reset capabilities used on Octeon TX2 and CN10K:
  user-enabled reset, operating-system boot option, no reboot on limit, and a
  watchdog timer.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context. Unused.

  @retval  EFI_SUCCESS  The record was installed.
  @retval  Others       The record could not be added.
**/
EFI_STATUS
InstallType23SystemReset (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  return SmbiosPlatformAddEntry (
           Smbios,
           (SMBIOS_STRUCTURE *)&mSmbiosType23Template,
           NULL,
           NULL
           );
}
