/** @file
  Provides functions for communication with System Firmware (SMpro/PMpro)
  via interfaces like Mailbox.

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MailboxInterfaceLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>

/**
  Setup runtime date configuration.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Otherwise             Errors returned from the MailboxRuntimeSetup() functions.

**/
EFI_STATUS
EFIAPI
MailboxMsgDateConfigRuntimeSetup (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = MailboxRuntimeSetup (0, PMproDoorbellChannel1);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
