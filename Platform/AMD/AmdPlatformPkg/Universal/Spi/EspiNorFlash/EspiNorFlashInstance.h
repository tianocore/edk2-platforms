/** @file

  eSPI NOR flash instance header file.

  Copyright (C) 2018 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <PiDxe.h>
#include <Protocol/SpiNorFlash.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Library/FchEspiCmdLib.h>

///
/// CERT-C scan flags all the macros starting with 'E' as DCL37-C issues, even
/// though these macros are not defined in errno.h and other C Standard library files.
/// These issues are false positives.
///
// coverity[cert_dcl37_c_violation]
#define ESPI_NOR_FLASH_SIGNATURE  SIGNATURE_32 ('e', 's', 'n', 'f')
#define ROM_ADDRESS_MASK          0x00FFFFFF // 16MB ROM area

typedef struct {
  UINTN                         Signature;
  EFI_HANDLE                    Handle;
  EFI_SPI_NOR_FLASH_PROTOCOL    Protocol;
  EFI_SPI_IO_PROTOCOL           *SpiIo;
  BOOLEAN                       EspiSafsMode;
  UINT32                        EspiBaseAddress;
  UINT32                        EspiMaxReadReqSize;
  UINT32                        EspiMaxPayloadSize;
  UINT32                        EspiEraseBlockMap;
  UINT32                        EspiFlashOffset;
  ///
  /// CERT-C scan flags all the macros starting with 'E' as DCL37-C issues, even
  /// though these macros are not defined in errno.h and other C Standard library files.
  /// These issues are false positives.
  ///
  // coverity[cert_dcl37_c_violation]
} ESPI_NOR_FLASH_INSTANCE;

///
/// CERT-C scan flags all the macros starting with 'E' as DCL37-C issues, even
/// though these macros are not defined in errno.h and other C Standard library files.
/// These issues are false positives.
///
// coverity[cert_dcl37_c_violation]
#define ESPI_NOR_FLASH_FROM_THIS(a) \
  CR (a, ESPI_NOR_FLASH_INSTANCE, Protocol, \
      ESPI_NOR_FLASH_SIGNATURE)
