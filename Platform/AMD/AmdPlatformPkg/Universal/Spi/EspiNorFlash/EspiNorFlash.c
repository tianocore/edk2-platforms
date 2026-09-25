/** @file

  eSPI NOR flash implementation.

  Copyright (C) 2018 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <Protocol/SpiSmmNorFlash.h>
#include "EspiNorFlashInstance.h"

/**
  Wrapper function of the ESPI_NOR_FLASH_FROM_THIS CR Macro.
  Necessary to reduce amount of CERT-C errors in this module.

  @param[in]  This    Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                      structure.

  @retval ESPI_NOR_FLASH_INSTANCE  Pointer to the ESPI_NOR_FLASH_INSTANCE data structure.
**/
ESPI_NOR_FLASH_INSTANCE *
EFIAPI
EspiNorFlashFromThis (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This
  )
{
  // coverity[cert_exp36_c_violation]
  // coverity[cert_exp39_c_violation]
  // coverity[cert_exp40_c_violation]
  return ESPI_NOR_FLASH_FROM_THIS (This);
}

/**
  Read the flash status register. Not supported for eSPI SAFS.

  @param[in]  This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                             structure.
  @param[in]  LengthInBytes  Number of status bytes to read.
  @param[out] FlashStatus    Pointer to a buffer to receive the flash status.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
ReadStatus (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN  UINT32                            LengthInBytes,
  OUT UINT8                             *FlashStatus
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Write the flash status register. Not supported for eSPI SAFS.

  @param[in] This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                            structure.
  @param[in] LengthInBytes  Number of status bytes to write.
  @param[in] FlashStatus    Pointer to a buffer containing the new status.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
WriteStatus (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            LengthInBytes,
  IN UINT8                             *FlashStatus
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Read the 3 byte manufacture and device ID from the SPI flash.
  Not supported for eSPI SAFS, so return 0x000000 for FlashId.

  @param[in]  This    Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data structure.
  @param[out] Buffer  Pointer to a 3 byte buffer to receive the manufacture and
                      device ID.

  @retval EFI_SUCCESS            Buffer is not NULL
  @retval EFI_INVALID_PARAMETER  Buffer is NULL
**/
EFI_STATUS
EFIAPI
GetFlashId (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  OUT UINT8                             *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // ESPI SAFS
  Buffer[0] = 0;
  Buffer[1] = 0;
  Buffer[2] = 0;

  return EFI_SUCCESS;
}

