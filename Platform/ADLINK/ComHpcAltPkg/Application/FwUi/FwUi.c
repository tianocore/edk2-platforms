/** @file
  This is a application that demonstrates how to enter firmware setup during next boot

  Copyright (c) 2020, . All rights reserved.<BR>

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc     The number of items in Argv.
  @param[in] Argv     Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

  Below entry point assignment also do:

  in C:
    EFI_APPLICATION_ENTRY_POINT (ShellAppMain)

  in INF:
    [nmake.common]
      IMAGE_ENTRY_POINT=ShellAppMain

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndicationsSupported;
  UINT64      OsIndications;
  UINTN       DataSize;

  //
  // Read OsIndicationsSupported variable to determine if it is a UEFI OS
  //
  DataSize = sizeof (OsIndicationsSupported);
  Print (L"Read OsIndicationsSupported...\n");
  Status = gRT->GetVariable (
                  L"OsIndicationsSupported",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &OsIndicationsSupported
                  );
  if (EFI_ERROR (Status) || (DataSize == 0)) {
    Print (L"Error reading OsIndicationsSupported - %r\n", Status);
    return Status;
  }

  //
  // Test if EFI_OS_INDICATIONS_BOOT_TO_FW_UI supported
  //
  Print (L"Test OsIndicationsSupported...\n");
  if ((OsIndicationsSupported & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) == 0) {
    Print (L"EFI_OS_INDICATIONS_BOOT_TO_FW_UI is not supported - %r\n", Status);
    return EFI_UNSUPPORTED;
  }

  //
  // Read OsIndications variable
  //
  Print (L"Read OsIndications...\n");
  DataSize = sizeof (OsIndications);
  Status   = gRT->GetVariable (
                    L"OsIndications",
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    &OsIndications
                    );

  if (EFI_ERROR (Status) || (DataSize == 0)) {
    Print (L"Error reading OsIndications - %r\n", Status);
    OsIndications = 0; // initialize OsIndications.
    // return Status;
  }

  //
  // set EFI_OS_INDICATIONS_BOOT_TO_FW_UI to boot to BIOS setup menu
  //
  Print (L"Setting OsIndications with EFI_OS_INDICATIONS_BOOT_TO_FW_UI\n");
  OsIndications |= ((UINT64)EFI_OS_INDICATIONS_BOOT_TO_FW_UI);
  Status         = gRT->SetVariable (
                          L"OsIndications",
                          &gEfiGlobalVariableGuid,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                          sizeof (OsIndications),
                          &OsIndications
                          );
  if (EFI_ERROR (Status)) {
    Print (L"Error writing OsIndications - %r\n", Status);
    return Status;
  } else {
    //
    // Reboot to Firmware UI
    //
    Print (L"OsIndications successfully written, will boot to Setup Menu\n");
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  }

  return 0;
}
