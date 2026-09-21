/** @file
  SMM Driver to implement restore primary variable region.

  Copyright (C) 2023 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <FchRegistersCommon.h>
#include <Library/IoLib.h>

#define PM_POWER_RESET_CONFIG  (0xFED80300 + 0x10)
#define RST_CNT                0xCF9
#define SOFTRESET              0x04
#define HARDRESET              0x06

VOID    *mSmmSpiNorRegistration;
UINT32  mSpiFlashOffset;

/**
  Get the flash offset for SPI boot based on SoC.

  @param[in]  FlashSize -  Size of physical flash part
  @retval     SPI Offset of current ROM image.
**/
UINT32
GetSpiFlashOffset (
  IN UINT32  FlashSize
  )
{
  UINT32  Value32;
  UINT8   SpiRomPageXor;
  UINT32  RomPage;
  UINT32  RomBase;
  UINT32  RomOffset;

  if (FlashSize == 0) {
    ASSERT (FALSE);
    DEBUG ((
      DEBUG_ERROR,
      "%a - Error: Flash size should not be zero.\n",
      __func__
      ));
    return 0xFFFFFFFF;
  }

  // Check if SPI Flash Size is less than the BIOS Image Size
  if (FlashSize < FixedPcdGet32 (PcdRom3FlashAreaSize)) {
    ASSERT (FALSE);
    DEBUG ((
      DEBUG_ERROR,
      "%a - Error: Image size larger than flash size.\n",
      __func__
      ));
    return 0xFFFFFFFF;
  }

  RomPage = FixedPcdGet32 (PcdFlashAreaBaseAddress) >> 24;

  // Rom Base is at top of address space
  RomBase = (0xFFFFFFFF - FlashSize) + 1;

  // Determine Bit [24], [25] Address Override Settings
  // SPI ROM Addr[25:24] |= Bit Override Settings
  Value32 = MmioRead32 ((UINTN)(FCH_SPI_BASE_ADDRESS + FCH_SPI_MMIO_REG30));
  if (Value32 & FCH_SPI_R2MSK24) {
    RomPage &= ~(UINT32)FCH_SPI_R2VAL24;
    RomPage |= Value32 & FCH_SPI_R2VAL24;
  }

  if (Value32 & FCH_SPI_R2MSK25) {
    RomPage &= ~(UINT32)FCH_SPI_R2VAL25;
    RomPage |= Value32 & FCH_SPI_R2VAL25;
  }

  // Read SPI XOR Value from Register 0x5C
  // SPI ROM Addr[31:24] = SPI ROM Page [31:24] ^ Host Mem Addr [31:24]
  Value32 = MmioRead32 ((UINTN)(FCH_SPI_BASE_ADDRESS + FCH_SPI_MMIO_REG5C_Addr32_Ctrl3));
  DEBUG ((DEBUG_INFO, "%a - ROM 0x5C ADDR32Ctrl3 = 0x%X\n", __func__, Value32));

  // Compute SPI Address
  SpiRomPageXor = (UINT8)(Value32 & FCH_SPI_SPIROM_PAGE_MASK);
  RomPage      ^= SpiRomPageXor;

  // SPI ROM Addr = Current SPI address - SPI Address Base
  if (RomBase < (RomPage << 24)) {
    RomOffset = (RomPage << 24) - RomBase;
  } else {
    RomOffset = 0;
    ASSERT (FALSE);
  }

  return RomOffset;
}

