/** @file

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/RamDisk.h>

/**
  Entrypoint of Platform Dxe Driver

  @param  ImageHandle[in]      The firmware allocated handle for the EFI image.
  @param  SystemTable[in]      A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The RAM disk has been registered.
  @retval EFI_NOT_FOUND        No RAM disk protocol instances were found.
  @retval EFI_UNSUPPORTED      The ImageType is not supported.
  @retval Others               Unexpected error happened.

**/
EFI_STATUS
EFIAPI
ArmN1SdpEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_RAM_DISK_PROTOCOL      *RamDisk;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;

  Status = EFI_UNSUPPORTED;
  if (FeaturePcdGet (PcdRamDiskSupported)) {
    Status = gBS->LocateProtocol (
                    &gEfiRamDiskProtocolGuid,
                    NULL,
                    (VOID**)&RamDisk
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Couldn't find the RAM Disk protocol - %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = RamDisk->Register (
                        (UINTN)PcdGet32 (PcdRamDiskBase),
                        (UINTN)PcdGet32 (PcdRamDiskSize),
                        &gEfiVirtualCdGuid,
                        NULL,
                        &DevicePath
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR, "%a: Failed to register RAM Disk - %r\n",
        __func__,
        Status
        ));
    }
  }
  return Status;
}
