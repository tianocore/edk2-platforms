/** @file
  Implements PlatformFlashAccessLib.c
  Platform Flash Access library.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformFlashAccessLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/Spi.h>
#include <Protocol/SpiCommon.h>
#include <Protocol/FirmwareManagement.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#define BLOCK_SIZE  0x1000

//
// Prefix Opcode Index on the host SPI controller
//
typedef enum {
  SPI_WREN,             // Prefix Opcode 0: Write Enable
  SPI_EWSR,             // Prefix Opcode 1: Enable Write Status Register
} PREFIX_OPCODE_INDEX;

STATIC EFI_PHYSICAL_ADDRESS  mInternalFdAddress;

EFI_SPI_PROTOCOL  *mSpiProtocol;

/**
  Read NumBytes bytes of data from the address specified by
  PAddress into Buffer.

  @param[in]      Address       The starting physical address of the read.
  @param[in,out]  NumBytes      On input, the number of bytes to read. On output, the number
                                of bytes actually read.
  @param[out]     Buffer        The destination data buffer for the read.

  @retval         EFI_SUCCESS       Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
SpiFlashRead (
  IN     UINTN   Address,
  IN OUT UINT32  *NumBytes,
  OUT UINT8      *Buffer
  )
{
  CopyMem (Buffer, (VOID *)Address, *NumBytes);
  return EFI_SUCCESS;
}

/**
  Write NumBytes bytes of data from Buffer to the address specified by
  PAddresss.

  @param[in]      Address         The starting physical address of the write.
  @param[in,out]  NumBytes        On input, the number of bytes to write. On output,
                                  the actual number of bytes written.
  @param[in]      Buffer          The source data buffer for the write.

  @retval         EFI_SUCCESS       Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
SpiFlashWrite (
  IN     UINTN   Address,
  IN OUT UINT32  *NumBytes,
  IN     UINT8   *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       Offset;
  UINT32      Length;
  UINT32      RemainingBytes;

  ASSERT ((NumBytes != NULL) && (Buffer != NULL));
  ASSERT (Address >= (UINTN)PcdGet32 (PcdFlashAreaBaseAddress));

  Offset = Address - (UINTN)PcdGet32 (PcdFlashAreaBaseAddress);
  ASSERT ((*NumBytes + Offset) <= (UINTN)(PcdGet32 (PcdFlashAreaSize)*2));
  Status         = EFI_SUCCESS;
  RemainingBytes = *NumBytes;

  while (RemainingBytes > 0) {
    if (RemainingBytes > SIZE_4KB) {
      Length = SIZE_4KB;
    } else {
      Length = RemainingBytes;
    }

    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_OPCODE_WRITE_INDEX,
                             SPI_WREN,
                             TRUE,
                             TRUE,
                             TRUE,
                             (UINT32)Offset,
                             Length,
                             Buffer,
                             EnumSpiRegionAll
                             );
    if (EFI_ERROR (Status)) {
      break;
    }

    RemainingBytes -= Length;
    Offset         += Length;
    Buffer         += Length;
  }

  //
  // Actual number of bytes written
  //
  *NumBytes -= RemainingBytes;

  return Status;
}

/**
  Read the block starting at Address.

  @param[in]      BaseAddress         The starting physical address of the block to be read.
  @param[in,out]  ReadBuffer          The pointer to a system memory buffer receiving the data read.

  @return         The status returned from SpiFlashRead().

**/
EFI_STATUS
InternalReadBlock (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  OUT VOID                  *ReadBuffer
  )
{
  EFI_STATUS  Status;
  UINT32      BlockSize;

  BlockSize = BLOCK_SIZE;

  Status = SpiFlashRead ((UINTN)BaseAddress, &BlockSize, ReadBuffer);

  return Status;
}

