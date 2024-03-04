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
#include <Library/FwsPlatformLib.h>

#include <Protocol/BlockIo.h>

#include "DiskGpt.h"
#include "FwsMetadata.h"

#define INTERNAL_BUFFER_SIZE  SIZE_1MB

#define IMG_DIR_VER  2

/**
  Get the Image directory size.

  @param [in]   NumImages            Number of Images.

  @retval       Size of Image directory in byte.

**/
#define IMG_DIR_SIZE(NumImages)          \
  (sizeof(PSA_MM_FWU_IMAGE_DIRECTORY) + sizeof(PSA_MM_FWU_IMG_INFO_ENTRY) * (NumImages))

#define FWS_IFD_F_DIRTY  (1 << 0)

typedef enum {
  FWS_UPD_OFF,
  FWS_UPD_ON,
  FWS_UPD_STATE_MAX,
} FWS_UPD_STATE;

/**
 * Firmware Device Instance Data saved in FWS_DEVICE_INSTANCE->Private.
 */
typedef struct {
  /// Gpt Handle.
  GPT_PARTITION_HANDLE          *GptHandle;

  /// Firmware metadata.
  FWS_METADATA                  *FwsMetadata;

  /// Image Directory.
  PSA_MM_FWU_IMAGE_DIRECTORY    *ImageDirectory;

  /// Boot Index in Banks.
  UINT32                        BootIndex;

  /// Active Index in Banks.
  UINT32                        ActiveIndex;

  /// Update Index in Banks.
  UINT32                        UpdateIndex;

  /// Current update state.
  FWS_UPD_STATE                 UpdateState;
} FWS_DEVICE_DATA;

/** Image File data saved in FWS_IMAGE_FILE->Private
*/
typedef struct {
  /// Start Lba.
  EFI_LBA    StartLba;

  /// File Size.
  UINTN      FileSize;

  /// Maximum File Size.
  UINTN      MaxSize;

  /// Flags, See the FWS_IFD_F_*
  UINTN      Flags;
} FWS_IMAGE_FILE_DATA;

STATIC CONST CHAR16  *BankPartitionName[FWU_NUMBER_OF_BANKS] = {
  L"FIP_A",
  L"FIP_B",
};

