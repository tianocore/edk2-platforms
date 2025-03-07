/*****************************************************************************
 *
 * Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 ******************************************************************************
 */

#include "FlashUpdateCommon.h"

EFI_EVENT                       mVirtualAddressChangeEvent = NULL;
UINTN                           *mFlashSize                = NULL;
UINTN                           *mBlockSize                = NULL;
EFI_SMM_COMMUNICATION_PROTOCOL  *mSmmCommunication         = NULL;
UINT8                           *mRweBuffer                = NULL;
UINTN                           mRweBufferSize;

EFI_SPI_FLASH_UPDATE_PROTOCOL  mEfiSpiFlashUpdateProtocol = {
  SfuProtocolFlashFdRead,
  SfuProtocolFlashFdErase,
  SfuProtocolFlashFdWrite,
  SfuProtocolGetFlashSizeBlockSize,
};

/**
  Initialize the communicate buffer.

  The communicate buffer is: SMM_COMMUNICATE_HEADER + SMM_COMM_RWE_FLASH + Payload.
  The communicate size is: SMM_COMMUNICATE_HEADER_SIZE + SMM_COMM_RWE_FLASH_SIZE + FlashSize.

  @param[out]      DataPtr          Points to the data in the communicate buffer.

**/
VOID
EFIAPI
InitCommunicateBuffer (
  OUT VOID  **DataPtr
  )
{
  EFI_SMM_COMMUNICATE_HEADER  *SmmCommunicateHeader;

  SmmCommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)mRweBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmSpiFlashUpdateProtocolGuid);
  SmmCommunicateHeader->MessageLength = SMM_COMM_RWE_FLASH_SIZE;

  *DataPtr = (SMM_COMM_RWE_FLASH *)((EFI_SMM_COMMUNICATE_HEADER *)mRweBuffer)->Data;
}

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
  )
{
  EFI_STATUS          Status;
  SMM_COMM_RWE_FLASH  *RweFlashBuffer;

  if ((FlashAddress >= *mFlashSize) || (NumBytes == 0) || (Buffer == NULL) ||
      (FlashAddress + NumBytes > *mFlashSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  RweFlashBuffer = NULL;

  InitCommunicateBuffer ((VOID **)&RweFlashBuffer);

  if (RweFlashBuffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  RweFlashBuffer->id           = SPI_SMM_COMM_ID_READ_FLASH;
  RweFlashBuffer->FlashAddress = FlashAddress;
  RweFlashBuffer->NumBytes     = NumBytes;
  CopyMem (RweFlashBuffer->Buffer, Buffer, NumBytes);

  //
  // Send data to SMM.
  //
  Status = mSmmCommunication->Communicate (
                                mSmmCommunication,
                                mRweBuffer,
                                &mRweBufferSize
                                );
  ASSERT_EFI_ERROR (Status);

  //
  // Get data from SMM
  //
  if (!EFI_ERROR (RweFlashBuffer->ReturnStatus)) {
    CopyMem (Buffer, RweFlashBuffer->Buffer, NumBytes);
  }

  return RweFlashBuffer->ReturnStatus;
}

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
  )
{
  EFI_STATUS          Status;
  SMM_COMM_RWE_FLASH  *RweFlashBuffer;

  if ((FlashAddress >= *mFlashSize) || (NumBytes == 0) || (NumBytes > *mFlashSize) ||
      (FlashAddress + NumBytes > *mFlashSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  RweFlashBuffer = NULL;

  InitCommunicateBuffer ((VOID **)&RweFlashBuffer);

  if (RweFlashBuffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  RweFlashBuffer->id           = SPI_SMM_COMM_ID_ERASE_FALSH;
  RweFlashBuffer->FlashAddress = FlashAddress;
  RweFlashBuffer->NumBytes     = NumBytes;

  //
  // Send data to SMM.
  //
  Status = mSmmCommunication->Communicate (
                                mSmmCommunication,
                                mRweBuffer,
                                &mRweBufferSize
                                );
  ASSERT_EFI_ERROR (Status);

  return RweFlashBuffer->ReturnStatus;
}

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
  )
{
  EFI_STATUS          Status;
  SMM_COMM_RWE_FLASH  *RweFlashBuffer;

  if ((FlashAddress >= *mFlashSize) || (NumBytes == 0) || (NumBytes > *mFlashSize) || (Buffer == NULL) ||
      (FlashAddress + NumBytes > *mFlashSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  RweFlashBuffer = NULL;

  InitCommunicateBuffer ((VOID **)&RweFlashBuffer);

  if (RweFlashBuffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  RweFlashBuffer->id           = SPI_SMM_COMM_ID_WRITE_FALSH;
  RweFlashBuffer->FlashAddress = FlashAddress;
  RweFlashBuffer->NumBytes     = NumBytes;
  CopyMem (RweFlashBuffer->Buffer, Buffer, NumBytes);

  //
  // Send data to SMM.
  //
  Status = mSmmCommunication->Communicate (
                                mSmmCommunication,
                                mRweBuffer,
                                &mRweBufferSize
                                );
  ASSERT_EFI_ERROR (Status);

  return RweFlashBuffer->ReturnStatus;
}

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
  )
{
  if ((FlashSize == NULL) || (BlockSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *FlashSize = *mFlashSize;
  *BlockSize = *mBlockSize;

  return EFI_SUCCESS;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
SpiFlashAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mRweBuffer);
  EfiConvertPointer (0x0, (VOID **)&mSmmCommunication);
  EfiConvertPointer (0x0, (VOID **)&mBlockSize);
  EfiConvertPointer (0x0, (VOID **)&mFlashSize);
}

/**
  Get flash device size and flash block size from SMM.

  @param[in] VOID

  @retval EFI_SUCCESS                     Get successfully.
  @retval others                          Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
GetFlashSizeBlockSize (
  VOID
  )
{
  EFI_STATUS                          Status;
  UINTN                               CommSize;
  UINT8                               *CommBuffer;
  EFI_SMM_COMMUNICATE_HEADER          *SmmCommunicateHeader;
  SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE  *SmmGetFlashSizeBlockSize;

  CommBuffer               = NULL;
  SmmCommunicateHeader     = NULL;
  SmmGetFlashSizeBlockSize = NULL;

  //
  // Init the communicate buffer. The buffer size is:
  // SMM_COMMUNICATE_HEADER_SIZE + sizeof (SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE)
  //
  CommSize   = SMM_COMMUNICATE_HEADER_SIZE + sizeof (SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE);
  CommBuffer = AllocateRuntimePool (CommSize);
  if (CommBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  SmmCommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)CommBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmSpiFlashUpdateProtocolGuid);
  SmmCommunicateHeader->MessageLength = sizeof (SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE);

  SmmGetFlashSizeBlockSize     = (SMM_COMM_GET_FLASH_SIZE_BLOCK_SIZE *)SmmCommunicateHeader->Data;
  SmmGetFlashSizeBlockSize->id = SPI_SMM_COMM_ID_GET_FLASH_SIZE_BLOCK_SIZE;

  //
  // Send data to SMM.
  //
  Status = mSmmCommunication->Communicate (mSmmCommunication, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);

  Status = SmmGetFlashSizeBlockSize->ReturnStatus;
  if (EFI_ERROR (Status)) {
    if (CommBuffer != NULL) {
      FreePool (CommBuffer);
    }

    return Status;
  }

  //
  // Get data from SMM.
  //
  *mFlashSize = SmmGetFlashSizeBlockSize->FlashSize;
  *mBlockSize = SmmGetFlashSizeBlockSize->BlockSize;

  if (CommBuffer != NULL) {
    FreePool (CommBuffer);
  }

  return Status;
}

/**
  Update Flash Driver main entry point.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Update Flash service successfully initialized.

**/
EFI_STATUS
EFIAPI
FlashUpdateSmmRuntimeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Status = gBS->LocateProtocol (
                  &gEfiSmmCommunicationProtocolGuid,
                  NULL,
                  (VOID **)&mSmmCommunication
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate memory for flash device size communicate buffer.
  //
  mFlashSize = AllocateRuntimePool (sizeof (UINTN));
  ASSERT (mFlashSize != NULL);

  //
  // Allocate memory for flash device block size communicate buffer.
  //
  mBlockSize = AllocateRuntimePool (sizeof (UINTN));
  ASSERT (mBlockSize != NULL);

  Status = GetFlashSizeBlockSize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate memory for update flash communicate buffer.
  //
  mRweBufferSize = SMM_COMMUNICATE_HEADER_SIZE + SMM_COMM_RWE_FLASH_SIZE + *mFlashSize;
  mRweBuffer     = AllocateRuntimePool (mRweBufferSize);
  ASSERT (mRweBuffer != NULL);

  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         SpiFlashAddressChangeEvent,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVirtualAddressChangeEvent
         );

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiSpiFlashUpdateProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mEfiSpiFlashUpdateProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
