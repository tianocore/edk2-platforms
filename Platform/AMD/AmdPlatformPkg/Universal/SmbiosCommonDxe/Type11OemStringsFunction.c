/** @file
  AMD SMBIOS Type 11 Record

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "SmbiosCommon.h"

/**
  This function adds OEM strings smbios record (Type 11).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
OemStringsFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE11  *SmbiosRecord;
  UINT8                OemStrCount;
  UINTN                OemStrLen;
  UINTN                OemStrListSize;
  CHAR8                *OemStrPtr;
  UINT8                Idx;
  UINTN                StringOffset;

  Status         = EFI_SUCCESS;
  SmbiosRecord   = NULL;
  OemStrListSize = 0;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get number of OEM strings
  OemStrCount = PcdGet8 (PcdType11OemStringsCount);

  // Calculate size of all OEM Strings
  OemStrPtr = (CHAR8 *)PcdGetPtr (PcdType11OemStrings);
  for (Idx = 0; Idx < OemStrCount; Idx++) {
    OemStrLen       = AsciiStrSize (OemStrPtr);
    OemStrPtr      += OemStrLen;
    OemStrListSize += OemStrLen;
  }

  // Allocate memory for Type11 record
  SmbiosRecord = AllocateZeroPool (
                   sizeof (SMBIOS_TABLE_TYPE11) + OemStrListSize + 1
                   );

  if (SmbiosRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SmbiosRecord->Hdr.Type    = SMBIOS_TYPE_OEM_STRINGS;
  SmbiosRecord->Hdr.Length  = sizeof (SMBIOS_TABLE_TYPE11);
  SmbiosRecord->Hdr.Handle  = 0;
  SmbiosRecord->StringCount = OemStrCount;

  StringOffset = SmbiosRecord->Hdr.Length;

  // Append strings at the end
  OemStrPtr = (CHAR8 *)PcdGetPtr (PcdType11OemStrings);
  for (Idx = 0; Idx < OemStrCount; Idx++) {
    OemStrLen = AsciiStrSize (OemStrPtr);
    CopyMem (
      (UINT8 *)SmbiosRecord + StringOffset,
      OemStrPtr,
      OemStrLen
      );
    OemStrPtr    += OemStrLen;
    StringOffset += OemStrLen;
  }

  Status = AddCommonSmbiosRecord (
             Smbios,
             &SmbiosHandle,
             (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
             );

  FreePool (SmbiosRecord);

  return Status;
}
