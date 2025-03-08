/** @file
  Library for verify the BIOS Password.

  Copyright (C) 2021 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BiosPasswordLib.h>

/**
  Check if the password is full zero.

  @param[in]   Password       Points to the data buffer.

  @retval      TRUE           This password string is full zero.
  @retval      FALSE          This password string is not full zero.

**/
BOOLEAN
EFIAPI
PasswordIsFullZero (
  IN CHAR8  *Password
  )
{
  UINTN  Index;

  for (Index = 0; Index < BIOS_PASSWORD_MAX_LENGTH; Index++) {
    if (Password[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Hash the data to get credential.

  @param[in]   Buffer         Points to the data buffer.
  @param[in]   BufferSize     Buffer size.
  @param[in]   SaltValue      Points to the salt buffer, 32 bytes.
  @param[out]  Credential     Points to the hashed result.

  @retval      TRUE           Hash the data successfully.
  @retval      FALSE          Failed to hash the data.

**/
BOOLEAN
EFIAPI
GenerateCredential (
  IN      UINT8  *Buffer,
  IN      UINTN  BufferSize,
  IN      UINT8  *SaltValue,
  OUT  UINT8     *Credential
  )
{
  BOOLEAN  Status;
  UINTN    HashSize;
  VOID     *Hash;
  VOID     *HashData;

  Hash     = NULL;
  HashData = NULL;
  Status   = FALSE;

  HashSize = Sha256GetContextSize ();
  Hash     = AllocateZeroPool (HashSize);
  ASSERT (Hash != NULL);
  if (Hash == NULL) {
    goto Done;
  }

  Status = Sha256Init (Hash);
  if (!Status) {
    goto Done;
  }

  HashData = AllocateZeroPool (PASSWORD_SALT_SIZE + BufferSize);
  ASSERT (HashData != NULL);
  if (HashData == NULL) {
    goto Done;
  }

  CopyMem (HashData, SaltValue, PASSWORD_SALT_SIZE);
  CopyMem ((UINT8 *)HashData + PASSWORD_SALT_SIZE, Buffer, BufferSize);

  Status = Sha256Update (Hash, HashData, PASSWORD_SALT_SIZE + BufferSize);
  if (!Status) {
    goto Done;
  }

  Status = Sha256Final (Hash, Credential);

Done:
  if (Hash != NULL) {
    FreePool (Hash);
  }

  if (HashData != NULL) {
    ZeroMem (HashData, PASSWORD_SALT_SIZE + BufferSize);
    FreePool (HashData);
  }

  return Status;
}

/**
  Get saved Bios Password Variable that will be used to validate Bios Password.

  @param[out] BiosPasswordVariable   The Variable for the Bios Password.

  @retval TRUE      The Variable for the Bios Password is found and returned.
  @retval FALSE     The Variable for the Bios Password is not found.

**/
BOOLEAN
GetSavedBiosPasswordVariable (
  OUT BIOS_PASSWORD_VARIABLE  *BiosPasswordVariable
  )
{
  EFI_STATUS              Status;
  BIOS_PASSWORD_VARIABLE  *Variable;
  UINTN                   VariableSize;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __func__));

  Variable     = NULL;
  VariableSize = 0;

  Status = GetVariable2 (
             BIOS_PASSWORD_VARIABLE_NAME,
             &gAmdPlatformBiosPasswordGuid,
             (VOID **)&Variable,
             &VariableSize
             );
  if (EFI_ERROR (Status) || (Variable == NULL)) {
    DEBUG ((DEBUG_INFO, "Bios Password Variable get failed (%r)\n", Status));
    return FALSE;
  }

  CopyMem (BiosPasswordVariable, Variable, sizeof (BIOS_PASSWORD_VARIABLE));

  FreePool (Variable);

  return TRUE;
}

/**
  Use saved Bios Password Variable to validate Bios Password.

  @param[in] Password           Bios Password.

  @retval EFI_SUCCESS           Pass to validate the Bios Password.
  @retval EFI_NOT_FOUND         The Variable for the Bios Password is not found.
  @retval EFI_DEVICE_ERROR      Failed to generate credential for the Bios Password.
  @retval EFI_INVALID_PARAMETER Failed to validate the Bios Password.

**/
EFI_STATUS
ValidateBiosPassword (
  IN CHAR8  *Password
  )
{
  EFI_STATUS              Status;
  BIOS_PASSWORD_VARIABLE  BiosPasswordVariable;
  BOOLEAN                 HashOk;
  UINT8                   HashData[SHA256_DIGEST_SIZE];

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __func__));

  if (!GetSavedBiosPasswordVariable (&BiosPasswordVariable)) {
    DEBUG ((DEBUG_INFO, "GetSavedBiosPasswordVariable failed\n"));
    return EFI_NOT_FOUND;
  }

  ZeroMem (HashData, sizeof (HashData));
  HashOk = GenerateCredential ((UINT8 *)Password, BIOS_PASSWORD_MAX_LENGTH, BiosPasswordVariable.PasswordSalt, HashData);
  if (!HashOk) {
    DEBUG ((DEBUG_INFO, "GenerateCredential failed\n"));
    return EFI_DEVICE_ERROR;
  }

  if (CompareMem (BiosPasswordVariable.PasswordHash, HashData, sizeof (HashData)) != 0) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a() - exit (%r)\n", __func__, Status));
  return Status;
}

