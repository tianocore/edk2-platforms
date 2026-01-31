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
  Matching function used to find out Firmware Update Metadata GPT partition.

  @param [in]   PartitionEntry       GPT partition.
  @param [in]   Data                 Partition Name.

  @retval       TRUE                 Found.
  @retval       FALSE                Not found.

**/
BOOLEAN
EFIAPI
MatchedMetaDataPartition (
  IN CONST EFI_PARTITION_ENTRY  *PartitionEntry,
  IN CONST VOID                 *Data
  )
{
  CONST CHAR16  *PartitionName = (CONST CHAR16 *)Data;

  if (CompareGuid (&PartitionEntry->PartitionTypeGUID, &gPsaFwuMetadataGuid) &&
      (StrCmp (PartitionEntry->PartitionName, PartitionName) == 0))
  {
    return TRUE;
  }

  return FALSE;
}

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
  )
{
  PSA_MM_FWU_METADATA_COMMON_HEADER  *Header;
  UINT32                             Crc32;

  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Header = (PSA_MM_FWU_METADATA_COMMON_HEADER *)FwsMetadata->Metadata;
  Crc32  = CalculateCrc32 (
             (VOID *)Header + sizeof (Header->Crc32),
             FwsMetadata->MetadataSize  - sizeof (Header->Crc32)
             );

  if (Crc32 != Header->Crc32) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_CRC_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                         Status;
  PSA_MM_FWU_METADATA_COMMON_HEADER  *Header;
  UINT32                             Crc32;
  CONST UINTN                        MaxRetries = 3;
  UINTN                              Retries;

  if ((FwsMetadata == NULL) ||
      (FwsMetadata->GptHandle == NULL) ||
      (FwsMetadata->Metadata == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Header = (PSA_MM_FWU_METADATA_COMMON_HEADER *)FwsMetadata->Metadata;
  Crc32  = CalculateCrc32 (
             (VOID *)Header + sizeof (Header->Crc32),
             FwsMetadata->MetadataSize  - sizeof (Header->Crc32)
             );

  Header->Crc32 = Crc32;

  for (Retries = MaxRetries; Retries > 0; Retries--) {
    Status = GptWritePartition (
               FwsMetadata->GptHandle,
               FwsMetadata->Metadata,
               FwsMetadata->MetadataSize,
               0,
               FwsMetadata->ActiveLba
               );
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Active metadata failed to persist! FW Store may be inoperative.\n"));
    return Status;
  }

  for (Retries = MaxRetries; Retries > 0; Retries--) {
    Status = GptWritePartition (
               FwsMetadata->GptHandle,
               FwsMetadata->Metadata,
               FwsMetadata->MetadataSize,
               0,
               FwsMetadata->BackupLba
               );
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Backup metadata failed to persist! FW Store may be inoperative.\n"));
    return Status;
  }

  return EFI_SUCCESS;
}

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
  )
{
  PSA_MM_FWU_METADATA_COMMON_HEADER  *Header;

  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (ActiveIndex == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Header       = (PSA_MM_FWU_METADATA_COMMON_HEADER *)FwsMetadata->Metadata;
  *ActiveIndex = Header->ActiveIndex;

  return EFI_SUCCESS;
}

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
  )
{
  PSA_MM_FWU_METADATA_COMMON_HEADER  *Header;
  UINT8                              NumBanks;

  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FwsMetadata->FwsMetadataOps->GetNumBanks (FwsMetadata, &NumBanks);

  if (ActiveIndex >= NumBanks) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Header              = (PSA_MM_FWU_METADATA_COMMON_HEADER *)FwsMetadata->Metadata;
  Header->ActiveIndex = ActiveIndex;

  return EFI_SUCCESS;
}

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
  )
{
  PSA_MM_FWU_METADATA_COMMON_HEADER  *Header;

  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (PreviousActiveIndex == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Header               = (PSA_MM_FWU_METADATA_COMMON_HEADER *)FwsMetadata->Metadata;
  *PreviousActiveIndex = Header->PreviousActiveIndex;

  return EFI_SUCCESS;
}

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
  )
{
  PSA_MM_FWU_METADATA_COMMON_HEADER  *Header;
  UINT8                              NumBanks;

  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FwsMetadata->FwsMetadataOps->GetNumBanks (FwsMetadata, &NumBanks);

  if (PreviousActiveIndex >= NumBanks) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Header                      = (PSA_MM_FWU_METADATA_COMMON_HEADER *)FwsMetadata->Metadata;
  Header->PreviousActiveIndex = PreviousActiveIndex;

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                         Status;
  PSA_MM_FWU_METADATA_COMMON_HEADER  Header;
  EFI_LBA                            StartLba;
  FWS_METADATA_OPS                   *FwsMetadataOps;

  if ((GptHandle == NULL) || (FwsMetadata == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = GptGetMatchedPartitionStats (
             GptHandle,
             MatchedMetaDataPartition,
             (VOID *)FWU_METADATA_PARTITION_NAME,
             &StartLba,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get partition info: %r\n", __func__, Status));
    goto ErrorHandler;
  }

  Status = GptReadPartition (
             GptHandle,
             (VOID *)&Header,
             sizeof (PSA_MM_FWU_METADATA_COMMON_HEADER),
             0,
             StartLba
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read metadata common header: %r\n", __func__, Status));
    goto ErrorHandler;
  }

  switch (Header.Version) {
    case FWS_METADATA_FORMAT_V2:
      FwsMetadataOps = &gFwsMetadataV2Ops;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid Metadata Version: %d\n", __func__, Header.Version));
      return EFI_UNSUPPORTED;
  }

  Status = FwsMetadataOps->InitMetadata (GptHandle, FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to alloc: %r\n", __func__, Status));
    goto ErrorHandler;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *FwsMetadata = NULL;
  return Status;
}

/**
  Cleanup Loaded Firmware Update Store Metadata.

  @param [in]  FwsMetadata   Firmware Update Store Metadata.

**/
VOID
EFIAPI
FwsMetadataExit (
  IN FWS_METADATA  *FwsMetadata
  )
{
  if (FwsMetadata == NULL) {
    return;
  }

  FwsMetadata->FwsMetadataOps->ExitMetadata (FwsMetadata);
}

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
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) || (NumBanks == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->GetNumBanks (FwsMetadata, NumBanks);
}

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
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) || (NumImages == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->GetNumImages (FwsMetadata, NumImages);
}

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
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) || (TrialRunState == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->CheckTrialRunState (FwsMetadata, TrialRunState);
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
EFI_STATUS
EFIAPI
FwsMetadataGetImageTypeGuids (
  IN     FWS_METADATA  *FwsMetadata,
  OUT    EFI_GUID      *ImageTypeGuids,
  IN OUT UINT32        *Size
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (ImageTypeGuids == NULL) || (Size == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->GetImageTypeGuids (
                                        FwsMetadata,
                                        ImageTypeGuids,
                                        Size
                                        );
}

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
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (ImageTypeGuid == NULL) || (LocationGuid == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->GetLocationGuid (
                                        FwsMetadata,
                                        ImageTypeGuid,
                                        LocationGuid
                                        );
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
EFI_STATUS
EFIAPI
FwsMetadataGetImageGuid (
  IN    FWS_METADATA    *FwsMetadata,
  IN    CONST EFI_GUID  *ImageTypeGuid,
  IN    UINT32          BankIdx,
  OUT EFI_GUID          *ImageGuid
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (ImageTypeGuid == NULL) || (ImageGuid == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->GetImageGuid (
                                        FwsMetadata,
                                        ImageTypeGuid,
                                        BankIdx,
                                        ImageGuid
                                        );
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
EFI_STATUS
EFIAPI
FwsMetadataGetAcceptState (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  OUT    BOOLEAN         *AcceptState
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (ImageTypeGuid == NULL) || (AcceptState == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->GetAcceptState (
                                        FwsMetadata,
                                        ImageTypeGuid,
                                        BankIdx,
                                        AcceptState
                                        );
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
EFI_STATUS
EFIAPI
FwsMetadataSetAcceptState (
  IN     FWS_METADATA    *FwsMetadata,
  IN     CONST EFI_GUID  *ImageTypeGuid,
  IN     UINT32          BankIdx,
  IN     FWS_ACCEPT_REQ  AcceptReq
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL) ||
      (ImageTypeGuid == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (AcceptReq > FwsImageWriteUnAcceptReq) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->SetAcceptState (
                                        FwsMetadata,
                                        ImageTypeGuid,
                                        BankIdx,
                                        AcceptReq
                                        );
}

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
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->UpdateBankState (FwsMetadata, BankIdx);
}

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
  )
{
  if ((FwsMetadata == NULL) || (FwsMetadata->Metadata == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid arguments...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return FwsMetadata->FwsMetadataOps->RollBack (FwsMetadata, BackupIdx, TargetIdx);
}
