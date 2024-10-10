/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/EventGroup.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/FirmwareUpdateLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/MmCommunication2.h>
#include <Uefi/UefiBaseType.h>

#include "SystemFirmwareUpdateDxe.h"

#define FWU_STR_TYPE_LENGTH    32
#define FWU_STR_STATUS_LENGTH  64

static EFI_GET_VARIABLE  mOriginGetVariable;
static EFI_SET_VARIABLE  mOriginSetVariable;

static UINT8      *mDataBuf     = NULL;
static UINT8      *mVirtDataBuf = NULL;
static UINTN      mDataCount;
static EFI_EVENT  mVirtualAddressChangeEvent = NULL;
static UINTN      mUpdateState               = FWU_STATE_NOT_STARTED;
static UINTN      mUpdateFWImageId;
static UINTN      mUpdateFWSubId;
static UINT64     mUpdateStatus;
static UINTN      mUpdateFWPayloadBytes;
static UINT64     mUpdateFWProcess;

/**
  Handles GetVariable runtime service calls.

  This function will filter action get variable with VendorGuid is gAmpereFWUpgradeGuid and redirect to
  firmware update MM communicate. Otherwise, call UEFI Runtime Service GetVariable().

  @param  VariableName The name of the vendor's variable, it's a Null-Terminated Unicode String
  @param  VendorGuid   A unique identifier for the vendor.
  @param  Attributes   Pointer to memory location to return the attributes of variable. If the pointer
                       is NULL, the parameter would be ignored.
  @param  DataSize     As input, pointer to the maximum size of return Data-Buffer.
                       As output, pointer to the actual size of the returned Data-Buffer.
  @param  Data         Pointer to return Data-Buffer.

  @retval EFI_SUCCESS            Get response data successfully.
  @retval EFI_BUFFER_TOO_SMALL   Input Data size is invalid.
**/
EFI_STATUS
SystemFirmwareUpdateGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  CHAR8       AsciiStrType[FWU_STR_TYPE_LENGTH];
  CHAR8       AsciiStrStatus[FWU_STR_STATUS_LENGTH];

  if ((VendorGuid == NULL) || (!CompareGuid (VendorGuid, &gAmpereFWUpgradeGuid))) {
    return mOriginGetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  if ((VariableName == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Fill in attributes if not null.
  //
  if (Attributes != NULL) {
    *Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  // The return is a ASCII string with format "<TypeFWUpgrading>,<Status>"
  if (mUpdateState == FWU_STATE_NOT_STARTED) {
    AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_NULL);
    AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_NOTSTART);
  } else {
    switch (mUpdateFWImageId) {
      case FWU_IMG_ID_SCP:
        AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_SCP);
        break;
      case FWU_IMG_ID_ATFUEFI:
        AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_ATFUEFI);
        break;
      case FWU_IMG_ID_CFGUEFI:
        AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_CFGUEFI);
        break;
      case FWU_IMG_ID_UEFI:
        AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_UEFI);
        break;
      case FWU_IMG_ID_SINGLE_ATFUEFI:
        switch (mUpdateFWSubId) {
          case FWU_IMG_SUBID_SINGLE_ATFUEFI_FULL_FLASH:
            AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_SINGLE_ATFUEFI_FULL_FLASH);
            break;
          case FWU_IMG_SUBID_SINGLE_ATFUEFI_FW:
            AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_SINGLE_ATFUEFI_CLEAR_SETTING);
            break;
          case FWU_IMG_SUBID_SINGLE_ATFUEFI_CLEAR_SETTING:
            AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_SINGLE_ATFUEFI_FW);
            break;
          default:
            AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_SINGLE_ATFUEFI_UNKNOWN);
            break;
        }

        break;
      default:
        AsciiSPrint (AsciiStrType, sizeof (AsciiStrType), FWU_IMG_STR_UNKNOWN);
        break;
    }

    if (mUpdateStatus == FWU_RET_IN_PROGRESS) {
      // Send request to have upgrading continue
      Status = FWUpdateProcess (
                 mUpdateFWImageId,
                 mUpdateFWSubId,
                 TRUE,
                 NULL,
                 0,
                 &mUpdateStatus,
                 &mUpdateFWProcess
                 );
      if (EFI_ERROR (Status)) {
        AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), "%r", Status);
      } else {
        AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_IN_PROCESS ",%d", mUpdateFWProcess);
      }
    } else {
      switch (mUpdateStatus) {
        case FWU_RET_SUCCESS:
          AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_SUCCESS);
          break;
        case FWU_RET_ERR_IO_ERROR:
          AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_IO_ERROR);
          break;
        case FWU_RET_ERR_OUT_OF_RESOURCES:
          AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_OUT_OF_RESOURCE);
          break;
        case FWU_RET_ERR_SECURITY_VIOLATION:
          AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_SECURITY_VIOLATION);
          break;
        case FWU_RET_ERR_GENERIC:
          AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_FAILED);
          break;
        default:
          AsciiSPrint (AsciiStrStatus, sizeof (AsciiStrStatus), FWU_STATUS_STR_UNKNOWN ":%X", mUpdateStatus);
          break;
      }
    }
  }

  Size = AsciiStrLen (AsciiStrType) + AsciiStrLen (AsciiStrStatus) +  sizeof (CHAR16); /* 2 because "," plus Null-terminal */
  if (*DataSize < Size) {
    *DataSize = Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsciiSPrint (Data, Size, "%a,%a", AsciiStrType, AsciiStrStatus);

  return EFI_SUCCESS;
}

