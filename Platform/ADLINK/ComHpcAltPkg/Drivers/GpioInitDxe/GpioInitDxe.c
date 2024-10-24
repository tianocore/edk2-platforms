/** @file

   Copyright (c) 2022, ADLink. All rights reserved.<BR>

   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Uefi.h>
#include <Library/GpioLib.h>

EFI_STATUS
EFIAPI
GpioInitDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return GpioModeConfig (7, GpioConfigIn);
}
