/** @file

  IPMI Blob Transfer driver

  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/IpmiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/Ipmi.h>

#define IPMI_NETFN_OEM                     0x2E
#define IPMI_OEM_BLOB_TRANSFER_CMD         0x80
#define IPMI_OEM_BLOB_MAX_DATA_PER_PACKET  64

#define BLOB_TRANSFER_STAT_OPEN_R        BIT0
#define BLOB_TRANSFER_STAT_OPEN_W        BIT1
#define BLOB_TRANSFER_STAT_COMMITING     BIT2
#define BLOB_TRANSFER_STAT_COMMITTED     BIT3
#define BLOB_TRANSFER_STAT_COMMIT_ERROR  BIT4
// Bits 5-7 are reserved
// Bits 8-15 are blob-specific definitions

//
//  Blob Transfer Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_GET_COUNT)(
  OUT UINT32 *Count
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_ENUMERATE)(
  IN  UINT32      BlobIndex,
  OUT CHAR8 *BlobId
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_OPEN)(
  IN  CHAR8 *BlobId,
  IN  UINT16      Flags,
  OUT UINT16 *SessionId
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_READ)(
  IN  UINT16      SessionId,
  IN  UINT32      Offset,
  IN  UINT32      RequestedSize,
  OUT UINT8 *Data
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_WRITE)(
  IN  UINT16      SessionId,
  IN  UINT32      Offset,
  IN  UINT8 *Data,
  IN  UINT32      WriteLength
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_COMMIT)(
  IN  UINT16      SessionId,
  IN  UINT8       CommitDataLength,
  IN  UINT8 *CommitData
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_CLOSE)(
  IN  UINT16      SessionId
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_DELETE)(
  IN  CHAR8 *BlobId
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_STAT)(
  IN  CHAR8 *BlobId,
  OUT UINT16 *BlobState,
  OUT UINT32 *Size,
  OUT UINT8 *MetadataLength,
  OUT UINT8 *Metadata
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_SESSION_STAT)(
  IN  UINT16      SessionId,
  OUT UINT16 *BlobState,
  OUT UINT32 *Size,
  OUT UINT8 *MetadataLength,
  OUT UINT8 *Metadata
  );

typedef
EFI_STATUS
(EFIAPI *IPMI_BLOB_TRANSFER_PROTOCOL_WRITE_META)(
  IN  UINT16      SessionId,
  IN  UINT32      Offset,
  IN  UINT8 *Data,
  IN  UINT32      WriteLength
  );

//
// Structure of IPMI_BLOB_TRANSFER_PROTOCOL
//
struct _IPMI_BLOB_TRANSFER_PROTOCOL {
  IPMI_BLOB_TRANSFER_PROTOCOL_GET_COUNT       BlobGetCount;
  IPMI_BLOB_TRANSFER_PROTOCOL_ENUMERATE       BlobEnumerate;
  IPMI_BLOB_TRANSFER_PROTOCOL_OPEN            BlobOpen;
  IPMI_BLOB_TRANSFER_PROTOCOL_READ            BlobRead;
  IPMI_BLOB_TRANSFER_PROTOCOL_WRITE           BlobWrite;
  IPMI_BLOB_TRANSFER_PROTOCOL_COMMIT          BlobCommit;
  IPMI_BLOB_TRANSFER_PROTOCOL_CLOSE           BlobClose;
  IPMI_BLOB_TRANSFER_PROTOCOL_DELETE          BlobDelete;
  IPMI_BLOB_TRANSFER_PROTOCOL_STAT            BlobStat;
  IPMI_BLOB_TRANSFER_PROTOCOL_SESSION_STAT    BlobSessionStat;
  IPMI_BLOB_TRANSFER_PROTOCOL_WRITE_META      BlobWriteMeta;
};

typedef struct _IPMI_BLOB_TRANSFER_PROTOCOL IPMI_BLOB_TRANSFER_PROTOCOL;

extern EFI_GUID  gEdkiiIpmiBlobTransferProtocolGuid;
