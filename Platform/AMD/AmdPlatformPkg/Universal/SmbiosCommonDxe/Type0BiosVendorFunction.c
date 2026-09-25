/** @file
  SMBIOS Specific driver for Type0.

  Copyright (C) 2017 - 2026, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosCommon.h"
#include <Library/AmdPlatformSocLib.h>

/**
  Get the BIOS information for SMBIOS type 0.

  @param[out]  SmBiosType0Info          The structure to hold BIOS information for SMBIOS type0.

  @retval EFI_SUCCESS                   Success to get BIOS information for SMBIOS type0.
  @retval EFI_INVALID_PARAMETER         The input paramters are invalid.

**/
EFI_STATUS
GetDefaultBiosInfo (
  OUT AMD_SMBIOS_TYPE0_BIOS_INFO  *SmBiosType0Info
  )
{
  EFI_STATUS  Status;
  UINTN       StrSize;

  if (SmBiosType0Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get BIOS information from board side as high priority.
  Status = GetBoardBiosInfo (SmBiosType0Info);
  if (EFI_ERROR (Status)) {
    CONST CHAR8  *PcdVendor  = (CONST CHAR8 *)PcdGetPtr (PcdSmbiosType0StringVendor);
    CONST CHAR8  *PcdVersion = (CONST CHAR8 *)PcdGetPtr (PcdSmbiosType0StringBiosVersion);
    CONST CHAR8  *PcdDate    = (CONST CHAR8 *)PcdGetPtr (PcdSmbiosType0StringBiosReleaseDate);

    // Get default value if board side does not override these informations.
    if (PcdVendor != NULL) {
      StrSize                       = AsciiStrSize (PcdVendor);
      SmBiosType0Info->VendorStrLen = MIN (StrSize, SMBIOS_STRING_MAX_LENGTH);
      CopyMem ((UINT8 *)(SmBiosType0Info->VendorStr), PcdVendor, SmBiosType0Info->VendorStrLen);
    }

    if (PcdVersion != NULL) {
      StrSize                    = AsciiStrSize (PcdVersion);
      SmBiosType0Info->VerStrLen = MIN (StrSize, SMBIOS_STRING_MAX_LENGTH);
      CopyMem ((UINT8 *)(SmBiosType0Info->VersionStr), PcdVersion, SmBiosType0Info->VerStrLen);
    }

    if (PcdDate != NULL) {
      StrSize                     = AsciiStrSize (PcdDate);
      SmBiosType0Info->DateStrLen = MIN (StrSize, SMBIOS_STRING_MAX_LENGTH);
      CopyMem ((UINT8 *)(SmBiosType0Info->DateStr), PcdDate, SmBiosType0Info->DateStrLen);
    }
  }

  DEBUG ((DEBUG_INFO, "Bios vendor string  : %a\n", SmBiosType0Info->VendorStr));
  DEBUG ((DEBUG_INFO, "Bios version string : %a\n", SmBiosType0Info->VersionStr));
  DEBUG ((DEBUG_INFO, "Bios release date   : %a\n", SmBiosType0Info->DateStr));

  DEBUG ((DEBUG_INFO, "Bios vendor string, size  : %d\n", SmBiosType0Info->VendorStrLen));
  DEBUG ((DEBUG_INFO, "Bios version string, size : %d\n", SmBiosType0Info->VerStrLen));
  DEBUG ((DEBUG_INFO, "Bios release date, size   : %d\n", SmBiosType0Info->DateStrLen));

  return EFI_SUCCESS;
}

/**
  This function makes boot time changes to the contents of the
  BiosVendor (Type 0).

  @param[in]  Smbios                 The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
BiosVendorFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                  Status;
  SMBIOS_TABLE_TYPE0          *SmbiosRecord;
  EFI_SMBIOS_HANDLE           SmbiosHandle;
  UINTN                       StringOffset;
  AMD_SMBIOS_TYPE0_BIOS_INFO  BiosInfo;
  UINT64                      FlashAreaSize;

  DEBUG ((DEBUG_INFO, "%a - enter\n", __func__));

  ZeroMem (&BiosInfo, sizeof (BiosInfo));
  Status = GetDefaultBiosInfo (&BiosInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (  (BiosInfo.VendorStrLen > SMBIOS_STRING_MAX_LENGTH)
     || (BiosInfo.VerStrLen > SMBIOS_STRING_MAX_LENGTH)
     || (BiosInfo.DateStrLen > SMBIOS_STRING_MAX_LENGTH))
  {
    ASSERT_EFI_ERROR (EFI_BAD_BUFFER_SIZE);
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // One extra zero following the last null-terminated string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE0) + BiosInfo.VendorStrLen + BiosInfo.VerStrLen + BiosInfo.DateStrLen + 1);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  if (PcdGetPtr (PcdSmbiosType0BiosInformation) != NULL) {
    CopyMem (SmbiosRecord, PcdGetPtr (PcdSmbiosType0BiosInformation), sizeof (SMBIOS_TABLE_TYPE0));
  }

  SmbiosRecord->Hdr.Type   = SMBIOS_TYPE_BIOS_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE0);
  SmbiosRecord->Hdr.Handle = 0;

  StringOffset = SmbiosRecord->Hdr.Length;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, BiosInfo.VendorStr, BiosInfo.VendorStrLen);
  StringOffset += BiosInfo.VendorStrLen;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, BiosInfo.VersionStr, BiosInfo.VerStrLen);
  StringOffset += BiosInfo.VerStrLen;
  CopyMem ((UINT8 *)SmbiosRecord + StringOffset, BiosInfo.DateStr, BiosInfo.DateStrLen);

  FlashAreaSize = (UINT64)PcdGet32 (PcdFlashAreaSize);
  if (FlashAreaSize >= SIZE_16MB) {
    //
    // Update Extended BIOS ROM Size for bios image bigger then 16MB.
    //
    SmbiosRecord->BiosSize = 0xFF;
    if (FlashAreaSize >= SIZE_1GB) {
      SmbiosRecord->ExtendedBiosSize.Unit = 0x1;
      SmbiosRecord->ExtendedBiosSize.Size = DivU64x32 (FlashAreaSize, SIZE_1GB) & MAX_UINT8;
    } else {
      SmbiosRecord->ExtendedBiosSize.Unit = 0x0;
      SmbiosRecord->ExtendedBiosSize.Size = DivU64x32 (FlashAreaSize, SIZE_1MB) & MAX_UINT8;
    }
  }

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = AddCommonSmbiosRecord (
             Smbios,
             &SmbiosHandle,
             (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
             );

  FreePool (SmbiosRecord);

  DEBUG ((DEBUG_INFO, "%a - exit with status: %r\n", __func__, Status));
  return Status;
}