/**
  Erase the block starting at Address.

  @param[in]  Address         The starting physical address of the block to be erased.
                              This library assume that caller garantee that the PAddress
                              is at the starting address of this block.
  @param[in]  NumBytes        On input, the number of bytes of the logical block to be erased.
                              On output, the actual number of bytes erased.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
SpiFlashBlockErase (
  IN UINTN  Address,
  IN UINTN  *NumBytes
  )
{
  EFI_STATUS  Status;
  UINTN       Offset;
  UINTN       RemainingBytes;

  ASSERT (NumBytes != NULL);
  ASSERT (Address >= (UINTN)PcdGet32 (PcdFlashAreaBaseAddress));

  Offset = Address - (UINTN)PcdGet32 (PcdFlashAreaBaseAddress);

  ASSERT ((*NumBytes % SIZE_4KB) == 0);
  // -  ASSERT ((*NumBytes + Offset) <= (UINTN)PcdGet32 (PcdFlashAreaSize));
  ASSERT ((*NumBytes + Offset) <= (UINTN)(PcdGet32 (PcdFlashAreaSize)*2));
  Status         = EFI_SUCCESS;
  RemainingBytes = *NumBytes;

  while (RemainingBytes > 0) {
    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_OPCODE_ERASE_INDEX,
                             SPI_WREN,
                             FALSE,
                             TRUE,
                             TRUE,
                             (UINT32)Offset,
                             0,
                             NULL,
                             EnumSpiRegionAll
                             );
    if (EFI_ERROR (Status)) {
      break;
    }

    RemainingBytes -= SIZE_4KB;
    Offset         += SIZE_4KB;
  }

  //
  // Actual number of bytes erased
  //
  *NumBytes -= RemainingBytes;

  return Status;
}

/**
  Erase the whole block.

  @param[in]  BaseAddress    Base address of the block to be erased.

  @retval  EFI_SUCCESS   The command completed successfully.
  @retval  Other         Device error or wirte-locked, operation failed.

**/
EFI_STATUS
InternalEraseBlock (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  EFI_STATUS  Status;
  UINTN       NumBytes;

  NumBytes = BLOCK_SIZE;

  Status = SpiFlashBlockErase ((UINTN)BaseAddress, &NumBytes);

  return Status;
}

/**
  Compare the block value with buggfer.

  @param[in]  BaseAddress    Base address of the block to be compare.
  @param[in]  Buffer         The buffer to be compare.

  @retval  EFI_SUCCESS                  The command compare successfully.
  @retval  EFI_OUT_OF_RESOURCES         The resource has run out..

**/
EFI_STATUS
InternalCompareBlock (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT8                 *Buffer
  )
{
  EFI_STATUS  Status;
  VOID        *CompareBuffer;
  UINT32      NumBytes;
  INTN        CompareResult;

  NumBytes      = BLOCK_SIZE;
  CompareBuffer = AllocatePool (NumBytes);
  if (CompareBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = SpiFlashRead ((UINTN)BaseAddress, &NumBytes, CompareBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  CompareResult = CompareMem (CompareBuffer, Buffer, BLOCK_SIZE);
  if (CompareResult != 0) {
    Status = EFI_VOLUME_CORRUPTED;
  }

Done:
  if (CompareBuffer != NULL) {
    FreePool (CompareBuffer);
  }

  return Status;
}

/**
  Write a block of data.

  @param[in]  BaseAddress    Base address of the block.
  @param[in]  Buffer         Data buffer.
  @param[in]  BufferSize     Size of the buffer.

  @retval  EFI_SUCCESS             The command completed successfully.
  @retval  EFI_INVALID_PARAMETER   Invalid parameter, can not proceed.
  @retval  Other                   Device error or wirte-locked, operation failed.

**/
EFI_STATUS
InternalWriteBlock (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT8                 *Buffer,
  IN  UINT32                BufferSize
  )
{
  EFI_STATUS  Status;

  Status = SpiFlashWrite ((UINTN)BaseAddress, &BufferSize, Buffer);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "\nFlash write error."));
    return Status;
  }

  WriteBackInvalidateDataCacheRange ((VOID *)(UINTN)BaseAddress, BLOCK_SIZE);

  Status = InternalCompareBlock (BaseAddress, Buffer);
  Status = EFI_SUCCESS;

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "\nError when writing to BaseAddress %x with different at offset %x.\n", BaseAddress, Status));
  } else {
    DEBUG ((DEBUG_INFO, "\nVerified data written to Block at %x is correct.\n", BaseAddress));
  }

  return Status;
}

