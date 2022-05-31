/** @file

  Copyright (c) 2021 - 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/RamDisk.h>
#include <Protocol/Tda19988.h>

#include "I2cBusHdmi.h"
#include "MorelloPlatform.h"

/// The DXE singleton data
STATIC
struct {
  I2C_BUS_HDMI    *I2cBusHdmi;
} mModule;

/**
  Entrypoint of Platform Dxe Driver

  @param  ImageHandle[in]       The firmware allocated handle for the EFI image.
  @param  SystemTable[in]       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval *                     Errors are possible.
**/
EFI_STATUS
EFIAPI
ArmMorelloEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_RAM_DISK_PROTOCOL     *RamDisk;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  Status = EFI_SUCCESS;

  if (FeaturePcdGet (PcdRamDiskSupported)) {
    Status = gBS->LocateProtocol (
                    &gEfiRamDiskProtocolGuid,
                    NULL,
                    (VOID **)&RamDisk
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Couldn't find the RAM Disk protocol %r\n",
        __FUNCTION__,
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
        DEBUG_ERROR,
        "%a: Failed to register RAM Disk - %r\n",
        __FUNCTION__,
        Status
        ));
      return Status;
    }
  }

  Status = I2cBusHdmiStart (&mModule.I2cBusHdmi);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: unable to initalize I2C bus \"HDMI\" - %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}