/**
  To restore Primary Nv Storage region with Default Nv Storeage region.

  @param[in] Protocol           Protocol unique ID.
  @param[in] Interface          Interface instance.
  @param[in] Handle             The handle on which the interface is installed.

  @retval EFI_SUCCESS           Successfully changed NV Store PCDs to Default Store.
  @retval EFI_DEVICE_ERROR      Failed to change NV Store PCDs to Default Store.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate memory, or fail to locate protocol.

**/
EFI_STATUS
EFIAPI
NvStoreRestoreDefaultEvent (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                  Status;
  VOID                        *PrimaryImage;
  VOID                        *DefaultImage;
  UINT32                      PrimaryOffset;
  UINT32                      DefaultOffset;
  UINT32                      DefaultSize;
  UINT32                      VariableStoreSize;
  UINT32                      BlockSize;
  UINT32                      BlockCount;
  UINT32                      FtwOffset;
  UINT32                      FtwSize;
  EFI_SPI_NOR_FLASH_PROTOCOL  *SpiNor;

  DEBUG ((
    DEBUG_INFO,
    "%a:   PcdFlashNvStorageVariableBase    = 0x%X\n\tPcdFlashNvStorageVariableDefaultBase = 0x%X\n\tPcdFlashNvStorageVariablePrimaryBase = 0x%X\n",
    __func__,
    PcdGet32 (PcdFlashNvStorageVariableBase),
    FixedPcdGet32 (PcdFlashNvStorageVariableDefaultBase),
    FixedPcdGet32 (PcdFlashNvStorageVariablePrimaryBase)
    ));

  // Only need to restore if PcdFlashNvStorageVariableBase is pointing to PcdFlashNvStorageVariableDefaultBase, otherwise no action needed.
  if (PcdGet32 (PcdFlashNvStorageVariableBase) != FixedPcdGet32 (PcdFlashNvStorageVariableDefaultBase)) {
    DEBUG ((DEBUG_ERROR, "%a: VarBase != DefaultBase -- not in restore path, exit.\n", __func__));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a: VarBase == DefaultBase -- proceeding with restore.\n", __func__));

  SpiNor = NULL;
  Status = gMmst->MmLocateProtocol (&gEfiSpiSmmNorFlashProtocolGuid, NULL, (VOID **)&SpiNor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmLocateProtocol(SpiSmmNorFlash) failed: %r\n", __func__, Status));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "%a: SpiNor->FlashSize = 0x%X\n", __func__, SpiNor->FlashSize));
  mSpiFlashOffset = GetSpiFlashOffset (SpiNor->FlashSize);
  DEBUG ((DEBUG_INFO, "%a: mSpiFlashOffset = 0x%X\n", __func__, mSpiFlashOffset));

  PrimaryImage      = NULL;
  DefaultImage      = NULL;
  PrimaryOffset     = FixedPcdGet32 (PcdFlashNvStorageVariablePrimaryOffset);
  DefaultOffset     = FixedPcdGet32 (PcdFlashNvStorageVariableDefaultOffset);
  DefaultSize       = FixedPcdGet32 (PcdFlashNvStorageVariableDefaultSize);
  VariableStoreSize = FixedPcdGet32 (PcdFlashNvStorageVariableSize);
  BlockSize         = FixedPcdGet32 (PcdFlashNvStorageBlockSize);

  // Range check before calculating FtwOffset
  if (FixedPcdGet32 (PcdFlashNvStorageFtwWorkingBase) <  FixedPcdGet32 (PcdFlashAreaBaseAddress)) {
    DEBUG ((DEBUG_ERROR, "%a: FtwWorkingBase less than FlashAreaBase\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  FtwOffset = FixedPcdGet32 (PcdFlashNvStorageFtwWorkingBase) - FixedPcdGet32 (PcdFlashAreaBaseAddress);

  // Range check before calculating FtwSize
  if (FtwOffset + FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) + FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize) > SpiNor->FlashSize) {
    DEBUG ((DEBUG_ERROR, "%a: FtwWorkingSize + FtwSpareSize is greater than FlashSize\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  FtwSize = FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) + FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize);

  DEBUG ((
    DEBUG_INFO,
    "%a: PrimaryOffset=0x%X DefaultOffset=0x%X DefaultSize=0x%X\n",
    __func__,
    PrimaryOffset,
    DefaultOffset,
    DefaultSize
    ));
  DEBUG ((
    DEBUG_INFO,
    "%a: VarStoreSize=0x%X BlockSize=0x%X FtwOffset=0x%X FtwSize=0x%X\n",
    __func__,
    VariableStoreSize,
    BlockSize,
    FtwOffset,
    FtwSize
    ));
  DEBUG ((
    DEBUG_INFO,
    "%a: SPI Primary @ 0x%X, SPI Default @ 0x%X, SPI FTW @ 0x%X\n",
    __func__,
    PrimaryOffset + mSpiFlashOffset,
    DefaultOffset + mSpiFlashOffset,
    FtwOffset + mSpiFlashOffset
    ));

  if (BlockSize == 0) {
    DEBUG ((DEBUG_ERROR, "%a: BlockSize is 0, cannot proceed.\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  // Check block size alignment.
  if ((VariableStoreSize % BlockSize) != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Variable store size=0x%X is not a multiple of BlockSize=0x%X.\n", __func__, VariableStoreSize, BlockSize));
    return EFI_DEVICE_ERROR;
  }

  // Check block size alignment.
  if ((FtwSize % BlockSize) != 0) {
    DEBUG ((DEBUG_ERROR, "%a: FTW working + FTW spare size=0x%X is not a multiple of BlockSize=0x%X.\n", __func__, FtwSize, BlockSize));
    return EFI_DEVICE_ERROR;
  }

  // To make sure the size of these 2 var region are the same.
  if (DefaultSize != VariableStoreSize) {
    DEBUG ((DEBUG_ERROR, "%a: Error: Variable store sizes do not match Primary=0x%X, Default=0x%X.\n", __func__, VariableStoreSize, DefaultSize));
    return EFI_DEVICE_ERROR;
  } else {
    BlockCount = VariableStoreSize / BlockSize;
  }

  // Allocate memory and also to check.
  PrimaryImage = AllocatePool (VariableStoreSize);
  DefaultImage = AllocatePool (VariableStoreSize);
  if ((PrimaryImage == NULL) || (DefaultImage == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Use default var region to restore primary var region, with below steps:
  // 1. Read primary var region to PrimaryImage
  // 2. Read default var region to DefaultImage
  // 3. If PrimaryImage != DefaultImage, that means primary var region is corrupted.
  //  3.1. Erase primary var region
  //  3.2. Write DefaultImage to primary var region
  //  3.3. Read FTW header, to saved in PrimaryImage
  //  3.4. Erase FWT region
  //  3.5. Write PrimaryImage(FWT header) back to FWT region
  //

  Status = SpiNor->ReadData (SpiNor, PrimaryOffset + mSpiFlashOffset, VariableStoreSize, PrimaryImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read Primary image Status=%r.\n", __func__, Status));
    goto ON_EXIT;
  }

  Status = SpiNor->ReadData (SpiNor, DefaultOffset + mSpiFlashOffset, VariableStoreSize, DefaultImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read Default image Status=%r.\n", __func__, Status));
    goto ON_EXIT;
  }

  if (CompareMem (DefaultImage, PrimaryImage, VariableStoreSize) != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Primary != Default -- recovering from backup.\n", __func__));

    Status = SpiNor->Erase (SpiNor, PrimaryOffset + mSpiFlashOffset, BlockCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Primary variable store erase failed Status=%r\n", __func__, Status));
      goto ON_EXIT;
    }

    Status = SpiNor->WriteData (SpiNor, PrimaryOffset + mSpiFlashOffset, VariableStoreSize, DefaultImage);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Primary variable store write failed Status=%r\n", __func__, Status));
      goto ON_EXIT;
    }

    Status = SpiNor->ReadData (SpiNor, FtwOffset + mSpiFlashOffset, 0x20, PrimaryImage);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to read FTW header Status=%r.\n", __func__, Status));
      goto ON_EXIT;
    }

    Status = SpiNor->Erase (SpiNor, FtwOffset + mSpiFlashOffset, FtwSize / BlockSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: FTW area erase failed Status=%r\n", __func__, Status));
      goto ON_EXIT;
    }

    Status = SpiNor->WriteData (SpiNor, FtwOffset + mSpiFlashOffset, 0x20, PrimaryImage);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: FTW area write failed Status=%r\n", __func__, Status));
      goto ON_EXIT;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Primary == Default -- no recovery needed.\n", __func__));
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Switching VarBase back to PrimaryBase (0x%X).\n",
    __func__,
    FixedPcdGet32 (PcdFlashNvStorageVariablePrimaryBase)
    ));

  PcdSet32S (PcdFlashNvStorageVariableBase, FixedPcdGet32 (PcdFlashNvStorageVariablePrimaryBase));

ON_EXIT:
  if (PrimaryImage != NULL) {
    FreePool (PrimaryImage);
  }

  if (DefaultImage != NULL) {
    FreePool (DefaultImage);
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: EXIT -- Status=%r, VarBase=0x%X\n",
    __func__,
    Status,
    PcdGet32 (PcdFlashNvStorageVariableBase)
    ));
  return Status;
}

/**
  Register notification for gEfiSpiSmmNorFlashProtocolGuid.

  @param[in] ImageHandle     Module's image handle.
  @param[in] SystemTable     Pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS       Successfully changed NV Store PCDs to Default Store.
  @retval EFI_DEVICE_ERROR  Failed to change NV Store PCDs to Default Store.

**/
EFI_STATUS
EFIAPI
NvStoreRestoreDefaultEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((
    DEBUG_INFO,
    "%a: ENTRY\n\tVarBase=0x%X\n\tDefaultBase=0x%X \n\tPrimaryOffset=0x%X \n\tDefaultOffset=0x%X\n",
    __func__,
    PcdGet32 (PcdFlashNvStorageVariableBase),
    FixedPcdGet32 (PcdFlashNvStorageVariableDefaultBase),
    PcdGet32 (PcdFlashNvStorageVariablePrimaryOffset),
    PcdGet32 (PcdFlashNvStorageVariableDefaultOffset)
    ));

  if ((FixedPcdGet32 (PcdFlashNvStorageVariablePrimaryOffset) == 0) ||
      (FixedPcdGet32 (PcdFlashNvStorageVariableDefaultOffset) == 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: Switching to default not supported (offset PCDs are zero).\n", __func__));
    return EFI_SUCCESS;
  }

  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiSpiSmmNorFlashProtocolGuid,
                    NvStoreRestoreDefaultEvent,
                    &mSmmSpiNorRegistration
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmRegisterProtocolNotify failed: %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: EXIT -- registered SpiNorFlash notify (Status=%r)\n", __func__, Status));
  return Status;
}