/**
  Matching function used to find out the GPT partition based on
  the name of partition.

  @param [in]   PartitionEntry         GPT partition.
  @param [in]   Data                   Partition Name

  @retval       TRUE                 Found.
  @retval       FALSE                Not found.

**/
STATIC
BOOLEAN
MatchPartitionByName (
  IN CONST EFI_PARTITION_ENTRY  *PartitionEntry,
  IN CONST VOID                 *Data
  )
{
  CONST CHAR16  *PartitionName = (CONST CHAR16 *)Data;

  if (StrCmp (PartitionEntry->PartitionName, PartitionName) == 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Generate Firmware Update Image Directory.

  @param [in,out]   FwsDeviceData     FwsDevice's opened data.

  @retval EFI_SUCCESS            Success to initialize Firmware Update Image Directory.
  @retval EFI_OUT_OF_RESOURCES   Out of Memory.
  @retval EFI_NOT_FOUND          Fail to find boot-idx GPT partition or
                                 system firmware information in Firmware Update Metadata.
  @retval Others                 Error on reading boot-idx GPT partition.

**/
STATIC
EFI_STATUS
InitImageDirectory (
  IN OUT FWS_DEVICE_DATA  *FwsDeviceData
  )
{
  EFI_STATUS                  Status;
  GUID                        *SystemFirmwareImageTypeGuid;
  UINTN                       Idx;
  UINTN                       PartSize;
  PSA_MM_FWU_IMAGE_DIRECTORY  *ImageDirectory;
  PSA_MM_FWU_IMG_INFO_ENTRY   *ImgInfoEntry;
  FWS_METADATA                *FwsMetadata;
  UINT8                       NumBanks;
  UINT16                      NumImages;
  BOOLEAN                     AcceptState;
  BOOLEAN                     SysGuidFound;
  EFI_GUID                    *ImageTypeGuids;
  UINT32                      GuidsSize;

  if ((FwsDeviceData == NULL) ||
      (FwsDeviceData->GptHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SystemFirmwareImageTypeGuid = PcdGetPtr (PcdSystemFirmwareImageTypeGuid);

  FwsMetadata = FwsDeviceData->FwsMetadata;

  Status = FwsMetadataGetNumBanks (FwsMetadata, &NumBanks);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get number of banks...\n", __func__));
    return Status;
  }

  Status = FwsMetadataGetNumImages (FwsMetadata, &NumImages);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get number of images...\n", __func__));
    return Status;
  }

  ImageTypeGuids = NULL;
  ImageDirectory = NULL;

  GuidsSize      = sizeof (EFI_GUID) * NumImages;
  ImageTypeGuids = AllocateRuntimePool (GuidsSize);
  if (ImageTypeGuids == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to Allocate Memory...\n", __func__));
    goto ErrorHandler;
  }

  Status = FwsMetadataGetImageTypeGuids (FwsMetadata, ImageTypeGuids, &GuidsSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get ImageTypeGuids... %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  ImageDirectory = AllocateRuntimeZeroPool (IMG_DIR_SIZE (NumImages));
  if (ImageDirectory == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to Allocate Memory...\n", __func__));
    goto ErrorHandler;
  }

  ImageDirectory->DirectoryVersion = IMG_DIR_VER;
  ImageDirectory->ImgInfoOffset    = sizeof (PSA_MM_FWU_IMAGE_DIRECTORY);
  ImageDirectory->CorrectBoot      =
    (FwsDeviceData->BootIndex == FwsDeviceData->ActiveIndex) ? TRUE : FALSE;
  ImageDirectory->ImgInfoSize = sizeof (PSA_MM_FWU_IMG_INFO_ENTRY) * NumImages;
  ImageDirectory->NumImages   = NumImages;

  Status = GptGetMatchedPartitionStats (
             FwsDeviceData->GptHandle,
             MatchPartitionByName,
             (VOID *)BankPartitionName[FwsDeviceData->BootIndex],
             NULL,
             &PartSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Partition Size.\n", __func__));
    goto ErrorHandler;
  }

  SysGuidFound = FALSE;

  for (Idx = 0; Idx < NumImages; Idx++) {
    ImgInfoEntry = GET_FWU_IMG_INFO_ENTRY (ImageDirectory, Idx);
    Status       = FwsMetadataGetAcceptState (
                     FwsMetadata,
                     &ImageTypeGuids[Idx],
                     FwsDeviceData->BootIndex,
                     &AcceptState
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to get accept state for %g.\n", __func__, &ImageTypeGuids[Idx]));
      goto ErrorHandler;
    }

    CopyGuid (&ImgInfoEntry->ImgTypeGuid, &ImageTypeGuids[Idx]);
    ImgInfoEntry->ClientPermissions =
      FWU_WRITE_PERMISSION | FWU_READ_PERMISSION | FWU_ACCEPT_AFTER_ACTIVATION;
    ImgInfoEntry->Accepted              = (AcceptState) ? FWU_IMAGE_ACCEPTED : FWU_IMAGE_UNACCEPTED;
    ImgInfoEntry->LowestAcceptedVersion = PcdGet32 (PcdSystemFirmwareFmpLowestSupportedVersion);
    ImgInfoEntry->ImgVersion            = PcdGet32 (PcdSystemFirmwareFmpVersion);
    ImgInfoEntry->ImgMaxSize            = PartSize / NumImages;
    ImgInfoEntry->Reserved              = 0x00;

    if (CompareGuid (SystemFirmwareImageTypeGuid, &ImgInfoEntry->ImgTypeGuid)) {
      SysGuidFound = TRUE;
    }
  }

  if (!SysGuidFound) {
    Status = EFI_NOT_FOUND;
    DEBUG ((DEBUG_ERROR, "%a: No system firmware information couldn't be found in metadata!\n", __func__));
    goto ErrorHandler;
  }

  FwsDeviceData->ImageDirectory = ImageDirectory;
  FreePool (ImageTypeGuids);

  return EFI_SUCCESS;

ErrorHandler:
  if (ImageDirectory != NULL) {
    FreePool (ImageDirectory);
  }

  if (ImageTypeGuids != NULL) {
    FreePool (ImageTypeGuids);
  }

  return Status;
}

/**
  Get private data for opened image according to Platform.
  Private data would be used in file operation on the opened image.
  Therefore, it includes some of information of the storage related to
  opened image or any others required to process file operation.

  @param [in]    FwsDeviceData       FwsDevice's opened data.
  @param [in]    ImageTypeGuid       Image type guid trying to open.
  @param [in]    BankIdx             bank index where image data will get from.
  @param [OUT]   FileData            Private Data related to opened image.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES       Out of Memory.
  @retval Others                     Fail to read information from the GPT partition.

**/
STATIC
EFI_STATUS
InternalFwsOpen (
  IN FWS_DEVICE_DATA            *FwsDeviceData,
  IN CONST EFI_GUID             *ImageTypeGuid,
  IN       UINTN                BankIdx,
  OUT      FWS_IMAGE_FILE_DATA  **FileData
  )
{
  EFI_STATUS           Status;
  FWS_IMAGE_FILE_DATA  *ImageFileData;

  if ((FwsDeviceData == NULL) ||
      (BankIdx > FWU_NUMBER_OF_BANKS) ||
      (FileData == NULL) ||
      (ImageTypeGuid == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *FileData = NULL;

  ImageFileData = AllocateRuntimePool (sizeof (FWS_IMAGE_FILE_DATA));
  if (ImageFileData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (ImageFileData, sizeof (FWS_IMAGE_FILE_DATA));

  Status = GptGetMatchedPartitionStats (
             FwsDeviceData->GptHandle,
             MatchPartitionByName,
             (VOID *)BankPartitionName[BankIdx],
             &ImageFileData->StartLba,
             &ImageFileData->MaxSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Fail to Named partition (Name:%s): %d!\n",
      MatchPartitionByName,
      Status
      ));
    FreePool (ImageFileData);

    return Status;
  }

  // Couldn't get correct image size. so return maximum image size.
  ImageFileData->FileSize = ImageFileData->MaxSize;

  *FileData = ImageFileData;

  return EFI_SUCCESS;
}

/**
  Release Private Data related to opened file.
  This function should clear the data which allocated by InternalFwsOpen
  And apply changes of opened Image File on the storage if required.

  @param [in]  FileData          FileData generated by InternalFwsOpen.

**/
STATIC
VOID
InternalFwsRelease (
  IN FWS_IMAGE_FILE_DATA  *FileData
  )
{
  if (FileData != NULL) {
    FreePool (FileData);
  }
}

/**
  Rollback the Image from specified bank.

  @param [in]   FwsDeviceData  FwsDevice's opened data.
  @param [in]   BackupIdx      bank index where images to rollback are located.
  @param [in]   TargetIdx      bank index where rollback image will be written.

  @retval EFI_SUCCESS            Success to rollback image.
  @retval EFI_INVALID_PARAMETER  Invalid Parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of Memory.
  @retval EFI_NOT_READY          Couldn't find io protocol for NV storage.
  @retval Others                 Fail to read/write from/to GPT partition.

**/
STATIC
EFI_STATUS
InternalFwsRollBackBank (
  IN FWS_DEVICE_DATA  *FwsDeviceData,
  IN UINTN            BackupIdx,
  IN UINTN            TargetIdx
  )
{
  EFI_STATUS  Status;
  UINTN       BackupStartLba;
  UINTN       BackupPartSize;
  UINTN       TargetStartLba;
  UINTN       TargetPartSize;
  UINTN       ReadSize;
  UINTN       Offset;
  UINT16      NumImages;
  VOID        *Buffer;

  if ((FwsDeviceData == NULL) ||
      (FwsDeviceData->GptHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = FwsMetadataGetNumImages (FwsDeviceData->FwsMetadata, &NumImages);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get number of images...\n", __func__));
    return Status;
  }

  ReadSize = 0;
  Offset   = 0;

  Buffer = AllocateRuntimePool (INTERNAL_BUFFER_SIZE);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GptGetMatchedPartitionStats (
             FwsDeviceData->GptHandle,
             MatchPartitionByName,
             (VOID *)BankPartitionName[BackupIdx],
             &BackupStartLba,
             &BackupPartSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to get Backup Index(%d) Partition(%s) Info (%d).\n",
      __func__,
      BackupIdx,
      BankPartitionName[BackupIdx],
      Status
      ));
    goto ErrorHandler;
  }

  Status = GptGetMatchedPartitionStats (
             FwsDeviceData->GptHandle,
             MatchPartitionByName,
             (VOID *)BankPartitionName[TargetIdx],
             &TargetStartLba,
             &TargetPartSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to get Target Index(%d) Partition(%s) Info (%d).\n",
      __func__,
      TargetIdx,
      BankPartitionName[TargetIdx],
      Status
      ));
    goto ErrorHandler;
  }

  ASSERT (BackupPartSize <= TargetPartSize);

  ReadSize = BackupPartSize;

  for (Offset = 0; Offset < BackupPartSize;) {
    ReadSize = MIN (INTERNAL_BUFFER_SIZE, BackupPartSize);

    Status = GptReadPartition (
               FwsDeviceData->GptHandle,
               Buffer,
               ReadSize,
               Offset,
               BackupStartLba
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to read from Backup Partition: %r\n",
        __func__,
        Status
        ));
      goto ErrorHandler;
    }

    Status = GptWritePartition (
               FwsDeviceData->GptHandle,
               Buffer,
               ReadSize,
               Offset,
               TargetStartLba
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to write from Backup Partition: %r\n",
        __func__,
        Status
        ));
      goto ErrorHandler;
    }

    Offset  += ReadSize;
    ReadSize = BackupPartSize - Offset;
  }

  Status = FwsMetadataRollBack (FwsDeviceData->FwsMetadata, BackupIdx, TargetIdx);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to Rollback Metadata... %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  Status = FwsMetadataSave (FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to save Metadata... %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

ErrorHandler:
  FreePool (Buffer);

  return Status;
}

