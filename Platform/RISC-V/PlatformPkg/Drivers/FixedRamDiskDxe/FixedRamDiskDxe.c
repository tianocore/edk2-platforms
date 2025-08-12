/** @file
  Initialize a RamDisk with a fixed memory region

  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/RamDisk.h>

EFI_STATUS
EFIAPI
InitializeFixedRamDisk (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_DEVICE_PATH_PROTOCOL   *FixedRamDiskDevPath;
  EFI_RAM_DISK_PROTOCOL      *RamDisk;
  EFI_STATUS                 Status;

  Status = gBS->LocateProtocol (&gEfiRamDiskProtocolGuid, NULL, (VOID **)&RamDisk);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeFixedRamDisk Couldn't find the RAM Disk protocol - %r\n", Status));
    return Status;
  }

  //
  // Create Ramdisk from fixed address if already defined
  //
  if ((PcdGet64 (PcdFixedRamdiskBase) != 0) && (PcdGet32 (PcdFixedRamdiskSize) != 0)) {
    Status = RamDisk->Register (
               PcdGet64 (PcdFixedRamdiskBase),
               PcdGet32 (PcdFixedRamdiskSize),
               &gEfiVirtualDiskGuid,
               NULL,
               &FixedRamDiskDevPath
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "Unable to create RAM disk from %p size %X\n",
        PcdGet64 (PcdFixedRamdiskBase),
        PcdGet32 (PcdFixedRamdiskSize)
        ));
    }
  }

  return Status;
}
