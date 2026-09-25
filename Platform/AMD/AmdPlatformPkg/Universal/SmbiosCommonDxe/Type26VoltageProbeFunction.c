/** @file
  SMBIOS Specific driver for Type26.

  Copyright (C) 2022 - 2026, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosCommon.h"
#include <Library/AmdPlatformSocLib.h>
#include <Pcd/SmbiosPcd.h>

/**
  Voltage Probe (Type 26).

  @param[in]  Smbios                 The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
VoltageProbeFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                        Status;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_VOLTAGE_PROBE_INFO_RECORD  *SmbiosRecord;
  SMBIOS_VOLTAGE_PROBE_INFO_RECORD  *VoltageProbe;

  SmbiosRecord = NULL;
  UINT8  *TempPointer;
  UINTN  Size;
  UINTN  StringCount;
  UINTN  Type26ListCount;
  UINTN  VoltageProbeIndex;
  UINTN  StringIndex;

  Status = GetVoltageProbeInfo ((VOID **)&VoltageProbe, &Type26ListCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (VoltageProbeIndex = 0; VoltageProbeIndex < Type26ListCount; VoltageProbeIndex++) {
    Size        = sizeof (SMBIOS_TABLE_TYPE26);
    StringCount = VoltageProbe[VoltageProbeIndex].VoltageProbe.Description;
    for (StringIndex = 0; StringIndex < StringCount; StringIndex++) {
      Size += AsciiStrSize (VoltageProbe[VoltageProbeIndex].String[StringIndex]);
    }

    //
    // One extra zero following the last null-terminated string.
    //
    SmbiosRecord = AllocateZeroPool (Size + 1);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    //
    // Copy Voltage info, and then copy string array.
    //
    TempPointer = (UINT8 *)SmbiosRecord;
    CopyMem (TempPointer, &VoltageProbe[VoltageProbeIndex], sizeof (SMBIOS_TABLE_TYPE26));
    TempPointer += sizeof (SMBIOS_TABLE_TYPE26);
    for (StringIndex = 0; StringIndex < StringCount; StringIndex++) {
      CopyMem (
        (UINT8 *)TempPointer,
        VoltageProbe[VoltageProbeIndex].String[StringIndex],
        AsciiStrSize (
          VoltageProbe[VoltageProbeIndex].String[StringIndex]
          )
        );
      TempPointer += AsciiStrSize (VoltageProbe[VoltageProbeIndex].String[StringIndex]);
    }

    //
    // Now we have got the full smbios record, call smbios protocol to add this record.
    //
    Status = AddCommonSmbiosRecord (
               Smbios,
               &SmbiosHandle,
               (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    FreePool (SmbiosRecord);
    SmbiosRecord = NULL;
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (VoltageProbe != NULL) {
    FreePool (VoltageProbe);
  }

  return Status;
}
