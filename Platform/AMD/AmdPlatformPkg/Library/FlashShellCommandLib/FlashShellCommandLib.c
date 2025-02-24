/** @file
  Shell Application for AMD BIOS Flash.

  Copyright (C) 2021 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/Shell.h>
#include <Library/TimerLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/HiiLib.h>
#include <Library/ShellLib.h>
#include <Library/BiosFlashUpdate.h>

STATIC EFI_HII_HANDLE  mHiiHandle = NULL;

// An array of AmdFlashAppLib command line parameters.
STATIC SHELL_PARAM_ITEM  FlashParamList[] = {
  { L"-nv", TypeFlag },  // -nv   Preserve NV Storage regions
  { NULL,   TypeMax  }
};

// {ABE7DC2B-B2A6-4570-AC4C-ED987BCF8F71}
EFI_GUID  mFormSetGuid = {
  0xabe7dc2b, 0xb2a6, 0x4570, { 0xac, 0x4c, 0xed, 0x98, 0x7b, 0xcf, 0x8f, 0x71 }
};

/**
  Print APP header.

**/
VOID
PrintHeader (
  VOID
  )
{
  Print (L"+-----------------------------------+\n");
  Print (L"|  AMD EDKII BIOS Flash Tool, v0.2  |\n");
  Print (L"+-----------------------------------+\n");
}

/**
  Print APP usage.

**/
VOID
PrintUsage (
  VOID
  )
{
  Print (L"AmdFlash Usage :\n");
  Print (L"    AmdFlash [Option]\n");
  Print (L"Option:\n");
  Print (L"    -nv : Preserve NV Storage regions\n");
}

/**
  Wait for user input.

  @param[in]  Seconds   Waiting seconds.

  @retval     TRUE      No user input.
  @retval     FALSE     User input ESC as exit.

**/
BOOLEAN
Wait4UserInput (
  UINT32  Seconds
  )
{
  EFI_STATUS     Status;
  EFI_INPUT_KEY  Key;
  UINT32         SecIndex;
  UINT32         TryIndex;

  for (SecIndex = 0; SecIndex < Seconds; SecIndex++) {
    Print (L"Press ESC to exit update or it will reboot and flash BIOS in (%d) second, any key to skip the wait ...\n", Seconds - SecIndex);

    // 10 millisecond
    for (TryIndex = 0; TryIndex < 100; TryIndex++) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (!EFI_ERROR (Status)) {
        if (SCAN_ESC == Key.ScanCode) {
          return FALSE;
        }
      }

      MicroSecondDelay (10000);
    }
  }

  return TRUE;
}

/**
  Configure Flash Flags.

  @param[out]  Flags             Pointer to flash flags.

  @retval EFI_SUCCESS            Option checked successfully.
  @retval EFI_UNSUPPORTED        Option unsupported.
  @retval EFI_INVALID_PARAMETER  Parameter invalid.

**/
EFI_STATUS
ConfigFlashFlags (
  OUT UINT16  *Flags
  )
{
  EFI_STATUS  Status;
  LIST_ENTRY  *ParamPackage;

  if (Flags == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse the command line
  //
  Status = ShellCommandLineParseEx (FlashParamList, &ParamPackage, NULL, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    PrintUsage ();
    return EFI_UNSUPPORTED;
  }

  //
  // Parse the actual arguments provided
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-nv")) {
    *Flags |= AMD_BIOS_FLASH_OPTION_NV_STORAGE;
  }

  //
  // Free the command line package
  //
  ShellCommandLineFreeVarList (ParamPackage);

  return EFI_SUCCESS;
}

