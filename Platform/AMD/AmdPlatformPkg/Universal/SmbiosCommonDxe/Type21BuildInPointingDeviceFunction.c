/** @file
  SMBIOS Specific driver for Type21.

  Copyright (C) 2022 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosCommon.h"
#include <Library/AmdPlatformSocLib.h>

/**
  Build-in Pointing Device (Type 21).

  @param[in]  Smbios                 The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
BuildInPointingDeviceFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE21  *SmbiosRecord;
  SMBIOS_TABLE_TYPE21  *PortingDeviceBuffer;
  UINTN                PortingDeviceCounts;
  UINTN                Index;
  UINTN                RecordSize;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PortingDeviceBuffer = NULL;
  PortingDeviceCounts = 0;

  Status = GetPortingDeviceInfo (&PortingDeviceBuffer, &PortingDeviceCounts);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RecordSize   = sizeof (SMBIOS_TABLE_TYPE21) + 2;
  SmbiosRecord = AllocateZeroPool (RecordSize);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < PortingDeviceCounts; Index++) {
      ZeroMem (SmbiosRecord, RecordSize);
      CopyMem (SmbiosRecord, &PortingDeviceBuffer[Index], sizeof (SMBIOS_TABLE_TYPE21));
      Status = AddCommonSmbiosRecord (
                 Smbios,
                 &SmbiosHandle,
                 (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
                 );
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  if (PortingDeviceBuffer != NULL) {
    FreePool (PortingDeviceBuffer);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  return Status;
}
