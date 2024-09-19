/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformManager.h"

PLATFORM_MANAGER_CALLBACK_DATA  gPlatformManagerPrivate = {
  NULL,
  NULL
};

EFI_GUID  mPlatformManagerGuid = FORMSET_GUID;

HII_VENDOR_DEVICE_PATH  mPlatformManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {FC587265-0750-44D1-B68D-D1DDD3F29B0B}
    //
    { 0xFC587265, 0x0750, 0x44D1, { 0xB6, 0x8D, 0xD1, 0xDD, 0xD3, 0xF2, 0x9B, 0x0B }
    }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
PmExtractDevicePathFromHiiHandle (
  IN EFI_HII_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get device path string.
  //
  return ConvertDevicePathToText (DevicePathFromHandle (DriverHandle), FALSE, FALSE);
}

/**
  Dynamic create Hii information for Platform Manager.

  @param   NextShowFormId     The FormId which need to be show.

**/
VOID
CreatePlatformManagerForm (
  IN EFI_FORM_ID  NextShowFormId
  )
{
  UINTN               Index;
  EFI_STRING          String;
  EFI_STRING_ID       Token;
  EFI_STRING_ID       TokenHelp;
  EFI_HII_HANDLE      *HiiHandles;
  EFI_HII_HANDLE      HiiHandle;
  EFI_GUID            FormSetGuid;
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;
  CHAR16              *DevicePathStr;
  EFI_STRING_ID       DevicePathId;
  EFI_IFR_FORM_SET    *Buffer;
  UINTN               BufferSize;
  UINT8               ClassGuidNum;
  EFI_GUID            *ClassGuid;
  UINTN               TempSize;
  UINT8               *Ptr;
  EFI_STATUS          Status;

  TempSize   = 0;
  BufferSize = 0;
  Buffer     = NULL;

  HiiHandle = gPlatformManagerPrivate.HiiHandle;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  ASSERT (StartLabel != NULL);
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  //
  // According to the next show Form id(mNextShowFormId) to decide which form need to update.
  //
  StartLabel->Number = (UINT16)(LABEL_FORM_ID_OFFSET + NextShowFormId);

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  ASSERT (EndLabel != NULL);
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  //
  // Search for formset of each class type
  //
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    Status = HiiGetFormSetFromHiiHandle (HiiHandles[Index], &Buffer, &BufferSize);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Ptr = (UINT8 *)Buffer;

    while (TempSize < BufferSize) {
      TempSize += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
      if (((EFI_IFR_OP_HEADER *)Ptr)->Length <= OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
        Ptr += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
        continue;
      }

      ClassGuidNum = (UINT8)(((EFI_IFR_FORM_SET *)Ptr)->Flags & 0x3);
      ClassGuid    = (EFI_GUID *)(VOID *)(Ptr + sizeof (EFI_IFR_FORM_SET));
      while (ClassGuidNum-- > 0) {
        if (CompareGuid (&gPlatformManagerFormsetGuid, ClassGuid) == 0) {
          ClassGuid++;
          continue;
        }

        String = HiiGetString (HiiHandles[Index], ((EFI_IFR_FORM_SET *)Ptr)->FormSetTitle, NULL);
        if (String == NULL) {
          String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
          ASSERT (String != NULL);
        }

        Token = HiiSetString (HiiHandle, 0, String, NULL);
        FreePool (String);

        String = HiiGetString (HiiHandles[Index], ((EFI_IFR_FORM_SET *)Ptr)->Help, NULL);
        if (String == NULL) {
          String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
          ASSERT (String != NULL);
        }

        TokenHelp = HiiSetString (HiiHandle, 0, String, NULL);
        FreePool (String);

        CopyMem (&FormSetGuid, &((EFI_IFR_FORM_SET *)Ptr)->Guid, sizeof (EFI_GUID));

        if (NextShowFormId == PLATFORM_MANAGER_FORM_ID) {
          DevicePathStr = PmExtractDevicePathFromHiiHandle (HiiHandles[Index]);
          DevicePathId  = 0;
          if (DevicePathStr != NULL) {
            DevicePathId =  HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
            FreePool (DevicePathStr);
          }

          HiiCreateGotoExOpCode (
            StartOpCodeHandle,
            0,
            Token,
            TokenHelp,
            0,
            (EFI_QUESTION_ID)(Index + ENTRY_KEY_OFFSET),
            0,
            &FormSetGuid,
            DevicePathId
            );
        }

        break;
      }

      Ptr += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
    }

    FreePool (Buffer);
    Buffer     = NULL;
    TempSize   = 0;
    BufferSize = 0;
  }

  HiiUpdateForm (
    HiiHandle,
    &mPlatformManagerGuid,
    NextShowFormId,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  FreePool (HiiHandles);
}

/**
  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
PlatformManagerUiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   PlatformUiEntryEvent;

  gPlatformManagerPrivate.DriverHandle = NULL;
  Status                               = gBS->InstallMultipleProtocolInterfaces (
                                                &gPlatformManagerPrivate.DriverHandle,
                                                &gEfiDevicePathProtocolGuid,
                                                &mPlatformManagerHiiVendorDevicePath,
                                                NULL
                                                );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data.
  //
  gPlatformManagerPrivate.HiiHandle = HiiAddPackages (
                                        &mPlatformManagerGuid,
                                        gPlatformManagerPrivate.DriverHandle,
                                        PlatformManagerVfrBin,
                                        PlatformManagerUiLibStrings,
                                        NULL
                                        );
  if (gPlatformManagerPrivate.HiiHandle != NULL) {
    //
    // Update platform manager page
    //
    CreatePlatformManagerForm (PLATFORM_MANAGER_FORM_ID);
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Failed to add Hii package\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Signal Entry event
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EfiEventEmptyFunction,
                  NULL,
                  &gPlatformManagerEntryEventGuid,
                  &PlatformUiEntryEvent
                  );
  ASSERT_EFI_ERROR (Status);
  gBS->SignalEvent (PlatformUiEntryEvent);
  gBS->CloseEvent (PlatformUiEntryEvent);

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param  ImageHandle     Handle that identifies the image to be unloaded.
  @param  SystemTable     The system table.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
PlatformManagerUiLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   PlatformUiExitEvent;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gPlatformManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mPlatformManagerHiiVendorDevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  HiiRemovePackages (gPlatformManagerPrivate.HiiHandle);

  // Signal Exit event
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EfiEventEmptyFunction,
                  NULL,
                  &gPlatformManagerExitEventGuid,
                  &PlatformUiExitEvent
                  );
  ASSERT_EFI_ERROR (Status);
  gBS->SignalEvent (PlatformUiExitEvent);
  gBS->CloseEvent (PlatformUiExitEvent);

  return EFI_SUCCESS;
}
