/** @file
  AMD SMBIOS Type 13 Record

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "SmbiosCommon.h"

/**
  This function adds bios language information smbios record (Type 13).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
  @retval EFI_NOT_FOUND              Not able to locate PlatformLanguage.

**/
EFI_STATUS
EFIAPI
BiosLanguageInfoFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE13  *SmbiosRecord;
  UINTN                TotalSize;
  UINTN                StringOffset;
  UINTN                VarSize;
  UINTN                Idx;
  UINT8                NumSupportedLang;
  UINT8                CurrLangIdx;
  CHAR8                *CurrLang;
  CHAR8                *SupportedLang;
  CHAR8                *LangStr;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CurrLang      = NULL;
  SupportedLang = NULL;

  // Get the current language.
  Status = GetEfiGlobalVariable2 (
             L"PlatformLang",
             (void **)&CurrLang,
             &VarSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get PlatformLang: %r\n", Status));

    VarSize = AsciiStrSize (
                (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLang)
                );
    CurrLang = AllocateCopyPool (
                 VarSize,
                 (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLang)
                 );
    ASSERT (CurrLang != NULL);
  }

  // Get the list of supported languages.
  Status = GetEfiGlobalVariable2 (
             L"PlatformLangCodes",
             (void **)&SupportedLang,
             &VarSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get PlatformLangCodes: %r\n", Status));

    VarSize = AsciiStrSize (
                (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)
                );
    SupportedLang = AllocateCopyPool (
                      VarSize,
                      (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)
                      );
    ASSERT (SupportedLang != NULL);
  }

  // Calculate number of supported languages and index of current language in list.
  CurrLangIdx      = 0;
  NumSupportedLang = 0;
  LangStr          = SupportedLang;

  for (Idx = 0; Idx < VarSize; Idx++) {
    if ((SupportedLang[Idx] == ';') || (SupportedLang[Idx] == '\0')) {
      // Found a language string, increment the language count.
      NumSupportedLang++;

      // Replace string separator with null termination.
      SupportedLang[Idx] = '\0';
      if (!AsciiStrCmp (LangStr, CurrLang)) {
        CurrLangIdx = NumSupportedLang;
      }

      // Point LangStr to next string in list.
      LangStr = &SupportedLang[Idx + 1];
    }
  }

  if (CurrLangIdx == 0) {
    DEBUG ((DEBUG_ERROR, "Failed to locate PlatformLang in PlatformLangCode.\n"));
    Status = EFI_NOT_FOUND;
  } else {
    // Calculate record size and allocate memory for smbios record.
    TotalSize = sizeof (SMBIOS_TABLE_TYPE13) + VarSize + 1;

    SmbiosRecord = AllocateZeroPool (TotalSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      // Fill record data and strings.
      SmbiosRecord->Hdr.Type             = SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION;
      SmbiosRecord->Hdr.Length           = sizeof (SMBIOS_TABLE_TYPE13);
      SmbiosRecord->Hdr.Handle           = 0;
      SmbiosRecord->InstallableLanguages = NumSupportedLang;
      SmbiosRecord->Flags                = 1; // Abbreviated Format.
      SmbiosRecord->CurrentLanguages     = CurrLangIdx;

      // Add strings to bottom of data block
      StringOffset = SmbiosRecord->Hdr.Length;
      CopyMem ((UINT8 *)SmbiosRecord + StringOffset, SupportedLang, VarSize);

      Status = AddCommonSmbiosRecord (
                 Smbios,
                 &SmbiosHandle,
                 (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
                 );
      FreePool (SmbiosRecord);
    }
  }

  if (CurrLang != NULL) {
    FreePool (CurrLang);
  }

  if (SupportedLang != NULL) {
    FreePool (SupportedLang);
  }

  return Status;
}