/**
  Release the firmware storage device instance.

  @param [in]  FwsDevice             Opened firmware storage device.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_READY              Some image files opened using this device.

**/
EFI_STATUS
EFIAPI
FwsReleaseDevice (
  IN FWS_DEVICE_INSTANCE  *FwsDevice
  )
{
  FWS_DEVICE_DATA  *FwsDeviceData;

  if ((FwsDevice == NULL) || (FwsDevice->Private == NULL)) {
    return EFI_SUCCESS;
  }

  if (FwsDevice->ImageFileCount != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "FwsReleaseDevice: Busy! Open image file count: %d\n",
      FwsDevice->ImageFileCount
      ));
    return EFI_NOT_READY;
  }

  FwsDeviceData = FwsDevice->Private;

  if (FwsDeviceData->ImageDirectory) {
    FreePool (FwsDeviceData->ImageDirectory);
    FwsDeviceData->ImageDirectory = NULL;
  }

  FwsMetadataExit (FwsDeviceData->FwsMetadata);

  if (FwsDeviceData->GptHandle) {
    GptReleasePartition (FwsDeviceData->GptHandle);
    FreePool (FwsDeviceData->GptHandle);
    FwsDeviceData->GptHandle = NULL;
  }

  FreePool (FwsDeviceData);

  FwsDevice->Private  = NULL;
  FwsDevice->Instance = NULL;

  FreePool (FwsDevice);

  return EFI_SUCCESS;
}

