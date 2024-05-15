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
#include <PiMm.h>

#include <IndustryStandard/PsaMmFwUpdate.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/MmServicesTableLib.h>

#include <Protocol/BlockIo.h>

#include "DiskGpt.h"
#include "FwsMetadata.h"

/**
  Validate loaded Metadata version 2

  @param [in]       Metadata      Firmware update storage metadata version 2.

  @retval TRUE                    Valid.
  @retval FALSE

**/
STATIC
BOOLEAN
EFIAPI
ValidateMetadataV2 (
  IN PSA_MM_FWU_METADATA_V2  *Metadata
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  PSA_MM_FWU_IMAGE_PROPERTIES  *ImageBankInfo;
  UINT8                        BankIdx;
  UINT16                       ImgIdx;
  UINT32                       Crc32;
  UINT32                       MetadataSize;
  UINT32                       ImageEntrySize;

  if (Metadata->DescriptorOffset == 0x00) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Metadata's DescriptorOffset shouldn't be zero...\n",
      __func__
      ));
    return FALSE;
  }

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (Metadata);
  if ((FwFwsDesc->NumBanks > FWU_METADATA_V2_MAX_NUM_BANKS) ||
      (FwFwsDesc->NumBanks > FWU_NUMBER_OF_BANKS))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NumBanks should be <= %d... Current:%d.\n",
      __func__,
      MIN (FWU_NUMBER_OF_BANKS, FWU_METADATA_V2_MAX_NUM_BANKS),
      FwFwsDesc->NumBanks
      ));
    return FALSE;
  }

  if ((Metadata->Header.ActiveIndex >= FwFwsDesc->NumBanks) ||
      (Metadata->Header.PreviousActiveIndex >= FwFwsDesc->NumBanks))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Active or Previous Index over bank count...\n",
      __func__
      ));
    return FALSE;
  }

  if (FwFwsDesc->BankInfoEntrySize != sizeof (PSA_MM_FWU_IMAGE_PROPERTIES)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: BankInfoEntrySize should be %d... Current:%d.\n",
      __func__,
      sizeof (PSA_MM_FWU_IMAGE_PROPERTIES),
      FwFwsDesc->BankInfoEntrySize
      ));
    return FALSE;
  }

  ImageEntrySize = (FwFwsDesc->NumImages *
                    (sizeof (PSA_MM_FWU_IMAGE_ENTRY_V2) +
                     (FwFwsDesc->NumBanks * sizeof (PSA_MM_FWU_IMAGE_PROPERTIES))));

  if (ImageEntrySize != FwFwsDesc->ImageEntrySize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: ImageEntrySize should be %d... Current:%d.\n",
      __func__,
      ImageEntrySize,
      FwFwsDesc->ImageEntrySize
      ));
    return FALSE;
  }

  MetadataSize = sizeof (PSA_MM_FWU_METADATA_V2) +
                 sizeof (PSA_MM_FWU_FW_STORE_DESC_V2) + FwFwsDesc->ImageEntrySize;

  if (MetadataSize != Metadata->MetadataSize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MetadataSize should be %d... Current:%d.\n",
      __func__,
      MetadataSize,
      Metadata->MetadataSize
      ));
    return FALSE;
  }

  for (BankIdx = 0; BankIdx < FwFwsDesc->NumBanks; BankIdx++) {
    if ((Metadata->BankState[BankIdx] != FWU_BANK_STATE_VALID) &&
        (Metadata->BankState[BankIdx] != FWU_BANK_STATE_ACCEPTED))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: BankState[%d] is Invalid: 0x%x...\n",
        __func__,
        BankIdx,
        Metadata->BankState[BankIdx]
        ));
      return FALSE;
    }

    for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
      ImageEntry    = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);
      ImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, BankIdx);

      if ((ImageBankInfo->Accepted == FWU_IMAGE_UNACCEPTED) &&
          (Metadata->BankState[BankIdx] == FWU_BANK_STATE_ACCEPTED))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: BankState[%d] should be VALID(0x%x) not ACCEPTED...\n",
          __func__,
          BankIdx,
          FWU_BANK_STATE_VALID
          ));
        return FALSE;
      }
    }
  }

  for (BankIdx = FwFwsDesc->NumBanks; BankIdx < FwFwsDesc->NumBanks; BankIdx++) {
    if (Metadata->BankState[BankIdx] != FWU_BANK_STATE_INVALID) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: BankState[%d] should be Invalid...\n",
        __func__
        ));
      return FALSE;
    }
  }

  Crc32 = CalculateCrc32 (
            (VOID *)Metadata + sizeof (Metadata->Header.Crc32),
            Metadata->MetadataSize - sizeof (Metadata->Header.Crc32)
            );

  if (Crc32 != Metadata->Header.Crc32) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Crc32 value should be 0x%lx... Current:0x%lx.\n",
      __func__,
      Crc32,
      Metadata->Header.Crc32
      ));
    return FALSE;
  }

  return TRUE;
}

