/** @file

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "SmbiosPlatformDxe.h"

/**
  This function adds SMBIOS Table (Type 38) records.

  @param  RecordData                 Pointer to SMBIOS Table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS Table was successfully added.
  @retval Other                      Failed to update the SMBIOS Table.

**/
SMBIOS_PLATFORM_DXE_TABLE_FUNCTION (PlatformIpmiDevice) {
  EFI_STATUS           Status;
  SMBIOS_TABLE_TYPE38  *InputData;

  InputData = (SMBIOS_TABLE_TYPE38 *)RecordData;
  while (InputData->Hdr.Type != NULL_TERMINATED_TYPE) {
    Status = SmbiosPlatformDxeAddRecord ((UINT8 *)InputData, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    InputData++;
  }

  return Status;
}
