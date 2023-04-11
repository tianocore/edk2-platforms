/** @file

  IPMI Blob Transfer driver

  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Protocol/IpmiBlobTransfer.h>

#include "InternalIpmiBlobTransfer.h"

#define BLOB_TRANSFER_DEBUG  0

STATIC CONST IPMI_BLOB_TRANSFER_PROTOCOL  mIpmiBlobTransfer = {
  (IPMI_BLOB_TRANSFER_PROTOCOL_GET_COUNT)*IpmiBlobTransferGetCount,
  (IPMI_BLOB_TRANSFER_PROTOCOL_ENUMERATE)*IpmiBlobTransferEnumerate,
  (IPMI_BLOB_TRANSFER_PROTOCOL_OPEN)*IpmiBlobTransferOpen,
  (IPMI_BLOB_TRANSFER_PROTOCOL_READ)*IpmiBlobTransferRead,
  (IPMI_BLOB_TRANSFER_PROTOCOL_WRITE)*IpmiBlobTransferWrite,
  (IPMI_BLOB_TRANSFER_PROTOCOL_COMMIT)*IpmiBlobTransferCommit,
  (IPMI_BLOB_TRANSFER_PROTOCOL_CLOSE)*IpmiBlobTransferClose,
  (IPMI_BLOB_TRANSFER_PROTOCOL_DELETE)*IpmiBlobTransferDelete,
  (IPMI_BLOB_TRANSFER_PROTOCOL_STAT)*IpmiBlobTransferStat,
  (IPMI_BLOB_TRANSFER_PROTOCOL_SESSION_STAT)*IpmiBlobTransferSessionStat,
  (IPMI_BLOB_TRANSFER_PROTOCOL_WRITE_META)*IpmiBlobTransferWriteMeta
};

const UINT8  OpenBmcOen[] = { 0xCF, 0xC2, 0x00 };          // OpenBMC OEN code in little endian format

/**
  Calculate CRC-16-CCITT with poly of 0x1021

  @param[in]  Data              The target data.
  @param[in]  DataSize          The target data size.

  @return UINT16     The CRC16 value.

**/
UINT16
CalculateCrc16 (
  IN UINT8  *Data,
  IN UINTN  DataSize
  )
{
  UINTN    Index;
  UINTN    BitIndex;
  UINT16   Crc     = 0xFFFF;
  UINT16   Poly    = 0x1021;
  BOOLEAN  XorFlag = FALSE;

  for (Index = 0; Index < (DataSize + 2); ++Index) {
    for (BitIndex = 0; BitIndex < 8; ++BitIndex) {
      XorFlag = (Crc & 0x8000) ? TRUE : FALSE;
      Crc   <<= 1;
      if ((Index < DataSize) && (Data[Index] & (1 << (7 - BitIndex)))) {
        Crc++;
      }

      if (XorFlag == TRUE) {
        Crc ^= Poly;
      }
    }
  }

 #if BLOB_TRANSFER_DEBUG
  DEBUG ((DEBUG_INFO, "%a: CRC-16-CCITT %x\n", __FUNCTION__, Crc));
 #endif

  return Crc;
}

