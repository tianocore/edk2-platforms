/** @file

  Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  Entry point of Act Dynamic Command.

  Produce the DynamicCommand protocol to handle "act" command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           Act command is executed sucessfully.
  @retval EFI_ABORTED           HII package was failed to initialize.
  @retval others                Other errors when executing act command.
**/
EFI_STATUS
EFIAPI
ActCommandInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
