/** @file
 *
 *  The definitions of SPI NOR Flash commands and registers are from Linux Kernel.
 *
 *  Copyright (c) 2024, SOPHGO Inc. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef __NOR_FLASH_PROTOCOL_H__
#define __NOR_FLASH_PROTOCOL_H__

#include <Include/Spifmc.h>

#define SPI_NOR_MAX_ID_LEN      6

//
// Flash opcodes.
//
#define SPINOR_OP_WRDI          0x04    /* Write disable */
#define SPINOR_OP_WREN          0x06    /* Write enable */
#define SPINOR_OP_RDSR          0x05    /* Read status register */
#define SPINOR_OP_WRSR          0x01    /* Write status register 1 byte */
#define SPINOR_OP_READ          0x03    /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST     0x0b    /* Read data bytes (high frequency) */

#define SPINOR_OP_PP            0x02    /* Page program (up to 256 bytes) */
#define SPINOR_OP_SE            0xd8    /* Sector erase (usually 64KiB) */
#define SPINOR_OP_BE_4K         0x20    /* Erase 4KiB block */
#define SPINOR_OP_CHIP_ERASE    0xc7    /* Erase whole flash chip */
#define SPINOR_OP_RDID          0x9f    /* Read JEDEC ID */
#define SPINOR_OP_RDCR          0x35    /* Read configuration register */

#define SPINOR_SRSTEN_OP        0x66    /* Soft reset enable */
#define SPINOR_SRST_OP          0x99    /* Soft reset */

//
// 4-byte address opcodes.
//
#define SPINOR_OP_READ_4B       0x13    /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST_4B  0x0c    /* Read data bytes (high frequency) */
#define SPINOR_OP_PP_4B         0x12    /* Page program (up to 256 bytes) */
#define SPINOR_OP_SE_4B         0xdc    /* Sector erase (usually 64KiB) */
#define SPINOR_OP_BE_4K_4B      0x21    /* Erase 4KiB block */
#define SPINOR_OP_EN4B          0xb7    /* Enter 4-byte mode */
#define SPINOR_OP_EX4B          0xe9    /* Exit 4-byte mode */

//
// Status Register bits.
//
#define SR_WIP                  BIT0  /* Write in progress */
#define SR_WEL                  BIT1  /* Write enable latch */
#define SR_BP2                  BIT4  /* Block Protect Bits */
#define SR_BP3                  BIT5  /* Block Protect Bits */
#define SR_SRP0                 BIT7  /* Status Register Protection */


extern EFI_GUID  gSophgoNorFlashProtocolGuid;

typedef struct _SOPHGO_NOR_FLASH_PROTOCOL SOPHGO_NOR_FLASH_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_GET_FLASH_ID)(
  IN  SPI_NOR                          *Nor,
  IN  BOOLEAN                           UseInRuntime
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_READ_DATA)(
  IN  SPI_NOR                          *Nor,
  IN  UINTN                            FlashAddress,
  IN  UINTN                            LengthInBytes,
  OUT UINT8                            *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_READ_STATUS)(
  IN  SPI_NOR                          *Nor,
  OUT UINT8                            *FlashStatus
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_WRITE_STATUS)(
  IN SPI_NOR                          *Nor,
  IN UINT8                            *FlashStatus,
  IN UINTN                            LengthInBytes
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_WRITE_DATA)(
  IN SPI_NOR                          *Nor,
  IN UINTN                            FlashAddress,
  IN UINTN                            LengthInBytes,
  IN UINT8                            *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_ERASE)(
  IN SPI_NOR                          *Nor,
  IN UINTN                            FlashAddress,
  IN UINTN                            Length
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_ERASE_CHIP)(
  IN SPI_NOR                          *Nor
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_GET_FLASH_VARIABLE_OFFSET)(
  IN SPI_NOR                          *Nor
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_INIT)(
  IN SOPHGO_NOR_FLASH_PROTOCOL       *This,
  IN SPI_NOR                         *Nor
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_SOFT_RESET)(
  IN SPI_NOR                          *Nor
  );

typedef
EFI_STATUS
(EFIAPI *SG_NOR_FLASH_PROTOCOL_SET_PROTECT_ALL)(
  IN SPI_NOR                          *Nor,
  IN BOOLEAN                           IsProtectAll
  );

struct _SOPHGO_NOR_FLASH_PROTOCOL {
  SG_NOR_FLASH_PROTOCOL_GET_FLASH_ID                GetFlashid;
  SG_NOR_FLASH_PROTOCOL_READ_DATA                   ReadData;
  SG_NOR_FLASH_PROTOCOL_READ_STATUS                 ReadStatus;
  SG_NOR_FLASH_PROTOCOL_WRITE_STATUS                WriteStatus;
  SG_NOR_FLASH_PROTOCOL_WRITE_DATA                  WriteData;
  SG_NOR_FLASH_PROTOCOL_ERASE                       Erase;
  SG_NOR_FLASH_PROTOCOL_ERASE_CHIP                  EraseChip;
  SG_NOR_FLASH_PROTOCOL_INIT                        Init;
  SG_NOR_FLASH_PROTOCOL_GET_FLASH_VARIABLE_OFFSET   GetFlashVariableOffset;
  SG_NOR_FLASH_PROTOCOL_SOFT_RESET                  SoftReset;
  SG_NOR_FLASH_PROTOCOL_SET_PROTECT_ALL             SetProtectAll;
};

typedef struct {
  UINTN                      Signature;
  EFI_HANDLE                 Handle;
  SOPHGO_NOR_FLASH_PROTOCOL  NorFlashProtocol;
} NOR_FLASH_INSTANCE;

#endif // __NOR_FLASH_PROTOCOL_H__
