/** @file

  Headers for IPMI Blob Transfer driver

  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#define PROTOCOL_RESPONSE_OVERHEAD  (4 * sizeof(UINT8))     // 1 byte completion code + 3 bytes OEN

// Subcommands for this protocol
typedef enum {
  IpmiBlobTransferSubcommandGetCount = 0,
  IpmiBlobTransferSubcommandEnumerate,
  IpmiBlobTransferSubcommandOpen,
  IpmiBlobTransferSubcommandRead,
  IpmiBlobTransferSubcommandWrite,
  IpmiBlobTransferSubcommandCommit,
  IpmiBlobTransferSubcommandClose,
  IpmiBlobTransferSubcommandDelete,
  IpmiBlobTransferSubcommandStat,
  IpmiBlobTransferSubcommandSessionStat,
  IpmiBlobTransferSubcommandWriteMeta,
} IPMI_BLOB_TRANSFER_SUBCOMMANDS;

#pragma pack(1)

typedef struct {
  UINT8    OEN[3];
  UINT8    SubCommand;
} IPMI_BLOB_TRANSFER_HEADER;

//
// Command 0 - BmcBlobGetCount
// The BmcBlobGetCount command expects to receive an empty body.
// The BMC will return the number of enumerable blobs
//
typedef struct {
  UINT32    BlobCount;
} IPMI_BLOB_TRANSFER_GET_COUNT_RESPONSE;

//
// Command 1 - BmcBlobEnumerate
// The BmcBlobEnumerate command expects to receive a body of:
//
typedef struct {
  UINT32    BlobIndex; // 0-based index of blob to receive
} IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_SEND_DATA;

typedef struct {
  CHAR8    BlobId[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_RESPONSE;

//
// Command 2 - BmcBlobOpen
// The BmcBlobOpen command expects to receive a body of:
//
typedef struct {
  UINT16    Flags;
  CHAR8     BlobId[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_OPEN_SEND_DATA;

#define BLOB_OPEN_FLAG_READ   0
#define BLOB_OPEN_FLAG_WRITE  1
// Bits 2-7 are reserved
// Bits 8-15 are blob-specific definitions

typedef struct {
  UINT16    SessionId;
} IPMI_BLOB_TRANSFER_BLOB_OPEN_RESPONSE;

//
// Command 3 - BmcBlobRead
// The BmcBlobRead command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
  UINT32    Offset;
  UINT32    RequestedSize;
} IPMI_BLOB_TRANSFER_BLOB_READ_SEND_DATA;

typedef struct {
  UINT8    Data[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_READ_RESPONSE;

//
// Command 4 - BmcBlobWrite
// The BmcBlobWrite command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
  UINT32    Offset;
  UINT8     Data[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA;

//
// Command 5 - BmcBlobCommit
// The BmcBlobCommit command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
  UINT8     CommitDataLength;
  UINT8     CommitData[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_COMMIT_SEND_DATA;

//
// Command 6 - BmcBlobClose
// The BmcBlobClose command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
} IPMI_BLOB_TRANSFER_BLOB_CLOSE_SEND_DATA;

//
// Command 7 - BmcBlobDelete
// NOTE: This command will fail if there are open sessions for this blob
// The BmcBlobDelete command expects to receive a body of:
//
typedef struct {
  CHAR8    BlobId[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_DELETE_SEND_DATA;

//
// Command 8 - BmcBlobStat
// This command returns statistics about a blob.
// This command expects to receive a body of:
//
typedef struct {
  CHAR8    BlobId[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_STAT_SEND_DATA;

typedef struct {
  UINT16    BlobState;
  UINT32    Size; // Size in bytes of the blob
  UINT8     MetaDataLen;
  UINT8     MetaData[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE;

//
// Command 9 - BmcBlobSessionStat
// Returns same data as BmcBlobState expect for a session, not a blob
// This command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId;
} IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_SEND_DATA;

typedef struct {
  UINT16    BlobState;
  UINT32    Size; // Size in bytes of the blob
  UINT8     MetaDataLen;
  UINT8     MetaData[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE;

//
// Command 10 - BmcBlobWriteMeta
// The BmcBlobWriteMeta command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId;
  UINT32    Offset;
  UINT8     Data[IPMI_OEM_BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_WRITE_META_SEND_DATA;

#define IPMI_BLOB_TRANSFER_BLOB_WRITE_META_RESPONSE  NULL

#pragma pack()

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
  );

EFI_STATUS
IpmiBlobTransferSendIpmi (
  IN  UINT8   SubCommand,
  IN  UINT8   *SendData,
  IN  UINT32  SendDataSize,
  OUT UINT8   *ResponseData,
  OUT UINT32  *ResponseDataSize
  );

/**
  @param[out]        Count       The number of active blobs

  @retval EFI_SUCCESS            Successfully retrieved the number of active blobs.
  @retval Other                  An error occurred
**/
EFI_STATUS
IpmiBlobTransferGetCount (
  OUT UINT32  *Count
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  @param[in]         SessionId       The session ID returned from a call to BlobOpen

  @retval EFI_SUCCESS                The blob was closed.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferClose (
  IN  UINT16  SessionId
  );

/**
  @param[in]         BlobId          The BlobId to be deleted

  @retval EFI_SUCCESS                The blob was deleted.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferDelete (
  IN  CHAR8  *BlobId
  );

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
  );

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
  );

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
  );
