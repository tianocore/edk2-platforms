/** @file
  SetImage instance to update system firmware.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SystemFirmwareDxe.h"

//
// SystemFmp driver private data
//
SYSTEM_FMP_PRIVATE_DATA  *mSystemFmpPrivate = NULL;

EFI_GUID  mCurrentImageTypeId;

BOOLEAN  mNvRamUpdated = FALSE;

UINT8  mUpdateSlot = 0;

/**
  Parse Config data file to get the updated data array.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] ConfigHeader    Pointer to the config header.
  @param[in, out] UpdateArray     Pointer to the config of update data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseUpdateDataFile (
  IN      UINT8               *DataBuffer,
  IN      UINTN               BufferSize,
  IN OUT  CONFIG_HEADER       *ConfigHeader,
  IN OUT  UPDATE_CONFIG_DATA  **UpdateArray
  );

/**
  Update System Firmware image component.

  @param[in]  SystemFirmwareImage     Points to the System Firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[in]  ConfigData              Points to the component configuration structure.
  @param[out] LastAttemptVersion      The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in]  Progress                A function used by the driver to report the progress of the firmware update.
  @param[in]  StartPercentage         The start completion percentage value that may be used to report progress during the flash write operation.
  @param[in]  EndPercentage           The end completion percentage value that may be used to report progress during the flash write operation.

  @retval EFI_SUCCESS             The System Firmware image is updated.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
PerformUpdate (
  IN VOID                                           *SystemFirmwareImage,
  IN UINTN                                          SystemFirmwareImageSize,
  IN UPDATE_CONFIG_DATA                             *ConfigData,
  OUT UINT32                                        *LastAttemptVersion,
  OUT UINT32                                        *LastAttemptStatus,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "PlatformUpdate:"));
  DEBUG ((DEBUG_INFO, "  BaseAddress - 0x%lx,", ConfigData->BaseAddress));
  DEBUG ((DEBUG_INFO, "  ImageOffset - 0x%x,", ConfigData->ImageOffset));
  DEBUG ((DEBUG_INFO, "  Legnth - 0x%x\n", ConfigData->Length));
  if (Progress != NULL) {
    Progress (StartPercentage);
  }

  Status = PerformFlashWriteWithProgress (
             ConfigData->FirmwareType,
             ConfigData->BaseAddress,
             ConfigData->AddressType,
             (VOID *)((UINTN)SystemFirmwareImage + (UINTN)ConfigData->ImageOffset),
             ConfigData->Length,
             Progress,
             StartPercentage,
             EndPercentage
             );
  if (Progress != NULL) {
    Progress (EndPercentage);
  }

  if (!EFI_ERROR (Status)) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
    if (ConfigData->FirmwareType == PlatformFirmwareTypeNvRam) {
      mNvRamUpdated = TRUE;
    }
  } else {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
  }

  return Status;
}

/**
  Get layout of system firmware image.

  @param[in]  SystemFirmwareImage     Points to the System firmware image.
  @param[out] SlotAOffset             Points to the offste of slot A image.
  @param[out] SlotBOffset             Points to the offste of slot B image.

  @retval EFI_SUCCESS                 Get system firmware image layout successfully.
  @retval others                      Some error occurs when executing this routine.

**/
EFI_STATUS
GetImageLayout (
  IN VOID     *SystemFirmwareImage,
  OUT UINT32  *SlotAOffset,
  OUT UINT32  *SlotBOffset,
  OUT UINT8   *ActiveSlot
  )
{
  FIRMWARE_ENTRY_TABLEV2  *EfsAddressPtr;
  PSP_DIRECTORY           *PspL1DirectoryPtr;
  UINT32                  SlotCount;
  UINT32                  Index;
  IMAGE_SLOT_HEADER       *IshSlotAInfoPtr;
  IMAGE_SLOT_HEADER       *IshSlotBInfoPtr;

  if (SystemFirmwareImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((SlotAOffset == NULL) || (SlotBOffset == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check EFS structure of firmware image
  //
  EfsAddressPtr = (FIRMWARE_ENTRY_TABLEV2 *)(UINTN)((UINT8 *)SystemFirmwareImage + EFS_LOCATION);
  if (EfsAddressPtr->Signature != FIRMWARE_TABLE_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "EFS signature incorrect.\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Check PSP_L1_DIRECTORY of firmware image
  //
  DEBUG ((DEBUG_INFO, "Base Address for PSP directory: 0x%x\n", EfsAddressPtr->PspDirBase));
  PspL1DirectoryPtr = (PSP_DIRECTORY *)(UINTN)((UINT8 *)SystemFirmwareImage + EfsAddressPtr->PspDirBase);
  if ((PspL1DirectoryPtr->Header.Cookie != PSP_DIRECTORY_HEADER_SIGNATURE) ||
      (!IS_VALID_ADDR32 (EfsAddressPtr->PspDirBase)) ||
      (!ALIGN_4K_CHECK (EfsAddressPtr->PspDirBase)) ||
      (PspL1DirectoryPtr->Header.TotalEntries == 0) ||
      (PspL1DirectoryPtr->Header.TotalEntries > MAX_IMAGE_SLOT_COUNT)
      )
  {
    DEBUG ((DEBUG_ERROR, "PSP L1 directory address, slot count or signature error!\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Check Image Slot entries of firmware image
  //
  SlotCount = PspL1DirectoryPtr->Header.TotalEntries;
  for (Index = 0; Index < SlotCount; Index++) {
    if (((PspL1DirectoryPtr->PspEntry[Index].Type.Value != PSP_REGION_A_DIR) &&
         (PspL1DirectoryPtr->PspEntry[Index].Type.Value != PSP_REGION_B_DIR)) ||
        (!IS_VALID_ADDR32 (PspL1DirectoryPtr->PspEntry[Index].Location)) ||
        (!ALIGN_4K_CHECK (PspL1DirectoryPtr->PspEntry[Index].Location)) ||
        (((PspL1DirectoryPtr->PspEntry[Index].Location) & 0xFFFFFFFF00000000) != 0)
        )
    {
      DEBUG ((DEBUG_ERROR, "PSP L1 directory slot %d data error!\n", Index));
      return EFI_NOT_FOUND;
    }
  }

  //
  // Get offset of specific slot
  //
  IshSlotAInfoPtr = (IMAGE_SLOT_HEADER *)(UINTN)((UINT8 *)SystemFirmwareImage + PspL1DirectoryPtr->PspEntry[0].Location);
  *SlotAOffset    = IshSlotAInfoPtr->ImageSlotAddr;
  DEBUG ((DEBUG_ERROR, "Slot A image offset: 0x%x\n", *SlotAOffset));

  IshSlotBInfoPtr = (IMAGE_SLOT_HEADER *)(UINTN)((UINT8 *)SystemFirmwareImage + PspL1DirectoryPtr->PspEntry[1].Location);
  *SlotBOffset    = IshSlotBInfoPtr->ImageSlotAddr;
  DEBUG ((DEBUG_ERROR, "Slot B image offset: 0x%x\n", *SlotBOffset));

  if ((*SlotAOffset == 0) || (*SlotBOffset == 0)) {
    return EFI_NOT_FOUND;
  }

  if (ActiveSlot != NULL) {
    if (IshSlotAInfoPtr->Priority > IshSlotBInfoPtr->Priority) {
      *ActiveSlot = SLOT_A;
    } else {
      *ActiveSlot = SLOT_B;
    }
  }

  return EFI_SUCCESS;
}

/**
  Verify layout of OTA Capsule firmware image, and return offset and size of required update slot.

  @param[in]  SystemFirmwareImage     Points to the System firmware image.
  @param[in]  UpdateSlot              The slot number need to be updated.
  @param[out] UpdateOffset            The firmware image offset need to updated.
  @param[out] UpdateSize              The firmware image size need to updated.

  @retval EFI_SUCCESS                 Verify OTA capsule image and get updated offset/size successfully.
  @retval others                      Some error occurs when executing this routine.

**/
EFI_STATUS
VerifyImageLayout (
  IN  VOID    *SystemFirmwareImage,
  IN  UINT8   UpdateSlot,
  OUT UINT32  *UpdateOffset,
  OUT UINT32  *UpdateSize
  )
{
  EFI_STATUS  Status;
  UINT32      OtaSlotAOffset;
  UINT32      OtaSlotBOffset;
  UINT32      FlashSlotAOffset;
  UINT32      FlashSlotBOffset;
  UINT8       CurrentActiveSlot;

  if (SystemFirmwareImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UpdateOffset == NULL) || (UpdateSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OtaSlotAOffset   = 0;
  OtaSlotBOffset   = 0;
  FlashSlotAOffset = 0;
  FlashSlotBOffset = 0;

  //
  // Get image layout of OTA Capsule
  //
  DEBUG ((DEBUG_INFO, "Get image layout of OTA Capsule.\n"));
  Status = GetImageLayout (SystemFirmwareImage, &OtaSlotAOffset, &OtaSlotBOffset, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetImageLayout of Capsule failed: %r\n", Status));
    return Status;
  }

  //
  // Get image layout of firmware in flash ROM
  //
  DEBUG ((DEBUG_INFO, "Get image layout of flash ROM.\n"));
  Status = GetImageLayout ((VOID *)(UINTN)(PcdGet32 (PcdFlashAreaBaseAddress)), &FlashSlotAOffset, &FlashSlotBOffset, &CurrentActiveSlot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetImageLayout of Flash failed: %r\n", Status));
    return Status;
  }

  //
  // Check current active slot and update slot
  //
  // -  if (CurrentActiveSlot == mUpdateSlot) {
  // -   DEBUG ((DEBUG_ERROR, "Can't update Capsule on current active slot. CurrentActiveSlot: %d, UpdateSlot: %d\n", CurrentActiveSlot, mUpdateSlot));
  // -   return EFI_INVALID_PARAMETER;
  // -  }
  //
  // Compare layout of OTA capsule image and flash firmware
  //
  if ((OtaSlotAOffset != FlashSlotAOffset) || (OtaSlotBOffset != FlashSlotBOffset)) {
    DEBUG ((DEBUG_ERROR, "Layout is different between Capsule and Flash.\n"));
    return EFI_NOT_FOUND;
  }

  if (UpdateSlot == SLOT_A) {
    *UpdateOffset = OtaSlotAOffset;
  } else if (UpdateSlot == SLOT_B) {
    *UpdateOffset = OtaSlotBOffset;
  } else {
    DEBUG ((DEBUG_ERROR, "Invalid update slot number: %d\n", UpdateSlot));
    return EFI_INVALID_PARAMETER;
  }

  // -  *UpdateSize = (UINT32) DivU64x64Remainder ((UINTN) PcdGet32 (PcdFlashAreaSize) - OtaSlotAOffset, 2, NULL);
  *UpdateSize = (UINT32)((UINTN)PcdGet32 (PcdFlashAreaSize) - OtaSlotAOffset);

  return EFI_SUCCESS;
}

/**
  Get OTA Capsule firmware image info.

  @param[in]  SystemFirmwareImage     Points to the System firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[out] OtaCapsuleOffset        The firmware image offset need to updated.
  @param[out] OtaCapsuleSize          The firmware image size need to updated.

  @retval EFI_SUCCESS                 Get OTA Capsule firmware image info successfully.
  @retval others                      Some error occurs when executing this routine.

**/
EFI_STATUS
GetOtaCapsuleInfo (
  IN VOID     *SystemFirmwareImage,
  IN UINTN    SystemFirmwareImageSize,
  OUT UINT32  *OtaCapsuleOffset,
  OUT UINT32  *OtaCapsuleSize
  )
{
  EFI_STATUS          Status;
  OTA_CAPSULE_UPDATE  OtaCapsuleUpdateVal;
  // -  UINTN                        VarSize;
  UINT32  UpdateOffset;
  UINT32  UpdateSize;

  if ((OtaCapsuleOffset == NULL) || (OtaCapsuleSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "GetOtaCapsuleInfo:"));
  DEBUG ((DEBUG_INFO, "  Legnth - 0x%x\n", SystemFirmwareImageSize));

  // -  if (SystemFirmwareImageSize != (UINTN) PcdGet32 (PcdFlashAreaSize)) {
  // -    return EFI_INVALID_PARAMETER;
  // -  }
  if (SystemFirmwareImageSize != (UINTN)(PcdGet32 (PcdFlashAreaSize)*2)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&OtaCapsuleUpdateVal, sizeof (OTA_CAPSULE_UPDATE));

  /*
    VarSize = sizeof (OTA_CAPSULE_UPDATE);
    Status = gRT->GetVariable (
                    OTA_CAPSULE_VAR_NAME,
                    &gOtaCapsuleUpdateGuid,
                    NULL,
                    &VarSize,
                    (VOID *) &OtaCapsuleUpdateVal
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "GetOtaCapsuleInfo: GetVariable failed: %r\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "UpdateFlag: 0x%x, UpdateSlot: 0x%x\n", OtaCapsuleUpdateVal.UpdateFlag, OtaCapsuleUpdateVal.UpdateSlot));

    mUpdateSlot = OtaCapsuleUpdateVal.UpdateSlot;
    if (mUpdateSlot >= MAX_SLOT_NUM) {
      DEBUG ((DEBUG_ERROR, "Invalid Slot number: %d\n", mUpdateSlot));
      return EFI_NOT_FOUND;
    }
  */
  mUpdateSlot = 0;

  Status = VerifyImageLayout (SystemFirmwareImage, mUpdateSlot, &UpdateOffset, &UpdateSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "VerifyImageLayout failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "UpdateSlot: %d, UpdateOffset: 0x%x, UpdateSize: 0x%x\n", mUpdateSlot, UpdateOffset, UpdateSize));

  *OtaCapsuleOffset = UpdateOffset;
  *OtaCapsuleSize   = UpdateSize;

  return EFI_SUCCESS;
}

/**
  Get OTA Capsule firmware image info.

  @param[in]  SystemFirmwareImage     Points to the System firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[out] OtaCapsuleOffset        The firmware image offset need to updated.
  @param[out] OtaCapsuleSize          The firmware image size need to updated.

  @retval EFI_SUCCESS                 Get OTA Capsule firmware image info successfully.
  @retval others                      Some error occurs when executing this routine.

**/
EFI_STATUS
GetOtaCapsuleInfoSlotB (
  IN VOID     *SystemFirmwareImage,
  IN UINTN    SystemFirmwareImageSize,
  OUT UINT32  *OtaCapsuleOffset,
  OUT UINT32  *OtaCapsuleSize
  )
{
  EFI_STATUS          Status;
  OTA_CAPSULE_UPDATE  OtaCapsuleUpdateVal;
  // -  UINTN                        VarSize;
  UINT32  UpdateOffset;
  UINT32  UpdateSize;

  // -  if ((OtaCapsuleOffset == NULL) || (OtaCapsuleSize == NULL)) {
  // -    return EFI_INVALID_PARAMETER;
  // -  }

  DEBUG ((DEBUG_INFO, "GetOtaCapsuleInfo:"));
  DEBUG ((DEBUG_INFO, "  Legnth - 0x%x\n", SystemFirmwareImageSize));
  // -  if (SystemFirmwareImageSize != (UINTN) PcdGet32 (PcdFlashAreaSize)) {
  // -    return EFI_INVALID_PARAMETER;
  // -  }
  // -  if (SystemFirmwareImageSize != (UINTN) (PcdGet32 (PcdFlashAreaSize)*2)) {
  // -    return EFI_INVALID_PARAMETER;
  // -  }
  ZeroMem (&OtaCapsuleUpdateVal, sizeof (OTA_CAPSULE_UPDATE));

  /*
    VarSize = sizeof (OTA_CAPSULE_UPDATE);
    Status = gRT->GetVariable (
                    OTA_CAPSULE_VAR_NAME,
                    &gOtaCapsuleUpdateGuid,
                    NULL,
                    &VarSize,
                    (VOID *) &OtaCapsuleUpdateVal
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "GetOtaCapsuleInfo: GetVariable failed: %r\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "UpdateFlag: 0x%x, UpdateSlot: 0x%x\n", OtaCapsuleUpdateVal.UpdateFlag, OtaCapsuleUpdateVal.UpdateSlot));

    mUpdateSlot = OtaCapsuleUpdateVal.UpdateSlot;
    if (mUpdateSlot >= MAX_SLOT_NUM) {
      DEBUG ((DEBUG_ERROR, "Invalid Slot number: %d\n", mUpdateSlot));
      return EFI_NOT_FOUND;
    }
  */
  mUpdateSlot = 1;

  Status = VerifyImageLayout (SystemFirmwareImage, mUpdateSlot, &UpdateOffset, &UpdateSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "VerifyImageLayout failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "UpdateSlot: %d, UpdateOffset: 0x%x, UpdateSize: 0x%x\n", mUpdateSlot, UpdateOffset, UpdateSize));

  *OtaCapsuleOffset = UpdateOffset;
  *OtaCapsuleSize   = UpdateSize;

  return EFI_SUCCESS;
}

/**
  Update active slot information in ISH.

  @param[in]  SlotNum                 The slot number will be set as active.

  @retval EFI_SUCCESS                 Set active slto successfully.
  @retval others                      Some error occurs when executing this routine.

**/
EFI_STATUS
UpdateAbActiveSlot (
  IN UINT8  SlotNum
  )
{
  EFI_STATUS              Status;
  IMAGE_SLOT_HEADER_INFO  IshInfo;
  UINTN                   VarSize;

  DEBUG ((DEBUG_INFO, "UpdateAbActiveSlot...\n"));

  if (SlotNum >= MAX_SLOT_NUM) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&IshInfo, sizeof (IMAGE_SLOT_HEADER_INFO));

  VarSize = sizeof (IMAGE_SLOT_HEADER_INFO);
  Status  = gRT->GetVariable (
                   ISH_VAR_NAME,
                   &gABSupportUpdateIshGuid,
                   NULL,
                   &VarSize,
                   (VOID *)&IshInfo
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get A/B slot info failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Get IshInfo.SlotA_Priority:      0x%x\n", IshInfo.SlotA_Priority));
  DEBUG ((DEBUG_INFO, "Get IshInfo.SlotA_UpdateRetries: 0x%x\n", IshInfo.SlotA_UpdateRetries));
  DEBUG ((DEBUG_INFO, "Get IshInfo.SlotA_GlitchRetries: 0x%x\n", IshInfo.SlotA_GlitchRetries));
  DEBUG ((DEBUG_INFO, "Get IshInfo.SlotB_Priority:      0x%x\n", IshInfo.SlotB_Priority));
  DEBUG ((DEBUG_INFO, "Get IshInfo.SlotB_UpdateRetries: 0x%x\n", IshInfo.SlotB_UpdateRetries));
  DEBUG ((DEBUG_INFO, "Get IshInfo.SlotB_GlitchRetries: 0x%x\n\n", IshInfo.SlotB_GlitchRetries));

  if (SlotNum == SLOT_A) {
    // Slot A
    if (IshInfo.SlotB_Priority == MAX_UINT32) {
      IshInfo.SlotA_Priority = PcdGet32 (PcdFlashAbImageSlotDefaultPriority);
      IshInfo.SlotB_Priority = IshInfo.SlotA_Priority - 1;
    } else {
      IshInfo.SlotA_Priority = MAX (IshInfo.SlotA_Priority, IshInfo.SlotB_Priority) + 1;
    }

    IshInfo.SlotA_UpdateRetries = 0xFF;
  } else if (SlotNum == SLOT_B) {
    // Slot B
    if (IshInfo.SlotA_Priority == MAX_UINT32) {
      IshInfo.SlotB_Priority = PcdGet32 (PcdFlashAbImageSlotDefaultPriority);
      IshInfo.SlotA_Priority = IshInfo.SlotB_Priority - 1;
    } else {
      IshInfo.SlotB_Priority = MAX (IshInfo.SlotA_Priority, IshInfo.SlotB_Priority) + 1;
    }

    IshInfo.SlotB_UpdateRetries = 0xFF;
  }

  DEBUG ((DEBUG_INFO, "Set IshInfo.SlotA_Priority:      0x%x\n", IshInfo.SlotA_Priority));
  DEBUG ((DEBUG_INFO, "Set IshInfo.SlotA_UpdateRetries: 0x%x\n", IshInfo.SlotA_UpdateRetries));
  DEBUG ((DEBUG_INFO, "Set IshInfo.SlotA_GlitchRetries: 0x%x\n", IshInfo.SlotA_GlitchRetries));
  DEBUG ((DEBUG_INFO, "Set IshInfo.SlotB_Priority:      0x%x\n", IshInfo.SlotB_Priority));
  DEBUG ((DEBUG_INFO, "Set IshInfo.SlotB_UpdateRetries: 0x%x\n", IshInfo.SlotB_UpdateRetries));
  DEBUG ((DEBUG_INFO, "Set IshInfo.SlotB_GlitchRetries: 0x%x\n", IshInfo.SlotB_GlitchRetries));

  Status = gRT->SetVariable (
                  ISH_VAR_NAME,
                  &gABSupportUpdateIshGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (IMAGE_SLOT_HEADER_INFO),
                  (VOID *)&IshInfo
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Set Slot info failed: %r\n", Status));
    return Status;
  }

  return Status;
}

/**
  Update System Firmware image.

  @param[in]  SystemFirmwareImage     Points to the System Firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[in]  ConfigImage             Points to the config file image.
  @param[in]  ConfigImageSize         The length of the config file image in bytes.
  @param[out] LastAttemptVersion      The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in]  Progress                A function used by the driver to report the progress of the firmware update.

  @retval EFI_SUCCESS             The System Firmware image is updated.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
UpdateImage (
  IN VOID                                           *SystemFirmwareImage,
  IN UINTN                                          SystemFirmwareImageSize,
  IN VOID                                           *ConfigImage,
  IN UINTN                                          ConfigImageSize,
  OUT UINT32                                        *LastAttemptVersion,
  OUT UINT32                                        *LastAttemptStatus,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress
  )
{
  EFI_STATUS          Status;
  UPDATE_CONFIG_DATA  *ConfigData;
  UPDATE_CONFIG_DATA  *UpdateConfigData;
  CONFIG_HEADER       ConfigHeader;
  UINTN               Index;
  UINTN               TotalSize;
  UINTN               BytesWritten;
  UINTN               StartPercentage;
  UINTN               EndPercentage;
  UINT32              OtaCapsuleOffset;
  UINT32              OtaCapsuleSize;
  UINT32              ECImageSize;
  UINT32              ECImageOffset;

  if (ConfigImage == NULL) {
    DEBUG ((DEBUG_INFO, "PlatformUpdate (NoConfig):"));
    // ASSUME the whole System Firmware include NVRAM region.
    StartPercentage = 0;
    EndPercentage   = 100;
    if (Progress != NULL) {
      Progress (StartPercentage);
    }

    ECImageSize   = 0x20000;
    ECImageOffset = 0x0;

    DEBUG ((DEBUG_INFO, "  BaseAddress - 0x%x,", ECImageOffset));
    DEBUG ((DEBUG_INFO, "  Length - 0x%x\n", ECImageSize));

    Status = PerformFlashWriteWithProgress (
               PlatformFirmwareTypeNvRam,
               (EFI_PHYSICAL_ADDRESS)ECImageOffset,
               FlashAddressTypeRelativeAddress,
               (VOID *)((UINT8 *)SystemFirmwareImage + ECImageOffset),
               ECImageSize,
               Progress,
               StartPercentage,
               EndPercentage
               );
    if (Progress != NULL) {
      Progress (EndPercentage);
    }

    if (!EFI_ERROR (Status)) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
      mNvRamUpdated      = TRUE;
    } else {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
    }

    StartPercentage = 0;
    EndPercentage   = 100;
    if (Progress != NULL) {
      Progress (StartPercentage);
    }

    Status = GetOtaCapsuleInfo (SystemFirmwareImage, SystemFirmwareImageSize, &OtaCapsuleOffset, &OtaCapsuleSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "GetOtaCapsuleInfo failed: %r\n", Status));
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
      return Status;
    }

    DEBUG ((DEBUG_INFO, "  BaseAddress - 0x%x,", OtaCapsuleOffset));
    DEBUG ((DEBUG_INFO, "  Length - 0x%x\n", OtaCapsuleSize));

    Status = PerformFlashWriteWithProgress (
               PlatformFirmwareTypeNvRam,
               (EFI_PHYSICAL_ADDRESS)OtaCapsuleOffset,
               FlashAddressTypeRelativeAddress,
               (VOID *)((UINT8 *)SystemFirmwareImage + OtaCapsuleOffset),
               OtaCapsuleSize,
               Progress,
               StartPercentage,
               EndPercentage
               );
    if (Progress != NULL) {
      Progress (EndPercentage);
    }

    if (!EFI_ERROR (Status)) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
      mNvRamUpdated      = TRUE;
    } else {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
    }

    Status = GetOtaCapsuleInfoSlotB (SystemFirmwareImage, SystemFirmwareImageSize, &OtaCapsuleOffset, &OtaCapsuleSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "GetOtaCapsuleInfo failed: %r\n", Status));
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
      return Status;
    }

    DEBUG ((DEBUG_INFO, "  BaseAddress - 0x%x,", OtaCapsuleOffset));
    DEBUG ((DEBUG_INFO, "  Length - 0x%x\n", OtaCapsuleSize));

    Status = PerformFlashWriteWithProgress (
               PlatformFirmwareTypeNvRam,
               (EFI_PHYSICAL_ADDRESS)OtaCapsuleOffset,
               FlashAddressTypeRelativeAddress,
               (VOID *)((UINT8 *)SystemFirmwareImage + OtaCapsuleOffset),
               OtaCapsuleSize,
               Progress,
               StartPercentage,
               EndPercentage
               );
    if (Progress != NULL) {
      Progress (EndPercentage);
    }

    if (!EFI_ERROR (Status)) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
      mNvRamUpdated      = TRUE;
    } else {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
    }

    return Status;
  }

  DEBUG ((DEBUG_INFO, "PlatformUpdate (With Config):\n"));
  ConfigData = NULL;
  ZeroMem (&ConfigHeader, sizeof (ConfigHeader));
  Status = ParseUpdateDataFile (
             ConfigImage,
             ConfigImageSize,
             &ConfigHeader,
             &ConfigData
             );
  DEBUG ((DEBUG_INFO, "ParseUpdateDataFile - %r\n", Status));
  if (EFI_ERROR (Status)) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "ConfigHeader.NumOfUpdates - 0x%x\n", ConfigHeader.NumOfUpdates));
  DEBUG ((DEBUG_INFO, "PcdEdkiiSystemFirmwareFileGuid - %g\n", PcdGetPtr (PcdEdkiiSystemFirmwareFileGuid)));

  TotalSize = 0;
  for (Index = 0; Index < ConfigHeader.NumOfUpdates; Index++) {
    if (CompareGuid (&ConfigData[Index].FileGuid, PcdGetPtr (PcdEdkiiSystemFirmwareFileGuid))) {
      TotalSize = TotalSize + ConfigData[Index].Length;
    }
  }

  BytesWritten     = 0;
  Index            = 0;
  UpdateConfigData = ConfigData;
  while (Index < ConfigHeader.NumOfUpdates) {
    if (CompareGuid (&UpdateConfigData->FileGuid, PcdGetPtr (PcdEdkiiSystemFirmwareFileGuid))) {
      DEBUG ((DEBUG_INFO, "FileGuid - %g (processing)\n", &UpdateConfigData->FileGuid));
      StartPercentage = (BytesWritten * 100) / TotalSize;
      EndPercentage   = ((BytesWritten + UpdateConfigData->Length) * 100) / TotalSize;
      Status          = PerformUpdate (
                          SystemFirmwareImage,
                          SystemFirmwareImageSize,
                          UpdateConfigData,
                          LastAttemptVersion,
                          LastAttemptStatus,
                          Progress,
                          StartPercentage,
                          EndPercentage
                          );
      //
      // Shall updates be serialized so that if an update is not successfully completed,
      // the remaining updates won't be performed.
      //
      if (EFI_ERROR (Status)) {
        break;
      }
    } else {
      DEBUG ((DEBUG_INFO, "FileGuid - %g (ignored)\n", &UpdateConfigData->FileGuid));
    }

    BytesWritten += UpdateConfigData->Length;

    Index++;
    UpdateConfigData++;
  }

  return Status;
}

/**
  Authenticate and update System Firmware image.

  Caution: This function may receive untrusted input.

  @param[in]  Image              The EDKII system FMP capsule image.
  @param[in]  ImageSize          The size of the EDKII system FMP capsule image in bytes.
  @param[out] LastAttemptVersion The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in]  Progress           A function used by the driver to report the progress of the firmware update.

  @retval EFI_SUCCESS             EDKII system FMP capsule passes authentication and the System Firmware image is updated.
  @retval EFI_SECURITY_VIOLATION  EDKII system FMP capsule fails authentication and the System Firmware image is not updated.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
SystemFirmwareAuthenticatedUpdate (
  IN VOID                                           *Image,
  IN UINTN                                          ImageSize,
  OUT UINT32                                        *LastAttemptVersion,
  OUT UINT32                                        *LastAttemptStatus,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress
  )
{
  EFI_STATUS  Status;
  VOID        *SystemFirmwareImage;
  UINTN       SystemFirmwareImageSize;
  VOID        *ConfigImage;
  UINTN       ConfigImageSize;
  VOID        *AuthenticatedImage;
  UINTN       AuthenticatedImageSize;

  AuthenticatedImage     = NULL;
  AuthenticatedImageSize = 0;

  DEBUG ((DEBUG_INFO, "SystemFirmwareAuthenticatedUpdate...\n"));

  Status = CapsuleAuthenticateSystemFirmware (Image, ImageSize, FALSE, LastAttemptVersion, LastAttemptStatus, &AuthenticatedImage, &AuthenticatedImageSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SystemFirmwareAuthenticateImage - %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "ExtractSystemFirmwareImage ...\n"));
  ExtractSystemFirmwareImage (AuthenticatedImage, AuthenticatedImageSize, &SystemFirmwareImage, &SystemFirmwareImageSize);
  DEBUG ((DEBUG_INFO, "ExtractConfigImage ...\n"));
  ExtractConfigImage (AuthenticatedImage, AuthenticatedImageSize, &ConfigImage, &ConfigImageSize);

  DEBUG ((DEBUG_INFO, "UpdateImage ...\n"));
  Status = UpdateImage (SystemFirmwareImage, SystemFirmwareImageSize, ConfigImage, ConfigImageSize, LastAttemptVersion, LastAttemptStatus, Progress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UpdateImage - %r\n", Status));
    return Status;
  }

 #if 0
  // DO NOT KNOW THE REASON to update A/B Active Slot.
  // Removed FOR NOW.

  //
  // Update A/B active slot info
  //
  Status = UpdateAbActiveSlot (mUpdateSlot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UpdateAbActiveSlot failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "SystemFirmwareAuthenticatedUpdate Done\n"));
 #endif
  return Status;
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param[in]      VariableName               Name of Variable to be found.
  @param[in]      VendorGuid                 Variable vendor GUID.
  @param[out]     Attributes                 Attribute value of the variable found.
  @param[in, out] DataSize                   Size of Data found. If size is less than the
                                             data, this value contains the required size.
  @param[out]     Data                       Data pointer.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
GetVariableHook (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data
  )
{
  DEBUG ((DEBUG_INFO, "GetVariableHook - %S, %g\n", VariableName, VendorGuid));
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This code Finds the Next available variable.

  @param[in, out] VariableNameSize           Size of the variable name.
  @param[in, out] VariableName               Pointer to variable name.
  @param[in, out] VendorGuid                 Variable Vendor Guid.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
GetNextVariableNameHook (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  DEBUG ((DEBUG_INFO, "GetNextVariableNameHook - %S, %g\n", VariableName, VendorGuid));
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param[in] VariableName                     Name of Variable to be found.
  @param[in] VendorGuid                       Variable vendor GUID.
  @param[in] Attributes                       Attribute value of the variable found
  @param[in] DataSize                         Size of Data found. If size is less than the
                                              data, this value contains the required size.
  @param[in] Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
SetVariableHook (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  DEBUG ((DEBUG_INFO, "SetVariableHook - %S, %g, 0x%x (0x%x)\n", VariableName, VendorGuid, Attributes, DataSize));
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This code returns information about the EFI variables.

  @param[in]  Attributes                     Attributes bitmask to specify the type of variables
                                             on which to return information.
  @param[out] MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                             for the EFI variables associated with the attributes specified.
  @param[out] RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                             for EFI variables associated with the attributes specified.
  @param[out] MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                             associated with the attributes specified.

  @return EFI_SUCCESS                   Query successfully.

**/
EFI_STATUS
EFIAPI
QueryVariableInfoHook (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  DEBUG ((DEBUG_INFO, "QueryVariableInfoHook - 0x%x\n", Attributes));
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Updates the firmware image of the device.

  This function updates the hardware with the new firmware image.
  This function returns EFI_UNSUPPORTED if the firmware image is not updatable.
  If the firmware image is updatable, the function should perform the following minimal validations
  before proceeding to do the firmware image update.
  - Validate the image authentication if image has attribute
    IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED. The function returns
    EFI_SECURITY_VIOLATION if the validation fails.
  - Validate the image is a supported image for this device. The function returns EFI_ABORTED if
    the image is unsupported. The function can optionally provide more detailed information on
    why the image is not a supported image.
  - Validate the data from VendorCode if not null. Image validation must be performed before
    VendorCode data validation. VendorCode data is ignored or considered invalid if image
    validation failed. The function returns EFI_ABORTED if the data is invalid.

  VendorCode enables vendor to implement vendor-specific firmware image update policy. Null if
  the caller did not specify the policy or use the default policy. As an example, vendor can implement
  a policy to allow an option to force a firmware image update when the abort reason is due to the new
  firmware image version is older than the current firmware image version or bad image checksum.
  Sensitive operations such as those wiping the entire firmware image and render the device to be
  non-functional should be encoded in the image itself rather than passed with the VendorCode.
  AbortReason enables vendor to have the option to provide a more detailed description of the abort
  reason to the caller.

  @param[in]  This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]  ImageIndex         A unique number identifying the firmware image(s) within the device.
                                 The number is between 1 and DescriptorCount.
  @param[in]  Image              Points to the new image.
  @param[in]  ImageSize          Size of the new image in bytes.
  @param[in]  VendorCode         This enables vendor to implement vendor-specific firmware image update policy.
                                 Null indicates the caller did not specify the policy or use the default policy.
  @param[in]  Progress           A function used by the driver to report the progress of the firmware update.
  @param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
                                 details for the aborted operation. The buffer is allocated by this function
                                 with AllocatePool(), and it is the caller's responsibility to free it with a
                                 call to FreePool().

  @retval EFI_SUCCESS            The device was successfully updated with the new image.
  @retval EFI_ABORTED            The operation is aborted.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATION The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
FmpSetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL               *This,
  IN  UINT8                                          ImageIndex,
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  OUT CHAR16                                         **AbortReason
  )
{
  EFI_STATUS               Status;
  EFI_STATUS               VarStatus;
  SYSTEM_FMP_PRIVATE_DATA  *SystemFmpPrivate;

  if ((Image == NULL) || (ImageSize == 0) || (AbortReason == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SystemFmpPrivate = SYSTEM_FMP_PRIVATE_DATA_FROM_FMP (This);
  *AbortReason     = NULL;

  if ((ImageIndex == 0) || (ImageIndex > SystemFmpPrivate->DescriptorCount)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SystemFirmwareAuthenticatedUpdate ((VOID *)Image, ImageSize, &SystemFmpPrivate->LastAttempt.LastAttemptVersion, &SystemFmpPrivate->LastAttempt.LastAttemptStatus, Progress);
  DEBUG ((DEBUG_INFO, "SetImage - LastAttempt Version - 0x%x, State - 0x%x\n", SystemFmpPrivate->LastAttempt.LastAttemptVersion, SystemFmpPrivate->LastAttempt.LastAttemptStatus));

  //
  // If NVRAM is updated, we should no longer touch variable services, because
  // the current variable driver may not manage the new NVRAM region.
  //
  if (mNvRamUpdated) {
    DEBUG ((DEBUG_INFO, "NvRamUpdated, Update Variable Serivces\n"));
    gRT->GetVariable         = GetVariableHook;
    gRT->GetNextVariableName = GetNextVariableNameHook;
    gRT->SetVariable         = SetVariableHook;
    gRT->QueryVariableInfo   = QueryVariableInfoHook;

    gRT->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
           (UINT8 *)&gRT->Hdr,
           gRT->Hdr.HeaderSize,
           &gRT->Hdr.CRC32
           );
  }

  VarStatus = gRT->SetVariable (
                     SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                     &gSystemFmpLastAttemptVariableGuid,
                     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                     sizeof (SystemFmpPrivate->LastAttempt),
                     &SystemFmpPrivate->LastAttempt
                     );
  DEBUG ((DEBUG_INFO, "SetLastAttemp - %r\n", VarStatus));

  return Status;
}

/**
  Get the set of EFI_FIRMWARE_IMAGE_DESCRIPTOR structures from an FMP Protocol.

  @param[in]  Handle             Handle with an FMP Protocol or a System FMP
                                 Protocol.
  @param[in]  ProtocolGuid       Pointer to the FMP Protocol GUID or System FMP
                                 Protocol GUID.
  @param[out] FmpImageInfoCount  Pointer to the number of
                                 EFI_FIRMWARE_IMAGE_DESCRIPTOR structures.
  @param[out] DescriptorSize     Pointer to the size, in bytes, of each
                                 EFI_FIRMWARE_IMAGE_DESCRIPTOR structure.

  @return NULL   No EFI_FIRMWARE_IMAGE_DESCRIPTOR structures found.
  @return !NULL  Pointer to a buffer of EFI_FIRMWARE_IMAGE_DESCRIPTOR structures
                 allocated using AllocatePool().  Caller must free buffer with
                 FreePool().
**/
EFI_FIRMWARE_IMAGE_DESCRIPTOR *
GetFmpImageDescriptors (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *ProtocolGuid,
  OUT UINT8       *FmpImageInfoCount,
  OUT UINTN       *DescriptorSize
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             ImageInfoSize;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;

  *FmpImageInfoCount = 0;
  *DescriptorSize    = 0;

  Status = gBS->HandleProtocol (
                  Handle,
                  ProtocolGuid,
                  (VOID **)&Fmp
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Determine the size required for the set of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  //
  ImageInfoSize = 0;
  Status        = Fmp->GetImageInfo (
                         Fmp,                        // FMP Pointer
                         &ImageInfoSize,             // Buffer Size (in this case 0)
                         NULL,                       // NULL so we can get size
                         &FmpImageInfoDescriptorVer, // DescriptorVersion
                         FmpImageInfoCount,          // DescriptorCount
                         DescriptorSize,             // DescriptorSize
                         &PackageVersion,            // PackageVersion
                         &PackageVersionName         // PackageVersionName
                         );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "SystemFirmwareUpdateDxe: Unexpected Failure.  Status = %r\n", Status));
    return NULL;
  }

  //
  // Allocate buffer for the set of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  //
  FmpImageInfoBuf = NULL;
  FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
  if (FmpImageInfoBuf == NULL) {
    DEBUG ((DEBUG_ERROR, "SystemFirmwareUpdateDxe: Failed to allocate memory for descriptors.\n"));
    return NULL;
  }

  //
  // Retrieve the set of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  //
  PackageVersionName = NULL;
  Status             = Fmp->GetImageInfo (
                              Fmp,
                              &ImageInfoSize,             // ImageInfoSize
                              FmpImageInfoBuf,            // ImageInfo
                              &FmpImageInfoDescriptorVer, // DescriptorVersion
                              FmpImageInfoCount,          // DescriptorCount
                              DescriptorSize,             // DescriptorSize
                              &PackageVersion,            // PackageVersion
                              &PackageVersionName         // PackageVersionName
                              );

  //
  // Free unused PackageVersionName return buffer
  //
  if (PackageVersionName != NULL) {
    FreePool (PackageVersionName);
    PackageVersionName = NULL;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SystemFirmwareUpdateDxe: Failure in GetImageInfo.  Status = %r\n", Status));
    if (FmpImageInfoBuf != NULL) {
      FreePool (FmpImageInfoBuf);
    }

    return NULL;
  }

  return FmpImageInfoBuf;
}

/**
  Search for handles with an FMP protocol whose EFI_FIRMWARE_IMAGE_DESCRIPTOR
  ImageTypeId matches the ImageTypeId produced by this module.

  @param[in]  ProtocolGuid  Pointer to the GUID of the protocol to search.
  @param[out] HandleCount   Pointer to the number of returned handles.

  @return NULL   No matching handles found.
  @return !NULL  Pointer to a buffer of handles allocated using AllocatePool().
                 Caller must free buffer with FreePool().
**/
EFI_HANDLE *
FindMatchingFmpHandles (
  IN  EFI_GUID  *ProtocolGuid,
  OUT UINTN     *HandleCount
  )
{
  EFI_STATUS                     Status;
  UINTN                          TempHandleCount;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;
  UINTN                          Index2;
  UINTN                          Index3;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *OriginalFmpImageInfoBuf;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfoBuf;
  UINT8                          FmpImageInfoCount;
  UINTN                          DescriptorSize;
  BOOLEAN                        MatchFound;

  *HandleCount    = 0;
  TempHandleCount = 0;
  HandleBuffer    = NULL;
  Status          = gBS->LocateHandleBuffer (
                           ByProtocol,
                           ProtocolGuid,
                           NULL,
                           &TempHandleCount,
                           &HandleBuffer
                           );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < TempHandleCount; Index++) {
    OriginalFmpImageInfoBuf = GetFmpImageDescriptors (
                                HandleBuffer[Index],
                                ProtocolGuid,
                                &FmpImageInfoCount,
                                &DescriptorSize
                                );

    //
    // Loop through the set of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
    //
    MatchFound = FALSE;
    if (OriginalFmpImageInfoBuf != NULL) {
      FmpImageInfoBuf = OriginalFmpImageInfoBuf;

      for (Index2 = 0; Index2 < FmpImageInfoCount; Index2++) {
        for (Index3 = 0; Index3 < mSystemFmpPrivate->DescriptorCount; Index3++) {
          MatchFound = CompareGuid (
                         &FmpImageInfoBuf->ImageTypeId,
                         &mSystemFmpPrivate->ImageDescriptor[Index3].ImageTypeId
                         );
          if (MatchFound) {
            break;
          }
        }

        if (MatchFound) {
          break;
        }

        //
        // Increment the buffer pointer ahead by the size of the descriptor
        //
        FmpImageInfoBuf = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)(((UINT8 *)FmpImageInfoBuf) + DescriptorSize);
      }

      if (MatchFound) {
        HandleBuffer[*HandleCount] = HandleBuffer[Index];
        (*HandleCount)++;
      }

      FreePool (OriginalFmpImageInfoBuf);
    }
  }

  if ((*HandleCount) == 0) {
    //
    // No any matching handle.
    //
    FreePool (HandleBuffer);
    return NULL;
  }

  return HandleBuffer;
}

/**
  Uninstall System FMP Protocol instances that may have been installed by
  SystemFirmwareUpdateDxe drivers dispatches by other capsules.

  @retval EFI_SUCCESS  All System FMP Protocols found were uninstalled.
  @return Other        One or more System FMP Protocols could not be uninstalled.

**/
EFI_STATUS
UninstallMatchingSystemFmpProtocols (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             Index;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *SystemFmp;

  //
  // Uninstall SystemFmpProtocol instances that may have been produced by
  // the SystemFirmwareUpdate drivers in FVs dispatched by other capsules.
  //
  HandleBuffer = FindMatchingFmpHandles (
                   &gSystemFmpProtocolGuid,
                   &HandleCount
                   );
  DEBUG ((DEBUG_INFO, "SystemFirmwareUpdateDxe: Found %d matching System FMP instances\n", HandleCount));

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gSystemFmpProtocolGuid,
                    (VOID **)&SystemFmp
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "SystemFirmwareUpdateDxe: Uninstall SystemFmp produced by another capsule\n"));
    Status = gBS->UninstallProtocolInterface (
                    HandleBuffer[Index],
                    &gSystemFmpProtocolGuid,
                    SystemFmp
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SystemFirmwareUpdateDxe: Failed to uninstall SystemFmp %r.  Exiting.\n", Status));
      FreePool (HandleBuffer);
      return Status;
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}

/**
  System FMP module entrypoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS           System FMP module is initialized.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources avaulable to
                                initialize this module.
  @retval Other                 System FMP Protocols could not be uninstalled.
  @retval Other                 System FMP Protocol could not be installed.
  @retval Other                 FMP Protocol could not be installed.
**/
EFI_STATUS
EFIAPI
SystemFirmwareUpdateMainDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleCount;

  //
  // Initialize SystemFmpPrivateData
  //
  mSystemFmpPrivate = AllocateZeroPool (sizeof (SYSTEM_FMP_PRIVATE_DATA));
  if (mSystemFmpPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitializePrivateData (mSystemFmpPrivate);
  if (EFI_ERROR (Status)) {
    FreePool (mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
    return Status;
  }

  //
  // Uninstall SystemFmpProtocol instances that may have been produced by
  // the SystemFirmwareUpdate drivers in FVs dispatched by other capsules.
  //
  Status = UninstallMatchingSystemFmpProtocols ();
  if (EFI_ERROR (Status)) {
    FreePool (mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
    return Status;
  }

  //
  // Look for a handle with matching Firmware Management Protocol
  //
  HandleCount  = 0;
  HandleBuffer = FindMatchingFmpHandles (
                   &gEfiFirmwareManagementProtocolGuid,
                   &HandleCount
                   );
  DEBUG ((DEBUG_INFO, "SystemFirmwareUpdateDxe: Found %d matching FMP instances\n", HandleCount));

  switch (HandleCount) {
    case 0:
      //
      // Install FMP protocol onto a new handle.
      //
      DEBUG ((DEBUG_INFO, "SystemFirmwareUpdateDxe: Install FMP onto a new handle\n"));
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mSystemFmpPrivate->Handle,
                      &gEfiFirmwareManagementProtocolGuid,
                      &mSystemFmpPrivate->Fmp,
                      NULL
                      );
      break;
    case 1:
      //
      // Install System FMP protocol onto handle with matching FMP Protocol
      //
      DEBUG ((DEBUG_INFO, "SystemFirmwareUpdateDxe: Install System FMP onto matching FMP handle\n"));
      mSystemFmpPrivate->Handle = HandleBuffer[0];
      Status                    = gBS->InstallMultipleProtocolInterfaces (
                                         &HandleBuffer[0],
                                         &gSystemFmpProtocolGuid,
                                         &mSystemFmpPrivate->Fmp,
                                         NULL
                                         );
      break;
    default:
      //
      // More than one matching handle is not expected.  Unload driver.
      //
      DEBUG ((DEBUG_ERROR, "SystemFirmwareUpdateDxe: More than one matching FMP handle.  Unload driver.\n"));
      Status = EFI_DEVICE_ERROR;
      break;
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (EFI_ERROR (Status)) {
    FreePool (mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
  }

  return Status;
}
