/** @file

  Marvell CN10K FMP device library.

  Implements the FmpDeviceLib interface for updating the boot SPI NOR flash.
  Capsule validation is done by the generic firmware management driver; this
  library copies the image to a contiguous buffer and forwards it to the
  secure monitor through an SMC update descriptor, which performs the actual
  secure flash programming.

  Copyright (c) 2025-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <IndustryStandard/SmcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CN10KConfigLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpDeviceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/SystemResourceTable.h>

#define SMC_MAX_OBJECTS             32
#define UPDATE_MAGIC                0x55504454
#define UPDATE_VERSION              0x0001
#define UPDATE_FLAG_IGNORE_VERSION  0x0008

//
// Placeholder for the per-object result information returned by the secure
// monitor. It is unused by this driver but must occupy its slot in the SMC
// update descriptor ABI.
//
typedef struct {
} SMC_UPDT_OBJ_INFO;

//
// Descriptor passed to the secure monitor to program a capsule image. The
// layout and the UPDATE_* constants must match the secure firmware ABI.
//
typedef struct {
  UINT32               Magic;
  UINT16               Version;
  UINT16               ImageFlags;
  UINT64               ImageAddr;
  UINT64               ImageSize;
  UINT32               Bus;
  UINT32               Cs;
  UINT32               AsyncOperation;
  UINT32               Reserved;
  UINT64               UserAddr;
  UINT64               UserSize;
  UINT64               UserFlags;
  UINTN                *WorkBuffer;
  UINT64               WorkBufferSize;
  SMC_UPDT_OBJ_INFO    ObjRetInfo[SMC_MAX_OBJECTS];
} SMC_UPDATE_DESC;

STATIC EFI_GUID  mFmpImageTypeIdGuid = {
  0xE3B2F1A0, 0x6C4D, 0x4B8E,
  { 0x9F,     0x1A,   0x2D,  0x7C,0x5E, 0x8B, 0x3A, 0x61 }
};

/**
  Program a capsule image into the boot SPI NOR flash.

  Reads the BOOT-DEVICE configuration property and expects it to describe a
  SPI boot device. The bus and chip select are extracted from the property
  string, a secure update descriptor is filled in with the image and the
  ignore-version flag, and the descriptor is passed to SmcSpiSecureUpdate().

  @param[in]  Image  Pointer to the capsule image in contiguous memory.
  @param[in]  Size   Size of the image in bytes.

  @retval  EFI_SUCCESS            The secure monitor programmed the image.
  @retval  EFI_NO_MEDIA           No SPI boot device is configured.
  @retval  EFI_OUT_OF_RESOURCES   The update descriptor could not be allocated.
  @retval  EFI_INVALID_PARAMETER  The secure monitor rejected the parameters.
  @retval  EFI_DEVICE_ERROR       The secure monitor reported a failure.
**/
STATIC
EFI_STATUS
ProgramCapsuleImage (
  IN VOID   *Image,
  IN UINTN  Size
  )
{
  CONST VOID       *BootProperty;
  INT32            PropertySize;
  CHAR8            *BootString;
  UINT8            Bus;
  UINT8            Cs;
  INT32            ReturnCode;
  SMC_UPDATE_DESC  *Desc;

  BootProperty = CN10KGetConfigProperty ("BOOT-DEVICE", &PropertySize);
  if ((BootProperty == NULL) || (PropertySize == 0)) {
    return EFI_NO_MEDIA;
  }

  BootString = (CHAR8 *)BootProperty;
  if ((BootString[0] != 'S') || (BootString[1] != 'P') || (BootString[2] != 'I')) {
    return EFI_NO_MEDIA;
  }

  Bus = BootString[3] - '0';
  Cs  = BootString[7] - '0';

  Desc = (SMC_UPDATE_DESC *)AllocatePages (1);
  if (Desc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Desc->Magic          = UPDATE_MAGIC;
  Desc->Version        = UPDATE_VERSION;
  Desc->Bus            = Bus;
  Desc->Cs             = Cs;
  Desc->AsyncOperation = 0;
  Desc->ImageFlags     = UPDATE_FLAG_IGNORE_VERSION;
  Desc->ImageAddr      = (UINT64)(UINTN)Image;
  Desc->ImageSize      = Size;

  ReturnCode = (INT32)SmcSpiSecureUpdate (Desc, sizeof (SMC_UPDATE_DESC));

  FreePages (Desc, 1);

  if (ReturnCode == 0) {
    return EFI_SUCCESS;
  } else if (ReturnCode == -1) {
    return EFI_INVALID_PARAMETER;
  } else {
    return EFI_DEVICE_ERROR;
  }
}

/**
  Register the FMP installer callback.

  Not supported by this library; the generic FMP driver is expected to manage
  protocol installation itself.

  @param[in]  FmpInstaller  Installer callback to register. Unused.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
EFI_STATUS
EFIAPI
RegisterFmpInstaller (
  IN FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER  FmpInstaller
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Register the FMP uninstaller callback.

  Not supported by this library; the generic FMP driver is expected to manage
  protocol removal itself.

  @param[in]  FmpUninstaller  Uninstaller callback to register. Unused.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
EFI_STATUS
EFIAPI
RegisterFmpUninstaller (
  IN FMP_DEVICE_LIB_REGISTER_FMP_UNINSTALLER  FmpUninstaller
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set the per-device context.

  Not supported because the CN10K update path does not need per-device
  context.

  @param[in]      Handle   The FMP device handle. Unused.
  @param[in,out]  Context  Device context. Unused.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
EFI_STATUS
EFIAPI
FmpDeviceSetContext (
  IN EFI_HANDLE  Handle,
  IN OUT VOID    **Context
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Return the maximum image size accepted by the device.

  The size is the configured flash device size, PcdFlashDeviceSize.

  @param[out]  Size  Receives the image size in bytes.

  @retval  EFI_SUCCESS            The size was returned.
  @retval  EFI_INVALID_PARAMETER  Size is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetSize (
  OUT UINTN  *Size
  )
{
  if (Size == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Size = FixedPcdGet32 (PcdFlashDeviceSize);
  return EFI_SUCCESS;
}

/**
  Return the image type GUID identifying this device.

  @param[out]  Guid  Receives a pointer to the static image type GUID.

  @retval  EFI_SUCCESS            The GUID pointer was returned.
  @retval  EFI_INVALID_PARAMETER  Guid is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetImageTypeIdGuidPtr (
  OUT EFI_GUID  **Guid
  )
{
  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Guid = &mFmpImageTypeIdGuid;
  return EFI_SUCCESS;
}

/**
  Return the supported and currently enabled image attributes.

  The image is updatable, requires authentication, and requires a reset after
  it is applied. All supported attributes are also enabled.

  @param[out]  Supported  Receives the supported attribute bit mask.
  @param[out]  Setting    Receives the enabled attribute bit mask.

  @retval  EFI_SUCCESS            The attributes were returned.
  @retval  EFI_INVALID_PARAMETER  Supported or Setting is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetAttributes (
  OUT UINT64  *Supported,
  OUT UINT64  *Setting
  )
{
  if ((Supported == NULL) || (Setting == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Supported = IMAGE_ATTRIBUTE_IMAGE_UPDATABLE |
               IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED |
               IMAGE_ATTRIBUTE_RESET_REQUIRED;
  *Setting = *Supported;

  return EFI_SUCCESS;
}

/**
  Return the lowest firmware version that may be installed.

  The value comes from PcdFmpDeviceBuildTimeLowestSupportedVersion.

  @param[out]  LowestSupportedVersion  Receives the lowest supported version.

  @retval  EFI_SUCCESS            The version was returned.
  @retval  EFI_INVALID_PARAMETER  LowestSupportedVersion is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetLowestSupportedVersion (
  OUT UINT32  *LowestSupportedVersion
  )
{
  if (LowestSupportedVersion == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *LowestSupportedVersion = FixedPcdGet32 (PcdFmpDeviceBuildTimeLowestSupportedVersion);
  return EFI_SUCCESS;
}

/**
  Return a human-readable firmware version string.

  Not supported; the version is reported numerically by FmpDeviceGetVersion().

  @param[out]  VersionString  Receives the version string. Unused.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersionString (
  OUT CHAR16  **VersionString
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Return the current firmware version.

  The version is not tracked by this library, so zero is always reported.

  @param[out]  Version  Receives the firmware version.

  @retval  EFI_SUCCESS            The version was returned.
  @retval  EFI_INVALID_PARAMETER  Version is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersion (
  OUT UINT32  *Version
  )
{
  if (Version == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Version = 0;
  return EFI_SUCCESS;
}

/**
  Return the hardware instance of this device.

  Not supported; the driver exposes a single FMP device.

  @param[out]  HardwareInstance  Receives the hardware instance. Unused.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetHardwareInstance (
  OUT UINT64  *HardwareInstance
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Read the current image from the device.

  Not supported; reading the secure flash image back through this interface is
  not implemented.

  @param[out]     Image      Receives the image. Unused.
  @param[in,out]  ImageSize  On input, the buffer size. Unused.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetImage (
  OUT    VOID   *Image,
  IN OUT UINTN  *ImageSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Check whether an image can be applied.

  Delegates to FmpDeviceCheckImageWithStatus() and discards the detailed
  last-attempt status.

  @param[in]   Image           Pointer to the image.
  @param[in]   ImageSize       Size of the image in bytes.
  @param[out]  ImageUpdatable  Receives the image updatable state.

  @retval  EFI_SUCCESS            The image is valid.
  @retval  EFI_INVALID_PARAMETER  ImageUpdatable is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceCheckImage (
  IN  CONST VOID  *Image,
  IN  UINTN       ImageSize,
  OUT UINT32      *ImageUpdatable
  )
{
  UINT32  LastAttemptStatus;

  return FmpDeviceCheckImageWithStatus (
           Image,
           ImageSize,
           ImageUpdatable,
           &LastAttemptStatus
           );
}

/**
  Check whether an image can be applied and report the attempt status.

  The secure monitor performs the real validation, so every image is reported
  as valid here and the generic FMP code proceeds to FmpDeviceSetImage().

  @param[in]   Image              Pointer to the image.
  @param[in]   ImageSize          Size of the image in bytes.
  @param[out]  ImageUpdatable     Receives IMAGE_UPDATABLE_VALID.
  @param[out]  LastAttemptStatus  Receives LAST_ATTEMPT_STATUS_SUCCESS.

  @retval  EFI_SUCCESS            The image was accepted.
  @retval  EFI_INVALID_PARAMETER  ImageUpdatable or LastAttemptStatus is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceCheckImageWithStatus (
  IN  CONST VOID  *Image,
  IN  UINTN       ImageSize,
  OUT UINT32      *ImageUpdatable,
  OUT UINT32      *LastAttemptStatus
  )
{
  if ((ImageUpdatable == NULL) || (LastAttemptStatus == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ImageUpdatable    = IMAGE_UPDATABLE_VALID;
  *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

  return EFI_SUCCESS;
}

/**
  Apply an image to the device.

  Delegates to FmpDeviceSetImageWithStatus() and discards the detailed
  last-attempt status.

  @param[in]   Image             Pointer to the image.
  @param[in]   ImageSize         Size of the image in bytes.
  @param[in]   VendorCode        Optional vendor-specific data.
  @param[in]   Progress          Optional progress callback.
  @param[in]   CapsuleFwVersion  Firmware version from the capsule.
  @param[out]  AbortReason       Receives the abort reason on failure.

  @retval  EFI_SUCCESS            The image was applied.
  @retval  EFI_INVALID_PARAMETER  Image is NULL or ImageSize is zero.
  @retval  Others                 Allocation or secure update failed.
**/
EFI_STATUS
EFIAPI
FmpDeviceSetImage (
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode        OPTIONAL,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress           OPTIONAL,
  IN  UINT32                                         CapsuleFwVersion,
  OUT CHAR16                                         **AbortReason
  )
{
  UINT32  LastAttemptStatus;

  return FmpDeviceSetImageWithStatus (
           Image,
           ImageSize,
           VendorCode,
           Progress,
           CapsuleFwVersion,
           AbortReason,
           &LastAttemptStatus
           );
}

/**
  Apply an image to the device and report the attempt status.

  Reports zero progress, copies the image into a contiguous boot-services
  allocation, and forwards it to ProgramCapsuleImage(). On success the
  last-attempt status is set to success and the progress callback, if any, is
  advanced to 100.

  @param[in]   Image              Pointer to the image.
  @param[in]   ImageSize          Size of the image in bytes.
  @param[in]   VendorCode         Optional vendor-specific data. Unused.
  @param[in]   Progress           Optional progress callback.
  @param[in]   CapsuleFwVersion   Firmware version from the capsule. Unused.
  @param[out]  AbortReason        Receives the abort reason on failure. Unused.
  @param[out]  LastAttemptStatus  Receives the detailed update status.

  @retval  EFI_SUCCESS            The image was applied.
  @retval  EFI_INVALID_PARAMETER  Image or LastAttemptStatus is NULL, or
                                  ImageSize is zero.
  @retval  EFI_OUT_OF_RESOURCES   The image buffer could not be allocated.
  @retval  Others                 The secure update failed.
**/
EFI_STATUS
EFIAPI
FmpDeviceSetImageWithStatus (
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode        OPTIONAL,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress           OPTIONAL,
  IN  UINT32                                         CapsuleFwVersion,
  OUT CHAR16                                         **AbortReason,
  OUT UINT32                                         *LastAttemptStatus
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Buffer;
  UINTN                 NumPages;

  if ((Image == NULL) || (ImageSize == 0) || (LastAttemptStatus == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;

  if (Progress != NULL) {
    Progress (0);
  }

  NumPages = EFI_SIZE_TO_PAGES (ImageSize);
  Status   = gBS->AllocatePages (
                    AllocateAnyPages,
                    EfiBootServicesData,
                    NumPages,
                    &Buffer
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem ((VOID *)(UINTN)Buffer, (VOID *)Image, ImageSize);

  Status = ProgramCapsuleImage ((VOID *)(UINTN)Buffer, ImageSize);

  gBS->FreePages (Buffer, NumPages);

  if (!EFI_ERROR (Status)) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
    if (Progress != NULL) {
      Progress (100);
    }
  }

  return Status;
}

/**
  Lock the device to prevent further updates.

  No persistent lock is maintained by this library, so there is nothing to do
  here.

  @retval  EFI_SUCCESS  Always returned.
**/
EFI_STATUS
EFIAPI
FmpDeviceLock (
  VOID
  )
{
  return EFI_SUCCESS;
}
