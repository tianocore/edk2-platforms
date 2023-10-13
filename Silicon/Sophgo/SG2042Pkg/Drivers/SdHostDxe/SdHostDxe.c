/** @file
  This file implements the SD host controller driver for UEFI systems.
  The file contains the implementation of the EFI_MMC_HOST_PROTOCOL, which provides
  the necessary interfaces for handling communication and data transfer with SD cards.

  Copyright (c) 2017, Andrei Warkentin <andrey.warkentin@gmail.com>
  Copyright (c) Microsoft Corporation. All rights reserved.
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DmaLib.h>
#include <Library/TimerLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Include/MmcHost.h>

#include "SdHci.h"

#define SDHOST_BLOCK_BYTE_LENGTH  512

#define DEBUG_MMCHOST_SD          DEBUG_VERBOSE
#define DEBUG_MMCHOST_SD_INFO     DEBUG_INFO
#define DEBUG_MMCHOST_SD_ERROR    DEBUG_ERROR

STATIC BOOLEAN            mCardIsPresent   = FALSE;
STATIC CARD_DETECT_STATE  mCardDetectState = CardDetectRequired;
BM_SD_PARAMS              BmParams;

/**
  Check if the SD card is read-only.

  @param[in] This      Pointer to the EFI_MMC_HOST_PROTOCOL instance.

  @retval FALSE        The SD card is not read-only.

**/
STATIC
BOOLEAN
SdIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL *This
  )
{
  return FALSE;
}

/**
  Build the device path for the SD card.

  @param[in]  This           Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[out] DevicePath     Pointer to the location to store the newly created device path.

  @retval EFI_SUCCESS        The device path is built successfully.

**/
STATIC
EFI_STATUS
SdBuildDevicePath (
  IN  EFI_MMC_HOST_PROTOCOL       *This,
  OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL *NewDevicePathNode;
  EFI_GUID DevicePathGuid = EFI_CALLER_ID_GUID;

  DEBUG ((DEBUG_MMCHOST_SD, "SdHost: SdBuildDevicePath ()\n"));

  NewDevicePathNode = CreateDeviceNode (HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&((VENDOR_DEVICE_PATH*)NewDevicePathNode)->Guid, &DevicePathGuid);
  *DevicePath = NewDevicePathNode;

  return EFI_SUCCESS;
}

/**
  Send a command to the SD card.

  @param[in] This       Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[in] MmcCmd     The MMC command to send.
  @param[in] Argument   The argument for the command.
  @param[in] Type       The type of response expected.
  @param[in] Buffer     Pointer to the buffer to store the response.

  @retval EFI_SUCCESS   The command was sent successfully and the response was retrieved.
  @retval Other         An error occurred while sending a command.
**/
STATIC
EFI_STATUS
SdSendCommand (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN MMC_IDX                  MmcCmd,
  IN UINT32                   Argument,
  IN MMC_RESPONSE_TYPE        Type,
  IN UINT32*                  Buffer
  )
{
  EFI_STATUS Status;

  Status = BmSdSendCmd (MmcCmd, Argument, Type, Buffer);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdSendCommand Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Read block data from an SD card.

  @param[in] This       Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[in] Lba        Logical Block Address of the starting block to read.
  @param[in] Length     Number of blocks to read.
  @param[in] Buffer     Pointer to the buffer to store the read data.

  @retval EFI_SUCCESS   The operation completed successfully.
  @retval Other         The operation failed.

**/
STATIC
EFI_STATUS
SdReadBlockData (
  IN  EFI_MMC_HOST_PROTOCOL    *This,
  IN  EFI_LBA                  Lba,
  IN  UINTN                    Length,
  OUT UINT32*                  Buffer
  )
{
  EFI_STATUS Status;

  ASSERT (Buffer != NULL);
  ASSERT (Length % 4 == 0);

  Status = BmSdRead (Lba, Buffer, Length);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdReadBlockData Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Write block data to an SD card.

  @param[in] This       Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[in] Lba        Logical Block Address of the starting block to write.
  @param[in] Length     Number of blocks to write.
  @param[in] Buffer     Pointer to the buffer containing the data to be written.

  @retval EFI_SUCCESS   The operation completed successfully.
  @retval Other         The operation failed.

**/
STATIC
EFI_STATUS
SdWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN EFI_LBA                  Lba,
  IN UINTN                    Length,
  IN UINT32*                  Buffer
  )
{
  EFI_STATUS Status;

  ASSERT (Buffer != NULL);
  ASSERT (Length % SDHOST_BLOCK_BYTE_LENGTH == 0);

  Status = BmSdWrite (Lba, Buffer, Length);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdWriteBlockData Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Set the I/O settings for an SD card.

  @param[in]  This           Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[in]  BusClockFreq   Bus clock frequency in Hz.
  @param[in]  BusWidth       Bus width setting.

  @retval EFI_SUCCESS        The operation completed successfully.
  @retval Other              The operation failed.

**/
STATIC
EFI_STATUS
SdSetIos (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN  UINT32                    BusClockFreq,
  IN  UINT32                    BusWidth
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_MMCHOST_SD_INFO, "%a: Setting Freq %u Hz\n", __func__, BusClockFreq));
  DEBUG ((DEBUG_MMCHOST_SD_INFO, "%a: Setting BusWidth %u\n", __func__, BusWidth));

  Status = BmSdSetIos (BusClockFreq,BusWidth);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdSetIos Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Prepare the SD card for data transfer.

  @param[in]  This       Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[in]  Lba        Logical Block Address of the starting block to prepare.
  @param[in]  Length     Number of blocks to prepare.
  @param[in]  Buffer     Buffer containing the data to be prepared.

  @retval EFI_SUCCESS    The operation completed successfully.
  @retval Other          The operation failed.

**/
STATIC
EFI_STATUS
SdPrepare (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN EFI_LBA                  Lba,
  IN UINTN                    Length,
  IN UINTN                    Buffer
  )
{
  EFI_STATUS Status;

  Status = BmSdPrepare (Lba, Buffer, Length);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdPrepare Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Notify the state of the SD card.

  @param[in]  This    Pointer to the EFI_MMC_HOST_PROTOCOL instance.
  @param[in]  State   State of the SD card.

  @retval EFI_SUCCESS       The operation completed successfully.
  @retval EFI_NOT_READY     The card detection has not completed yet.
  @retval Other             The operation failed.

**/
STATIC
EFI_STATUS
SdNotifyState (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN MMC_STATE                State
  )
{
  // Stall all operations except init until card detection has occurred.
  if (State != MmcHwInitializationState && mCardDetectState != CardDetectCompleted) {
    return EFI_NOT_READY;
  }

  switch (State) {
    case MmcHwInitializationState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcHwInitializationState\n", State));

      EFI_STATUS Status = SdInit (SD_USE_PIO);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_MMCHOST_SD_ERROR,"SdHost: SdNotifyState(): Fail to initialize!\n"));
        return Status;
      }
      break;
    case MmcIdleState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcIdleState\n", State));
      break;
    case MmcReadyState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcReadyState\n", State));
      break;
    case MmcIdentificationState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcIdentificationState\n", State));
      break;
    case MmcStandByState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcStandByState\n", State));
      break;
    case MmcTransferState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcTransferState\n", State));
      break;
    case MmcSendingDataState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcSendingDataState\n", State));
      break;
    case MmcReceiveDataState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcReceiveDataState\n", State));
      break;
    case MmcProgrammingState:
      DEBUG ((DEBUG_MMCHOST_SD, "MmcProgrammingState\n", State));
      break;
    case MmcDisconnectState:
    case MmcInvalidState:
    default:
      DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdHost: SdNotifyState(): Invalid State: %d\n", State));
      ASSERT (0);
  }

  return EFI_SUCCESS;
}

