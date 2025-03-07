/** @file
  Implements AMD PcRtc

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_FLASH_UPDATE_H_
#define SPI_FLASH_UPDATE_H_

#include <Uefi/UefiBaseType.h>

//
// Spi Flash Update Protocol GUID
// EDK and EDKII have different GUID formats
//
#if !defined (EDK_RELEASE_VERSION) || (EDK_RELEASE_VERSION < 0x00020000)
#define EFI_SPI_FLASH_UPDATE_PROTOCOL_GUID \
  { \
    0x9cf897ac, 0xc8cd, 0x4564, 0x8d, 0x8f, 0x1b, 0x88, 0xd4, 0xcf, 0xde, 0x22 \
  }
#define EFI_SMM_SPI_FLASH_UPDATE_PROTOCOL_GUID \
  { \
    0xc5922181, 0x7a76, 0x4777, 0x96, 0x85, 0x8a, 0xd3, 0x4e, 0xca, 0x0, 0x8c \
  }
#else
#define EFI_SPI_FLASH_UPDATE_PROTOCOL_GUID \
  { \
    0x9cf897ac, 0xc8cd, 0x4564, \
    { \
      0x8d, 0x8f, 0x1b, 0x88, 0xd4, 0xcf, 0xde, 0x22 \
    } \
  }
#define EFI_SMM_SPI_FLASH_UPDATE_PROTOCOL_GUID \
  { \
    0xc5922181, 0x7a76, 0x4777, \
    { \
      0x96, 0x85, 0x8a, 0xd3, 0x4e, 0xca, 0x0, 0x8c \
    } \
  }
#endif

//
// Extern the GUID for protocol users.
//
extern EFI_GUID  gEfiSpiFlashUpdateProtocolGuid;
extern EFI_GUID  gEfiSmmSpiFlashUpdateProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_SPI_FLASH_UPDATE_PROTOCOL EFI_SPI_FLASH_UPDATE_PROTOCOL;

//
// SMM SPI Flash Update protocol structure is the same as SPI Flash Update
// protocol. The SMM one is intend to run in SMM environment.
//
typedef EFI_SPI_FLASH_UPDATE_PROTOCOL EFI_SMM_SPI_FLASH_UPDATE_PROTOCOL;

//
// Protocol member functions
//

/**
  Read data from flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[out] Buffer                      Buffer contain the read data.

  @retval EFI_SUCCESS                     Read successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_FLASH_UPDATE_FD_READ)(
  IN  UINTN            FlashAddress,
  IN  UINTN            NumBytes,
  OUT VOID             *Buffer
  );

/**
  Erase flash region according to input in a block size.

  @param[in] FlashAddress                 Physical flash address.
  @param[in] NumBytes                     Number in Byte, a block size in flash device.

  @retval EFI_SUCCESS                     Erase successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_FLASH_UPDATE_FD_ERASE)(
  IN  UINTN       FlashAddress,
  IN  UINTN       NumBytes
  );

/**
  Write data to flash device.

  Write Buffer(FlashAddress|NumBytes) to flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[in]  Buffer                      Buffer contain the write data.

  @retval EFI_SUCCESS                     Write successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_FLASH_UPDATE_FD_Write)(
  IN  UINTN           FlashAddress,
  IN  UINTN           NumBytes,
  IN  UINT8           *Buffer
  );

/**
  Get flash device size and flash block size.

  @param[out] FlashSize                   Pointer to the size of flash device.
  @param[out] BlockSize                   Pointer to the size of block in flash device.

  @retval EFI_SUCCESS                     Get successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_FLASH_GET_FLASH_SIZE_BLOCK_SIZE)(
  OUT  UINTN            *FlashSize,
  OUT  UINTN            *BlockSize
  );

//
// Protocol definition
//
struct _EFI_SPI_FLASH_UPDATE_PROTOCOL {
  EFI_SPI_FLASH_UPDATE_FD_READ               Read;
  EFI_SPI_FLASH_UPDATE_FD_ERASE              Erase;
  EFI_SPI_FLASH_UPDATE_FD_Write              Write;
  EFI_SPI_FLASH_GET_FLASH_SIZE_BLOCK_SIZE    GetFlashSizeBlockSize;
};

#endif
