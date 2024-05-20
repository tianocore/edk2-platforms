/** @file
  AMD SMBIOS Type 12 Record

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "SmbiosCommon.h"

/**
  This function adds System Configuration Options record (Type 12).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
SystemCfgOptionsFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE12  *SmbiosRecord;
  UINT8                SystemCfgOptionsCount;
  UINTN                SystemCfgOptionsLen;
  UINTN                SystemCfgOptionsListSize;
  CHAR8                *SystemCfgOptionsPtr;
  UINT8                Idx;
  UINTN                StringOffset;

  Status                   = EFI_SUCCESS;
  SmbiosRecord             = NULL;
  SystemCfgOptionsListSize = 0;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get number of System Configuration Options
  SystemCfgOptionsCount = PcdGet8 (PcdType12SystemCfgOptionsCount);

  // Calculate size of all Strings
  SystemCfgOptionsPtr = (CHAR8 *)PcdGetPtr (PcdType12SystemCfgOptions);
  for (Idx = 0; Idx < SystemCfgOptionsCount; Idx++) {
    SystemCfgOptionsLen       = AsciiStrSize (SystemCfgOptionsPtr);
    SystemCfgOptionsPtr      += SystemCfgOptionsLen;
    SystemCfgOptionsListSize += SystemCfgOptionsLen;
  }

  // Allocate memory for Type12 record
  SmbiosRecord = AllocateZeroPool (
                   sizeof (SMBIOS_TABLE_TYPE12) + SystemCfgOptionsListSize + 1
                   );

  if (SmbiosRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SmbiosRecord->Hdr.Type    = SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS;
  SmbiosRecord->Hdr.Length  = sizeof (SMBIOS_TABLE_TYPE12);
  SmbiosRecord->Hdr.Handle  = 0;
  SmbiosRecord->StringCount = SystemCfgOptionsCount;

  StringOffset = SmbiosRecord->Hdr.Length;

  // Append strings at the end
  SystemCfgOptionsPtr = (CHAR8 *)PcdGetPtr (PcdType12SystemCfgOptions);
  for (Idx = 0; Idx < SystemCfgOptionsCount; Idx++) {
    SystemCfgOptionsLen = AsciiStrSize (SystemCfgOptionsPtr);
    CopyMem (
      (UINT8 *)SmbiosRecord + StringOffset,
      SystemCfgOptionsPtr,
      SystemCfgOptionsLen
      );
    SystemCfgOptionsPtr += SystemCfgOptionsLen;
    StringOffset        += SystemCfgOptionsLen;
  }

  Status = AddCommonSmbiosRecord (
             Smbios,
             &SmbiosHandle,
             (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
             );
  FreePool (SmbiosRecord);

  return Status;
}
