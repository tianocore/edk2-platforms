/** @file
  The header file for Bios password verification library.

  Copyright (C) 2023 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BIOS_PASSWORD_LIB_H_
#define BIOS_PASSWORD_LIB_H_

#define PASSWORD_SALT_SIZE  32

#define MAX_BIOS_PASSWORD_RETRY_COUNT  5

#define BIOS_PASSWORD_MAX_LENGTH  32

#define BIOS_PASSWORD_POPUP_STRING_MAX_LENGTH  100

#define BIOS_PASSWORD_VARIABLE_NAME  L"AmdBiosPassword"

#pragma pack(1)
typedef struct {
  UINT8    PasswordHash[SHA256_DIGEST_SIZE];
  UINT8    PasswordSalt[PASSWORD_SALT_SIZE];
} BIOS_PASSWORD_VARIABLE;
#pragma pack()

VOID
EFIAPI
ProcessBiosPasswordVerification (
  VOID
  );

BOOLEAN
EFIAPI
PasswordIsFullZero (
  IN CHAR8  *Password
  );

BOOLEAN
EFIAPI
GenerateCredential (
  IN  UINT8  *Buffer,
  IN  UINTN  BufferSize,
  IN  UINT8  *SaltValue,
  OUT UINT8  *Credential
  );

EFI_STATUS
EFIAPI
PopupPasswordInputWindows (
  IN CHAR16     *PopUpString1,
  IN CHAR16     *PopUpString2,
  IN OUT CHAR8  *Password
  );

#endif // BIOS_PASSWORD_LIB_H_
