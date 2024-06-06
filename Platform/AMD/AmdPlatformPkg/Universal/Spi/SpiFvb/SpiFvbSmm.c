/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Register/Cpuid.h>
#include <Library/BaseLib.h>
#include <FchRegistersCommon.h>
#include <Library/PciLib.h>
#define BLOCK_SIZE  (FixedPcdGet32 (PcdFlashNvStorageBlockSize))

extern EFI_SPI_NOR_FLASH_PROTOCOL          *mSpiNorFlashProtocol;
extern EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mSpiFvbProtocol;
extern EFI_PHYSICAL_ADDRESS                mNvStorageBase;
extern EFI_LBA                             mNvStorageLbaOffset;

STATIC EFI_HANDLE  mSpiFvbHandle;
UINT32             mSpiFlashOffset;

extern UINT32
SetSpiFlashOffset (
  UINT32  FlashSize
  );

extern UINT32
SetEspiFlashOffset (
  UINT32  FlashSize
  );

/**
  Check if SAFS mode is enabled

  @retval TRUE                   SAFS mode is enabled.
  @retval FALSE                  MAFS mode is enabled

**/
BOOLEAN
EFIAPI
IsEspiSafsMode (
  VOID
  )
{
  UINT32  MISC80;

  MISC80 = MmioRead32 (ACPI_MMIO_BASE + MISC_BASE + FCH_MISC_REG80);
  if ((MISC80 & BIT3)) {
    // romtype [5:4] 10: eSPI with SAFS support
    return TRUE;
  }

  return FALSE;
}

/**
  SPI firmware volume SMM driver EntryPoint.

  @param[in] ImageHandle    Driver Image Handle
  @param[in] MmSystemTable  MM System Table

  @retval EFI_SUCCESS           Driver initialization succeeded
  @retval all others            Driver initialization failed

**/
EFI_STATUS
EFIAPI
SpiFvbSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  UINT32  (*SetFlashOffset)(
    UINT32
    );
  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  if (IsEspiSafsMode ()) {
    DEBUG ((DEBUG_INFO, "Espi SAFS boot mode detected!\n"));
    Status = gSmst->SmmLocateProtocol (
                      &gAmdEspiSmmNorFlashProtocolGuid,
                      NULL,
                      (VOID **)&mSpiNorFlashProtocol
                      );
    SetFlashOffset = &SetEspiFlashOffset;
  } else {
    DEBUG ((DEBUG_INFO, "Default (SPIROM) boot mode detected!\n"));
    // Retrieve SPI NOR flash driver
    Status = gSmst->SmmLocateProtocol (
                      &gEfiSpiSmmNorFlashProtocolGuid,
                      NULL,
                      (VOID **)&mSpiNorFlashProtocol
                      );
    SetFlashOffset = &SetSpiFlashOffset;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  mNvStorageBase = (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdFlashNvStorageVariableBase);
  DEBUG ((DEBUG_INFO, "%a - mNvStorageBase = %X\n", __func__, mNvStorageBase));
  mNvStorageLbaOffset = (EFI_LBA)((PcdGet32 (PcdFlashNvStorageVariableBase)
                                   - FixedPcdGet32 (PcdFlashAreaBaseAddress))
                                  / FixedPcdGet32 (PcdFlashNvStorageBlockSize));
  DEBUG ((DEBUG_INFO, "%a - mNvStorageLbaOffset = 0x%X\n", __func__, mNvStorageLbaOffset));

  mSpiFvbHandle = NULL;
  Status        = gSmst->SmmInstallProtocolInterface (
                           &mSpiFvbHandle,
                           &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                           EFI_NATIVE_INTERFACE,
                           &mSpiFvbProtocol
                           );
  mSpiFlashOffset = (*SetFlashOffset)(mSpiNorFlashProtocol->FlashSize);

  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