/**
  Loaded Metadata version 2 from GPT partition.

  @param [in]       GptHandle       Gpt partition handle.
  @param [in]       PartitionKey    Key to find out gpt partition saving metadata.
  @param [out]      StartLba        Start LBA of metadata gpt partition.
  @param [out]      Metadata        Loaded metadata.

  @retval EFI_SUCCESS
  @retval Others                  Failed to read gpt partition.

**/
STATIC
EFI_STATUS
EFIAPI
GetMetadataV2 (
  IN  GPT_PARTITION_HANDLE    *GptHandle,
  IN  VOID                    *PartitionKey,
  OUT EFI_LBA                 *StartLba,
  OUT PSA_MM_FWU_METADATA_V2  **Metadata
  )
{
  EFI_STATUS              Status;
  PSA_MM_FWU_METADATA_V2  *Meta;
  PSA_MM_FWU_METADATA_V2  TmpHeader;

  Meta = NULL;

  /// Find out Metadata Partition.
  Status = GptGetMatchedPartitionStats (
             GptHandle,
             MatchedMetaDataPartition,
             PartitionKey,
             StartLba,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to get Metadata Partition LBA...\n", __func__));
    goto ErrorHandler;
  }

  Status = GptReadPartition (
             GptHandle,
             (VOID *)&TmpHeader,
             sizeof (PSA_MM_FWU_METADATA_V2),
             0,
             *StartLba
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to read Tmp Header...\n", __func__));
    goto ErrorHandler;
  }

  if (TmpHeader.Header.Version != FWS_METADATA_FORMAT_V2) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Metadata Version: %d...\n",
      __func__,
      TmpHeader.Header.Version
      ));
    goto ErrorHandler;
  }

  Meta = AllocateRuntimePool (TmpHeader.MetadataSize);
  if (Meta == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Out of Memory...\n", __func__));
    goto ErrorHandler;
  }

  Status = GptReadPartition (
             GptHandle,
             (VOID *)Meta,
             TmpHeader.MetadataSize,
             0,
             *StartLba
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to read Metadata Header...\n", __func__));
    goto ErrorHandler;
  }

  *Metadata = Meta;

  return EFI_SUCCESS;

ErrorHandler:
  if (Meta != NULL) {
    FreePool (Meta);
  }

  *StartLba = 0;

  return Status;
}