/**
  Open the firmware storage device instance.

  @param [in]   Instance             Firmware storage instance.
  @param [out]  FwsDevice            Opened firmware storage device.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES       Out of Memory.
  @retval Others                     Fail to open device internally.

**/
EFI_STATUS
EFIAPI
FwsOpenDevice (
  IN  EFI_BLOCK_IO_PROTOCOL  *Instance,
  OUT FWS_DEVICE_INSTANCE    **FwsDevice
  )
{
  EFI_STATUS            Status;
  FWS_DEVICE_INSTANCE   *TmpFwsDevice;
  FWS_DEVICE_DATA       *FwsDeviceData;
  GPT_PARTITION_HANDLE  *GptHandle;
  FWS_METADATA          *FwsMetadata;
  UINT32                ActiveIndex;
  UINT8                 NumBanks;

  if ((Instance == NULL) || (FwsDevice == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TmpFwsDevice = AllocateRuntimePool (sizeof (FWS_DEVICE_INSTANCE));
  if (TmpFwsDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FwsDeviceData = NULL;

  Status = GptOpenPartition (Instance, &GptHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FwsOpenDevice: Failed to open GptHandle.\n"));
    goto ErrorHandler;
  }

  FwsDeviceData = AllocateRuntimePool (sizeof (FWS_DEVICE_DATA));
  if (FwsDeviceData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  ZeroMem (FwsDeviceData, sizeof (FWS_DEVICE_DATA));

  FwsDeviceData->GptHandle = GptHandle;

  Status = FwsMetadataInit (GptHandle, &FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FwsOpenDevice: Failed to initialise Metadata.\n"));
    goto ErrorHandler;
  }

  FwsMetadata = FwsDeviceData->FwsMetadata;
  Status      = FwsMetadataGetNumBanks (FwsMetadata, &NumBanks);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FwsOpenDevice: Failed to get number of banks...\n"));
    goto ErrorHandler;
  }

  Status = FwsMetadataGetActiveIndex (FwsMetadata, &ActiveIndex);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FwsOpenDevice: Failed to get ActiveIndex...\n"));
    goto ErrorHandler;
  }

  /**
   * Currently, VExpress is booted from active_idx only by TF-A
   * Therefore, we can set FwsDeviceData->BootIndex as Metadata's active index.
   */
  FwsDeviceData->BootIndex   = ActiveIndex;
  FwsDeviceData->ActiveIndex = ActiveIndex;
  FwsDeviceData->UpdateIndex = (FwsDeviceData->ActiveIndex + 1) % NumBanks;

  DEBUG ((DEBUG_BLKIO, "FwuStore: BootIndex: %d\n", FwsDeviceData->BootIndex));
  DEBUG ((DEBUG_BLKIO, "FwuStore: ActiveIndex: %d\n", FwsDeviceData->ActiveIndex));
  DEBUG ((DEBUG_BLKIO, "FwuStore: UpdateIndex: %d\n", FwsDeviceData->UpdateIndex));

  Status = InitImageDirectory (FwsDeviceData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FwsOpenDevice: Failed to initialise Image directory.\n"));
    goto ErrorHandler;
  }

  FwsDeviceData->UpdateState = FWS_UPD_OFF;

  TmpFwsDevice->Instance = Instance;
  TmpFwsDevice->Private  = FwsDeviceData;

  *FwsDevice = TmpFwsDevice;

  return EFI_SUCCESS;

ErrorHandler:
  if (FwsDeviceData != NULL) {
    FwsMetadataExit (FwsDeviceData->FwsMetadata);
    FreePool (FwsDeviceData);
  }

  if (GptHandle != NULL) {
    GptReleasePartition (GptHandle);
  }

  if (TmpFwsDevice != NULL) {
    FreePool (TmpFwsDevice);
  }

  *FwsDevice = NULL;

  return Status;
}

/**
  Get Image Directory.

  @param [in]  FwsDevice              Opened firmware storage device instance.
  @param [out] ImageDirectory         Pointer to Image directory.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER       Invalid Parameter.

**/
EFI_STATUS
EFIAPI
FwsGetImageDirectory (
  IN  FWS_DEVICE_INSTANCE         *FwsDevice,
  OUT PSA_MM_FWU_IMAGE_DIRECTORY  **ImageDirectory
  )
{
  FWS_DEVICE_DATA  *FwsDeviceData;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL) ||
      (ImageDirectory == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData   = FwsDevice->Private;
  *ImageDirectory = NULL;

  if (FwsDeviceData->ImageDirectory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ImageDirectory = FwsDeviceData->ImageDirectory;

  return EFI_SUCCESS;
}

/**
  Open the @ImageTypeGuid Image from storage.

  @param [in]   FwsDevice              Opened firmware storage device instance.
  @param [in]   ImageTypeGuid
  @param [in]   OpType                 FwuOpStreamRead / FwuOpStreamWrite
  @param [out]  ImageFile              Opened Image File Handle.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES         Out of Memory.
  @retval Others

**/
EFI_STATUS
EFIAPI
FwsOpen (
  IN  FWS_DEVICE_INSTANCE  *FwsDevice,
  IN  CONST EFI_GUID       *ImageTypeGuid,
  IN  FWU_OP_TYPE          OpType,
  OUT FWS_IMAGE_FILE       **ImageFile
  )
{
  EFI_STATUS           Status;
  UINT32               BankIdx;
  FWS_IMAGE_FILE       *TmpImageFile;
  FWS_IMAGE_FILE_DATA  *ImageFileData;
  FWS_DEVICE_DATA      *FwsDeviceData;
  FWS_METADATA         *FwsMetadata;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL) ||
      (ImageTypeGuid == NULL) ||
      (ImageFile == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  TmpImageFile = AllocateRuntimePool (sizeof (FWS_IMAGE_FILE));
  if (TmpImageFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (TmpImageFile, sizeof (FWS_IMAGE_FILE));

  FwsMetadata = FwsDeviceData->FwsMetadata;
  BankIdx     = (OpType == FwuOpStreamWrite) ? FwsDeviceData->UpdateIndex : FwsDeviceData->BootIndex;

  Status = InternalFwsOpen (
             FwsDeviceData,
             ImageTypeGuid,
             BankIdx,
             (FWS_IMAGE_FILE_DATA **)&TmpImageFile->Private
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to Internal Image file Data! (%d)\n", Status));
    goto ErrorHandler;
  }

  ImageFileData = (FWS_IMAGE_FILE_DATA *)TmpImageFile->Private;
  CopyGuid (&TmpImageFile->ImageTypeGuid, ImageTypeGuid);

  Status = FwsMetadataGetImageGuid (
             FwsMetadata,
             ImageTypeGuid,
             BankIdx,
             &TmpImageFile->FileNameGuid
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Fail to find out image type(%g) in the bank!)\n",
      ImageTypeGuid
      ));
    goto ErrorHandler;
  }

  TmpImageFile->FwsDevice = FwsDevice;
  TmpImageFile->FileSize  = ImageFileData->FileSize;
  TmpImageFile->MaxSize   = ImageFileData->MaxSize;

  FwsDevice->ImageFileCount++;

  *ImageFile = TmpImageFile;

  return EFI_SUCCESS;

ErrorHandler:
  if (TmpImageFile) {
    FreePool (TmpImageFile);
  }

  *ImageFile = NULL;

  return Status;
}

/**
  Close the opened image via FwsOpen.
  This function performs:
     - If requires, apply changes on the image file to storage.
     - If image file is changed, set the image as unaccepted.

  @param [in]   ImageFile            Image File Handle.
  @param [in]   MaxAtomicTimeNs      Max Time to execute without yielding to client.
                                     0 means unbounded time.
  @param [out]  Progress             Unit of work completed.
  @param [out]  TotalWork            Unit of work must be completed.

  @retval  EFI_SUCCESS
  @retval  EFI_INVALID_PARAMETER
  @retval  Others

**/
EFI_STATUS
EFIAPI
FwsRelease (
  IN     FWS_IMAGE_FILE  *ImageFile,
  IN     UINT32          MaxAtomicTimeNs,
  OUT    UINT32          *Progress,
  OUT    UINT32          *TotalWork
  )
{
  FWS_IMAGE_FILE_DATA  *FileData;
  FWS_DEVICE_DATA      *FwsDeviceData;

  if ((ImageFile == NULL) ||
      (ImageFile->FwsDevice == NULL) ||
      (ImageFile->Private == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FileData      = ImageFile->Private;
  FwsDeviceData = ImageFile->FwsDevice->Private;

  /**
   * Currently, UpdateCapsule is blocking call in UEFI.
   * So we wouldn't use MaxAtomicTimeNs for yield parameter.
   */
  if (TotalWork != NULL) {
    *TotalWork = 100;
  }

  if (Progress != NULL) {
    *Progress = 100;
  }

  InternalFwsRelease ((FWS_IMAGE_FILE_DATA *)ImageFile->Private);
  ImageFile->FwsDevice->ImageFileCount--;
  ImageFile->FwsDevice = NULL;
  ImageFile->Private   = NULL;
  FreePool (ImageFile);

  return EFI_SUCCESS;
}

/**
  Read data from a partition stored in ImageFile.

  @param[in]       ImageFile    A pointer to the read destination buffer.
  @param[in,out]   Buffer       Read destination buffer.
  @param[in]       ReadSize     Size to read / Real read size from partition in bytes.
  @param[in]       Offset       The offset, within the partition to read from.

  @retval EFI_SUCCESS              The read operation was successful.
  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_READY            Storage is not ready.
  @retval Others                   Fail to read from partition.

**/
EFI_STATUS
EFIAPI
FwsRead (
  IN     FWS_IMAGE_FILE  *ImageFile,
  IN OUT VOID            *Buffer,
  IN     UINTN           *ReadSize,
  IN     UINTN           Offset
  )
{
  EFI_STATUS           Status;
  FWS_IMAGE_FILE_DATA  *FileData;
  FWS_DEVICE_DATA      *FwsDeviceData;

  if ((ImageFile == NULL) ||
      (Buffer == NULL) ||
      (ReadSize == NULL) ||
      (ImageFile->FwsDevice == NULL) ||
      (ImageFile->Private == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FileData      = ImageFile->Private;
  FwsDeviceData = ImageFile->FwsDevice->Private;

  if (Offset + *ReadSize > ImageFile->FileSize) {
    *ReadSize = ImageFile->FileSize - Offset;
  }

  Status = GptReadPartition (
             FwsDeviceData->GptHandle,
             Buffer,
             *ReadSize,
             Offset,
             FileData->StartLba
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read partition: %d.\n", Status));
    return Status;
  }

  return Status;
}

/**
  Write data to a partition stored in the Instance.

  @param[in]     ImageFile   Image File Handle.
  @param[in]     Buffer      Data to be written to the partition.
  @param[in,out] WriteSize   Size to write / real write to the partition in bytes.
  @param[in]     Offset      The offset, within the partition, to write to.

  @retval EFI_SUCCESS            The write operation was successful.
  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_READY          Storage is not ready.
  @retval Other                  Fail to write the data to partition.

**/
EFI_STATUS
EFIAPI
FwsWrite (
  IN      FWS_IMAGE_FILE  *ImageFile,
  IN      VOID            *Buffer,
  IN OUT  UINTN           *WriteSize,
  IN      UINTN           Offset
  )
{
  EFI_STATUS           Status;
  FWS_IMAGE_FILE_DATA  *FileData;
  FWS_DEVICE_DATA      *FwsDeviceData;

  if ((ImageFile == NULL) ||
      (Buffer == NULL) ||
      (WriteSize == NULL) ||
      (ImageFile->FwsDevice == NULL) ||
      (ImageFile->Private == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  FileData      = ImageFile->Private;
  FwsDeviceData = ImageFile->FwsDevice->Private;

  if ((Offset + *WriteSize) > ImageFile->MaxSize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Request write out of bounds. WriteOffset %x, BufferSize %x, PartSize %x\n",
      __func__,
      Offset,
      *WriteSize,
      ImageFile->MaxSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((FileData->Flags & FWS_IFD_F_DIRTY) == 0) {
    Status = FwsMetadataSetAcceptState (
               FwsDeviceData->FwsMetadata,
               &ImageFile->ImageTypeGuid,
               FwsDeviceData->UpdateIndex,
               FwsImageWriteUnAcceptReq
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to change accept state... %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = FwsMetadataSave (FwsDeviceData->FwsMetadata);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to save metadata... %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    Status = FwsMetadataCrcCheck (FwsDeviceData->FwsMetadata);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to check crc of metadata... %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    FileData->Flags |= FWS_IFD_F_DIRTY;
  }

  Status = GptWritePartition (
             FwsDeviceData->GptHandle,
             Buffer,
             *WriteSize,
             Offset,
             FileData->StartLba
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to write partition: %d\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Erase whole data in a partition stored related to Image.

  @param[in]  ImageFile                Image File Handle.

  @retval EFI_SUCCESS                  Erase data success.
  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_READY                Storage is not ready.
  @retval Others                       Fail to erase the data on partition.

**/
EFI_STATUS
EFIAPI
FwsErase (
  IN      FWS_IMAGE_FILE  *ImageFile
  )
{
  EFI_STATUS           Status;
  FWS_IMAGE_FILE_DATA  *FileData;
  FWS_DEVICE_DATA      *FwsDeviceData;
  UINTN                EraseSize;
  UINTN                Offset;
  VOID                 *Buffer;

  if ((ImageFile == NULL) ||
      (ImageFile->FwsDevice == NULL) ||
      (ImageFile->Private == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = AllocateRuntimePool (INTERNAL_BUFFER_SIZE);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FileData      = ImageFile->Private;
  FwsDeviceData = ImageFile->FwsDevice->Private;

  /**
   * There would be better way based on device.
   * But In this implementation, Write data with 0x00 to erase.
   */
  ZeroMem (Buffer, INTERNAL_BUFFER_SIZE);

  EraseSize = MIN (INTERNAL_BUFFER_SIZE, FileData->FileSize);
  Offset    = 0;

  while (EraseSize > 0) {
    Status = GptWritePartition (
               FwsDeviceData->GptHandle,
               Buffer,
               EraseSize,
               Offset,
               FileData->StartLba
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to write partition\n"));
      FreePool (Buffer);

      return Status;
    }

    Offset   += EraseSize;
    EraseSize = MIN (INTERNAL_BUFFER_SIZE, FileData->FileSize - Offset);
  }

  FreePool (Buffer);

  FileData->Flags |= FWS_IFD_F_DIRTY;

  return EFI_SUCCESS;
}

/**
  Signal that an image has been accepted.

  @param[in] FwsDevice         Opened firmware storage device instance.
  @param[in] ImgTypeGuid       The image type to be accepted
  @param[in] AcceptUpdateImage If true, Accept the image on the update banks.
                               other accept the image on the active banks.

  @retval EFI_SUCCESS              The image was accepted successfully.
  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_READY            Currently, incorrect boot state.
                                   (boot index != active index).
  @retval EFI_NOT_FOUND            Couldn't find Image related to @ImgTypeGuid.
  @retval Others                   Fail to write Metadata.

**/
EFI_STATUS
EFIAPI
FwsAcceptImage (
  IN  FWS_DEVICE_INSTANCE  *FwsDevice,
  IN CONST EFI_GUID        *ImgTypeGuid,
  IN BOOLEAN               AcceptUpdateImage
  )
{
  EFI_STATUS                 Status;
  UINTN                      BankIdx;
  UINTN                      ImgIdx;
  PSA_MM_FWU_IMG_INFO_ENTRY  *ImageInfoEntry;
  FWS_DEVICE_DATA            *FwsDeviceData;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL) ||
      (ImgTypeGuid == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  if (FwsDeviceData->BootIndex != FwsDeviceData->ActiveIndex) {
    return EFI_NOT_READY;
  }

  if (AcceptUpdateImage) {
    ASSERT (FwsDeviceData->UpdateState == FWS_UPD_ON);
    BankIdx = FwsDeviceData->UpdateIndex;
  } else {
    ASSERT (FwsDeviceData->UpdateState == FWS_UPD_OFF);
    BankIdx = FwsDeviceData->ActiveIndex;
  }

  Status = FwsMetadataSetAcceptState (
             FwsDeviceData->FwsMetadata,
             ImgTypeGuid,
             BankIdx,
             FwsImageAcceptReq
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set accept state on metadata... %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (AcceptUpdateImage) {
    return EFI_SUCCESS;
  }

  Status = FwsMetadataUpdateBankState (FwsDeviceData->FwsMetadata, BankIdx);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set accept state on metadata... %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = FwsMetadataSave (FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to save metadata... %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  for (ImgIdx = 0; ImgIdx < FwsDeviceData->ImageDirectory->NumImages; ImgIdx++) {
    ImageInfoEntry = GET_FWU_IMG_INFO_ENTRY (FwsDeviceData->ImageDirectory, ImgIdx);
    if (CompareGuid (&ImageInfoEntry->ImgTypeGuid, ImgTypeGuid)) {
      ImageInfoEntry->Accepted = FWU_IMAGE_ACCEPTED;

      break;
    }
  }

  return Status;
}

/**
  Start firmware image update process.

  @param [in] FwsDevice        Opened firmware storage device instance.
  @param [in] VendorFlags      Vendor related flags to control firmware update.

  @retval EFI_SUCCESS          firmware update process is started.
  @retval EFI_NOT_READY        Current is incorrect boot state.
                               Couldn't start firmware update.
  @retval Other                Errors

**/
EFI_STATUS
EFIAPI
FwsUpdateStart (
  IN FWS_DEVICE_INSTANCE  *FwsDevice,
  IN UINT32               VendorFlags
  )
{
  FWS_DEVICE_DATA  *FwsDeviceData;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  if (FwsDeviceData->BootIndex != FwsDeviceData->ActiveIndex) {
    return EFI_NOT_READY;
  }

  FwsDeviceData->UpdateState = FWS_UPD_ON;

  return EFI_SUCCESS;
}

/**
  Finish firmware image update process.

  @param [in] FwsDevice      Opened firmware storage device instance.
  @param [in] Abort          If true, cancel the firmware update process for error
                             or client request to cancel update.

  @retval EFI_SUCCESS        Success to finish firmware update process.
  @retval EFI_NOT_READY      firmware update process isn't started.
  @retval EFI_ABORTED        Error on firmware update metadata.

**/
EFI_STATUS
EFIAPI
FwsUpdateEnd (
  IN FWS_DEVICE_INSTANCE  *FwsDevice,
  IN BOOLEAN              Abort
  )
{
  EFI_STATUS       Status;
  FWS_DEVICE_DATA  *FwsDeviceData;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  if (FwsDeviceData->UpdateState != FWS_UPD_ON) {
    return EFI_NOT_READY;
  }

  if (Abort) {
    FwsDeviceData->UpdateState = FWS_UPD_OFF;

    return EFI_SUCCESS;
  }

  Status = FwsMetadataSetPreviousActiveIndex (
             FwsDeviceData->FwsMetadata,
             FwsDeviceData->ActiveIndex
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set prev active index... %r\n",
      __func__,
      Status
      ));
    return EFI_ABORTED;
  }

  Status = FwsMetadataSetActiveIndex (
             FwsDeviceData->FwsMetadata,
             FwsDeviceData->UpdateIndex
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set active index... %r\n",
      __func__,
      Status
      ));
    return EFI_ABORTED;
  }

  Status = FwsMetadataUpdateBankState (
             FwsDeviceData->FwsMetadata,
             FwsDeviceData->UpdateIndex
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to update Banks State... %r\n",
      __func__,
      Status
      ));
    return EFI_ABORTED;
  }

  Status = FwsMetadataSave (FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to save metadata... %r\n", __func__, Status));
    return EFI_ABORTED;
  }

  Status = FwsMetadataCrcCheck (FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to check crc of metadata... %r\n",
      __func__,
      Status
      ));
    return EFI_ABORTED;
  }

  FwsDeviceData->ActiveIndex = FwsDeviceData->UpdateIndex;
  FwsDeviceData->UpdateState = FWS_UPD_OFF;

  ASSERT (FwsDeviceData->ActiveIndex != FwsDeviceData->BootIndex);

  FwsDeviceData->ImageDirectory->CorrectBoot = FALSE;

  return EFI_SUCCESS;
}

/**
  Check if current is on Trial state.
  This function only check the if current in Trial state based on active index.
  In other word, for checking other state (normal state && incorrect boot),
  caller need to get that information by calling other function or
  checking image directory information.

  @param [in]   FwsDevice      Opened firmware storage device instance.
  @param [out]  IsTrialState   If true, It is in Trial State.

  @retval EFI_SUCCESS               Success to get Trial State.
  @retval EFI_INVALID_PARAMETER     Invalid Parameter.

**/
EFI_STATUS
EFIAPI
FwsCheckTrialState (
  IN FWS_DEVICE_INSTANCE  *FwsDevice,
  OUT BOOLEAN             *IsTrialState
  )
{
  FWS_DEVICE_DATA  *FwsDeviceData;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL) ||
      (IsTrialState == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  return FwsMetadataCheckTrialRunState (FwsDeviceData->FwsMetadata, IsTrialState);
}

/**
  Check if booted with active index or not.

  @param [in]   FwsDevice      Opened firmware storage device instance.
  @param [out]  IsCorrectBoot  If true, It is in Correct Boot.

  @retval EFI_SUCCESS               Success to get Correct Boot.
  @retval EFI_INVALID_PARAMETER     Invalid Parameter.

**/
EFI_STATUS
EFIAPI
FwsCheckCorrectBoot (
  IN FWS_DEVICE_INSTANCE  *FwsDevice,
  OUT BOOLEAN             *IsCorrectBoot
  )
{
  FWS_DEVICE_DATA  *FwsDeviceData;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL) ||
      (IsCorrectBoot == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  *IsCorrectBoot = (FwsDeviceData->BootIndex == FwsDeviceData->ActiveIndex);

  return EFI_SUCCESS;
}

/**
  Roll back the image.
  Based on the firmware update started or not,
  This function choice proper backup image.

  @param [in]   FwsDevice      Opened firmware storage device instance.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER   Invalid Parameter.
  @retval EFI_DEVICE_ERROR        Error on metadata.
  @retval Others                  Fail to rollback the image.

**/
EFI_STATUS
EFIAPI
FwsRollBack (
  IN FWS_DEVICE_INSTANCE  *FwsDevice
  )
{
  EFI_STATUS       Status;
  FWS_DEVICE_DATA  *FwsDeviceData;
  UINT32           BackupIdx;
  UINT32           TargetIdx;
  UINT32           PreviousActiveIndex;

  if ((FwsDevice == NULL) ||
      (FwsDevice->Private == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  FwsDeviceData = FwsDevice->Private;

  Status = FwsMetadataCrcCheck (FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to check crc of metadata before rollback.\n",
      __func__
      ));
    return EFI_DEVICE_ERROR;
  }

  if (FwsDeviceData->UpdateState == FWS_UPD_ON) {
    BackupIdx = FwsDeviceData->ActiveIndex;
    TargetIdx = FwsDeviceData->UpdateIndex;
  } else {
    Status = FwsMetadataGetPreviousActiveIndex (
               FwsDeviceData->FwsMetadata,
               &PreviousActiveIndex
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get previous active index... %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    BackupIdx = PreviousActiveIndex;
    TargetIdx = FwsDeviceData->ActiveIndex;
  }

  Status = InternalFwsRollBackBank (FwsDeviceData, BackupIdx, TargetIdx);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to error rollback (from:%d to%d).\n",
      __func__,
      BackupIdx,
      TargetIdx
      ));
    return Status;
  }

  Status = FwsMetadataCrcCheck (FwsDeviceData->FwsMetadata);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to check crc of metadata after rollback.\n",
      __func__
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
