/** @file
  Implements AMD FlashUpdateCommon.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLASH_UPDATE_COMMON_H_
#define FLASH_UPDATE_COMMON_H_

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/SpiFlashUpdate.h>
#include <Protocol/SmmCommunication.h>

#include <Uefi/UefiAcpiDataTable.h>
#include <Uefi/UefiSpec.h>

#include <Guid/EventGroup.h>

#define SPI_SMM_COMM_ID_GET_FLASH_SIZE_BLOCK_SIZE  0x0       // ID for get flash size and block size
#define SPI_SMM_COMM_ID_READ_FLASH                 0x1       // ID for Read Flash
#define SPI_SMM_COMM_ID_WRITE_FALSH                0x2       // ID for Write Flash
#define SPI_SMM_COMM_ID_ERASE_FALSH                0x3       // ID for Erase Flash

//
// SMM communication common buffer
//
typedef struct _FLASH_UPDATE_SMM_COMMUNICATION_CMN {
  UINT32    id;                     // Function ID of smm communication buffer
} FLASH_UPDATE_SMM_COMMUNICATION_CMN;

#pragma pack(1)

//
// SMM communication common buffer
//
typedef struct _SMM_COMM_RWE_FLASH {
  UINT32        id;                          // ID of smm communication buffer
  UINTN         FlashAddress;                // Flash devicd physical flash address
  UINTN         NumBytes;                    // Number in byte
  EFI_STATUS    ReturnStatus;                // Return status
  UINT8         Buffer[1];                   // Buffer start
} SMM_COMM_RWE_FLASH;

//
// SMM communication common buffer
//
typedef struct _SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE {
  UINT32        id;                           // ID of smm communication buffer
  UINTN         FlashSize;                    // Flash size
  UINTN         BlockSize;                    // Block size of flash device
  EFI_STATUS    ReturnStatus;                 // Return status
} SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE;

#pragma pack()

#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))
#define SMM_COMM_RWE_FLASH_SIZE      (OFFSET_OF (SMM_COMM_RWE_FLASH, Buffer))

/**
  Read data from flash device.

  @param[in]  FlashAddress                Physical flash address.
  @param[in]  NumBytes                    Number in Byte.
  @param[out] Buffer                      Buffer contain the read data.

  @retval EFI_SUCCESS                     Read successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
SfuProtocolFlashFdRead (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes,
  OUT VOID   *Buffer
  );

/**
  Erase flash region according to input in a block size.

  @param[in] FlashAddress                 Physical flash address.
  @param[in] NumBytes                     Number in Byte, a block size in flash device.

  @retval EFI_SUCCESS                     Erase successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
SfuProtocolFlashFdErase (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes
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
EFI_STATUS
EFIAPI
SfuProtocolFlashFdWrite (
  IN  UINTN  FlashAddress,
  IN  UINTN  NumBytes,
  IN  UINT8  *Buffer
  );

/**
  Get flash device size and flash block size.

  @param[out] FlashSize                   Pointer to the size of flash device.
  @param[out] BlockSize                   Pointer to the size of block in flash device.

  @retval EFI_SUCCESS                     Get successfully.
  @retval EFI_INVALID_PARAMETER           Invalid parameter.

**/
EFI_STATUS
EFIAPI
SfuProtocolGetFlashSizeBlockSize (
  OUT  UINTN  *FlashSize,
  OUT  UINTN  *BlockSize
  );

#endif // _FLASH_UPDATE_COMMON_H_