/**
  Handles SetVariable runtime service calls.

  This function will filter action set variable with VendorGuid is gAmpereFWUpgradeGuid and redirect to
  firmware update MM communicate. Otherwise, call UEFI Runtime Service SetVariable().

  @param  VariableName The name of the vendor's variable, as a
                       Null-Terminated Unicode String
  @param  VendorGuid   A unique identifier for the vendor.
  @param  Attributes   Pointer to memory location to return the attributes of variable. If the pointer
                       is NULL, the parameter would be ignored.
  @param  DataSize     The size in bytes of Data-Buffer.
  @param  Data         Pointer to the content of the variable.

  @retval EFI_INVALID_PARAMETER        An invalid data parameter or an invalid
                                       combination of data parameters.

**/
EFI_STATUS
SystemFirmwareUpdateSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_STATUS  Status;
  UINTN       ImageId, SubId;

  if ((VendorGuid == NULL) || (!CompareGuid (VendorGuid, &gAmpereFWUpgradeGuid))) {
    return mOriginSetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  if (((mDataCount == 0) && (DataSize == 0)) ||
      (DataSize > FIRMWARE_UPDATE_MAX_SIZE) ||
      (mDataCount > FIRMWARE_UPDATE_MAX_SIZE) ||
      (VariableName == NULL) ||
      (Data == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (StrCmp (VariableName, FWU_VARIABLE_SETUP_LOAD_OFFSET) == 0) {
    if (DataSize != sizeof (UINT32)) {
      return EFI_INVALID_PARAMETER;
    }

    mDataCount = *((UINT32 *)Data);
    return EFI_SUCCESS;
  } else if (StrCmp (VariableName, FWU_VARIABLE_CONTINUE_UPLOAD) == 0) {
    CopyMem ((VOID *)(mVirtDataBuf + mDataCount), (VOID *)Data, DataSize);
    mDataCount += DataSize;
    return EFI_SUCCESS;
  }

  if (StrCmp (VariableName, FWU_VARIABLE_SCP_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_SCP;
  } else if (StrCmp (VariableName, FWU_VARIABLE_ATFUEFI_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_ATFUEFI;
  } else if (StrCmp (VariableName, FWU_VARIABLE_CFGUEFI_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_CFGUEFI;
  } else if (StrCmp (VariableName, FWU_VARIABLE_UEFI_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_UEFI;
  } else if (StrCmp (VariableName, FWU_VARIABLE_SINGLE_IMG_FULL_FLASH_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_SINGLE_ATFUEFI;
    SubId   = FWU_IMG_SUBID_SINGLE_ATFUEFI_FULL_FLASH;
  } else if (StrCmp (VariableName, FWU_VARIABLE_SINGLE_IMG_FWONLY_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_SINGLE_ATFUEFI;
    SubId   = FWU_IMG_SUBID_SINGLE_ATFUEFI_FW;
  } else if (StrCmp (VariableName, FWU_VARIABLE_SINGLE_IMG_CLEAR_SETTING_REQUEST) == 0) {
    ImageId = FWU_IMG_ID_SINGLE_ATFUEFI;
    SubId   = FWU_IMG_SUBID_SINGLE_ATFUEFI_CLEAR_SETTING;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if ((mUpdateState == FWU_STATE_UPLOADING_STARTED) && (mUpdateStatus == FWU_RET_IN_PROGRESS)) {
    // On upgrading process, just return successfully
    return EFI_SUCCESS;
  }

  mUpdateFWImageId      = ImageId;
  mUpdateFWSubId        = SubId;
  mUpdateFWPayloadBytes = (mDataCount != 0) ? mDataCount : DataSize;
  mUpdateFWProcess      = 0;

  if (mDataCount == 0) {
    CopyMem ((VOID *)mVirtDataBuf, (VOID *)Data, DataSize);
  }

  mDataCount = 0;
  Status     = FWUpdateProcess (
                 mUpdateFWImageId,
                 mUpdateFWSubId,
                 FALSE,
                 (UINT8 *)mDataBuf,
                 mUpdateFWPayloadBytes,
                 &mUpdateStatus,
                 &mUpdateFWProcess
                 );
  if (!EFI_ERROR (Status)) {
    mUpdateState = FWU_STATE_UPLOADING_STARTED;
  }

  return Status;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
static
VOID
EFIAPI
SystemFwuVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mVirtDataBuf);
  EfiConvertPointer (0x0, (VOID **)&mOriginGetVariable);
  EfiConvertPointer (0x0, (VOID **)&mOriginSetVariable);
}

/**
  Override a pointer in the EFI System Table.

  @param[in,out]  AddressToUpdate   The pointer to the original pointer to be overrided.
  @param[in]      NewPointer        The new pointer to override.
  @param[out]     OriginalPointer   The pointer to the return original pointer.

  @retval EFI_SUCCESS   The pointer is overrided successfully.
  @retval Others         An error occurred.

**/
static
EFI_STATUS
SystemFirmwareUpdateOverridePointer (
  IN OUT VOID  **AddressToUpdate,
  IN VOID      *NewPointer,
  OUT VOID     **OriginalPointer OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_TPL     Tpl;

  ASSERT (*AddressToUpdate != NewPointer);

  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Save the current value if needed and update the pointer.
  //
  if (OriginalPointer != NULL) {
    *OriginalPointer = *AddressToUpdate;
  }

  *AddressToUpdate = NewPointer;

  //
  // Update the CRC32 in the EFI System Table header.
  //
  gST->Hdr.CRC32 = 0;
  Status         = gBS->CalculateCrc32 (&gST->Hdr, gST->Hdr.HeaderSize, &gST->Hdr.CRC32);
  ASSERT_EFI_ERROR (Status);

  gBS->RestoreTPL (Tpl);
  return Status;
}

/**
  Cleans up changes made by this module and release resources.

  This function handles pertical clean up and is safe

**/
static
VOID
SystemFirmwareUpdateCleanup (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mDataBuf != NULL) {
    FreePool (mDataBuf);
  }

  //
  // Close the End of DXE event.
  //
  if (mVirtualAddressChangeEvent != NULL) {
    Status = gBS->CloseEvent (mVirtualAddressChangeEvent);
    ASSERT_EFI_ERROR (Status);
    mVirtualAddressChangeEvent = NULL;
  }

  if (gRT->SetVariable == SystemFirmwareUpdateSetVariable) {
    Status = SystemFirmwareUpdateOverridePointer (
               (VOID **)&gRT->SetVariable,
               (VOID *)mOriginSetVariable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (gRT->GetVariable == SystemFirmwareUpdateGetVariable) {
    Status = SystemFirmwareUpdateOverridePointer (
               (VOID **)&gRT->GetVariable,
               (VOID *)mOriginGetVariable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  The constructor function.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully .
**/
EFI_STATUS
EFIAPI
SystemFirmwareUpdateDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mDataBuf     = AllocateRuntimeZeroPool (FIRMWARE_UPDATE_MAX_SIZE);
  mVirtDataBuf = mDataBuf;
  ASSERT (mDataBuf != NULL);

  mDataCount            = 0;
  mUpdateFWImageId      = 0;
  mUpdateFWSubId        = 0;
  mUpdateStatus         = FWU_STATE_NOT_STARTED;
  mUpdateFWPayloadBytes = 0;
  mUpdateFWProcess      = 0;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SystemFwuVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install hooks.
  //
  Status = SystemFirmwareUpdateOverridePointer (
             (VOID **)&gRT->GetVariable,
             (VOID *)SystemFirmwareUpdateGetVariable,
             (VOID **)&mOriginGetVariable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverrideRuntimeService(GetVariable) failed : %r\n", Status));
    goto Exit;
  }

  Status = SystemFirmwareUpdateOverridePointer (
             (VOID **)&gRT->SetVariable,
             (VOID *)SystemFirmwareUpdateSetVariable,
             (VOID **)&mOriginSetVariable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverrideRuntimeService(SetVariable) failed : %r\n", Status));
    goto Exit;
  }

Exit:
  if (EFI_ERROR (Status)) {
    SystemFirmwareUpdateCleanup ();
  }

  return Status;
}