/**
  Read data from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                             structure.
  @param[in]  FlashAddress   Address in the flash to start reading
  @param[in]  LengthInBytes  Read length in bytes
  @param[out] Buffer         Address of a buffer to receive the data

  @retval EFI_SUCCESS            The data was read successfully.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL, or
                                 FlashAddress >= This->FlashSize, or
                                 LengthInBytes > This->FlashSize - FlashAddress

**/
EFI_STATUS
EFIAPI
ReadData (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN  UINT32                            FlashAddress,
  IN  UINT32                            LengthInBytes,
  OUT UINT8                             *Buffer
  )
{
  EFI_STATUS               Status;
  ESPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                   ByteCounter;
  UINT32                   CurrentAddress;
  UINT8                    *CurrentBuffer;
  UINT32                   Length;
  UINT32                   MaximumTransferBytes;

  Status = EFI_DEVICE_ERROR;
  if ((Buffer == NULL) ||
      (FlashAddress >= This->FlashSize) ||
      (LengthInBytes > This->FlashSize - FlashAddress))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance = EspiNorFlashFromThis (This);
  if (Instance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - ERROR: EspiNorFlashFromThis failed\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  // Adjust flash offset, zero bits above 16MB.
  FlashAddress &= ROM_ADDRESS_MASK;
  FlashAddress += Instance->EspiFlashOffset;

  // ESPI SAFS
  MaximumTransferBytes = Instance->EspiMaxReadReqSize;

  CurrentBuffer = Buffer;
  for (ByteCounter = 0; ByteCounter < LengthInBytes;) {
    CurrentAddress = FlashAddress + ByteCounter;
    CurrentBuffer  = Buffer + ByteCounter;
    Length         = LengthInBytes - ByteCounter;
    // Length must be MaximumTransferBytes or less
    if (Length > MaximumTransferBytes) {
      Length = MaximumTransferBytes;
    }

    // ESPI SAFS
    Status = FchEspiCmd_SafsFlashRead (Instance->EspiBaseAddress, CurrentAddress, Length, CurrentBuffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Espi Read Data FchEspiCmd_SafsFlashRead ERROR Status -%r\n", Status));
    }

    ASSERT_EFI_ERROR (Status);
    ByteCounter += Length;
  }

  return Status;
}

/**
  Write data to the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine breaks up the write operation as necessary to write the data to
  the SPI part.

  @param[in] This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                            structure.
  @param[in] FlashAddress   Address in the flash to start writing
  @param[in] LengthInBytes  Write length in bytes
  @param[in] Buffer         Address of a buffer containing the data

  @retval EFI_SUCCESS            The data was written successfully.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL, or
                                 FlashAddress >= This->FlashSize, or
                                 LengthInBytes > This->FlashSize - FlashAddress
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory to copy buffer.

**/
EFI_STATUS
EFIAPI
WriteData (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            FlashAddress,
  IN UINT32                            LengthInBytes,
  IN UINT8                             *Buffer
  )
{
  EFI_STATUS               Status;
  ESPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                   ByteCounter;
  UINT32                   CurrentAddress;
  UINT32                   Length;
  UINT32                   BytesUntilBoundary;
  UINT8                    *CurrentBuffer;
  UINT32                   MaximumTransferBytes;
  UINT32                   SpiFlashPageSize;

  Status = EFI_DEVICE_ERROR;
  if ((Buffer == NULL) ||
      (LengthInBytes == 0) ||
      (FlashAddress >= This->FlashSize) ||
      (LengthInBytes > This->FlashSize - FlashAddress))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance = EspiNorFlashFromThis (This);
  if (Instance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - ERROR: EspiNorFlashFromThis failed\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  // Adjust flash offset, zero bits above 16MB.
  FlashAddress &= ROM_ADDRESS_MASK;
  FlashAddress += Instance->EspiFlashOffset;

  // ESPI SAFS
  MaximumTransferBytes = Instance->EspiMaxPayloadSize;

  SpiFlashPageSize = 256;

  CurrentBuffer = Buffer;
  for (ByteCounter = 0; ByteCounter < LengthInBytes;) {
    CurrentAddress = FlashAddress + ByteCounter;
    CurrentBuffer  = Buffer + ByteCounter;
    Length         = LengthInBytes - ByteCounter;
    // Length must be MaximumTransferBytes or less
    if (Length > MaximumTransferBytes) {
      Length = MaximumTransferBytes;
    }

    // Cannot cross SpiFlashPageSize boundary
    BytesUntilBoundary = SpiFlashPageSize
                         - (CurrentAddress % SpiFlashPageSize);
    if ((BytesUntilBoundary != 0) && (Length > BytesUntilBoundary)) {
      Length = BytesUntilBoundary;
    }

    // ESPI SAFS
    Status = FchEspiCmd_SafsFlashWrite (Instance->EspiBaseAddress, CurrentAddress, Length, CurrentBuffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "ESPI %a - ERROR after FchEspiCmd_SafsFlashWrite Status -%r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
      break;
    }

    ASSERT_EFI_ERROR (Status);
    ByteCounter += Length;
  }

  return Status;
}

/**
  Efficiently erases one or more 4KiB regions in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine uses a combination of 4 KiB and larger blocks to erase the
  specified area.

  @param[in] This          Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                           structure.
  @param[in] FlashAddress  Address within a 4 KiB block to start erasing
  @param[in] BlockCount    Number of 4 KiB blocks to erase

  @retval EFI_SUCCESS            The erase was completed successfully.
  @retval EFI_INVALID_PARAMETER  FlashAddress >= This->FlashSize, or
                                 BlockCount * 4 KiB
                                   > This->FlashSize - FlashAddress

**/
EFI_STATUS
EFIAPI
Erase (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            FlashAddress,
  IN UINT32                            BlockCount
  )
{
  EFI_STATUS                  Status;
  ESPI_NOR_FLASH_INSTANCE     *Instance;
  UINT32                      ByteCounter;
  UINT32                      EraseLength;
  UINT32                      CurrentAddress;
  UINT32                      Length;
  ESPI_SL44_SLAVE_FA_CAPCFG2  FaCapCfg2;

  Status = EFI_DEVICE_ERROR;

  if (BlockCount > MAX_UINT32 / SIZE_4KB) {
    DEBUG ((DEBUG_ERROR, "%a - ERROR: BlockCount too large, wrapping over UINT32\n", __FUNCTION__));
    return Status;
  }

  EraseLength = BlockCount * SIZE_4KB;
  Instance    = EspiNorFlashFromThis (This);
  if (Instance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - ERROR: EspiNorFlashFromThis failed\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  // Adjust flash offset, zero bits above 16MB.
  FlashAddress &= ROM_ADDRESS_MASK;
  FlashAddress += Instance->EspiFlashOffset;

  // Align start Address to 4KB
  CurrentAddress = FlashAddress & ~((UINT32)SIZE_4KB - 1);
  if ((BlockCount == 0) ||
      (CurrentAddress >= This->FlashSize) ||
      (EraseLength > This->FlashSize - CurrentAddress))
  {
    return EFI_INVALID_PARAMETER;
  }

  for (ByteCounter = 0; ByteCounter < EraseLength;) {
    CurrentAddress = FlashAddress + ByteCounter;
    Length         = EraseLength - ByteCounter;

    // ESPI SAFS
    FaCapCfg2.Value = Instance->EspiEraseBlockMap;
    // Calculate largest erase size for this pass
    if (((CurrentAddress % SIZE_128KB) == 0) &&
        (Length >= SIZE_128KB) &&
        ((FaCapCfg2.Field.RO_TargetFlashEraseBlockSize & BIT7) != 0))
    {
      Length = SIZE_128KB;
      DEBUG ((DEBUG_INFO, "128KB Block Erase at Address=0x%x\n", CurrentAddress));
      Status = FchEspiCmd_SafsFlashErase (Instance->EspiBaseAddress, CurrentAddress, 3);
    } else if (((CurrentAddress % SIZE_64KB) == 0) &&
               (Length >= SIZE_64KB) &&
               ((FaCapCfg2.Field.RO_TargetFlashEraseBlockSize & BIT6) != 0))
    {
      Length = SIZE_64KB;
      DEBUG ((DEBUG_INFO, "64KB Block Erase at Address=0x%x\n", CurrentAddress));
      Status = FchEspiCmd_SafsFlashErase (Instance->EspiBaseAddress, CurrentAddress, 2);
    } else if (((CurrentAddress % SIZE_32KB) == 0) &&
               (Length >= SIZE_32KB) &&
               ((FaCapCfg2.Field.RO_TargetFlashEraseBlockSize & BIT5) != 0))
    {
      Length = SIZE_32KB;
      DEBUG ((DEBUG_INFO, "32KB Block Erase at Address=0x%x\n", CurrentAddress));
      Status = FchEspiCmd_SafsFlashErase (Instance->EspiBaseAddress, CurrentAddress, 1);
    } else if (((CurrentAddress % SIZE_4KB) == 0) &&
               (Length >= SIZE_4KB) &&
               ((FaCapCfg2.Field.RO_TargetFlashEraseBlockSize & BIT2) != 0))
    {
      Length = SIZE_4KB;
      DEBUG ((DEBUG_INFO, "4KB Block Erase at Address=0x%x\n", CurrentAddress));
      Status = FchEspiCmd_SafsFlashErase (Instance->EspiBaseAddress, CurrentAddress, 0);
    } else {
      DEBUG ((DEBUG_ERROR, "%a - Unsupported Erase request\n", __func__));
      Status = EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "ESPI %a - ERROR after FchEspiCmd_SafsFlashErase Status -%r\n", __func__, Status));
      ASSERT_EFI_ERROR (Status);
      break;
    }

    ASSERT_EFI_ERROR (Status);
    ByteCounter += Length;
  }

  return Status;
}
