/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Protocol/SpiNorFlash.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Register/Cpuid.h>
#include <Library/BaseLib.h>

#define BLOCK_SIZE  (FixedPcdGet32 (PcdFlashNvStorageBlockSize))

extern EFI_SPI_NOR_FLASH_PROTOCOL          *mSpiNorFlashProtocol;
extern EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mSpiFvbProtocol;

extern EFI_PHYSICAL_ADDRESS  mNvStorageBase;
extern EFI_LBA               mNvStorageLbaOffset;
UINT32                       mSpiFlashOffset;

STATIC EFI_HANDLE  mSpiFvbHandle;
extern UINT32
SetSpiFlashOffset (
  UINT32  FlashSize
  );

/**
  SPI firmware volume driver EntryPoint.

  @param[in] ImageHandle    Driver Image Handle
  @param[in] SystemTable    System Table

  @retval EFI_SUCCESS       Driver initialization succeeded
  @retval all others        Driver initialization failed

**/
EFI_STATUS
EFIAPI
SpiFvbDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  // Retrieve SPI NOR flash driver
  Status = gBS->LocateProtocol (
                  &gEfiSpiNorFlashProtocolGuid,
                  NULL,
                  (VOID **)&mSpiNorFlashProtocol
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  mNvStorageBase = (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashNvStorageVariableBase);
  DEBUG ((
    DEBUG_INFO,
    "%a - mNvStorageBase = %X\n",
    __func__,
    mNvStorageBase
    ));
  mNvStorageLbaOffset = (EFI_LBA)((PcdGet32 (PcdFlashNvStorageVariableBase)
                                   - FixedPcdGet32 (PcdFlashAreaBaseAddress))
                                  / FixedPcdGet32 (PcdFlashNvStorageBlockSize));
  DEBUG ((
    DEBUG_INFO,
    "%a - mNvStorageLbaOffset = %X\n",
    __func__,
    mNvStorageLbaOffset
    ));

  mSpiFvbHandle = NULL;
  Status        = gBS->InstallProtocolInterface (
                         &mSpiFvbHandle,
                         &gEfiFirmwareVolumeBlockProtocolGuid,
                         EFI_NATIVE_INTERFACE,
                         &mSpiFvbProtocol
                         );
  mSpiFlashOffset = SetSpiFlashOffset (mSpiNorFlashProtocol->FlashSize);

  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
