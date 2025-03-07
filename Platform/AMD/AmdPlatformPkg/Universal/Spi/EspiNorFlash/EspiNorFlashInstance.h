/*****************************************************************************
 * Copyright (C) 2018-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *****************************************************************************/

#ifndef ESPI_NOR_FLASH_INSTANCE_H_
#define ESPI_NOR_FLASH_INSTANCE_H_

#include <PiDxe.h>
#include <Protocol/SpiNorFlash.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Library/FchEspiCmdLib.h>

#define ESPI_NOR_FLASH_SIGNATURE  SIGNATURE_32 ('e', 's', 'n', 'f')

typedef struct {
  UINTN                         Signature;
  EFI_HANDLE                    Handle;
  EFI_SPI_NOR_FLASH_PROTOCOL    Protocol;
  EFI_SPI_IO_PROTOCOL           *SpiIo;
  UINT32                        SfdpBasicFlashByteCount;
  SFDP_BASIC_FLASH_PARAMETER    *SfdpBasicFlash;
  UINT8                         *SpiTransactionWriteBuffer;
  UINT32                        SpiTransactionWriteBufferIndex;
  BOOLEAN                       EspiSafsMode;
  UINT32                        EspiBaseAddress;
  UINT32                        EspiMaxReadReqSize;
  UINT32                        EspiMaxPayloadSize;
  UINT32                        EspiEraseBlockMap;
} ESPI_NOR_FLASH_INSTANCE;

#define ESPI_NOR_FLASH_FROM_THIS(a) \
  CR (a, ESPI_NOR_FLASH_INSTANCE, Protocol, \
      ESPI_NOR_FLASH_SIGNATURE)

#endif // ESPI_NOR_FLASH_INSTANCE_H_