/**
  Initialize and Load Firmware Update Store Metadata version 2.

  @param [in]   GptHandle     GptHandle
  @param [out]  FwsMetadata   Firmware Update Store Metadata.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_ABORTED           Metadata is invalid.
  @retval Others                Failed to get metadata from gpt partition.

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2Init (
  IN  GPT_PARTITION_HANDLE  *GptHandle,
  OUT FWS_METADATA          **FwsMetadata
  )
{
  EFI_STATUS              Status;
  FWS_METADATA            *FwsMeta;
  EFI_LBA                 ActiveLba;
  EFI_LBA                 BackupLba;
  PSA_MM_FWU_METADATA_V2  *ActiveMeta;
  PSA_MM_FWU_METADATA_V2  *BackupMeta;
  BOOLEAN                 IsActiveValid;
  BOOLEAN                 IsBackupValid;

  ActiveMeta = NULL;
  BackupMeta = NULL;

  Status = GetMetadataV2 (
             GptHandle,
             (VOID *)FWU_METADATA_PARTITION_NAME,
             &ActiveLba,
             &ActiveMeta
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to get Active Metadata: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  Status = GetMetadataV2 (
             GptHandle,
             (VOID *)FWU_BKUP_METADATA_PARTITION_NAME,
             &BackupLba,
             &BackupMeta
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to get Backup Metadata: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  IsActiveValid = ValidateMetadataV2 (ActiveMeta);
  IsBackupValid = ValidateMetadataV2 (BackupMeta);

  if (!IsActiveValid && !IsBackupValid) {
    Status = EFI_ABORTED;
    DEBUG ((
      DEBUG_ERROR,
      "%a: Both of metadata are invalid. Error!\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  if (!IsActiveValid) {
    Status = GptWritePartition (
               GptHandle,
               (VOID *)BackupMeta,
               BackupMeta->MetadataSize,
               0,
               ActiveLba
               );
    FreePool (ActiveMeta);
    ActiveMeta = BackupMeta;
    BackupMeta = NULL;
  } else {
    if (!IsBackupValid) {
      Status = GptWritePartition (
                 GptHandle,
                 (VOID *)ActiveMeta,
                 ActiveMeta->MetadataSize,
                 0,
                 BackupLba
                 );
    }

    FreePool (BackupMeta);
    BackupMeta = NULL;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to overwrite metadata from %a to %a failure(%r)..!\n",
      __func__,
      (!IsActiveValid) ? "Backup" : "Active",
      (!IsActiveValid) ? "Active" : "Backup",
      Status
      ));
    goto ErrorHandler;
  }

  FwsMeta = AllocateRuntimePool (sizeof (FWS_METADATA));
  if (FwsMeta == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Out of memory...\n", __func__));
    goto ErrorHandler;
  }

  FwsMeta->GptHandle      = GptHandle;
  FwsMeta->ActiveLba      = ActiveLba;
  FwsMeta->BackupLba      = BackupLba;
  FwsMeta->Metadata       = (VOID *)ActiveMeta;
  FwsMeta->MetadataSize   = ActiveMeta->MetadataSize;
  FwsMeta->FwsMetadataOps = &gFwsMetadataV2Ops;

  *FwsMetadata = FwsMeta;

  return EFI_SUCCESS;

ErrorHandler:
  if (BackupMeta != NULL) {
    FreePool (BackupMeta);
  }

  if (ActiveMeta != NULL) {
    FreePool (ActiveMeta);
  }

  return Status;
}

/**
  Cleanup Loaded Firmware Update Store Metadata version 2.

  @param [in]  FwsMetadata   Firmware Update Store Metadata.

**/
STATIC
VOID
EFIAPI
FwsMetadataV2Exit (
  IN FWS_METADATA  *FwsMetadata
  )
{
  if (FwsMetadata != NULL) {
    if (FwsMetadata->Metadata != NULL) {
      FreePool (FwsMetadata->Metadata);
    }

    FreePool (FwsMetadata);
  }
}

