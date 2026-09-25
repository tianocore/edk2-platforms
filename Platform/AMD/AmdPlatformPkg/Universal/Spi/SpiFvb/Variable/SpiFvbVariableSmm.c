/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#define BLOCK_SIZE  (FixedPcdGet32 (PcdFlashNvStorageBlockSize))

extern EFI_SPI_NOR_FLASH_PROTOCOL          *mSpiNorFlashProtocol;
extern EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mSpiFvbProtocol;
extern EFI_PHYSICAL_ADDRESS                mNvStorageBase;
extern UINT64                              mNvStorageSize;
extern EFI_LBA                             mNvStorageLbaOffset;
extern UINT32                              mSpiFlashOffset;

STATIC EFI_HANDLE  mSpiFvbHandle;

extern UINT32
GetSpiFlashOffset (
  UINT32  FlashSize
  );

extern EFI_STATUS
ValidateFvHeader (
  IN UINT64  ExpectedFvLength
  );

/**
  SPI firmware volume SMM driver EntryPoint.

  @param[in] ImageHandle    Driver Image Handle
  @param[in] MmSystemTable  MM System Table

  @retval EFI_SUCCESS           Driver initialization succeeded
  @retval all others            Driver initialization failed

**/
EFI_STATUS
EFIAPI
SpiFvbVariableSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  // Retrieve SPI NOR flash driver
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSpiSmmNorFlashProtocolGuid,
                    NULL,
                    (VOID **)&mSpiNorFlashProtocol
                    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  mNvStorageBase = (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashNvStorageVariableBase);
  mNvStorageSize = FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
                   FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
                   FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize);

  if (PcdGet32 (PcdFlashNvStorageVariableBase) > FixedPcdGet32 (PcdFlashAreaBaseAddress)) {
    mNvStorageLbaOffset = (EFI_LBA)((PcdGet32 (PcdFlashNvStorageVariableBase)
                                     - FixedPcdGet32 (PcdFlashAreaBaseAddress))
                                    / FixedPcdGet32 (PcdFlashNvStorageBlockSize));
    DEBUG ((DEBUG_INFO, "%a - mNvStorageLbaOffset = 0x%X\n", __func__, mNvStorageLbaOffset));
  } else {
    ASSERT_EFI_ERROR (EFI_DEVICE_ERROR);
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "%a - mNvStorageBase = %X\n", __func__, mNvStorageBase));
  DEBUG ((DEBUG_INFO, "%a - mNvStorageSize = %X\n", __func__, mNvStorageSize));
  DEBUG ((DEBUG_INFO, "%a - mNvStorageLbaOffset = %X\n", __func__, mNvStorageLbaOffset));

  mSpiFlashOffset = GetSpiFlashOffset (mSpiNorFlashProtocol->FlashSize);

  //
  // Validate the FV header in SPI flash before installing the FVB protocol.
  // A corrupted header could cause variable services to read/write garbage.
  //
  Status = ValidateFvHeader (FixedPcdGet32 (PcdFlashNvStorageVariableSize));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - FV header validation failed: %r\n", __func__, Status));
    return Status;
  }

  mSpiFvbHandle = NULL;
  Status        = gSmst->SmmInstallProtocolInterface (
                           &mSpiFvbHandle,
                           &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                           EFI_NATIVE_INTERFACE,
                           &mSpiFvbProtocol
                           );
  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