/**
  Update Capsule image.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_UNSUPPORTED        Command usage unsupported.
  @retval EFI_INVALID_PARAMETER  Command usage invalid.
  @retval EFI_NOT_FOUND          The input file can not be found.

**/
SHELL_STATUS
EFIAPI
ShellAmdFlash (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_SHELL_PROTOCOL            *ShellProtocol;
  CONST CHAR16                  *CurDirectory;
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  UINTN                         TempDeviceSize;
  UINTN                         DeviceSize;
  AMD_BIOS_FLASH_OPTION         AmdFlashBootOption;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptionBuffer;
  UINTN                         BootOptionCount;
  UINTN                         Index;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  SHELL_FILE_HANDLE             TempHandle;
  EFI_BOOT_MANAGER_LOAD_OPTION  NewOption;

  PrintHeader ();

  AmdFlashBootOption.FlashOption = 0;

  //
  // Check and configure flash options
  //
  Status = ConfigFlashFlags (&AmdFlashBootOption.Option.Flags);
  if (Status == EFI_UNSUPPORTED) {
    Print (L"ERROR: Unsupported option\n");
    return SHELL_UNSUPPORTED;
  }

  //
  // Locate Shell Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiShellProtocolGuid,
                  NULL,
                  (VOID **)&ShellProtocol
                  );
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: gEfiShellProtocolGuid is not found \n");
    return SHELL_NOT_FOUND;
  }

  //
  // Check for BIOS image exists at location
  //
  Status = ShellProtocol->OpenFileByName (
                            EFI_AMD_FLASH_BIOS_FILE_NAME,
                            &TempHandle,
                            EFI_FILE_MODE_READ
                            );
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: BIOS Image File %s %r\n", EFI_AMD_FLASH_BIOS_FILE_NAME, Status);
    return SHELL_NOT_FOUND;
  }

  Status = ShellProtocol->CloseFile (TempHandle);

  //
  // Search for device path of current media holding BIOS image
  //
  CurDirectory     = ShellProtocol->GetCurDir (NULL);
  TempDevicePath   = ShellProtocol->GetDevicePathFromFilePath (CurDirectory);
  TempDeviceSize   = GetDevicePathSize (TempDevicePath);
  BootOptionBuffer = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // Get the boot option from the link list
    //
    DevicePath = BootOptionBuffer[Index].FilePath;
    DeviceSize =  GetDevicePathSize (DevicePath);
    if (TempDeviceSize >= DeviceSize) {
      if (!CompareMem ((void *)DevicePath, (void *)TempDevicePath, DeviceSize - END_DEVICE_PATH_LENGTH)) {
        if (Wait4UserInput (5)) {
          AmdFlashBootOption.Option.OptionNumber = (UINT16)BootOptionBuffer[Index].OptionNumber;
          Status                                 = gRT->SetVariable (
                                                          EFI_AMD_FLASH_VARIABLE_NAME,
                                                          &gAmdPlatformSetupOptionsGuid,
                                                          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                                                          sizeof (AmdFlashBootOption.Option.OptionNumber),
                                                          &AmdFlashBootOption.Option.OptionNumber
                                                          );

          Status = gRT->SetVariable (
                          EFI_AMD_FLASH_EX_VARIABLE_NAME,
                          &gAmdPlatformSetupOptionsGuid,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                          sizeof (AmdFlashBootOption.Option.Flags),
                          &AmdFlashBootOption.Option.Flags
                          );

          gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        } else {
          break;
        }
      }
    }
  }

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             L"Removable Media",
             TempDevicePath,
             NULL,
             0
             );
  if (!EFI_ERROR (Status)) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, (UINTN)-1);
    if (!EFI_ERROR (Status)) {
      AmdFlashBootOption.Option.OptionNumber = (UINT16)NewOption.OptionNumber;
      EfiBootManagerFreeLoadOption (&NewOption);
      if (Wait4UserInput (5)) {
        Status = gRT->SetVariable (
                        EFI_AMD_FLASH_VARIABLE_NAME,
                        &gAmdPlatformSetupOptionsGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        sizeof (AmdFlashBootOption.Option.OptionNumber),
                        &AmdFlashBootOption.Option.OptionNumber
                        );

        Status = gRT->SetVariable (
                        EFI_AMD_FLASH_EX_VARIABLE_NAME,
                        &gAmdPlatformSetupOptionsGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        sizeof (AmdFlashBootOption.Option.Flags),
                        &AmdFlashBootOption.Option.Flags
                        );

        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      } else {
        Print (L"Abort by user\n");
        return SHELL_NOT_FOUND;
      }
    }
  }

  Print (L"ERROR: Device Path for BIOS Image File Not Found\n");
  return SHELL_NOT_FOUND;
}

/**
  Function to get the filename with help context if HII will not be used.

  @retval CHAR16      The filename with help text in it.

**/
CONST CHAR16 *
EFIAPI
ShellCommandGetManFileNameAmdFlash (
  VOID
  )
{
  return (L"ShellCommands");
}

/**
  Constructor for the Shell Commands library.
  Install the handlers for level 3 UEFI Shell 2.0 commands.

  @param[in] ImageHandle     The image handle of the process.
  @param[in] SystemTable     The EFI System Table pointer.

  @retval EFI_SUCCESS        The shell command handlers were installed successfully.
  @retval EFI_UNSUPPORTED    The shell level required was not found.

**/
EFI_STATUS
EFIAPI
ShellCommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mHiiHandle != NULL) {
    return EFI_SUCCESS;
  }

  mHiiHandle = HiiAddPackages (&mFormSetGuid, gImageHandle, FlashShellCommandLibStrings, NULL);
  if (mHiiHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ShellCommandRegisterCommandName (
    L"AmdFlash",
    ShellAmdFlash,
    ShellCommandGetManFileNameAmdFlash,
    0,
    L"Install3",
    FALSE,
    mHiiHandle,
    STRING_TOKEN (STR_GET_HELP_FLASH)
    );
  return EFI_SUCCESS;
}

/**
  Destructor for the library, to free any resources.

  @param[in] ImageHandle            The image handle of the process.
  @param[in] SystemTable            The EFI System Table pointer.

  @retval    EFI_SUCCESS            Destructor success.

**/
EFI_STATUS
EFIAPI
ShellCommandLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mHiiHandle != NULL) {
    HiiRemovePackages (mHiiHandle);
  }

  mHiiHandle = NULL;
  return EFI_SUCCESS;
}
