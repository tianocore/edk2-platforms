/*****************************************************************************
 * Copyright (C) 2018-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *****************************************************************************/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "EspiNorFlash.h"
#include "EspiNorFlashInstance.h"

/**
  Read SFDP Header

  This routine reads the JEDEC SPI Flash Discoverable Parameter header from the
  SPI chip.  Fails if Major Revision is not = 1

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]  SfdpHeader  SFDP Header Buffer Pointer

  @retval EFI_SUCCESS            Header is filled in
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
EFIAPI
ReadSfdpHeader (
  IN      ESPI_NOR_FLASH_INSTANCE  *Instance,
  IN      SFDP_HEADER              *SfdpHeader
  )
{
  EFI_STATUS  Status;
  UINT32      TransactionBufferLength;

  // Check not WIP
  Status = WaitNotWip (Instance);

  // Read SFDP Header
  TransactionBufferLength = FillWriteBuffer (
                              Instance,
                              SPI_FLASH_RDSFDP,
                              SPI_FLASH_RDSFDP_DUMMY,
                              SPI_FLASH_RDSFDP_ADDR_BYTES,
                              TRUE,
                              0,
                              0,
                              NULL
                              );
  Status = Instance->SpiIo->Transaction (
                              Instance->SpiIo,
                              SPI_TRANSACTION_WRITE_THEN_READ,
                              FALSE,
                              0,
                              1,
                              8,
                              TransactionBufferLength,
                              Instance->SpiTransactionWriteBuffer,
                              sizeof (SFDP_HEADER),
                              (UINT8 *)SfdpHeader
                              );
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    // Read Basic Flash Parameter Header
    if ((SfdpHeader->Signature != SFDP_HEADER_SIGNATURE) ||
        (SfdpHeader->MajorRev != SFDP_SUPPORTED_MAJOR_REVISION))
    {
      Status = EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Read SFDP Basic Parameter Header

  This routine reads the JEDEC SPI Flash Discoverable Parameter header from the
  SPI chip.  Fails if Major Revision is not = 1

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]  SfdpHeader  SFDP Header Buffer Pointer

  @retval EFI_SUCCESS            Header is filled in
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
EFIAPI
ReadSfdpBasicParameterHeader (
  IN      ESPI_NOR_FLASH_INSTANCE  *Instance,
  IN      SFDP_PARAMETER_HEADER    *SfdpParameterHeader
  )
{
  EFI_STATUS             Status;
  UINT32                 Index;
  SFDP_HEADER            SfdpHeader;
  SFDP_PARAMETER_HEADER  LocalSfdpParameterHeader;
  UINT32                 TransactionBufferLength;

  Status = ReadSfdpHeader (Instance, &SfdpHeader);
  if (!EFI_ERROR (Status)) {
    // Parse Parameter Headers Starting at Index 1 = Byte 8
    ZeroMem (SfdpParameterHeader, sizeof (SFDP_PARAMETER_HEADER));
    for (Index = 1; Index <= (UINT32)(SfdpHeader.NumParameterHeaders + 1); Index++) {
      // Check not WIP
      Status = WaitNotWip (Instance);
      if (!EFI_ERROR (Status)) {
        TransactionBufferLength = FillWriteBuffer (
                                    Instance,
                                    SPI_FLASH_RDSFDP,
                                    SPI_FLASH_RDSFDP_DUMMY,
                                    SPI_FLASH_RDSFDP_ADDR_BYTES,
                                    TRUE,
                                    Index * 8, // Parameter Header Index
                                    0,
                                    NULL
                                    );
        Status = Instance->SpiIo->Transaction (
                                    Instance->SpiIo,
                                    SPI_TRANSACTION_WRITE_THEN_READ,
                                    FALSE,
                                    0,
                                    1,
                                    8,
                                    TransactionBufferLength,
                                    Instance->SpiTransactionWriteBuffer,
                                    sizeof (LocalSfdpParameterHeader),
                                    (UINT8 *)&LocalSfdpParameterHeader
                                    );
        ASSERT_EFI_ERROR (Status);
        if (!EFI_ERROR (Status)) {
          // Break if SfdParamHeader is Type 0, Basic SPI Protocol Parameters
          if ((LocalSfdpParameterHeader.IdLsb == 0x00) &&
              (LocalSfdpParameterHeader.IdMsb == 0xFF) &&
              (LocalSfdpParameterHeader.MajorRev == 1) &&
              (LocalSfdpParameterHeader.MinorRev >= SfdpParameterHeader->MinorRev))
          {
            CopyMem (
              SfdpParameterHeader,
              &LocalSfdpParameterHeader,
              sizeof (SFDP_PARAMETER_HEADER)
              );
          }
        } else {
          break;
        }
      } else {
        break;
      }
    }

    if ((SfdpParameterHeader->IdLsb != 0x00) ||
        (SfdpParameterHeader->IdMsb != 0xFF))
    {
      Status =  EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Read SFDP parameters into buffer

  This routine reads the JEDEC SPI Flash Discoverable Parameters from the SPI
  chip.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
  EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            The SPI part size is filled.
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
EFIAPI
ReadSfdpBasicParameterTable (
  IN  ESPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS             Status;
  SFDP_PARAMETER_HEADER  SfdpBasicFlashParamHeader;
  UINT32                 LengthInBytes;
  UINT32                 ByteCounter;
  UINT32                 CurrentAddress;
  UINT8                  *CurrentBuffer;
  UINT32                 Length;
  UINT32                 TransactionBufferLength;
  UINT32                 MaximumTransferBytes;

  Status = ReadSfdpBasicParameterHeader (Instance, &SfdpBasicFlashParamHeader);

  if (!EFI_ERROR (Status)) {
    // Read Basic Flash Parameters.  Already know it is MajorRev = 1
    Instance->SfdpBasicFlashByteCount = SfdpBasicFlashParamHeader.Length * 4;
    LengthInBytes                     = Instance->SfdpBasicFlashByteCount;
    Instance->SfdpBasicFlash          = AllocateZeroPool (LengthInBytes);

    if (Instance->SfdpBasicFlash != NULL) {
      MaximumTransferBytes = Instance->SpiIo->MaximumTransferBytes;

      CurrentBuffer = (UINT8 *)Instance->SfdpBasicFlash;
      for (ByteCounter = 0; ByteCounter < LengthInBytes; ByteCounter += MaximumTransferBytes) {
        CurrentAddress = SfdpBasicFlashParamHeader.TablePointer + ByteCounter;
        Length         = LengthInBytes - ByteCounter;
        // Length must be MaximumTransferBytes or less
        if (Length > MaximumTransferBytes) {
          Length = MaximumTransferBytes;
        }

        // Check not WIP
        Status = WaitNotWip (Instance);

        //  Read Data
        if (!EFI_ERROR (Status)) {
          TransactionBufferLength = FillWriteBuffer (
                                      Instance,
                                      SPI_FLASH_RDSFDP,
                                      SPI_FLASH_RDSFDP_DUMMY,
                                      SPI_FLASH_RDSFDP_ADDR_BYTES,
                                      TRUE,
                                      CurrentAddress,
                                      0,
                                      NULL
                                      );
          Status = Instance->SpiIo->Transaction (
                                      Instance->SpiIo,
                                      SPI_TRANSACTION_WRITE_THEN_READ,
                                      FALSE,
                                      0,
                                      1,
                                      8,
                                      TransactionBufferLength,
                                      Instance->SpiTransactionWriteBuffer,
                                      Length,
                                      CurrentBuffer
                                      );
          ASSERT_EFI_ERROR (Status);
          CurrentBuffer += Length;
        } else {
          break;
        }
      }
    }
  }

  return Status;
}