/**
  Perform flash write operation with progress indicator.  The start and end
  completion percentage values are passed into this function.  If the requested
  flash write operation is broken up, then completion percentage between the
  start and end values may be passed to the provided Progress function.  The
  caller of this function is required to call the Progress function for the
  start and end completion percentage values.  This allows the Progress,
  StartPercentage, and EndPercentage parameters to be ignored if the requested
  flash write operation can not be broken up

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.
  @param[in] Progress          A function used report the progress of the
                               firmware update.  This is an optional parameter
                               that may be NULL.
  @param[in] StartPercentage   The start completion percentage value that may
                               be used to report progress during the flash
                               write operation.
  @param[in] EndPercentage     The end completion percentage value that may
                               be used to report progress during the flash
                               write operation.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWriteWithProgress (
  IN PLATFORM_FIRMWARE_TYPE FirmwareType,
  IN EFI_PHYSICAL_ADDRESS FlashAddress,
  IN FLASH_ADDRESS_TYPE FlashAddressType,
  IN VOID *Buffer,
  IN UINTN Length,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS Progress, OPTIONAL
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 CountOfBlocks;
  EFI_TPL               OldTpl;
  BOOLEAN               FlashError;
  UINT8                 *Buf;

  Index         = 0;
  Address       = 0;
  CountOfBlocks = 0;
  FlashError    = FALSE;
  Buf           = Buffer;

  DEBUG ((DEBUG_INFO | DEBUG_ERROR, "PerformFlashWrite - 0x%x(%x) - 0x%x\n", (UINTN)FlashAddress, (UINTN)FlashAddressType, Length));
  if (FlashAddressType == FlashAddressTypeRelativeAddress) {
    FlashAddress = FlashAddress + mInternalFdAddress;
  }

  CountOfBlocks = (UINTN)(Length / BLOCK_SIZE);
  Address       = FlashAddress;

  //
  // Raise TPL to TPL_NOTIFY to block any event handler,
  // while still allowing RaiseTPL(TPL_NOTIFY) within
  // output driver during Print()
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  for (Index = 0; Index < CountOfBlocks; Index++) {
    if (Progress != NULL) {
      Progress (StartPercentage + ((Index * (EndPercentage - StartPercentage)) / CountOfBlocks));
    }

    //
    // Handle block based on address and contents.
    //
    if (!EFI_ERROR (InternalCompareBlock (Address, Buf))) {
      DEBUG ((DEBUG_INFO, "Skipping block at 0x%lx (already programmed)\n", Address));
    } else {
      //
      // Make updating process uninterruptable,
      // so that the flash memory area is not accessed by other entities
      // which may interfere with the updating process
      //
      Status = InternalEraseBlock (Address);
      if (EFI_ERROR (Status)) {
        gBS->RestoreTPL (OldTpl);
        FlashError = TRUE;
        goto Done;
      }

      Status = InternalWriteBlock (
                 Address,
                 Buf,
                 (UINT32)(Length > BLOCK_SIZE ? BLOCK_SIZE : Length)
                 );
      if (EFI_ERROR (Status)) {
        gBS->RestoreTPL (OldTpl);
        FlashError = TRUE;
        goto Done;
      }
    }

    //
    // Move to next block to update.
    //
    Address += BLOCK_SIZE;
    Buf     += BLOCK_SIZE;
    if (Length > BLOCK_SIZE) {
      Length -= BLOCK_SIZE;
    } else {
      Length = 0;
    }
  }

  gBS->RestoreTPL (OldTpl);

Done:

  if (Progress != NULL) {
    Progress (EndPercentage);
  }

  (VOID)FlashError;
  return Status;
}

/**
  Perform flash write operation.

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWrite (
  IN PLATFORM_FIRMWARE_TYPE  FirmwareType,
  IN EFI_PHYSICAL_ADDRESS    FlashAddress,
  IN FLASH_ADDRESS_TYPE      FlashAddressType,
  IN VOID                    *Buffer,
  IN UINTN                   Length
  )
{
  return PerformFlashWriteWithProgress (
           FirmwareType,
           FlashAddress,
           FlashAddressType,
           Buffer,
           Length,
           NULL,
           0,
           0
           );
}

/**
  Platform Flash Access Lib Constructor.
**/
EFI_STATUS
EFIAPI
PerformFlashAccessLibConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  mInternalFdAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdFlashAreaBaseAddress);
  DEBUG ((DEBUG_INFO, "PcdFlashAreaBaseAddress - 0x%x\n", mInternalFdAddress));

  Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                  (VOID **)&mSpiProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