/**
  Get password input from the popup windows.

  @param[in]      PopUpString1  Pop up string 1.
  @param[in]      PopUpString2  Pop up string 2.
  @param[in, out] Password      The buffer to hold the input password.

  @retval EFI_ABORTED           It is given up by pressing 'ESC' key.
  @retval EFI_SUCCESS           Get password input successfully.

**/
EFI_STATUS
EFIAPI
PopupPasswordInputWindows (
  IN CHAR16     *PopUpString1,
  IN CHAR16     *PopUpString2,
  IN OUT CHAR8  *Password
  )
{
  EFI_INPUT_KEY  Key;
  UINTN          Length;
  CHAR16         Mask[BIOS_PASSWORD_MAX_LENGTH + 1];
  CHAR16         Unicode[BIOS_PASSWORD_MAX_LENGTH + 1];
  CHAR8          Ascii[BIOS_PASSWORD_MAX_LENGTH + 1];

  ZeroMem (Unicode, sizeof (Unicode));
  ZeroMem (Ascii, sizeof (Ascii));
  ZeroMem (Mask, sizeof (Mask));

  gST->ConOut->ClearScreen (gST->ConOut);

  Length = 0;
  while (TRUE) {
    Mask[Length] = L'_';
    if (PopUpString2 == NULL) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        PopUpString1,
        L"---------------------",
        Mask,
        NULL
        );
    } else {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        PopUpString1,
        PopUpString2,
        L"---------------------",
        Mask,
        NULL
        );
    }

    //
    // Check key.
    //
    if (Key.ScanCode == SCAN_NULL) {
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Add the null terminator.
        //
        Unicode[Length] = 0;
        break;
      } else if ((Key.UnicodeChar == CHAR_NULL) ||
                 (Key.UnicodeChar == CHAR_TAB) ||
                 (Key.UnicodeChar == CHAR_LINEFEED)
                 )
      {
        continue;
      } else {
        if (Key.UnicodeChar == CHAR_BACKSPACE) {
          if (Length > 0) {
            Unicode[Length] = 0;
            Mask[Length]    = 0;
            Length--;
          }
        } else {
          Unicode[Length] = Key.UnicodeChar;
          Mask[Length]    = L'*';
          Length++;
          if (Length == BIOS_PASSWORD_MAX_LENGTH) {
            //
            // Add the null terminator.
            //
            Unicode[Length] = 0;
            Mask[Length]    = 0;
            break;
          }
        }
      }
    }

    if (Key.ScanCode == SCAN_ESC) {
      ZeroMem (Unicode, sizeof (Unicode));
      ZeroMem (Ascii, sizeof (Ascii));
      gST->ConOut->ClearScreen (gST->ConOut);
      return EFI_ABORTED;
    }
  }

  UnicodeStrToAsciiStrS (Unicode, Ascii, sizeof (Ascii));
  CopyMem (Password, Ascii, BIOS_PASSWORD_MAX_LENGTH);
  ZeroMem (Unicode, sizeof (Unicode));
  ZeroMem (Ascii, sizeof (Ascii));

  gST->ConOut->ClearScreen (gST->ConOut);
  return EFI_SUCCESS;
}

/**
  Show popup window and ask for Bios Password.

**/
VOID
EFIAPI
ProcessBiosPasswordVerification (
  VOID
  )
{
  EFI_STATUS     Status;
  CHAR16         PopUpString[100];
  EFI_INPUT_KEY  Key;
  UINT16         RetryCount;
  CHAR8          Password[BIOS_PASSWORD_MAX_LENGTH];

  RetryCount = 0;

  DEBUG ((DEBUG_INFO, "%a()\n", __func__));

  UnicodeSPrint (PopUpString, sizeof (PopUpString), L"Enter Bios Password to unlock");

  while (TRUE) {
    Status = PopupPasswordInputWindows (PopUpString, NULL, Password);
    if (!EFI_ERROR (Status)) {
      if (!PasswordIsFullZero (Password)) {
        Status = ValidateBiosPassword (Password);
      } else {
        Status = EFI_INVALID_PARAMETER;
      }

      if (!EFI_ERROR (Status)) {
        ZeroMem (Password, BIOS_PASSWORD_MAX_LENGTH);
        return;
      }

      ZeroMem (Password, BIOS_PASSWORD_MAX_LENGTH);

      if (EFI_ERROR (Status)) {
        RetryCount++;
        if (RetryCount < MAX_BIOS_PASSWORD_RETRY_COUNT) {
          do {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"Invalid password.",
              L"Press ENTER to retry",
              NULL
              );
          } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

          continue;
        } else {
          do {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"password retry count is expired. Please shutdown the machine.",
              L"Press ENTER to shutdown",
              NULL
              );
          } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

          gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
          break;
        }
      }
    } else if (Status == EFI_ABORTED) {
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Press ENTER to shutdown,",
          L"Press ESC to input password again",
          NULL
          );
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        break;
      } else {
        //
        // Let user input password again.
        //
        continue;
      }
    }
  }
}