/**
  Get the number of banks.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  NumBanks      Number of banks.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2GetNumBanks (
  IN FWS_METADATA  *FwsMetadata,
  OUT UINT8        *NumBanks
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (FwsMetadata->Metadata);

  *NumBanks = FwFwsDesc->NumBanks;

  return EFI_SUCCESS;
}

/**
  Get the number of images per bank.

  @param [in]   FwsMetadata   Firmware Update Store Metadata.
  @param [out]  NumImages     Number of images per bank.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2GetNumImages (
  IN FWS_METADATA  *FwsMetadata,
  OUT UINT16       *NumImages
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (FwsMetadata->Metadata);

  *NumImages = FwFwsDesc->NumImages;

  return EFI_SUCCESS;
}

/**
  Check trial state.

  @param [in]   FwsMetadata     Firmware Update Store Metadata.
  @param [out]  TrialRunState   If true, current is in the trial state.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2CheckTrialRunState (
  IN FWS_METADATA  *FwsMetadata,
  OUT BOOLEAN      *TrialRunState
  )
{
  PSA_MM_FWU_METADATA_V2       *Metadata;
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  UINT8                        BankIdx;

  Metadata  = (PSA_MM_FWU_METADATA_V2 *)FwsMetadata->Metadata;
  BankIdx   = Metadata->Header.ActiveIndex;
  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (Metadata);

  if (Metadata->BankState[BankIdx] == FWU_BANK_STATE_VALID) {
    *TrialRunState = TRUE;
  } else {
    *TrialRunState = FALSE;
  }

  return EFI_SUCCESS;
}

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
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2GetImageTypeGuids (
  IN     FWS_METADATA  *FwsMetadata,
  OUT    EFI_GUID      *ImageTypeGuids,
  IN OUT UINT32        *Size
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  UINT16                       ImgIdx;

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (FwsMetadata->Metadata);

  if (*Size < (FwFwsDesc->NumImages * sizeof (EFI_GUID))) {
    *Size = FwFwsDesc->NumImages * sizeof (EFI_GUID);
    DEBUG ((DEBUG_ERROR, "%a: Buffer is too small...\n", __func__));
    return EFI_BUFFER_TOO_SMALL;
  }

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);
    CopyGuid (&ImageTypeGuids[ImgIdx], &ImageEntry->Header.ImageTypeGuid);
  }

  return EFI_SUCCESS;
}

/**
  Get location guid of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [out]      LocationGuid    Location Guids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2GetLocationGuid (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  OUT EFI_GUID           *LocationGuid
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  UINT16                       ImgIdx;

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (FwsMetadata->Metadata);

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);

    if (CompareGuid (&ImageEntry->Header.ImageTypeGuid, ImageTypeGuid)) {
      CopyGuid (LocationGuid, &ImageEntry->Header.LocationGuid);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get image guid of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index.
  @param [out]      ImageGuid       Image Guids.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2GetImageGuid (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  OUT EFI_GUID           *ImageGuid
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  PSA_MM_FWU_IMAGE_PROPERTIES  *ImageBankInfo;
  UINT16                       ImgIdx;

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (FwsMetadata->Metadata);

  if (BankIdx >= FwFwsDesc->NumBanks) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);

    if (CompareGuid (&ImageEntry->Header.ImageTypeGuid, ImageTypeGuid)) {
      ImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, BankIdx);
      CopyGuid (ImageGuid, &ImageBankInfo->ImageGuid);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

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
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2GetAcceptState (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  OUT    BOOLEAN         *AcceptState
  )
{
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  PSA_MM_FWU_IMAGE_PROPERTIES  *ImageBankInfo;
  UINT16                       ImgIdx;

  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (FwsMetadata->Metadata);

  if (BankIdx >= FwFwsDesc->NumBanks) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);
    if (!CompareGuid (&ImageEntry->Header.ImageTypeGuid, ImageTypeGuid)) {
      continue;
    }

    ImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, BankIdx);
    if (ImageBankInfo->Accepted == FWU_IMAGE_ACCEPTED) {
      *AcceptState = TRUE;
    } else {
      *AcceptState = FALSE;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Set accept state of image type guid.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       ImageTypeGuid   Image type guid.
  @param [in]       BankIdx         Bank Index
  @param [in]       AcceptReq       Type of request.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2SetAcceptState (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  IN     FWS_ACCEPT_REQ  AcceptReq
  )
{
  PSA_MM_FWU_METADATA_V2       *Metadata;
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  PSA_MM_FWU_IMAGE_PROPERTIES  *ImageBankInfo;
  UINT16                       ImgIdx;

  Metadata  = (PSA_MM_FWU_METADATA_V2 *)FwsMetadata->Metadata;
  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (Metadata);

  if (BankIdx >= FwFwsDesc->NumBanks) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);
    if (!CompareGuid (&ImageEntry->Header.ImageTypeGuid, ImageTypeGuid)) {
      continue;
    }

    ImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, BankIdx);
    if (AcceptReq == FwsImageAcceptReq) {
      ImageBankInfo->Accepted = FWU_IMAGE_ACCEPTED;
    } else {
      ImageBankInfo->Accepted = FWU_IMAGE_UNACCEPTED;

      if (AcceptReq == FwsImageWriteUnAcceptReq) {
        Metadata->BankState[BankIdx] = FWU_BANK_STATE_INVALID;
      }
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Update bank state.

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       BankIdx         Bank Index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2UpdateBankState (
  IN     FWS_METADATA  *FwsMetadata,
  IN     UINT32        BankIdx
  )
{
  PSA_MM_FWU_METADATA_V2       *Metadata;
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  PSA_MM_FWU_IMAGE_PROPERTIES  *ImageBankInfo;
  UINT16                       ImgIdx;
  UINT8                        BankState;

  Metadata  = (PSA_MM_FWU_METADATA_V2 *)FwsMetadata->Metadata;
  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (Metadata);

  if (BankIdx >= FwFwsDesc->NumBanks) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  BankState = FWU_BANK_STATE_ACCEPTED;

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry    = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);
    ImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, BankIdx);

    if (ImageBankInfo->Accepted != FWU_IMAGE_ACCEPTED) {
      BankState = FWU_BANK_STATE_VALID;
      break;
    }
  }

  Metadata->BankState[BankIdx] = BankState;

  return EFI_SUCCESS;
}

/**
  Rollback metadata from @BackupIdx's one to @@TargetIdx's one

  @param [in]       FwsMetadata     Firmware Update Store Metadata.
  @param [in]       BackupIdx       Backup Bank Index
  @param [in]       TargetIdx       Target Bank Index

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
STATIC
EFI_STATUS
EFIAPI
FwsMetadataV2RollBack (
  IN     FWS_METADATA  *FwsMetadata,
  IN     UINT32        BackupIdx,
  IN     UINT32        TargetIdx
  )
{
  PSA_MM_FWU_METADATA_V2       *Metadata;
  PSA_MM_FWU_FW_STORE_DESC_V2  *FwFwsDesc;
  PSA_MM_FWU_IMAGE_ENTRY_V2    *ImageEntry;
  PSA_MM_FWU_IMAGE_PROPERTIES  *BkpImageBankInfo;
  PSA_MM_FWU_IMAGE_PROPERTIES  *TgtImageBankInfo;
  UINT16                       ImgIdx;

  Metadata  = (PSA_MM_FWU_METADATA_V2 *)FwsMetadata->Metadata;
  FwFwsDesc = GET_FWU_FW_STORE_DESC_V2 (Metadata);

  if ((BackupIdx >= FwFwsDesc->NumBanks) ||
      (TargetIdx >= FwFwsDesc->NumBanks))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (Metadata->BankState[BackupIdx] != FWU_BANK_STATE_ACCEPTED) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Backup Bank(%d) State(0x%x) isn't accepted state\n",
      __func__,
      BackupIdx,
      Metadata->BankState[BackupIdx]
      ));
    return EFI_INVALID_PARAMETER;
  }

  for (ImgIdx = 0; ImgIdx < FwFwsDesc->NumImages; ImgIdx++) {
    ImageEntry       = GET_FWU_IMAGE_ENTRY_V2 (FwFwsDesc, ImgIdx);
    BkpImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, BackupIdx);
    TgtImageBankInfo = GET_FWU_IMAGE_BANK_INFO_V2 (ImageEntry, TargetIdx);

    CopyMem (TgtImageBankInfo, BkpImageBankInfo, sizeof (PSA_MM_FWU_IMAGE_PROPERTIES));
  }

  Metadata->BankState[TargetIdx] = Metadata->BankState[BackupIdx];

  return EFI_SUCCESS;
}

FWS_METADATA_OPS  gFwsMetadataV2Ops = {
  FwsMetadataV2Init,
  FwsMetadataV2Exit,
  FwsMetadataV2GetNumBanks,
  FwsMetadataV2GetNumImages,
  FwsMetadataV2CheckTrialRunState,
  FwsMetadataV2GetImageTypeGuids,
  FwsMetadataV2GetLocationGuid,
  FwsMetadataV2GetImageGuid,
  FwsMetadataV2GetAcceptState,
  FwsMetadataV2SetAcceptState,
  FwsMetadataV2UpdateBankState,
  FwsMetadataV2RollBack,
};
