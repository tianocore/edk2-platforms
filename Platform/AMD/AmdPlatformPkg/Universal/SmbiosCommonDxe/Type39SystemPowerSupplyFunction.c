/** @file
  SMBIOS Specific driver for Type39.

  Copyright (C) 2015 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosCommon.h"
#include <Library/AmdPlatformSocLib.h>

/**
  System power supply function (Type 39).

  @param[in]  Smbios                 The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
SystemPowerSupplyFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE39  *SmbiosRecord;

  SmbiosRecord = NULL;

  Status = GetSystemPowerSupplyInfo (&SmbiosRecord);
  if (!EFI_ERROR (Status)) {
    Status = AddCommonSmbiosRecord (
               Smbios,
               &SmbiosHandle,
               (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
               );
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  return Status;
}
