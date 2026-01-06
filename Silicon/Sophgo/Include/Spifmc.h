/** @file
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __SPI_FLASH_MASTER_CONTROLLER_H__
#define __SPI_FLASH_MASTER_CONTROLLER_H__

#include <Library/NorFlashInfoLib.h>

extern EFI_GUID gSophgoSpiMasterProtocolGuid;

typedef struct _SOPHGO_SPI_MASTER_PROTOCOL SOPHGO_SPI_MASTER_PROTOCOL;

/**
  struct SPI_NOR - Structure for defining the SPI NOR layer

  @SpiBase            the SPI Nor Flash base address
  @BounceBuf          bounce buffer used when the buffer passed by the MTD
                      layer is not DMA-able
  @BounceBufSize      size of the bounce buffer
  @AddrNbytes         number of address bytes
  @EraseOpcode        the opcode for erasing a sector
  @ReadOpcode         the read opcode
  @ReadDummy          the dummy needed by the read operation
  @ProgramOpcode      the program opcode
  @Info               SPI NOR part JEDEC MFR ID and other info

**/
typedef struct {
  UINTN          SpiBase;
  UINT8          *BounceBuf;
  UINTN          BounceBufSize;
  UINT8          AddrNbytes;
  UINT8          EraseOpcode;
  UINT8          ReadOpcode;
  UINT8          ReadDummy;
  UINT8          ProgramOpcode;
  NOR_FLASH_INFO *Info;
} SPI_NOR;

typedef
EFI_STATUS
(EFIAPI *SG_SPI_MASTER_PROTOCOL_READ_REGISTER)(
  IN  SPI_NOR                                 *Nor,
  IN  UINT8                                   Opcode,
  IN  UINTN                                   Length,
  OUT UINT8                                   *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *SG_SPI_MASTER_PROTOCOL_WRITE_REGISTER)(
  IN  SPI_NOR                                 *Nor,
  IN  UINT8                                   Opcode,
  IN  CONST UINT8                             *Buffer,
  IN  UINTN                                   Length
  );

typedef
EFI_STATUS
(EFIAPI *SG_SPI_MASTER_PROTOCOL_READ)(
  IN  SPI_NOR                                 *Nor,
  IN  UINTN                                   From,
  IN  UINTN                                   Length,
  OUT UINT8                                   *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *SG_SPI_MASTER_PROTOCOL_WRITE)(
  IN  SPI_NOR                                 *Nor,
  IN  UINTN                                   To,
  IN  UINTN                                   Length,
  IN  CONST UINT8                             *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *SG_SPI_MASTER_PROTOCOL_ERASE)(
  IN  SPI_NOR                                 *Nor,
  IN  UINTN                                   Offs
  );

typedef
SPI_NOR *
(EFIAPI *SG_SPI_MASTER_PROTOCOL_SETUP_DEVICE) (
  IN SOPHGO_SPI_MASTER_PROTOCOL              *This,
  IN UINT8                                   SelectedFlashNumber
  );

struct _SOPHGO_SPI_MASTER_PROTOCOL {
  SG_SPI_MASTER_PROTOCOL_READ_REGISTER       ReadRegister;
  SG_SPI_MASTER_PROTOCOL_WRITE_REGISTER      WriteRegister;
  SG_SPI_MASTER_PROTOCOL_READ                Read;
  SG_SPI_MASTER_PROTOCOL_WRITE               Write;
  SG_SPI_MASTER_PROTOCOL_ERASE               Erase;
  SG_SPI_MASTER_PROTOCOL_SETUP_DEVICE        SetupDevice;
};

#endif // __SPI_FLASH_MASTER_CONTROLLER_H__