/**
  Check if an SD card is present.

  @param[in]  This    Pointer to the EFI_MMC_HOST_PROTOCOL instance.

  @retval TRUE        An SD card is present.
  @retval FALSE       No SD card is present.

**/
STATIC
BOOLEAN
SdIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL *This
  )
{
  //
  // If we are already in progress (we may get concurrent calls)
  // or completed the detection, just return the current value.
  //
  if (mCardDetectState != CardDetectRequired) {
    return mCardIsPresent;
  }

  mCardDetectState = CardDetectInProgress;
  mCardIsPresent = FALSE;

  if (BmSdCardDetect () == 1) {
    mCardIsPresent = TRUE;
    goto out;
  }
  else {
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdIsCardPresent: Error SdCardDetect.\n"));
    mCardDetectState = CardDetectRequired;
    return FALSE;
  }

  DEBUG ((DEBUG_MMCHOST_SD_INFO, "SdIsCardPresent: Not detected.\n"));

out:
  mCardDetectState = CardDetectCompleted;
  return mCardIsPresent;
}

/**
  Check if the SD card supports multi-block transfers.

  @param[in]  This     Pointer to the EFI_MMC_HOST_PROTOCOL instance.

  @retval TRUE         The SD card supports multi-block transfers.

**/
BOOLEAN
SdIsMultiBlock (
  IN EFI_MMC_HOST_PROTOCOL *This
  )
{
  return TRUE;
}

EFI_MMC_HOST_PROTOCOL gMmcHost = {
  MMC_HOST_PROTOCOL_REVISION,
  SdIsCardPresent,
  SdIsReadOnly,
  SdBuildDevicePath,
  SdNotifyState,
  SdSendCommand,
  SdReadBlockData,
  SdWriteBlockData,
  SdSetIos,
  SdPrepare,
  SdIsMultiBlock
};

/**
  Initialize the SD host.

  @param[in]  ImageHandle    The image handle.
  @param[in]  SystemTable    The system table.

  @retval EFI_SUCCESS        The operation completed successfully.
  @retval Other              The operation failed.
**/
EFI_STATUS
SdHostInitialize (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  UINTN       Base;

  DEBUG ((DEBUG_MMCHOST_SD, "SdHost: Initialize\n"));

  Handle            = NULL;
  Base              = SDIO_BASE;

  if(PcdGet32 (PcdCpuRiscVMmuMaxSatpMode) > 0UL){
    for (INT32 I = 39; I < 64; I++) {
      if (Base & (1ULL << 38)) {
        Base |= (1ULL << I);
      } else {
        Base &= ~(1ULL << I);
      }
    }
  }

  BmParams.RegBase  = Base;
  BmParams.ClkRate  = 50 * 1000 * 1000;
  BmParams.BusWidth = MMC_BUS_WIDTH_4;
  BmParams.Flags    = 0;
  BmParams.CardIn   = SDCARD_STATUS_UNKNOWN;

  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gSophgoMmcHostProtocolGuid,
    &gMmcHost,
    NULL
  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