EFI_STATUS
IpmiBlobTransferSendIpmi (
  IN  UINT8   SubCommand,
  IN  UINT8   *SendData,
  IN  UINT32  SendDataSize,
  OUT UINT8   *ResponseData,
  OUT UINT32  *ResponseDataSize
  )
{
  EFI_STATUS                 Status;
  UINT8                      CompletionCode;
  UINT16                     Crc;
  UINT8                      Oen[3];
  UINT8                      *IpmiSendData;
  UINT32                     IpmiSendDataSize;
  UINT8                      *IpmiResponseData;
  UINT8                      *ModifiedResponseData;
  UINT32                     IpmiResponseDataSize;
  IPMI_BLOB_TRANSFER_HEADER  Header;

  Crc = 0;

  //
  // Prepend the proper header to the SendData
  //
  IpmiSendDataSize = (sizeof (IPMI_BLOB_TRANSFER_HEADER));
  if (SendDataSize) {
    IpmiSendDataSize += sizeof (Crc) + (sizeof (UINT8) * SendDataSize);
  }

  IpmiSendData = AllocateZeroPool (IpmiSendDataSize);
  if (IpmiSendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Header.OEN[0]     = OpenBmcOen[0];
  Header.OEN[1]     = OpenBmcOen[1];
  Header.OEN[2]     = OpenBmcOen[2];
  Header.SubCommand = SubCommand;
  CopyMem (IpmiSendData, &Header, sizeof (IPMI_BLOB_TRANSFER_HEADER));
  if (SendDataSize) {
    //
    // Calculate the Crc of the send data
    //
    Crc = CalculateCrc16 (SendData, SendDataSize);
    CopyMem (IpmiSendData + sizeof (IPMI_BLOB_TRANSFER_HEADER), &Crc, sizeof (UINT16));
    CopyMem (IpmiSendData + sizeof (IPMI_BLOB_TRANSFER_HEADER) + sizeof (UINT16), SendData, SendDataSize);
  }

 #if BLOB_TRANSFER_DEBUG
  DEBUG ((DEBUG_INFO, "%a: Inputs:\n", __FUNCTION__));
  DEBUG ((DEBUG_INFO, "%a: SendDataSize: %02x\nData: ", __FUNCTION__, SendDataSize));
  UINT8  i;
  for (i = 0; i < SendDataSize; i++) {
    DEBUG ((DEBUG_INFO, "%02x", *((UINT8 *)SendData + i)));
  }

  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "%a: IpmiSendDataSize: %02x\nData: ", __FUNCTION__, IpmiSendDataSize));
  for (i = 0; i < IpmiSendDataSize; i++) {
    DEBUG ((DEBUG_INFO, "%02x", *((UINT8 *)IpmiSendData + i)));
  }

  DEBUG ((DEBUG_INFO, "\n"));
 #endif

  IpmiResponseDataSize = (*ResponseDataSize + PROTOCOL_RESPONSE_OVERHEAD);
  //
  // If expecting data to be returned, we have to also account for the 16 bit CRC
  //
  if (*ResponseDataSize) {
    IpmiResponseDataSize += sizeof (Crc);
  }

  IpmiResponseData = AllocateZeroPool (IpmiResponseDataSize);
  if (IpmiResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = IpmiSubmitCommand (
             IPMI_NETFN_OEM,
             IPMI_OEM_BLOB_TRANSFER_CMD,
             (VOID *)IpmiSendData,
             IpmiSendDataSize,
             (VOID *)IpmiResponseData,
             &IpmiResponseDataSize
             );

  FreePool (IpmiSendData);
  ModifiedResponseData = IpmiResponseData;

 #if BLOB_TRANSFER_DEBUG
  DEBUG ((DEBUG_INFO, "%a: IPMI Response:\n", __FUNCTION__));
  DEBUG ((DEBUG_INFO, "%a: ResponseDataSize: %02x\nData: ", __FUNCTION__, IpmiResponseDataSize));
  for (i = 0; i < IpmiResponseDataSize; i++) {
    DEBUG ((DEBUG_INFO, "%02x", *(ModifiedResponseData + i)));
  }

  DEBUG ((DEBUG_INFO, "\n"));
 #endif

  if (EFI_ERROR (Status)) {
    return Status;
  }

  CompletionCode = *ModifiedResponseData;
  if (CompletionCode != IPMI_COMP_CODE_NORMAL) {
    DEBUG ((DEBUG_ERROR, "%a: Returning because CompletionCode = 0x%x\n", __FUNCTION__, CompletionCode));
    FreePool (IpmiResponseData);
    return EFI_PROTOCOL_ERROR;
  }

  // Strip completion code, we are done with it
  ModifiedResponseData  = ModifiedResponseData + sizeof (CompletionCode);
  IpmiResponseDataSize -= sizeof (CompletionCode);

  // Check OEN code and verify it matches the OpenBMC OEN
  CopyMem (Oen, ModifiedResponseData, sizeof (OpenBmcOen));
  if (CompareMem (Oen, OpenBmcOen, sizeof (OpenBmcOen)) != 0) {
    FreePool (IpmiResponseData);
    return EFI_PROTOCOL_ERROR;
  }

  if (IpmiResponseDataSize == sizeof (OpenBmcOen)) {
    //
    // In this case, there was no response data sent. This is not an error.
    // Some messages do not require a response.
    //
    *ResponseDataSize = 0;
    FreePool (IpmiResponseData);
    return Status;
    // Now we need to validate the CRC then send the Response body back
  } else {
    // Strip the OEN, we are done with it now
    ModifiedResponseData  = ModifiedResponseData + sizeof (Oen);
    IpmiResponseDataSize -= sizeof (Oen);
    // Then validate the Crc
    CopyMem (&Crc, ModifiedResponseData, sizeof (Crc));
    ModifiedResponseData  = ModifiedResponseData + sizeof (Crc);
    IpmiResponseDataSize -= sizeof (Crc);

    if (Crc == CalculateCrc16 (ModifiedResponseData, IpmiResponseDataSize)) {
      CopyMem (ResponseData, ModifiedResponseData, IpmiResponseDataSize);
      CopyMem (ResponseDataSize, &IpmiResponseDataSize, sizeof (IpmiResponseDataSize));
      FreePool (IpmiResponseData);
      return EFI_SUCCESS;
    } else {
      FreePool (IpmiResponseData);
      return EFI_CRC_ERROR;
    }
  }
}

