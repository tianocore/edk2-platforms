/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile Specification 1.0
    (https://developer.arm.com/documentation/den0118/latest)

  @par Glossary:
    - FW          - Firmware
    - FWU         - Firmware Update
    - FWS         - Firmware Storage
    - PSA         - Platform Security update for the A-profile specification
    - IFD         - Image File Data
    - IF          - Image File
    - IMG         - Image
    - MM          - Management Mode
    - PROP        - Property
    - Bkup, Bkp   - Backup

**/

#ifndef FWS_METADATA_H_
#define FWS_METADATA_H_

#include <IndustryStandard/PsaMmFwUpdate.h>
#include "DiskGpt.h"

#define FWU_NUMBER_OF_BANKS  FixedPcdGet32 (PcdFwuNumberOfBanks)

#define FWU_METADATA_PARTITION_NAME       L"FWU-Metadata"
#define FWU_BKUP_METADATA_PARTITION_NAME  L"Bkup-FWU-Metadata"

#define FWS_METADATA_FORMAT_V2  2

typedef struct FwsMetadata FWS_METADATA;

/** Enum value used in FWS_METADATA_SET_ACCEPT_STATE.
 */
typedef enum {
  FwsImageUnAcceptReq,       /// Set Image state as Unaccepted.
  FwsImageAcceptReq,         /// Set Image state as Accepted
  FwsImageWriteUnAcceptReq,  /// Set Image state as Unaccepted when starting write.
  FwsAcceptReqMax,
} FWS_ACCEPT_REQ;

/**
  Initialize and Load Firmware Update Store Metadata.

  @param [in]   GptHandle     GptHandle
  @param [out]  FwsMetadata   Firmware Update Store Metadata.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_ABORTED           Metadata is invalid.
  @retval Others                Failed to get metadata from gpt partition.

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_INIT_METADATA)(
  IN  GPT_PARTITION_HANDLE *GptHandle,
  OUT FWS_METADATA         **FwsMetadata
  );

/**
  Cleanup Loaded Firmware Update Store Metadata.

  @param [in]  FwsMetadata   Firmware Update Store Metadata.

**/
typedef VOID (EFIAPI *FWS_METADATA_EXIT_METADATA)(
  IN  FWS_METADATA         *FwsMetadata
  );

/**
  Get the number of banks.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  NumBanks      Number of banks.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_GET_NUM_BANKS)(
  IN FWS_METADATA         *FwsMetadata,
  OUT UINT8               *NumBanks
  );

/**
  Get the number of images per bank.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  NumImages     Number of images per bank.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_GET_NUM_IMAGES)(
  IN FWS_METADATA         *FwsMetadata,
  OUT UINT16              *NumImages
  );

/**
  Check trial state.

  @param [in]   FwsMetadata     Firmware Update Store Metadata.
  @param [out]  TrialRunState   If true, current is in the trial state.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_CHECK_TRIAL_RUN_STATE)(
  IN FWS_METADATA         *FwsMetadata,
  OUT BOOLEAN             *TrialRunState
  );

/**
  Get all image type guids.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [out]      ImageTypeGuids  Image type guid array.
  @param [in,out]   Size            Byte size of @ImageTypeGuids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_BUFFER_TOO_SMALL      Size of @ImageTypeGuids is too small to
                                    contain all image type guids manged by
                                    firmware update store.

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_GET_IMAGE_TYPE_GUIDS)(
  IN     FWS_METADATA         *FwsMetadata,
  OUT    EFI_GUID             *ImageTypeGuids,
  IN OUT UINT32               *Size
  );

/**
  Get location guid of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [out]      LocationGuid    Location Guids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_GET_LOCATION_GUID)(
  IN  FWS_METADATA          *FwsMetadata,
  IN  CONST EFI_GUID        *ImageTypeGuid,
  OUT EFI_GUID              *LocationGuid
  );

/**
  Get image guid of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index.
  @param [out]      ImageGuid       Image Guids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_GET_IMAGE_GUID)(
  IN  FWS_METADATA          *FwsMetadata,
  IN  CONST EFI_GUID        *ImageTypeGuid,
  IN  UINT32                BankIdx,
  OUT EFI_GUID              *ImageGuid
  );

/**
  Get accept state of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index
  @param [out]      AcceptState     If true, @ImageTypeGuid in the @BankIdx
                                    is accepted.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_GET_ACCEPT_STATE)(
  IN FWS_METADATA           *FwsMetadata,
  IN CONST EFI_GUID         *ImageTypeGuid,
  IN UINT32                 BankIdx,
  OUT BOOLEAN               *AcceptState
  );

/**
  Set accept state of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index
  @param [in]       AcceptReq       Type of request.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_SET_ACCEPT_STATE)(
  IN FWS_METADATA *FwsMetadata,
  IN CONST EFI_GUID     *ImageTypeGuid,
  IN UINT32        BankIdx,
  IN FWS_ACCEPT_REQ AcceptReq
  );

/**
  Update bank state.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       BankIdx         Bank Index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_UPDATE_BANK_STATE)(
  IN FWS_METADATA *FwsMetadata,
  IN UINT32       BankIdx
  );

/**
  Rollback metadata from @BackupIdx's one to @@TargetIdx's one

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       BackupIdx       Backup Bank Index
  @param [in]       TargetIdx       Target Bank Index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
typedef EFI_STATUS (EFIAPI *FWS_METADATA_ROLLBACK)(
  IN FWS_METADATA *FwsMetadata,
  IN UINT32       BackupIdx,
  IN UINT32       TargetIdx
  );

/** Firmware update storage metadata operation to handle metadata.
 */
typedef struct FwsMetadataOps {
  /// Init firmware update storage metadata.
  FWS_METADATA_INIT_METADATA            InitMetadata;

  /// Cleanup firmware update storage metadata.
  FWS_METADATA_EXIT_METADATA            ExitMetadata;

  /// Get Number of banks.
  FWS_METADATA_GET_NUM_BANKS            GetNumBanks;

  /// Get Number of images per bank.
  FWS_METADATA_GET_NUM_IMAGES           GetNumImages;

  /// Check whether trial state or not.
  FWS_METADATA_CHECK_TRIAL_RUN_STATE    CheckTrialRunState;

  /// Get all image type guids managed by firmware update storage.
  FWS_METADATA_GET_IMAGE_TYPE_GUIDS     GetImageTypeGuids;

  /// Get location guid.
  FWS_METADATA_GET_LOCATION_GUID        GetLocationGuid;

  /// Get image guid.
  FWS_METADATA_GET_IMAGE_GUID           GetImageGuid;

  /// Get accept state.
  FWS_METADATA_GET_ACCEPT_STATE         GetAcceptState;

  /// Set accept state.
  FWS_METADATA_SET_ACCEPT_STATE         SetAcceptState;

  /// Update bank state.
  FWS_METADATA_UPDATE_BANK_STATE        UpdateBankState;

  /// Rollback firmware update storage metadata.
  FWS_METADATA_ROLLBACK                 RollBack;
} FWS_METADATA_OPS;

typedef struct FwsMetadata {
  /// Gpt Handle
  GPT_PARTITION_HANDLE    *GptHandle;

  /// active Metadata Lba.
  EFI_LBA                 ActiveLba;

  /// Backup Metadata Lba.
  EFI_LBA                 BackupLba;

  /// Firmware Store Metadata
  VOID                    *Metadata;

  /// Firmware Store Metadata Size
  UINT32                  MetadataSize;

  /// Metadata operation
  FWS_METADATA_OPS        *FwsMetadataOps;
} FWS_METADATA;

/// Firmware update storage metadata version 2 operation.
extern FWS_METADATA_OPS  gFwsMetadataV2Ops;

/**
  Matching function used to find out Firmware Update Metadata GPT partition.

  @param [in]   PartitionEntry       GPT partition.
  @param [in]   Data                 Partition Name.

  @retval       TRUE                 Found.
  @retval       FALSE                Not found.

**/
BOOLEAN
MatchedMetaDataPartition (
  IN CONST EFI_PARTITION_ENTRY  *PartitionEntry,
  IN CONST VOID                 *Data
  );

/**
  Check CRC32 of the FWU metadata against the CRC32 value
  present in the FWU metadata.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.

  @retval EFI_SUCCESS             The CRC is valid.
  @retval EFI_CRC_ERROR           The CRC is invalid.
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataCrcCheck (
  IN FWS_METADATA  *FwsMetadata
  );

/**
  Save FWU metadata structure to GPT partition.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval Others                    Failed to write metadata to GPT partition.

**/
EFI_STATUS
EFIAPI
FwsMetadataSave (
  IN FWS_METADATA  *FwsMetadata
  );

/**
  Get active bank index.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  ActiveIndex   Active bank index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetActiveIndex (
  IN FWS_METADATA  *FwsMetadata,
  UINT32           *ActiveIndex
  );

/**
  Set active bank index.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [in]   ActiveIndex   Active bank index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataSetActiveIndex (
  IN FWS_METADATA  *FwsMetadata,
  UINT32           ActiveIndex
  );

/**
  Get previous active bank index.

  @param [in]   FwsMetadata           Firmware Update Store Metadata.
  @param [out]  PreviousActiveIndex   Previous Active bank index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetPreviousActiveIndex (
  IN FWS_METADATA  *FwsMetadata,
  UINT32           *PreviousActiveIndex
  );

/**
  Set previous active bank index.

  @param [in]   FwsMetadata           Firmware Update Store Metadata.
  @param [in]   PreviousActiveIndex   Previous Active bank index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataSetPreviousActiveIndex (
  IN FWS_METADATA  *FwsMetadata,
  UINT32           PreviousActiveIndex
  );

/**
  Initialize and Load Firmware Update Store Metadata.

  @param [in]   GptHandle     GptHandle
  @param [out]  FwsMetadata   Firmware Update Store Metadata.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_ABORTED           Metadata is invalid.
  @retval Others                Failed to get metadata from gpt partition.

**/
EFI_STATUS
EFIAPI
FwsMetadataInit (
  IN  GPT_PARTITION_HANDLE  *GptHandle,
  OUT FWS_METADATA          **FwsMetadata
  );

/**
  Cleanup Loaded Firmware Update Store Metadata.

  @param [in]  FwsMetadata   Firmware Update Store Metadata.

**/
VOID
EFIAPI
FwsMetadataExit (
  IN FWS_METADATA  *FwsMetadata
  );

/**
  Get the number of banks.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  NumBanks      Number of banks.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetNumBanks (
  IN FWS_METADATA  *FwsMetadata,
  OUT UINT8        *NumBanks
  );

/**
  Get the number of images per bank.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  NumImages     Number of images per bank.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetNumImages (
  IN FWS_METADATA  *FwsMetadata,
  OUT UINT16       *NumImages
  );

/**
  Check trial state.

  @param [in]   FwsMetadata     Firmware Update Store Metadata.
  @param [out]  TrialRunState   If true, current is in the trial state.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataCheckTrialRunState (
  IN FWS_METADATA  *FwsMetadata,
  OUT BOOLEAN      *TrialRunState
  );

/**
  Get all image type guids.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [out]      ImageTypeGuids  Image type guid array.
  @param [in,out]   Size            Byte size of @ImageTypeGuids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_BUFFER_TOO_SMALL      Size of @ImageTypeGuids is too small to
                                    contain all image type guids manged by
                                    firmware update store.

**/
EFI_STATUS
EFIAPI
FwsMetadataGetImageTypeGuids (
  IN     FWS_METADATA  *FwsMetadata,
  OUT    EFI_GUID      *ImageTypeGuids,
  IN OUT UINT32        *Size
  );

/**
  Get location guid of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [out]      LocationGuid    Location Guids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetLocationGuid (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  OUT EFI_GUID           *LocationGuid
  );

/**
  Get image guid of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index.
  @param [out]      ImageGuid       Image Guids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetImageGuid (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  OUT EFI_GUID           *ImageGuid
  );

/**
  Get accept state of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index
  @param [out]      AcceptState     If true, @ImageTypeGuid in the @BankIdx
                                    is accepted.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataGetAcceptState (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  OUT    BOOLEAN         *AcceptState
  );

/**
  Set accept state of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index
  @param [in]       AcceptReq       Type of request.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataSetAcceptState (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  IN     FWS_ACCEPT_REQ  AcceptReq
  );

/**
  Update bank state.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       BankIdx         Bank Index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataUpdateBankState (
  IN     FWS_METADATA  *FwsMetadata,
  IN     UINT32        BankIdx
  );

/**
  Rollback metadata from @BackupIdx's one to @@TargetIdx's one

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       BackupIdx       Backup Bank Index
  @param [in]       TargetIdx       Target Bank Index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
FwsMetadataRollBack (
  IN     FWS_METADATA  *FwsMetadata,
  IN     UINT32        BackupIdx,
  IN     UINT32        TargetIdx
  );

#endif
