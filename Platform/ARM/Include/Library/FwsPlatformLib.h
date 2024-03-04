/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile Specification 1.0
    (https://developer.arm.com/documentation/den0118/latest)
           //
  @par Glossary:
    - FW          - Firmware
    - FWU         - Firmware Update
    - FWS         - Firmware Storage
    - PSA         - Platform Security update for the A-profile specification
    - IF          - Image File

**/

#ifndef FWS_PLATFORM_LIB_H_
#define FWS_PLATFORM_LIB_H_

#include <IndustryStandard/PsaMmFwUpdate.h>
#include <Protocol/BlockIo.h>

/** Ignore modification of image file.
 */
#define FWS_IF_F_IGNORE_DIRTY  (1 << 0)

#pragma pack (1)

/**
 * Firmware Storage Device Instance.
 */
typedef struct {
  /// Firmware storage Instance.
  EFI_BLOCK_IO_PROTOCOL    *Instance;

  // Opened Image File Count.
  UINT64                   ImageFileCount;

  // Private Data.
  VOID                     *Private;
} FWS_DEVICE_INSTANCE;

/** Image file instance describing image in firmware storage.
 */
typedef struct {
  /// Image Type Guid.
  EFI_GUID               ImageTypeGuid;

  /// Image File Name Guid saved in firmware storage.
  EFI_GUID               FileNameGuid;

  /// Image File Size.
  UINTN                  FileSize;

  /// Maximum Image File Size.
  UINTN                  MaxSize;

  /// Image File Flags. See FWS_IF_F_*
  UINTN                  Flags;

  /// Related Device Instance
  FWS_DEVICE_INSTANCE    *FwsDevice;

  /// File Private Data.
  VOID                   *Private;
} FWS_IMAGE_FILE;

#pragma pack ()

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
  );

/**
  Release the firmware storage device instance.

  @param [out]  FwsDevice            Opened firmware storage device.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_NOT_READY              Some image files opened using this device.

**/
EFI_STATUS
EFIAPI
FwsReleaseDevice (
  IN FWS_DEVICE_INSTANCE  *FwsDevice
  );

/**
  Get Image Directory.

  @param [in]  FwsDevice              Opened firmware storage device instance.
  @param [out] ImageDirectory        Pointer to Image directory.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FwsGetImageDirectory (
  IN  FWS_DEVICE_INSTANCE         *FwsDevice,
  OUT PSA_MM_FWU_IMAGE_DIRECTORY  **ImageDirectory
  );

/**
  Open the @ImageTypeGuid Image from storage.

  @param [in]   FwsDevice              Opened firmware storage device instance.
  @param [in]   ImageTypeGuid
  @param [in]   OpType                 FwuOpStreamRead / FwuOpStreamWrite
  @param [out]  ImageFile              Opened Image File Handle.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES       Out of Memory.
  @retval Others                     Fail to open internally.

**/
EFI_STATUS
EFIAPI
FwsOpen (
  IN  FWS_DEVICE_INSTANCE  *FwsDevice,
  IN  CONST EFI_GUID       *ImageTypeGuid,
  IN  FWU_OP_TYPE          OpType,
  OUT FWS_IMAGE_FILE       **ImageFile
  );

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
  );

/**
  Write data to a partition stored in the Instance.

  @param[in]     ImageFile   Image File Handle.
  @param[in]     Buffer      Data to be written to the partition.
  @param[in/out] WriteSize   Size to write / real write to the partition in bytes.
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
  );

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
  );

/**
  Read data from a partition stored in ImageFile.

  @param[in]       ImageFile    A pointer to the read destination buffer.
  @param[out]      Buffer       Read destination buffer.
  @param[in,out]   ReadSize     Size to read / Real read size from partition in bytes.
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
  );

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
  IN FWS_DEVICE_INSTANCE  *FwsDevice,
  IN CONST EFI_GUID       *ImgTypeGuid,
  IN BOOLEAN              AcceptUpdateImage
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif /* __FWS_PLATFORM_LIB_H */