/**
  @param[out]        Count       The number of active blobs

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_PROTOCOL_ERROR     The Ipmi command failed
  @retval EFI_CRC_ERROR          The Ipmi command returned a bad checksum
**/
EFI_STATUS
IpmiBlobTransferGetCount (
  OUT UINT32  *Count
  )
{
  EFI_STATUS  Status;
  UINT8       *ResponseData;
  UINT32      ResponseDataSize;

  ResponseDataSize = sizeof (IPMI_BLOB_TRANSFER_GET_COUNT_RESPONSE);
  ResponseData     = AllocateZeroPool (ResponseDataSize);
  if (ResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandGetCount, NULL, 0, (UINT8 *)ResponseData, &ResponseDataSize);
  if (!EFI_ERROR (Status)) {
    *Count = ((IPMI_BLOB_TRANSFER_GET_COUNT_RESPONSE *)ResponseData)->BlobCount;
  }

  FreePool (ResponseData);
  return Status;
}

/**
  @param[in]         BlobIndex       The 0-based Index of the blob to enumerate
  @param[out]        BlobId          The ID of the blob

  @retval EFI_SUCCESS                Successfully enumerated the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferEnumerate (
  IN  UINT32  BlobIndex,
  OUT CHAR8   *BlobId
  )
{
  EFI_STATUS  Status;

  UINT8   *SendData;
  UINT8   *ResponseData;
  UINT32  SendDataSize;
  UINT32  ResponseDataSize;

  if (BlobId == NULL) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  ResponseDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_RESPONSE);
  ResponseData     = AllocateZeroPool (ResponseDataSize);
  if (ResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_SEND_DATA *)SendData)->BlobIndex = BlobIndex;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandEnumerate, SendData, SendDataSize, (UINT8 *)ResponseData, &ResponseDataSize);
  if (!EFI_ERROR (Status)) {
    AsciiStrCpyS (BlobId, ResponseDataSize, (CHAR8 *)ResponseData);
  }

  FreePool (ResponseData);
  return Status;
}

/**
  @param[in]         BlobId          The ID of the blob to open
  @param[in]         Flags           Flags to control how the blob is opened
  @param[out]        SessionId       A unique session identifier

  @retval EFI_SUCCESS                Successfully opened the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferOpen (
  IN  CHAR8   *BlobId,
  IN  UINT16  Flags,
  OUT UINT16  *SessionId
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT8       *ResponseData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;
  CHAR8       *BlobSearch;
  UINT32      NumBlobs;
  UINT16      Index;
  BOOLEAN     BlobFound;

  //
  // Before opening a blob, need to check if it exists
  //
  Status = IpmiBlobTransferGetCount (&NumBlobs);
  if (EFI_ERROR (Status) || (NumBlobs == 0)) {
    if (Status == EFI_UNSUPPORTED) {
      return Status;
    }

    DEBUG ((DEBUG_ERROR, "%a: Could not find any blobs: %r\n", __FUNCTION__, Status));
    return EFI_NOT_FOUND;
  }

  BlobSearch = AllocateZeroPool (sizeof (CHAR8) * IPMI_OEM_BLOB_MAX_DATA_PER_PACKET);
  if (BlobSearch == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlobFound = FALSE;
  for (Index = 0; Index < NumBlobs; Index++) {
    Status = IpmiBlobTransferEnumerate (Index, BlobSearch);
    if ((!EFI_ERROR (Status)) && (AsciiStrCmp (BlobSearch, BlobId) == 0)) {
      BlobFound = TRUE;
      break;
    } else {
      continue;
    }
  }

  if (!BlobFound) {
    DEBUG ((DEBUG_ERROR, "%a: Could not find a blob that matches %s\n", __FUNCTION__, BlobId));
    FreePool (BlobSearch);
    return EFI_NOT_FOUND;
  }

  ResponseDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_OPEN_RESPONSE);
  ResponseData     = AllocateZeroPool (ResponseDataSize);
  if (ResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Format send data
  //
  SendDataSize = sizeof (((IPMI_BLOB_TRANSFER_BLOB_OPEN_SEND_DATA *)SendData)->Flags) + ((AsciiStrLen (BlobId)) * sizeof (CHAR8)) + sizeof (CHAR8);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (((IPMI_BLOB_TRANSFER_BLOB_OPEN_SEND_DATA *)SendData)->BlobId, AsciiStrSize (BlobId) / sizeof (CHAR8), BlobId);
  ((IPMI_BLOB_TRANSFER_BLOB_OPEN_SEND_DATA *)SendData)->Flags = Flags;
  // append null char to SendData
  SendData[SendDataSize-1] = 0;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandOpen, SendData, SendDataSize, (UINT8 *)ResponseData, &ResponseDataSize);
  if (!EFI_ERROR (Status)) {
    *SessionId = ((IPMI_BLOB_TRANSFER_BLOB_OPEN_RESPONSE *)ResponseData)->SessionId;
  }

  FreePool (ResponseData);
  FreePool (SendData);
  FreePool (BlobSearch);
  return Status;
}

/**
  @param[in]         SessionId       The session ID returned from a call to BlobOpen
  @param[in]         Offset          The offset of the blob from which to start reading
  @param[in]         RequestedSize   The length of data to read
  @param[out]        Data            Data read from the blob

  @retval EFI_SUCCESS                Successfully read from the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferRead (
  IN  UINT16  SessionId,
  IN  UINT32  Offset,
  IN  UINT32  RequestedSize,
  OUT UINT8   *Data
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT8       *ResponseData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  if (Data == NULL) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  ResponseDataSize = RequestedSize * sizeof (UINT8);
  ResponseData     = AllocateZeroPool (ResponseDataSize);
  if (ResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_READ_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_READ_SEND_DATA *)SendData)->SessionId     = SessionId;
  ((IPMI_BLOB_TRANSFER_BLOB_READ_SEND_DATA *)SendData)->Offset        = Offset;
  ((IPMI_BLOB_TRANSFER_BLOB_READ_SEND_DATA *)SendData)->RequestedSize = RequestedSize;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandRead, SendData, SendDataSize, (UINT8 *)ResponseData, &ResponseDataSize);
  if (!EFI_ERROR (Status)) {
    CopyMem (Data, ((IPMI_BLOB_TRANSFER_BLOB_READ_RESPONSE *)ResponseData)->Data, ResponseDataSize * sizeof (UINT8));
  }

  FreePool (ResponseData);
  FreePool (SendData);
  return Status;
}

/**
  @param[in]         SessionId       The session ID returned from a call to BlobOpen
  @param[in]         Offset          The offset of the blob from which to start writing
  @param[in]         Data            A pointer to the data to write

  @retval EFI_SUCCESS                Successfully wrote to the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferWrite (
  IN  UINT16  SessionId,
  IN  UINT32  Offset,
  IN  UINT8   *Data,
  IN  UINT32  WriteLength
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  //
  // Format send data
  //
  SendDataSize = sizeof (SessionId) + sizeof (Offset) + WriteLength;
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA *)SendData)->SessionId = SessionId;
  ((IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA *)SendData)->Offset    = Offset;
  CopyMem (((IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA *)SendData)->Data, Data, sizeof (UINT8) * WriteLength);

  ResponseDataSize = 0;
  Status           = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandWrite, SendData, SendDataSize, NULL, &ResponseDataSize);

  FreePool (SendData);
  return Status;
}

/**
  @param[in]         SessionId        The session ID returned from a call to BlobOpen
  @param[in]         CommitDataLength The length of data to commit to the blob
  @param[in]         CommitData       A pointer to the data to commit

  @retval EFI_SUCCESS                Successful commit to the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferCommit (
  IN  UINT16  SessionId,
  IN  UINT8   CommitDataLength,
  IN  UINT8   *CommitData
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  //
  // Format send data
  //
  SendDataSize = sizeof (SessionId) + sizeof (CommitDataLength) + CommitDataLength;
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_COMMIT_SEND_DATA *)SendData)->SessionId        = SessionId;
  ((IPMI_BLOB_TRANSFER_BLOB_COMMIT_SEND_DATA *)SendData)->CommitDataLength = CommitDataLength;
  CopyMem (((IPMI_BLOB_TRANSFER_BLOB_COMMIT_SEND_DATA *)SendData)->CommitData, CommitData, sizeof (UINT8) * CommitDataLength);

  ResponseDataSize = 0;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandCommit, SendData, SendDataSize, NULL, &ResponseDataSize);

  FreePool (SendData);
  return Status;
}

/**
  @param[in]         SessionId       The session ID returned from a call to BlobOpen

  @retval EFI_SUCCESS                The blob was closed.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferClose (
  IN  UINT16  SessionId
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_CLOSE_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_CLOSE_SEND_DATA *)SendData)->SessionId = SessionId;

  ResponseDataSize = 0;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandClose, SendData, SendDataSize, NULL, &ResponseDataSize);

  FreePool (SendData);
  return Status;
}

/**
  @param[in]         BlobId          The BlobId to be deleted

  @retval EFI_SUCCESS                The blob was deleted.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferDelete (
  IN  CHAR8  *BlobId
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_DELETE_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (((IPMI_BLOB_TRANSFER_BLOB_DELETE_SEND_DATA *)SendData)->BlobId, AsciiStrLen (BlobId), BlobId);

  ResponseDataSize = 0;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandDelete, SendData, SendDataSize, NULL, &ResponseDataSize);

  FreePool (SendData);
  return Status;
}

/**
  @param[in]         BlobId          The Blob ID to gather statistics for
  @param[out]        BlobState       The current state of the blob
  @param[out]        Size            Size in bytes of the blob
  @param[out]        MetadataLength  Length of the optional metadata
  @param[out]        Metadata        Optional blob-specific metadata

  @retval EFI_SUCCESS                The blob statistics were successfully gathered.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferStat (
  IN  CHAR8   *BlobId,
  OUT UINT16  *BlobState,
  OUT UINT32  *Size,
  OUT UINT8   *MetadataLength,
  OUT UINT8   *Metadata
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT8       *ResponseData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  if (Metadata == NULL) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  ResponseDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE);
  ResponseData     = AllocateZeroPool (ResponseDataSize);
  if (ResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_STAT_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (((IPMI_BLOB_TRANSFER_BLOB_STAT_SEND_DATA *)SendData)->BlobId, IPMI_OEM_BLOB_MAX_DATA_PER_PACKET, BlobId);

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandStat, SendData, SendDataSize, (UINT8 *)ResponseData, &ResponseDataSize);
  if (!EFI_ERROR (Status)) {
    *BlobState      = ((IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE *)ResponseData)->BlobState;
    *Size           = ((IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE *)ResponseData)->Size;
    *MetadataLength = ((IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE *)ResponseData)->MetaDataLen;

    CopyMem (&Metadata, &((IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE *)ResponseData)->MetaData, sizeof (((IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE *)ResponseData)->MetaData));
  }

  FreePool (ResponseData);
  FreePool (SendData);
  return Status;
}

/**
  @param[in]         SessionId       The ID of the session to gather statistics for
  @param[out]        BlobState       The current state of the blob
  @param[out]        Size            Size in bytes of the blob
  @param[out]        MetadataLength  Length of the optional metadata
  @param[out]        Metadata        Optional blob-specific metadata

  @retval EFI_SUCCESS                The blob statistics were successfully gathered.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferSessionStat (
  IN  UINT16  SessionId,
  OUT UINT16  *BlobState,
  OUT UINT32  *Size,
  OUT UINT8   *MetadataLength,
  OUT UINT8   *Metadata
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT8       *ResponseData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  if ((BlobState == NULL) || (Size == NULL) || (MetadataLength == NULL) || (Metadata == NULL)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  ResponseDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE);
  ResponseData     = AllocateZeroPool (ResponseDataSize);
  if (ResponseData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_SEND_DATA *)SendData)->SessionId = SessionId;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandSessionStat, SendData, SendDataSize, (UINT8 *)ResponseData, &ResponseDataSize);

  if (!EFI_ERROR (Status)) {
    *BlobState      = ((IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE *)ResponseData)->BlobState;
    *Size           = ((IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE *)ResponseData)->Size;
    *MetadataLength = ((IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE *)ResponseData)->MetaDataLen;

    CopyMem (&Metadata, &((IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE *)ResponseData)->MetaData, sizeof (((IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE *)ResponseData)->MetaData));
  }

  FreePool (ResponseData);
  FreePool (SendData);
  return Status;
}

/**
  @param[in]         SessionId       The ID of the session to write metadata for
  @param[in]         Offset          The offset of the metadata to write to
  @param[in]         Data            The data to write to the metadata

  @retval EFI_SUCCESS                The blob metadata was successfully written.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferWriteMeta (
  IN  UINT16  SessionId,
  IN  UINT32  Offset,
  IN  UINT8   *Data,
  IN  UINT32  WriteLength
  )
{
  EFI_STATUS  Status;
  UINT8       *SendData;
  UINT32      SendDataSize;
  UINT32      ResponseDataSize;

  //
  // Format send data
  //
  SendDataSize = sizeof (IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA);
  SendData     = AllocateZeroPool (SendDataSize);
  if (SendData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA *)SendData)->SessionId = SessionId;
  ((IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA *)SendData)->Offset    = Offset;
  CopyMem (((IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA *)SendData)->Data, Data, sizeof (UINT8) * WriteLength);

  ResponseDataSize = 0;

  Status = IpmiBlobTransferSendIpmi (IpmiBlobTransferSubcommandWriteMeta, SendData, SendDataSize, NULL, &ResponseDataSize);

  FreePool (SendData);
  return Status;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.

**/
EFI_STATUS
EFIAPI
IpmiBlobTransferDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gEdkiiIpmiBlobTransferProtocolGuid,
                (VOID *)&mIpmiBlobTransfer,
                NULL
                );
}
