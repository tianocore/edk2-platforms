/** @file
  Platform flash device access library.

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PlatformFlashAccessLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MmCommunication2.h>

EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunication2 = NULL;
EFI_MM_COMMUNICATION_PROTOCOL   *mMmCommunication  = NULL;

#define EFI_MM_MAX_PAYLOAD_U64_E  10
#define EFI_MM_MAX_PAYLOAD_SIZE   (EFI_MM_MAX_PAYLOAD_U64_E * sizeof (UINT64))

// FWU MM GUID
STATIC CONST EFI_GUID  mFwuMmGuid = {
  0x452240CD, 0xB3B3, 0x4695, { 0x9A, 0x63, 0xDF, 0xEC, 0x50, 0x82, 0xE7, 0x7A }
};

typedef struct {
  // Allows for disambiguation of the message format
  EFI_GUID    HeaderGuid;

  //
  // Describes the size of Data (in bytes) and does not include the size
  // of the header
  //
  UINTN       MsgLength;
} EFI_MM_COMM_HEADER_NOPAYLOAD;

typedef struct {
  UINT64    Data[EFI_MM_MAX_PAYLOAD_U64_E];
} EFI_MM_COMM_FWU_PAYLOAD;

typedef struct {
  EFI_MM_COMM_HEADER_NOPAYLOAD    EfiMmHdr;
  EFI_MM_COMM_FWU_PAYLOAD         PayLoad;
} EFI_MM_COMM_REQUEST;

EFI_MM_COMM_REQUEST  mEfiMmSysFwuReq;

typedef struct {
  UINT64    Status;
  UINT64    Progress;
} EFI_MM_COMMUNICATE_FWU_RES;

#define FWU_MM_RES_SUCCESS             0xAABBCC00
#define FWU_MM_RES_IN_PROGRESS         0xAABBCC01
#define FWU_MM_RES_SECURITY_VIOLATION  0xAABBCC02
#define FWU_MM_RES_OUT_OF_RESOURCES    0xAABBCC03
#define FWU_MM_RES_IO_ERROR            0xAABBCC04
#define FWU_MM_RES_FAIL                0xAABBCCFF

//
// The ARM Trusted Firmware (ATF) defined image types to support updating
// various firmware via SMC Firmware Update service.
// The value of definition here must be the same in ATF code.
//
#define FWU_IMG_TYPE_INVALID  0
#define FWU_IMG_TYPE_ATFBIOS  2  // ARM Trusted Firmware and UEFI Image
#define FWU_IMG_TYPE_MAX      5

//
// According to PLATFORM_FIRMWARE_TYPE,
// Type 0x80000000 ~ 0xFFFFFFFF is reserved for OEM.
// A type of firmware image is determined by FirmwareType value
// in the Configuration of Firmware Update.
// Formula: FirmwareType = ImageType + 0x80000000
//
#define FIRMWARE_TYPE_OEM_OFFSET  0x80000000

//
// Size of Firmware Descriptor File Volume
//
#define FIRMWARE_DESCRIPTOR_SIZE  0x10000

VOID
UefiMmCreateSysFwuReq (
  VOID    *Data,
  UINT64  Size
  )
{
  CopyGuid (&mEfiMmSysFwuReq.EfiMmHdr.HeaderGuid, &mFwuMmGuid);
  mEfiMmSysFwuReq.EfiMmHdr.MsgLength = Size;

  if (Size != 0) {
    ASSERT (Data != NULL);
    ASSERT (Size <= EFI_MM_MAX_PAYLOAD_SIZE);

    CopyMem (&mEfiMmSysFwuReq.PayLoad.Data, Data, Size);
  }
}

EFI_STATUS
MmFlashUpdate (
  IN UINT32                                         ImageType,
  IN VOID                                           *FirmwareImage,
  IN UINT64                                         ImageSize,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_MM_COMMUNICATE_FWU_RES  *MmFwuStatus;
  UINTN                       ProgressUpdate = StartPercentage;
  UINTN                       Size;
  UINT64                      MmData[5];
  EFI_STATUS                  Status;

  if (FirmwareImage == NULL) {
    DEBUG ((DEBUG_ERROR, "%a:%d Invalid inputs.\n", __func__, __LINE__));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "%a:%d Image: %p Size 0x%lx\n", __func__, __LINE__, FirmwareImage, ImageSize));

  if ((mMmCommunication2 == NULL) && (mMmCommunication == NULL)) {
    Status = gBS->LocateProtocol (
                    &gEfiMmCommunication2ProtocolGuid,
                    NULL,
                    (VOID **)&mMmCommunication2
                    );
    if (EFI_ERROR (Status)) {
      Status = gBS->LocateProtocol (
                      &gEfiMmCommunicationProtocolGuid,
                      NULL,
                      (VOID **)&mMmCommunication
                      );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Can't locate MM Communication protocol.\n", __func__));
        return Status;
      }
    }
  }

  MmData[0] = ImageType;
  MmData[1] = ImageSize;
  MmData[2] = (UINT64)FirmwareImage;
  MmData[3] = 1; // MM yield for progress reporting.

  while (TRUE) {
    UefiMmCreateSysFwuReq ((VOID *)&MmData, sizeof (MmData));

    Size = sizeof (EFI_MM_COMM_HEADER_NOPAYLOAD) + sizeof (MmData);
    if (mMmCommunication2 != NULL) {
      Status = mMmCommunication2->Communicate (
                                    mMmCommunication2,
                                    (VOID *)&mEfiMmSysFwuReq,
                                    (VOID *)&mEfiMmSysFwuReq,
                                    &Size
                                    );
    } else if (mMmCommunication != NULL) {
      Status =  mMmCommunication->Communicate (
                                    mMmCommunication,
                                    (VOID *)&mEfiMmSysFwuReq,
                                    &Size
                                    );
    } else {
      return EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:%d MM communication error - %r\n", __func__, __LINE__, Status));
      return EFI_DEVICE_ERROR;
    }

    // Return data in the first double word of payload
    MmFwuStatus = (EFI_MM_COMMUNICATE_FWU_RES *)mEfiMmSysFwuReq.PayLoad.Data;
    if (MmFwuStatus->Status == FWU_MM_RES_IN_PROGRESS) {
      if (NULL != Progress) {
        Progress (ProgressUpdate);
      }

      ProgressUpdate = StartPercentage +
                       (((EndPercentage - StartPercentage) * (MmFwuStatus->Progress)) / 100);
      continue;
    }

    break;
  }

  switch (MmFwuStatus->Status) {
    case FWU_MM_RES_SUCCESS:
      if (NULL != Progress) {
        Progress (EndPercentage);
      }

      Status = EFI_SUCCESS;
      break;

    case FWU_MM_RES_SECURITY_VIOLATION:
      DEBUG ((DEBUG_ERROR, "%a:%d Failed to update - Security Violation!\n", __func__, __LINE__));
      Status = EFI_SECURITY_VIOLATION;
      break;

    case FWU_MM_RES_OUT_OF_RESOURCES:
      DEBUG ((DEBUG_ERROR, "%a:%d Failed to update - Insufficient resources!\n", __func__, __LINE__));
      Status = EFI_OUT_OF_RESOURCES;
      break;

    case FWU_MM_RES_IO_ERROR:
      DEBUG ((DEBUG_ERROR, "%a:%d Failed to update - IO Error!\n", __func__, __LINE__));
      Status = EFI_DEVICE_ERROR;
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a:%d Failed to update - Unknown Error!\n", __func__, __LINE__));
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/**
  Perform flash write operation with progress indicator.  The start and end
  completion percentage values are passed into this function.  If the requested
  flash write operation is broken up, then completion percentage between the
  start and end values may be passed to the provided Progress function.  The
  caller of this function is required to call the Progress function for the
  start and end completion percentage values.  This allows the Progress,
  StartPercentage, and EndPercentage parameters to be ignored if the requested
  flash write operation can not be broken up

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.
  @param[in] Progress          A function used report the progress of the
                               firmware update.  This is an optional parameter
                               that may be NULL.
  @param[in] StartPercentage   The start completion percentage value that may
                               be used to report progress during the flash
                               write operation.
  @param[in] EndPercentage     The end completion percentage value that may
                               be used to report progress during the flash
                               write operation.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWriteWithProgress (
  IN PLATFORM_FIRMWARE_TYPE                         FirmwareType,
  IN EFI_PHYSICAL_ADDRESS                           FlashAddress,
  IN FLASH_ADDRESS_TYPE                             FlashAddressType,
  IN VOID                                           *Buffer,
  IN UINTN                                          Length,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress         OPTIONAL,
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  UINT32  ImageType;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // As per the design of firmware capsule, the Firmware Descriptor File
  // Volume which is used to provide description of firmware update is
  // attached at the beginning of the firmware file. It must be removed
  // before writing.
  //
  if (Length <= FIRMWARE_DESCRIPTOR_SIZE) {
    DEBUG ((DEBUG_ERROR, "%a %d The length of firmware image is invalid.\n", __func__, __LINE__));
    return EFI_INVALID_PARAMETER;
  }

  Length -= FIRMWARE_DESCRIPTOR_SIZE;
  Buffer += FIRMWARE_DESCRIPTOR_SIZE;

  ImageType = FirmwareType - FIRMWARE_TYPE_OEM_OFFSET;
  if (ImageType >= FWU_IMG_TYPE_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  return MmFlashUpdate (ImageType, Buffer, Length, Progress, StartPercentage, EndPercentage);
}

/**
  Perform flash write operation.

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWrite (
  IN PLATFORM_FIRMWARE_TYPE  FirmwareType,
  IN EFI_PHYSICAL_ADDRESS    FlashAddress,
  IN FLASH_ADDRESS_TYPE      FlashAddressType,
  IN VOID                    *Buffer,
  IN UINTN                   Length
  )
{
  return PerformFlashWriteWithProgress (FirmwareType, FlashAddress, FlashAddressType, Buffer, Length, NULL, 0, 0);
}
