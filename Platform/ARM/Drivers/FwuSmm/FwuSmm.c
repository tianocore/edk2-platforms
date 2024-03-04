/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  FwuStmm.c receives firmware update protocol ABI request from PsaFmpLib
  and handle it by accessing the FwStore via FWS protocol.

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile Specification 1.0
    (https://developer.arm.com/documentation/den0118/latest)

  @par Glossary:
    - FW    - Firmware
    - FWU   - Firmware Update
    - FWS   - Firmware Storage
    - PSA   - Platform Security update for the A-profile specification
    - IFD   - Image File Descriptor
    - IF    - Image File
    - IMG   - Image
    - MM    - Management Mode
    - PROP  - Property

**/

/**
 * Just like simple file systems -- file == IFD, inode == IF,
 * open file is managed by file descriptor and
 * it occupies space in the file descriptor table.
 * However, different from general file system,
 * It dosen't have concept of dentry and
 * one image doesn't need to open multiple times while update image.
 * That means in fdtable, there's only one open image file per image.
 *
 * Therefore, in mFdTable, statically allocate for each image file
 * and doesn't allow to open multiple times for one image.
 *
 * So, mFdTable consists of like:
 * +--------------------------------+
 * |  IFD[0] - Image File Directory |
 * +--------------------------------+
 * |  IFD[1] - First Image          |
 * +--------------------------------+
 * |  IFD[2] - Second Image         |
 * +--------------------------------+
 * |  IFD[.] - ...                  |
 * +--------------------------------+
 * |  IFD[n] - Nth Image            |
 * +--------------------------------+
 *
 * And each dedicate IFD managed open status of each Image File.
 */

#include <PiMm.h>

#include <IndustryStandard/PsaMmFwUpdate.h>

#include <Library/ArmMmHandlerContext.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/MmServicesTableLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/MmCommunication2.h>

#include <Library/FwsPlatformLib.h>

#define PSA_FWU_ABI_MAJOR_VERSION  1
#define PSA_FWU_ABI_MINOR_VERSION  0

/**
 * Image File Descriptor Flags
 */
#define IFD_F_STATUS_STAGING  (1 << 0)   /**< Image File staging in progress */

#define IFD_IS_OPENED(Ifd)          ((((Ifd)->ImageFile != NULL)))
#define IFD_IS_STATUS_STAGING(Ifd)  ((BOOLEAN) ((((Ifd)->Status) & IFD_F_STATUS_STAGING) != 0))

/**
 * See the PSA specification 3.2.1 Firmware Store state machine.
 */
typedef enum {
  FW_STORE_REGULAR,   /**< all the image currently active have been accepted */
  FW_STORE_STAGING,   /**< updating image(s) in firmware storage is undergoing */
  FW_STORE_TRIAL,     /**< the system updated firmware storage but not accepted yet*/
  FW_STORE_STATE_MAX,
} FW_STORE_STATE;

/**
 * Clear staging mode.
 */
typedef enum {
  CLEAR_NORMAL,           /**< clear staging for normal end */
  CLEAR_ERROR,            /**< used to cleanup when error happen */
  CLEAR_CANCEL,           /**< clear staging for cancel request */
  CLEAR_STAGING_MODE_MAX,
} CLEAR_STAGING_MODE;

/**
 * Function protoype for handling firmware update ABI .
 */
typedef INT32 (EFIAPI *FwuSmmFunc)(PSA_MM_FWU_CMD_DATA *, UINTN);

typedef struct {
  /// Image Type Guid.
  EFI_GUID          ImageTypeGuid;

  /// Image File
  FWS_IMAGE_FILE    *ImageFile;

  /// ClientPermission See PSA spec 3.3.1 Image directory
  UINT32            ClientPermissions;

  /// Current Pos
  UINTN             Pos;

  /// Opened Operation Type
  FWU_OP_TYPE       OpType;

  /// See the IFD_F_STATUS_*
  UINT64            Status;
} IMAGE_FILE_DESCRIPTOR;

STATIC EFI_MMRAM_DESCRIPTOR        *mNsCommBufferRange;
STATIC EFI_MM_SYSTEM_TABLE         *mMmst           = NULL;
STATIC UINT32                      mStagingFlags    = 0;
STATIC IMAGE_FILE_DESCRIPTOR       *mFdTable        = NULL;
STATIC UINTN                       mFdTableSize     = 0;
STATIC PSA_MM_FWU_IMAGE_DIRECTORY  *mImageDirectory = NULL;
STATIC FWS_DEVICE_INSTANCE         *mFwsDevice      = NULL;
STATIC FW_STORE_STATE              mFwState         = FW_STORE_REGULAR;
STATIC UINT32                      mFwuFlags        = 0;
STATIC UINT32                      mFwuVendorFlags  = 0;

// Initialise the Service status to error init by default.
STATIC UINT32  mServiceStatus = SERVICE_STATUS_ERR_INIT;

// Firmware update ABI function array.
STATIC FwuSmmFunc  mFwuSmmFuncArray[PSA_MM_FWU_COMMAND_MAX_ID];
STATIC UINT16      mFwuSmmNumFunc = 0;

/**
 * Get Opened Image File Descriptor from fd-tables by handle.
 *
 * @param [in]   Handle       File Handle.
 * @param [out]  Ifd          Image File Descriptor.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_INVALID_PARAMETER  Invalid Image File Handle.
 */
STATIC
EFI_STATUS
EFIAPI
GetIfdByHandle (
  IN    UINT32                 Handle,
  OUT   IMAGE_FILE_DESCRIPTOR  **Ifd
  )
{
  if (Handle > mFdTableSize) {
    *Ifd = NULL;

    return EFI_INVALID_PARAMETER;
  }

  *Ifd = &mFdTable[Handle];

  if (!IFD_IS_OPENED (*Ifd)) {
    *Ifd = NULL;

    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
 * Get Image File Descriptor matched with @ImageTypeGuid from fd-tables.
 * Regardless of opened status, it returns related Image File Descriptor.
 *
 * @param [in]    ImageTypeGuid     Image Type Guid.
 * @param [out]   Ifd               Image File Descriptor.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            There's no Image file descriptor matched
 *                                  with @ImageTypeGuid.
 */
STATIC
EFI_STATUS
EFIAPI
GetIfdByGuid (
  IN CONST EFI_GUID          *ImageTypeGuid,
  OUT IMAGE_FILE_DESCRIPTOR  **Ifd
  )
{
  UINTN  Idx;

  *Ifd = NULL;

  for (Idx = 0; Idx < mFdTableSize; Idx++) {
    if (CompareGuid (&mFdTable[Idx].ImageTypeGuid, ImageTypeGuid)) {
      *Ifd = &mFdTable[Idx];

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
 * Clear staging status of image file descriptors.
 *
 * @param [in]   Guids      Pointer to an array of GUIDs to clear staging status.
 *                          If NULL, it iterates all image file descriptors.
 * @param [in]   Count      Number of guids.
 * @param [in]   Mode       Clear staging mode.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY    On end staging, there's any opened image file descriptor.
 */
STATIC
EFI_STATUS
EFIAPI
ClearStaging (
  IN EFI_GUID            *Guids,
  IN UINTN               Count,
  IN CLEAR_STAGING_MODE  Mode
  )
{
  EFI_STATUS             Status;
  UINTN                  Idx;
  IMAGE_FILE_DESCRIPTOR  *Ifd;

  for (Idx = 0; Idx < Count; Idx++) {
    if (Guids != NULL) {
      Status = GetIfdByGuid (&Guids[Idx], &Ifd);
      if (EFI_ERROR (Status)) {
        /**
         * This happens only when ClearStaging called with CLEAR_ERROR mode
         * to close former opened handle while SetStaging with Guids array.
         * Therefore, in case of invalid Guid found, it can skip it safely.
         */
        continue;
      }
    } else {
      Ifd = &mFdTable[Idx];
    }

    if (IFD_IS_OPENED (Ifd)) {
      if (Mode == CLEAR_CANCEL) {
        /**
         * This is called when cancel-staging.
         * When end or cancel of staging, all of opened file should be closed.
         */
        Ifd->ImageFile->Flags |= FWS_IF_F_IGNORE_DIRTY;
        if (!CompareGuid (&Ifd->ImageFile->ImageTypeGuid, &gPsaFwuImageDirectoryGuid)) {
          Status = FwsRelease (Ifd->ImageFile, 0, NULL, NULL);
        } else {
          FreePool (Ifd->ImageFile);
          Status = EFI_SUCCESS;
        }

        ASSERT (Status == EFI_SUCCESS);
        Ifd->Pos       = 0;
        Ifd->OpType    = FwuOpStreamRead;
        Ifd->ImageFile = NULL;
      } else if (Mode == CLEAR_ERROR) {
        /**
         * CLEAR_ERROR Mode could be called when an error occurred during the begining staging.
         * This also implies that there is no other image file opened with write permission.
         */
        ASSERT (Ifd->OpType != FwuOpStreamWrite);

        continue;
      } else {
        /**
         * See the PSA spec 4.3.2.3 fwu_end_staging.
         * This is called when end-staging.
         * When end of staging, all of opened file should be closed.
         * If there's open file in end staging, it should return error.
         */
        return EFI_NOT_READY;
      }
    }

    Ifd->Status &= ~(IFD_F_STATUS_STAGING);
  }

  return EFI_SUCCESS;
}

/**
 * Set staging status before firmware update.
 *
 * @param [in]   Guids      Array of guids to clear staging status.
 *                          If NULL, it iterates all image file descriptors.
 * @param [in]   Count      Number of guids.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND    There is no matched image file descriptor with @Guids.
 * @retval EFI_NOT_READY    Some of Image is opened.
 *
 */
STATIC
EFI_STATUS
EFIAPI
SetStaging (
  IN EFI_GUID  *Guids,
  IN UINTN     Count
  )
{
  EFI_STATUS             Status;
  UINTN                  Idx;
  IMAGE_FILE_DESCRIPTOR  *Ifd;

  if (Guids == NULL) {
    Count = mFdTableSize;
  }

  for (Idx = 0; Idx < Count; Idx++) {
    if (Guids != NULL) {
      Status = GetIfdByGuid (&Guids[Idx], &Ifd);
      if (EFI_ERROR (Status)) {
        ClearStaging (Guids, Idx, CLEAR_ERROR);

        return EFI_NOT_FOUND;
      }
    } else {
      Ifd = &mFdTable[Idx];
    }

    if (IFD_IS_OPENED (Ifd)) {
      if (Ifd->OpType == FwuOpStreamRead) {
        ClearStaging (Guids, Idx, CLEAR_ERROR);

        return EFI_NOT_READY;
      } else {
        /**
         * Before begin staging, It's impossible to have any opened image
         * file descriptor with write-permission.
         */
        ASSERT (0);
      }
    }

    if (!CompareGuid (&Ifd->ImageTypeGuid, &gPsaFwuImageDirectoryGuid)) {
      Ifd->Status |= IFD_F_STATUS_STAGING;
    }
  }

  return EFI_SUCCESS;
}

/**
 * Return the request data size based on the Firmware Update function id.
 *
 * @param [in]  Command         Firmware Update function id.
 *
 * @retval Size of request data based on @Command.
 */
STATIC
UINTN
EFIAPI
GetReqDataSize (
  IN UINT32  Command
  )
{
  switch (Command) {
    case PSA_MM_FWU_COMMAND_BEGIN_STAGING:
      return sizeof (PSA_MM_FWU_BEGIN_STAGING_REQ);
    case PSA_MM_FWU_COMMAND_OPEN:
      return sizeof (PSA_MM_FWU_OPEN_REQ);
    case PSA_MM_FWU_COMMAND_WRITE_STREAM:
      return sizeof (PSA_MM_FWU_WRITE_STREAM_REQ);
    case PSA_MM_FWU_COMMAND_READ_STREAM:
      return sizeof (PSA_MM_FWU_READ_STREAM_REQ);
    case PSA_MM_FWU_COMMAND_COMMIT:
      return sizeof (PSA_MM_FWU_COMMIT_REQ);
    case PSA_MM_FWU_COMMAND_ACCEPT_IMAGE:
      return sizeof (PSA_MM_FWU_ACCEPT_IMAGE_REQ);
    default:
      return 0;
  }
}

/**
 * Return the response data size based on Firmware Update function id.
 *
 * @param [in]  Command         Firmware Update function id.
 *
 * @retval Size of response data based on @Command.
 */
STATIC
UINTN
EFIAPI
GetRespDataSize (
  IN UINT32  Command
  )
{
  switch (Command) {
    case PSA_MM_FWU_COMMAND_DISCOVER:
      return sizeof (PSA_MM_FWU_DISCOVER_RESP) +
             (mFwuSmmNumFunc * sizeof (UINT16));
    case PSA_MM_FWU_COMMAND_OPEN:
      return sizeof (PSA_MM_FWU_OPEN_RESP);
    case PSA_MM_FWU_COMMAND_READ_STREAM:
      return sizeof (PSA_MM_FWU_READ_STREAM_RESP);
    case PSA_MM_FWU_COMMAND_COMMIT:
      return sizeof (PSA_MM_FWU_COMMIT_RESP);
    default:
      return 0;
  }
}

/**
 * Get required buffer size for handling firmware update function.
 *
 * @param [in] Command          Firmware Update ABI function id.
 *
 * @retval Size of buffer required to handle firmware update function.
 */
STATIC
UINTN
EFIAPI
GetRequiredBufferSize (
  IN UINT32  Command
  )
{
  return MAX (GetReqDataSize (Command), GetRespDataSize (Command)) +
         sizeof (PSA_MM_FWU_PARAMETER_HEADER);
}

/**
 * Initialize Image File Descriptor table.
 * In implementation, it allows only one opened image per image type guid.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_INVALID_PARAMETER        Image directory isn't initialized.
 * @retval EFI_OUT_OF_RESOURCE          Out of Memory.
 */
STATIC
EFI_STATUS
EFIAPI
InitImageFileDescTable (
  IN VOID
  )
{
  UINTN                      Idx;
  IMAGE_FILE_DESCRIPTOR      *Ifd;
  PSA_MM_FWU_IMG_INFO_ENTRY  *ImgInfo;

  if (mImageDirectory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  mFdTableSize = mImageDirectory->NumImages + 1;
  mFdTable     = AllocateRuntimePool (sizeof (IMAGE_FILE_DESCRIPTOR) * mFdTableSize);
  if (mFdTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Init Image Directory Image File Descriptor.
  Ifd = &mFdTable[0];
  CopyGuid (&Ifd->ImageTypeGuid, &gPsaFwuImageDirectoryGuid);
  Ifd->ImageFile         = NULL;
  Ifd->ClientPermissions = FWU_READ_PERMISSION;
  Ifd->Pos               = 0;
  Ifd->OpType            = FwuOpStreamRead;
  Ifd->Status            = 0;

  for (Idx = 1; Idx < mFdTableSize; Idx++) {
    Ifd     = &mFdTable[Idx];
    ImgInfo = GET_FWU_IMG_INFO_ENTRY (mImageDirectory, Idx - 1);
    CopyGuid (&Ifd->ImageTypeGuid, &ImgInfo->ImgTypeGuid);
    Ifd->ImageFile         = NULL;
    Ifd->ClientPermissions = ImgInfo->ClientPermissions;
    Ifd->Pos               = 0;
    Ifd->OpType            = FwuOpStreamRead;
    Ifd->Status            = 0;
  }

  return EFI_SUCCESS;
}

/**
 * Handler for fwu_discover ABI.
 * Get implemented firmware update function features,
 * Max buffer size for communicate with client, vendor specific information
 * and etc.
 *
 * See the PSA spec 3.4.2.1. all of return values based on the specification.
 *
 * @param [in,out]  Message        Request passed from UpdateClient (NS).
 *                                 Buffer for Response data.
 * @param [in]      MessageSize    Size of Message in bytes.
 *
 * @retval PSA_MM_FWU_SUCCESS
 */
STATIC
INT32
EFIAPI
FwuSmmDiscover (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  PSA_MM_FWU_DISCOVER_RESP  *RespData;
  UINT64                    MaxWritePayload;
  UINT64                    MaxReadPayload;
  UINT16                    Idx;
  UINT16                    *FunctionPresence;

  RespData = (PSA_MM_FWU_DISCOVER_RESP *)GET_FWU_DATA_BUFFER (Message);

  RespData->ServiceStatus       = mServiceStatus;
  RespData->VersionMajor        = PSA_FWU_ABI_MAJOR_VERSION;
  RespData->VersionMinor        = PSA_FWU_ABI_MINOR_VERSION;
  RespData->OffFunctionPresence = sizeof (PSA_MM_FWU_DISCOVER_RESP);
  RespData->NumFunc             = mFwuSmmNumFunc;

  MaxWritePayload = mNsCommBufferRange->PhysicalSize -
                    sizeof (EFI_MM_COMMUNICATE_HEADER) -
                    GetRequiredBufferSize (PSA_MM_FWU_COMMAND_WRITE_STREAM);

  MaxReadPayload = mNsCommBufferRange->PhysicalSize -
                   sizeof (EFI_MM_COMMUNICATE_HEADER) -
                   GetRequiredBufferSize (PSA_MM_FWU_COMMAND_READ_STREAM);

  /**
   * MaxPayloadSize used to get maximum payload size when fwu_read_stream and
   * fwu_write_stream operations.
   * Because it doesn't distinguish each read/write maximum size in
   * fwu_discover's response,
   * It should set the minimum of maximum payload among read/write stream
   * to prevent one of operation failure because of over sized payload.
   */
  RespData->MaxPayloadSize = MIN (MaxReadPayload, MaxWritePayload);

  /**
   * Not Support Partial Image Update.
   */
  RespData->Flags = mFwuFlags;

  RespData->VendorSpecificFlags = mFwuVendorFlags;

  FunctionPresence = (UINT16 *)GET_FWU_DATA_BUFFER (RespData);

  for (Idx = 0; Idx < PSA_MM_FWU_COMMAND_MAX_ID; Idx++) {
    if (mFwuSmmFuncArray[Idx] != NULL) {
      *FunctionPresence = Idx;
      FunctionPresence++;
    }
  }

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_begin_staging ABI.
 * start new staging process for firmware update.
 *
 * See the PSA spec 3.4.2.2. all of return values based on the specification.
 *
 * @param [in,out]   Message        Request passed from UpdateClient (NS)
 *                                  Buffer for Response data.
 * @param [in]       MessageSize    Size of Message in bytes.

 *
 * @retval PSA_MM_FWU_SUCCESS
 * @retval PSA_MM_FWU_DENIED        The firmware Store is in the Trial State
 *                                  or the platform did not boot correctly.
 * @retval PSA_MM_FWU_BUSY          Temporarily couldn't enter Staging state.
 * @retval PSA_MM_UNKNOWN           One of more GUIDs couldn't find in image directory.
 */
STATIC
INT32
EFIAPI
FwuSmmBeginStaging (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS                    Status;
  PSA_MM_FWU_BEGIN_STAGING_REQ  *ReqData;
  EFI_GUID                      *UpdateGuid;
  BOOLEAN                       IsCorrectBoot;
  BOOLEAN                       IsTrialState;

  FwsCheckCorrectBoot (mFwsDevice, &IsCorrectBoot);
  FwsCheckTrialState (mFwsDevice, &IsTrialState);

  if (!IsCorrectBoot || IsTrialState) {
    return PSA_MM_FWU_DENIED;
  }

  ReqData = (PSA_MM_FWU_BEGIN_STAGING_REQ *)GET_FWU_DATA_BUFFER (Message);

  if (((mFwuFlags & FWU_DISCOVER_FLAGS_PARTIAL_UPDATE_SUPPORT) == 0) &&
      (ReqData->PartialUpdateCount > 0))
  {
    return PSA_MM_FWU_BUSY;
  }

  if (ReqData->PartialUpdateCount > 0) {
    UpdateGuid = (EFI_GUID *)GET_FWU_DATA_BUFFER (ReqData);

    Status = SetStaging (UpdateGuid, ReqData->PartialUpdateCount);
  } else {
    Status = SetStaging (NULL, 0);
  }

  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      return PSA_MM_FWU_UNKNOWN;
    }

    return PSA_MM_FWU_BUSY;
  }

  Status = FwsUpdateStart (mFwsDevice, ReqData->VendorFlags & mFwuVendorFlags);
  if (EFI_ERROR (Status)) {
    ClearStaging (
      UpdateGuid,
      ReqData->PartialUpdateCount,
      CLEAR_ERROR
      );

    return PSA_MM_FWU_BUSY;
  }

  mFwState      = FW_STORE_STAGING;
  mStagingFlags = ReqData->VendorFlags;

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Current FwState: %d\n", mFwState));

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_end_staging.
 * quit the firmware update staging state.
 *
 * See the PSA spec 3.4.2.3. all of return values based on the specification.
 *
 * @param [in,out]   Message        Request passed from UpdateClient (NS).
 *                                  Buffer for Response data (Not used).
 * @param [in]       MessageSize    Size of Message in bytes (Not used).
 *
 * @retval PSA_MM_FWU_SUCCESS
 * @retval PSA_MM_FWU_DENIED         Current state isn't Staging State.
 * @retval PSA_MM_FWU_AUTH_FAIL      one of updated image fail to be authenticated
 * @retval PSA_MM_FWU_BUSY           There are open image handles.
 * @retval PSA_MM_FWU_NOT_AVAILABLE  Doesn't support partial update or
 *                                   the client has not updated all the image.
 */
STATIC
INT32
EFIAPI
FwuSmmEndStaging (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsTrialState;

  if (mFwState != FW_STORE_STAGING) {
    return PSA_MM_FWU_DENIED;
  }

  Status = ClearStaging (NULL, mFdTableSize, CLEAR_NORMAL);
  if (EFI_ERROR (Status)) {
    return PSA_MM_FWU_BUSY;
  }

  Status = FwsUpdateEnd (mFwsDevice, FALSE);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_SECURITY_VIOLATION) {
      return PSA_MM_FWU_AUTH_FAIL;
    }

    return PSA_MM_FWU_NOT_AVAILABLE;
  }

  FwsCheckTrialState (mFwsDevice, &IsTrialState);

  if (IsTrialState) {
    mFwState = FW_STORE_TRIAL;
  } else {
    mFwState = FW_STORE_REGULAR;
  }

  mStagingFlags = 0;

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Current FwState: %d\n", mFwState));

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_cancel_staging ABI.
 * Cancel current staging procedure.
 *
 * See the PSA spec 3.4.2.4. all of return values based on the specification.
 *
 * @param [in,out]   Message        Request passed from UpdateClient (NS).
 *                                  Buffer for Response data (Not used).
 * @param [in]       MessageSize    Size of Message in bytes (Not used).
 *
 * @retval PSA_MM_FWU_SUCCESS.
 * @retval PSA_MM_FWU_DENIED       The system is not in a Staging state.
 */
STATIC
INT32
EFIAPI
FwuSmmCancelStaging (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS  Status;

  if (mFwState != FW_STORE_STAGING) {
    return PSA_MM_FWU_DENIED;
  }

  Status = ClearStaging (NULL, mFdTableSize, CLEAR_CANCEL);
  ASSERT (Status == EFI_SUCCESS);

  /**
   * In the implementation, all of write buffer is written on storage
   * in fwu_write_stream.
   * So, if we cancel the staging, we couldn't boot with using update indexed
   * bank (In A/B firmware storage design, update index == previous index).
   *
   * To make previous index bootable always, In case of canceling staging,
   * write update index bank with active index bank's firmware image.
   */
  Status = FwsRollBack (mFwsDevice);
  ASSERT (Status == EFI_SUCCESS);

  Status = FwsUpdateEnd (mFwsDevice, TRUE);
  ASSERT (Status == EFI_SUCCESS);

  mFwState      = FW_STORE_REGULAR;
  mStagingFlags = 0;

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Current FwState: %d\n", mFwState));

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_open ABI.
 * Open a image file handle with matched guid.
 *
 * See the PSA spec 3.4.2.5. all of return values based on the specification.
 *
 * @param [in,out]   Message       Request passed from UpdateClient (NS).
 *                                 Buffer for Response data.
 * @param [in]       MessageSize   Size of Message in bytes.
 *
 * @retval PSA_MM_FWU_SUCCESS        The file was open, and Handle is valid.
 * @retval PSA_MM_FWU_UNKNOWN        Couldn't find matched image with passed image type guid.
 * @retval PSA_MM_FWU_DENIED         For write open request, the image being open is not on staging state.
 * @retval PSA_MM_FWU_NOT_AVAILABLE  No permission to open image.
 */
STATIC
INT32
EFIAPI
FwuSmmOpen (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS                 Status;
  IMAGE_FILE_DESCRIPTOR      *Ifd = NULL;
  CONST PSA_MM_FWU_OPEN_REQ  *ReqData;
  PSA_MM_FWU_OPEN_RESP       *RespData;

  ReqData  = (CONST PSA_MM_FWU_OPEN_REQ *)GET_FWU_DATA_BUFFER (Message);
  RespData = (PSA_MM_FWU_OPEN_RESP *)GET_FWU_DATA_BUFFER (Message);

  if ((ReqData->OperationType != FwuOpStreamRead) &&
      (ReqData->OperationType != FwuOpStreamWrite))
  {
    return PSA_MM_FWU_NOT_AVAILABLE;
  }

  Status = GetIfdByGuid (&ReqData->ImageTypeGuid, &Ifd);
  if (EFI_ERROR (Status)) {
    return PSA_MM_FWU_UNKNOWN;
  }

  ASSERT (Ifd != NULL);

  if (ReqData->OperationType == FwuOpStreamWrite) {
    if ((Ifd->ClientPermissions & FWU_WRITE_PERMISSION) == 0) {
      return PSA_MM_FWU_NOT_AVAILABLE;
    }

    if (!IFD_IS_STATUS_STAGING (Ifd)) {
      return PSA_MM_FWU_DENIED;
    }
  }

  if ((ReqData->OperationType == FwuOpStreamRead) &&
      ((Ifd->ClientPermissions & FWU_READ_PERMISSION) == 0))
  {
    return PSA_MM_FWU_NOT_AVAILABLE;
  }

  if (!CompareGuid (&ReqData->ImageTypeGuid, &gPsaFwuImageDirectoryGuid)) {
    Status = FwsOpen (
               mFwsDevice,
               &ReqData->ImageTypeGuid,
               ReqData->OperationType,
               &Ifd->ImageFile
               );
    if (EFI_ERROR (Status)) {
      return PSA_MM_FWU_NOT_AVAILABLE;
    }
  } else {
    Ifd->ImageFile = AllocateRuntimePool (sizeof (FWS_IMAGE_FILE));
    if (Ifd->ImageFile == NULL) {
      return PSA_MM_FWU_NOT_AVAILABLE;
    }

    ZeroMem (Ifd->ImageFile, sizeof (FWS_IMAGE_FILE));

    CopyGuid (&Ifd->ImageFile->ImageTypeGuid, &gPsaFwuImageDirectoryGuid);
    Ifd->ImageFile->FwsDevice = NULL;
    Ifd->ImageFile->MaxSize   = (sizeof (PSA_MM_FWU_IMAGE_DIRECTORY) +
                                 (mImageDirectory->ImgInfoSize * mImageDirectory->NumImages));
    Ifd->ImageFile->FileSize = (sizeof (PSA_MM_FWU_IMAGE_DIRECTORY) +
                                (mImageDirectory->ImgInfoSize * mImageDirectory->NumImages));
  }

  Ifd->Pos    = 0;
  Ifd->OpType = ReqData->OperationType;

  RespData->Handle = (UINT32)(Ifd - mFdTable);

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_write_stream ABI.
 * Write the new firmware image related to opened handle to update indexed bank.
 *
 * See the PSA spec 3.4.2.6. all of return values based on the specification.
 *
 * @param [in,out]   Message       Request passed from UpdateClient (NS).
 *                                 Buffer for Response data.
 * @param [in]       MessageSize   Size of Message in bytes.
 *
 * @retval PSA_MM_FWU_SUCCESS
 * @retval PSA_MM_FWU_UNKNOWN        Invalid image file handle.
 * @retval PSA_MM_FWU_OUT_OF_BOUND   Out of boundary of image size.
 * @retval PSA_MM_FWU_NO_PERMISSION  Permission error.
 * @retval PSA_MM_FWU_DENIED         The system is not staging state.
 *
 */
STATIC
INT32
EFIAPI
FwuSmmWriteStream (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS                         Status;
  IMAGE_FILE_DESCRIPTOR              *Ifd = NULL;
  CONST PSA_MM_FWU_WRITE_STREAM_REQ  *ReqData;
  VOID                               *WriteBuffer;
  UINTN                              WriteSize;

  if (mFwState != FW_STORE_STAGING) {
    return PSA_MM_FWU_DENIED;
  }

  ReqData = (CONST PSA_MM_FWU_WRITE_STREAM_REQ *)GET_FWU_DATA_BUFFER (Message);

  Status = GetIfdByHandle (ReqData->Handle, &Ifd);
  if (EFI_ERROR (Status)) {
    return PSA_MM_FWU_UNKNOWN;
  }

  ASSERT (Ifd != NULL);

  if (Ifd->OpType != FwuOpStreamWrite) {
    return PSA_MM_FWU_NO_PERMISSION;
  }

  WriteSize   = ReqData->DataLen;
  WriteBuffer = (VOID *)GET_FWU_DATA_BUFFER (ReqData);

  if (WriteSize == 0) {
    return PSA_MM_FWU_SUCCESS;
  }

  if (Ifd->Pos + WriteSize > Ifd->ImageFile->MaxSize) {
    return PSA_MM_FWU_OUT_OF_BOUNDS;
  }

  Status = FwsWrite (Ifd->ImageFile, WriteBuffer, &WriteSize, Ifd->Pos);
  if (EFI_ERROR (Status)) {
    return PSA_MM_FWU_NO_PERMISSION;
  }

  Ifd->Pos += ReqData->DataLen;

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_read_stream ABI.
 * Read new firmware image related to image file handle from boot indexed bank.
 *
 * See the PSA spec 3.4.2.7. all of return values based on the specification.
 *
 * NOTE:
 *    Currently, we don't allow to read firmware image except Image Directory.
 *
 * @param [in,out]   Message       Request passed from UpdateClient (NS).
 *                                 Buffer for Response data.
 * @param [in]       MessageSize   Size of Message in bytes.
 *
 * @retval PSA_MM_FWU_SUCCESS
 * @retval PSA_MM_FWU_UNKNOWN        Invalid Image file Handle.
 * @retval PSA_MM_FWU_NO_PERMISSION  Permission error.
 * @retval PSA_MM_FWU_DENIED         The image cannot be temporarily read.
 *
 */
STATIC
INT32
EFIAPI
FwuSmmReadStream (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS                        Status;
  IMAGE_FILE_DESCRIPTOR             *Ifd = NULL;
  CONST PSA_MM_FWU_READ_STREAM_REQ  *ReqData;
  PSA_MM_FWU_READ_STREAM_RESP       *RespData;
  VOID                              *ReadBuffer;
  UINTN                             ReadSize;

  ReqData  = (CONST PSA_MM_FWU_READ_STREAM_REQ *)GET_FWU_DATA_BUFFER (Message);
  RespData = (PSA_MM_FWU_READ_STREAM_RESP *)GET_FWU_DATA_BUFFER (Message);

  Status = GetIfdByHandle (ReqData->Handle, &Ifd);
  if (EFI_ERROR (Status)) {
    return PSA_MM_FWU_UNKNOWN;
  }

  ASSERT (Ifd != NULL);

  if (Ifd->OpType != FwuOpStreamRead) {
    return PSA_MM_FWU_NO_PERMISSION;
  }

  if (Ifd->Pos >= Ifd->ImageFile->FileSize) {
    return PSA_MM_FWU_OUT_OF_BOUNDS;
  }

  ReadSize = MessageSize - sizeof (PSA_MM_FWU_PARAMETER_HEADER) -
             GetRespDataSize (PSA_MM_FWU_COMMAND_READ_STREAM);

  ReadSize = MIN (ReadSize, Ifd->ImageFile->FileSize - Ifd->Pos);

  if (ReadSize == 0) {
    RespData->ReadBytes  = 0;
    RespData->TotalBytes = Ifd->ImageFile->FileSize;

    return PSA_MM_FWU_SUCCESS;
  }

  ReadBuffer = GET_FWU_DATA_BUFFER (RespData);

  if (CompareGuid (&Ifd->ImageFile->ImageTypeGuid, &gPsaFwuImageDirectoryGuid)) {
    ASSERT (mImageDirectory != NULL);
    CopyMem (ReadBuffer, (VOID *)((UINT8 *)mImageDirectory + Ifd->Pos), ReadSize);
  } else {
    Status = FwsRead (Ifd->ImageFile, ReadBuffer, &ReadSize, Ifd->Pos);
    if (EFI_ERROR (Status)) {
      return PSA_MM_FWU_NO_PERMISSION;
    }
  }

  RespData->ReadBytes  = ReadSize;
  RespData->TotalBytes = Ifd->ImageFile->FileSize;

  Ifd->Pos += ReadSize;

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_commit ABI.
 * This closes opened image file handle and
 * If required, applies new written firmware image to storage and
 * renewal firmware metadata with updated banks.
 *
 * See the PSA spec 3.4.2.8. all of return values based on the specification.
 *
 * @param [in,out]   Message       Request passed from UpdateClient (NS).
 *                                 Buffer for Response data.
 * @param [in]       MessageSize   Size of Message in bytes.

 * @retval PSA_MM_FWU_SUCCESS
 * @retval PSA_MM_FWU_UNKNOWN        Invalid image file handle.
 * @retval PSA_MM_FWU_AUTH_FAIL      Fail to authenticate new updated images
 * @retval PSA_MM_FWU_RESUME         Update Agent yielded, the client must try again.
 * @retval PSA_MM_FWU_DENIED         Image only be accepted after activation.
 *                                   couldn't be accepted on commit procedure.
 */
STATIC
INT32
EFIAPI
FwuSmmCommit (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS                   Status;
  INT32                        FwuStatus;
  IMAGE_FILE_DESCRIPTOR        *Ifd;
  CONST PSA_MM_FWU_COMMIT_REQ  *ReqData;
  PSA_MM_FWU_COMMIT_RESP       *RespData;
  BOOLEAN                      IsCorrectBoot;

  ReqData  = (CONST PSA_MM_FWU_COMMIT_REQ *)GET_FWU_DATA_BUFFER (Message);
  RespData = (PSA_MM_FWU_COMMIT_RESP *)GET_FWU_DATA_BUFFER (Message);

  Status = GetIfdByHandle (ReqData->Handle, &Ifd);
  if (EFI_ERROR (Status)) {
    return PSA_MM_FWU_UNKNOWN;
  }

  if (CompareGuid (&Ifd->ImageFile->ImageTypeGuid, &gPsaFwuImageDirectoryGuid)) {
    FreePool (Ifd->ImageFile);
    FwuStatus = PSA_MM_FWU_SUCCESS;

    goto ErrorHandler;
  }

  FwsCheckCorrectBoot (Ifd->ImageFile->FwsDevice, &IsCorrectBoot);

  if (ReqData->AcceptanceReq == 0) {
    if ((Ifd->ClientPermissions & FWU_ACCEPT_AFTER_ACTIVATION) &&
        ((Ifd->OpType == FwuOpStreamWrite) || !IsCorrectBoot))
    {
      return PSA_MM_FWU_DENIED;
    }

    if ((Ifd->OpType == FwuOpStreamWrite) && (Ifd->Pos == 0)) {
      return PSA_MM_FWU_DENIED;
    }
  }

  if ((Ifd->OpType == FwuOpStreamWrite) && (Ifd->Pos == 0)) {
    Status = FwsErase (Ifd->ImageFile);
    ASSERT (Status == EFI_SUCCESS);
  }

  Status = FwsRelease (
             Ifd->ImageFile,
             ReqData->MaxAtomicLen,
             &RespData->Progress,
             &RespData->TotalWork
             );
  if (EFI_ERROR (Status)) {
    switch (Status) {
      case EFI_TIMEOUT:
        return PSA_MM_FWU_RESUME;
      case EFI_SECURITY_VIOLATION:
        FwuStatus = PSA_MM_FWU_AUTH_FAIL;

        goto ErrorHandler;
      default:
        // Internal handle managed FwStore has problem. this never happen.
        ASSERT (0);
        return PSA_MM_FWU_UNKNOWN;
    }
  }

  if (ReqData->AcceptanceReq == 0) {
    /**
     * In case of FW_STORE_STAGING status, Accept the image on the update index.
     * Otherwise accept the image on the active index.
     */
    if ((Ifd->OpType == FwuOpStreamRead) && (mFwState != FW_STORE_STAGING)) {
      Status = FwsAcceptImage (
                 Ifd->ImageFile->FwsDevice,
                 &Ifd->ImageFile->ImageTypeGuid,
                 FALSE
                 );
    } else {
      Status = FwsAcceptImage (
                 Ifd->ImageFile->FwsDevice,
                 &Ifd->ImageFile->ImageTypeGuid,
                 TRUE
                 );
    }

    ASSERT (Status == EFI_SUCCESS);
  }

  FwuStatus = PSA_MM_FWU_SUCCESS;

ErrorHandler:
  Ifd->ImageFile = NULL;
  Ifd->Pos       = 0;
  Ifd->OpType    = FwuOpStreamRead;

  return FwuStatus;
}

/**
 * Handler for fwu_accept_image ABI.
 * Accpet image on active indexed bank.
 * This call could be called in correct boot only.
 *
 * See the PSA spec 3.4.2.9. all of return values based on the specification.
 *
 * @param [in,out]   Message       Request passed from UpdateClient (NS).
 *                                 Buffer for Response data.
 * @param [in]       MessageSize   Size of Message in bytes.

 *
 * @retval PSA_MM_FWU_SUCCESS
 * @retval PSA_MM_FWU_UNKNOWN    No matched image with image type guid.
 * @retval PSA_MM_FWU_DENIED     System doesn't boot with active indexed bank.
 */
STATIC
INT32
EFIAPI
FwuSmmAcceptImage (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS                         Status;
  CONST PSA_MM_FWU_ACCEPT_IMAGE_REQ  *ReqData;
  BOOLEAN                            IsCorrectBoot;
  BOOLEAN                            IsTrialState;

  ReqData = (CONST PSA_MM_FWU_ACCEPT_IMAGE_REQ *)GET_FWU_DATA_BUFFER (Message);

  ASSERT (mFwState != FW_STORE_STAGING);

  FwsCheckCorrectBoot (mFwsDevice, &IsCorrectBoot);
  if (!IsCorrectBoot) {
    return PSA_MM_FWU_DENIED;
  }

  Status = FwsAcceptImage (mFwsDevice, &ReqData->ImageTypeGuid, FALSE);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      return PSA_MM_FWU_UNKNOWN;
    }

    ASSERT (0);
  }

  FwsCheckTrialState (mFwsDevice, &IsTrialState);
  if (IsTrialState) {
    mFwState = FW_STORE_TRIAL;
  } else {
    mFwState = FW_STORE_REGULAR;
  }

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Current FwState: %d\n", mFwState));

  return PSA_MM_FWU_SUCCESS;
}

/**
 * Handler for fwu_select_previous ABI.
 * Rollback the firmware to the previous active firmware.
 *
 * See the PSA spec 3.4.2.10. all of return values based on the specification.
 *
 * @param [in,out]   Message        Request passed from UpdateClient (NS).
 *                                  Buffer for Response data (Not used).
 * @param [in]       MessageSize    Size of Message in bytes (Not used).
 *
 * @retval PSA_MM_FWU_UPD_SUCCESS
 * @retval PSA_MM_FWU_UPD_DENIED      previous bank cannot boot or
 *                                    System not in Trial State or
 *                                    Boot correct with active index.
 */
STATIC
INT32
EFIAPI
FwuSmmSelectPrevious (
  IN OUT  PSA_MM_FWU_CMD_DATA  *Message,
  IN      UINTN                MessageSize
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsTrialState;
  BOOLEAN     IsCorrectBoot;

  FwsCheckTrialState (mFwsDevice, &IsTrialState);
  FwsCheckCorrectBoot (mFwsDevice, &IsCorrectBoot);

  /**
   * fwu_select_previous could be called only:
   *     - the Firmware Store is in the Trial state, or
   *     - the platform failed to boot with the active bank (Not on correct boot)
   */
  if (IsTrialState || !IsCorrectBoot) {
    Status = FwsRollBack (mFwsDevice);
    ASSERT (Status == EFI_SUCCESS);
    mFwState = FW_STORE_REGULAR;
    return PSA_MM_FWU_SUCCESS;
  }

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Current FwState: %d\n", mFwState));

  return PSA_MM_FWU_DENIED;
}

STATIC FwuSmmFunc  mFwuSmmFuncArray[PSA_MM_FWU_COMMAND_MAX_ID] = {
  FwuSmmDiscover,                  // PSA_MM_FWU_COMMAND_DISCOVER
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  NULL,                            // Not impl (No matched func id in the spec).
  FwuSmmBeginStaging,              // PSA_MM_FWU_COMMAND_BEGIN_STAGING
  FwuSmmEndStaging,                // PSA_MM_FWU_COMMAND_END_STAGING
  FwuSmmCancelStaging,             // PSA_MM_FWU_COMMAND_CANCEL_STAGING
  FwuSmmOpen,                      // PSA_MM_FWU_COMMAND_OPEN
  FwuSmmWriteStream,               // PSA_MM_FWU_COMMAND_WRITE_STREAM
  FwuSmmReadStream,                // PSA_MM_FWU_COMMAND_READ_STREAM
  FwuSmmCommit,                    // PSA_MM_FWU_COMMAND_COMMIT
  FwuSmmAcceptImage,               // PSA_MM_FWU_COMMAND_ACCEPT_IMAGE
  FwuSmmSelectPrevious,            // PSA_MM_FWU_COMMAND_SELECT_PREVIOUS
};

/**
  Parse the request from firmware update client and
  Generate response for the request.

  @param [in,out]   Message        Request passed from UpdateClient (NS).
                                  Buffer for Response data.
  @param [in]      MessageSize     Size of Message in bytes.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Others            Some error occurs when executing this entry point.

**/
STATIC
EFI_STATUS
EFIAPI
ParseMessage (
  IN OUT PSA_MM_FWU_CMD_DATA  *Message,
  IN     UINTN                MessageSize
  )
{
  UINTN   RequiredBufferSize;
  UINT32  Command;

  if ((Message == NULL) || (MessageSize < sizeof (PSA_MM_FWU_PARAMETER_HEADER)) ||
      (Message->Header.Command >= PSA_MM_FWU_COMMAND_MAX_ID))
  {
    DEBUG ((DEBUG_ERROR, "Unknown (command code: %d)\n", Message->Header.Command));
    return EFI_INVALID_PARAMETER;
  }

  Command = Message->Header.Command;

  if (mFwuSmmFuncArray[Command] == NULL) {
    DEBUG ((DEBUG_ERROR, "Unknown (command code: %d)\n", Command));
    return EFI_UNSUPPORTED;
  }

  RequiredBufferSize = GetRequiredBufferSize (Command);
  if (MessageSize < RequiredBufferSize) {
    DEBUG ((
      DEBUG_ERROR,
      "The communication buffer is too small %d, need %d\n",
      MessageSize,
      RequiredBufferSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_BLKIO, "Command received: %d\n", Command));
  DEBUG ((DEBUG_BLKIO, "--> FWU message size: 0x%x bytes\n", MessageSize));
  DEBUG ((
    DEBUG_BLKIO,
    "--> FWU commnad data size: 0x%x bytes\n",
    MessageSize - sizeof (PSA_MM_FWU_PARAMETER_HEADER)
    ));

  Message->Header.ResponseStatus = mFwuSmmFuncArray[Command](Message, MessageSize);
  DEBUG ((DEBUG_BLKIO, "Command result: %d\n", Message->Header.ResponseStatus));

  return EFI_SUCCESS;
}

/**
  Firmware update driver event handler

  @param  [in]     DispatchHandle   The unique handle assigned to this handler
                                    by MmiHandlerRegister().
  @param  [in]     Context          Points to an optional handler context which
                                    was specified when the handler was registered.
  @param  [in,out] CommBuffer       A pointer to a collection of data in memory
                                    that will be conveyed from a non-MM environment
                                    into an MM environment.
  @param  [in,out] CommBufferSize   The size of the CommBuffer.

  @return EFI_SUCCESS
  @return Others                    Error.

**/
STATIC
EFI_STATUS
EFIAPI
FwuEventHandler (
  IN     EFI_HANDLE DispatchHandle,
  IN     CONST VOID *Context, OPTIONAL
  IN OUT VOID                     *CommBuffer, OPTIONAL
  IN OUT UINTN                    *CommBufferSize         OPTIONAL
  )
{
  EFI_STATUS                          Status;
  VOID                                *Buffer;
  UINTN                               BufferSize;
  CONST       ARM_MM_HANDLER_CONTEXT  *MmHandlerContext;
  CONST       FFA_MSG_INFO            *FfaMsgInfo;

  MmHandlerContext = (CONST ARM_MM_HANDLER_CONTEXT *)Context;
  if (MmHandlerContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, No MmHandler Context...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (MmHandlerContext->CommProtocol == CommProtocolFfa) {
    FfaMsgInfo = &MmHandlerContext->CtxData.FfaMsgInfo;

    if ((FfaMsgInfo->ServiceType == ServiceTypeMmCommunication) ||
        (FfaMsgInfo->DirectMsgVersion != DirectMsgV2))
    {
      // via mm communication protocol (FF-A).
      return EFI_UNSUPPORTED;
    }
  } else {
    // via mm communication protocol (SPM_MM).
    return EFI_UNSUPPORTED;
  }

  // Firmware update feature only supported by ff-a direct request message v2.
  Buffer     = (VOID *)mNsCommBufferRange->PhysicalStart;
  BufferSize = mNsCommBufferRange->PhysicalSize;

  DEBUG ((
    DEBUG_BLKIO,
    "Firmware received Update Event: CommBuffer - 0x%x, CommBufferSize - 0x%x\n",
    Buffer,
    BufferSize
    ));

  Status = ParseMessage (Buffer, BufferSize);

  return Status;
}

/**
  The entry point of firmware update Standalone MM Driver.

  @param  [in] ImageHandle    The image handle of the Standalone MM Driver.
  @param  [in] MmSystemTable  A pointer to the MM System Table.

  @return EFI_SUCCESS
  @return Others              Error.
**/
EFI_STATUS
EFIAPI
FwuSmmMain (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS             Status;
  EFI_HANDLE             DispatchHandle;
  EFI_HOB_GUID_TYPE      *NsCommBufHob;
  EFI_BLOCK_IO_PROTOCOL  *NorFlashBlockIo = NULL;
  UINT16                 Idx;
  BOOLEAN                IsTrialState;

  Status = gMmst->MmLocateProtocol (
                    &gEfiBlockIoProtocolGuid,
                    NULL,
                    (VOID **)&NorFlashBlockIo
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to get Firmware update NorFlash Device Instance!\n"));
    return Status;
  }

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Starting firmware update driver\n"));

  Status = FwsOpenDevice (NorFlashBlockIo, &mFwsDevice);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to open Firmware storage Device! %r\n", Status));
    return Status;
  }

  for (Idx = 0; Idx < PSA_MM_FWU_COMMAND_MAX_ID; Idx++) {
    if (mFwuSmmFuncArray[Idx] != NULL) {
      mFwuSmmNumFunc++;
    }
  }

  NsCommBufHob = GetFirstGuidHob (&gEfiStandaloneMmNonSecureBufferGuid);
  if (NsCommBufHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Firmware Update Driver: Fail to get Ns Buffer Hob!\n"));
    Status = EFI_NOT_READY;
    goto ErrorHandler;
  }

  mNsCommBufferRange = GET_GUID_HOB_DATA (NsCommBufHob);

  ASSERT (mNsCommBufferRange->PhysicalSize != 0);

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: NsMmBufferSize = 0x%lx\n", mNsCommBufferRange->PhysicalSize));

  Status = FwsGetImageDirectory (mFwsDevice, &mImageDirectory);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Firmware Update Driver: Fail to get Image Directory.\n"));
    goto ErrorHandler;
  }

  Status = InitImageFileDescTable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Firmware Update Driver: Fail to get Init Fd Table.\n"));
    goto ErrorHandler;
  }

  Status = FwsCheckTrialState (mFwsDevice, &IsTrialState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to get Trial State.\n"));
    goto ErrorHandler;
  }

  if (IsTrialState) {
    mFwState = FW_STORE_TRIAL;
  } else {
    mFwState = FW_STORE_REGULAR;
  }

  mMmst = MmSystemTable;
  // register the firmware update event handle
  Status = mMmst->MmiHandlerRegister (
                    FwuEventHandler,
                    &gPsaFwuUpdateAgentGuid,
                    &DispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Firmware Update Driver: event handler registration failed\n"));
    goto ErrorHandler;
  }

  DEBUG ((DEBUG_BLKIO, "Firmware Update Driver: Current FwState: %d\n", mFwState));

  mServiceStatus = SERVICE_STATUS_OPERATIVE;

  return EFI_SUCCESS;

ErrorHandler:
  if (mFwsDevice != NULL) {
    FwsReleaseDevice (mFwsDevice);
    mFwsDevice = NULL;
  }

  return Status;
}
