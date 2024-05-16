/** @file
  Driver for Platform Boot Options support.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BoardBdsHook.h"

EFI_GUID  gUefiShellFileGuid = { 0 };
EFI_GUID  mUiFile            = {
  0x462CAA21, 0x7614, 0x4503, { 0x83, 0x6E, 0x8A, 0xB6, 0xF4, 0x66, 0x23, 0x31 }
};
EFI_GUID  mBootMenuFile = {
  0xEEC25BDC, 0x67F2, 0x4D95, { 0xB1, 0xD5, 0xF8, 0x1B, 0x20, 0x39, 0xD1, 0x1D }
};

BOOLEAN    mContinueBoot  = FALSE;
BOOLEAN    mBootMenuBoot  = FALSE;
BOOLEAN    mPxeBoot       = FALSE;
BOOLEAN    mHotKeypressed = FALSE;
EFI_EVENT  HotKeyEvent    = NULL;

UINTN  mBootMenuOptionNumber;
UINTN  mSetupOptionNumber;

/**
  This function will create a SHELL BootOption to boot.

  @return Shell Device path for booting.
**/
EFI_DEVICE_PATH_PROTOCOL *
BdsCreateShellDevicePath (
  VOID
  )
{
  UINTN                          FvHandleCount;
  EFI_HANDLE                     *FvHandleBuffer;
  UINTN                          Index;
  EFI_STATUS                     Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  UINTN                          Size;
  UINT32                         AuthenticationStatus;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  VOID                           *Buffer;

  DevicePath = NULL;
  Status     = EFI_SUCCESS;

  DEBUG ((DEBUG_INFO, "BdsCreateShellDevicePath\n"));
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiFirmwareVolume2ProtocolGuid,
         NULL,
         &FvHandleCount,
         &FvHandleBuffer
         );

  for (Index = 0; Index < FvHandleCount; Index++) {
    gBS->HandleProtocol (
           FvHandleBuffer[Index],
           &gEfiFirmwareVolume2ProtocolGuid,
           (VOID **)&Fv
           );

    Buffer = NULL;
    Size   = 0;
    Status = Fv->ReadSection (
                   Fv,
                   &gUefiShellFileGuid,
                   EFI_SECTION_PE32,
                   0,
                   &Buffer,
                   &Size,
                   &AuthenticationStatus
                   );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no shell file in the FV
      //
      continue;
    } else {
      //
      // Found the shell
      //
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    //
    // No shell present
    //
    if (FvHandleCount) {
      FreePool (FvHandleBuffer);
    }

    return NULL;
  }

  //
  // Build the shell boot option
  //
  DevicePath = DevicePathFromHandle (FvHandleBuffer[Index]);

  if (FvHandleCount) {
    FreePool (FvHandleBuffer);
  }

  return DevicePath;
}

/**
  Create Boot option for passed Firmware Volume.

  @param[in] FileGuid         FV file name to use in FvDevicePathNode
  @param[in] Description      Description of the load option.
  @param[in, out] BootOption  Pointer to the load option to be initialized.
  @param[in] Attributes       Attributes of the load option.
  @param[in] OptionalData     Optional data of the load option.
  @param[in] OptionalDataSize Size of the optional data of the load option.

  @retval EFI_SUCCESS           The load option was initialized successfully.
  @retval EFI_INVALID_PARAMETER BootOption, Description or FileGuid is NULL.
**/
EFI_STATUS
CreateFvBootOption (
  IN EFI_GUID *FileGuid,
  IN CHAR16 *Description,
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
  IN UINT32 Attributes,
  IN UINT8 *OptionalData, OPTIONAL IN UINT32 OptionalDataSize
  )
{
  EFI_STATUS                         Status;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_FIRMWARE_VOLUME2_PROTOCOL      *Fv;
  UINT32                             AuthenticationStatus;
  VOID                               *Buffer;
  UINTN                              Size;

  if ((BootOption == NULL) || (FileGuid == NULL) || (Description == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);

  if (!CompareGuid (&gUefiShellFileGuid, FileGuid)) {
    Status = gBS->HandleProtocol (
                    gImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (
                      LoadedImage->DeviceHandle,
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (VOID **)&Fv
                      );
      if (!EFI_ERROR (Status)) {
        Buffer = NULL;
        Size   = 0;
        Status = Fv->ReadSection (
                       Fv,
                       FileGuid,
                       EFI_SECTION_PE32,
                       0,
                       &Buffer,
                       &Size,
                       &AuthenticationStatus
                       );
        if (Buffer != NULL) {
          FreePool (Buffer);
        }
      }
    }

    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    DevicePath = AppendDevicePathNode (
                   DevicePathFromHandle (LoadedImage->DeviceHandle),
                   (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                   );
  } else {
    DevicePath = AppendDevicePathNode (
                   BdsCreateShellDevicePath (),
                   (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                   );
  }

  Status = EfiBootManagerInitializeLoadOption (
             BootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             OptionalData,
             OptionalDataSize
             );
  FreePool (DevicePath);
  return Status;
}

/**
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the
  OptionType, Attributes, Description, FilePath and OptionalData are equal.

  @param[in] Key    Pointer to the load option to be found.
  @param[in] Array  Pointer to the array of load options to be found.
  @param[in] Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
PlatformFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Array,
  IN UINTN                               Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; Index++) {
    if ((Key->OptionType == Array[Index].OptionType) &&
        (Key->Attributes == Array[Index].Attributes) &&
        (StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0))
    {
      return (INTN)Index;
    }
  }

  return -1;
}

/**
  Registers a boot option.

  @param[in] FileGuid               Boot file GUID
  @param[in] Description            Boot option description
  @param[in] Position               Position of the new load option to put in the ****Order variable.
  @param[in] Attributes             Boot option attributes
  @param[in] OptionalData           Optional data of the boot option.
  @param[in] OptionalDataSize       Size of the optional data of the boot option

  @return boot option number
**/
UINTN
RegisterFvBootOption (
  IN EFI_GUID *FileGuid,
  IN CHAR16 *Description,
  IN UINTN Position,
  IN UINT32 Attributes,
  IN UINT8 *OptionalData, OPTIONAL IN UINT32 OptionalDataSize
  )
{
  EFI_STATUS                    Status;
  UINTN                         OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION  NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;

  NewOption.OptionNumber = LoadOptionNumberUnassigned;
  Status                 = CreateFvBootOption (FileGuid, Description, &NewOption, Attributes, OptionalData, OptionalDataSize);
  if (!EFI_ERROR (Status)) {
    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

    OptionIndex = PlatformFindLoadOption (&NewOption, BootOptions, BootOptionCount);

    if (OptionIndex == -1) {
      Status = EfiBootManagerAddLoadOptionVariable (&NewOption, Position);
      ASSERT_EFI_ERROR (Status);
    } else {
      NewOption.OptionNumber = BootOptions[OptionIndex].OptionNumber;
    }

    EfiBootManagerFreeLoadOption (&NewOption);
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  }

  return NewOption.OptionNumber;
}

/**
  Boot manager wait callback.

  @param[in] TimeoutRemain The remaining timeout period
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  IN UINT16  TimeoutRemain
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx;
  EFI_KEY_DATA                       KeyData;
  BOOLEAN                            PausePressed;

  //
  // Pause on PAUSE key
  //
  Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID **)&TxtInEx);
  ASSERT_EFI_ERROR (Status);

  PausePressed = FALSE;

  while (TRUE) {
    Status = TxtInEx->ReadKeyStrokeEx (TxtInEx, &KeyData);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (KeyData.Key.ScanCode == SCAN_PAUSE) {
      PausePressed = TRUE;
      break;
    }
  }

  //
  // Loop until non-PAUSE key pressed
  //
  while (PausePressed) {
    Status = TxtInEx->ReadKeyStrokeEx (TxtInEx, &KeyData);
    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "[PauseCallback] %x/%x %x/%x\n",
        KeyData.Key.ScanCode,
        KeyData.Key.UnicodeChar,
        KeyData.KeyState.KeyShiftState,
        KeyData.KeyState.KeyToggleState
        ));
      PausePressed = (BOOLEAN)(KeyData.Key.ScanCode == SCAN_PAUSE);
    }
  }
}

/**
   Registers default boot option.
**/
VOID
RegisterDefaultBootOption (
  VOID
  )
{
  UINT16  *ShellData;
  UINT32  ShellDataSize;

  ShellData     = NULL;
  ShellDataSize = 0;
  CopyMem (&gUefiShellFileGuid, PcdGetPtr (PcdShellFile), sizeof (GUID));
  RegisterFvBootOption (&gUefiShellFileGuid, (CHAR16 *)PcdGetPtr (PcdShellFileDesc), (UINTN)-1, LOAD_OPTION_ACTIVE, (UINT8 *)ShellData, ShellDataSize);

  //
  // Boot Menu
  //
  mBootMenuOptionNumber = RegisterFvBootOption (&mBootMenuFile, L"Boot Device List", (UINTN)-1, LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN, NULL, 0);

  if (mBootMenuOptionNumber == LoadOptionNumberUnassigned) {
    DEBUG ((DEBUG_INFO, "BootMenuOptionNumber (%d) should not be same to LoadOptionNumberUnassigned(%d).\n", mBootMenuOptionNumber, LoadOptionNumberUnassigned));
  }

  //
  // Boot Manager Menu
  //
  mSetupOptionNumber = RegisterFvBootOption (&mUiFile, L"Enter Setup", (UINTN)-1, LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN, NULL, 0);
}

/**
  Registers/Unregisters boot option hotkey.

  @param[in] OptionNumber  The boot option number for the key option.
  @param[in] Key           The key input
  @param[in] Add           Flag to indicate to add or remove a key
**/
VOID
RegisterBootOptionHotkey (
  IN UINT16         OptionNumber,
  IN EFI_INPUT_KEY  *Key,
  IN BOOLEAN        Add
  )
{
  EFI_STATUS  Status;

  if (!Add) {
    //
    // No enter hotkey when force to setup or there is no boot option
    //
    Status = EfiBootManagerDeleteKeyOptionVariable (NULL, 0, Key, NULL);
    ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);
  } else {
    //
    // Register enter hotkey for the first boot option
    //
    Status = EfiBootManagerAddKeyOptionVariable (NULL, OptionNumber, 0, Key, NULL);
    ASSERT (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
  }
}

/**
  Detect key press callback.

  @param[in] KeyData  The key data

  @retval   EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
DetectKeypressCallback (
  IN EFI_KEY_DATA  *KeyData
  )
{
  mHotKeypressed = TRUE;

  if (HotKeyEvent != NULL) {
    gBS->SignalEvent (HotKeyEvent);
  }

  return EFI_SUCCESS;
}

/**
  This function is called after all the boot options are enumerated and ordered properly.
**/
VOID
RegisterStaticHotkey (
  VOID
  )
{
  EFI_INPUT_KEY  Enter;
  EFI_KEY_DATA   F2;
  EFI_KEY_DATA   F7;
  BOOLEAN        EnterSetup;

  EnterSetup = FALSE;

  //
  // [Enter]
  //
  mContinueBoot = !EnterSetup;
  if (mContinueBoot) {
    Enter.ScanCode    = SCAN_NULL;
    Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
    EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);
  }

  //
  // [F2]
  //
  if (mSetupOptionNumber != LoadOptionNumberUnassigned) {
    F2.Key.ScanCode            = SCAN_F2;
    F2.Key.UnicodeChar         = CHAR_NULL;
    F2.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
    F2.KeyState.KeyToggleState = 0;
    RegisterBootOptionHotkey ((UINT16)mSetupOptionNumber, &F2.Key, TRUE);
  }

  //
  // Register [F7] only when the mBootMenuOptionNumber is valid
  //
  if (mBootMenuOptionNumber != LoadOptionNumberUnassigned) {
    F7.Key.ScanCode            = SCAN_F7;
    F7.Key.UnicodeChar         = CHAR_NULL;
    F7.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
    F7.KeyState.KeyToggleState = 0;
    mBootMenuBoot              = !EnterSetup;
    RegisterBootOptionHotkey ((UINT16)mBootMenuOptionNumber, &F7.Key, mBootMenuBoot);
  }
}

/**
  Returns the boot option type of a device.

  @param[in] DevicePath         The DevicePath whose boot option type is
                                to be returned
  @retval -1                    Device type not found
  @retval > -1                  Device type found
**/
UINT8
BootOptionType (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;

  for (Node = DevicePath; !IsDevicePathEndType (Node); Node = NextDevicePathNode (Node)) {
    if (DevicePathType (Node) == MESSAGING_DEVICE_PATH) {
      //
      // Make sure the device path points to the driver device.
      //
      NextNode = NextDevicePathNode (Node);
      if (DevicePathSubType (NextNode) == MSG_DEVICE_LOGICAL_UNIT_DP) {
        //
        // if the next node type is Device Logical Unit, which specify the Logical Unit Number (LUN),
        // skip it
        //
        NextNode = NextDevicePathNode (NextNode);
      }

      if (IsDevicePathEndType (NextNode)) {
        if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH)) {
          return DevicePathSubType (Node);
        } else {
          return MSG_SATA_DP;
        }
      }
    }
  }

  return (UINT8)-1;
}

/**
  Returns the priority number.

  @param[in] BootOption     Load option to get the priority value for
  @retval
    OptionType                 EFI
    ------------------------------------
    PXE                         2
    DVD                         4
    USB                         6
    NVME                        7
    HDD                         8
    EFI Shell                   9
    Others                      100

**/
UINTN
BootOptionPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  //
  // EFI boot options
  //
  switch (BootOptionType (BootOption->FilePath)) {
    case MSG_MAC_ADDR_DP:
    case MSG_VLAN_DP:
    case MSG_IPv4_DP:
    case MSG_IPv6_DP:
      return 2;

    case MSG_SATA_DP:
    case MSG_ATAPI_DP:
    case MSG_UFS_DP:
    case MSG_NVME_NAMESPACE_DP:
      return 4;

    case MSG_USB_DP:
      return 6;
  }

  if (StrCmp (BootOption->Description, (CHAR16 *)PcdGetPtr (PcdShellFileDesc)) == 0) {
    if (PcdGetBool (PcdBootToShellOnly)) {
      return 0;
    }

    return 9;
  }

  if (StrCmp (BootOption->Description, UEFI_HARD_DRIVE_NAME) == 0) {
    return 8;
  }

  return 100;
}

/**
  Returns the priority number while giving PXE highest
  priority.

  @param[in] BootOption   Load option to get the priority value for
  @retval
    OptionType                 EFI
    ------------------------------------
    PXE                         2
    DVD                         4
    USB                         6
    NVME                        7
    HDD                         8
    EFI Shell                   9
    Others                      100
**/
UINTN
PxeBootOptionPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  //
  // EFI boot options
  //
  switch (BootOptionType (BootOption->FilePath)) {
    case MSG_MAC_ADDR_DP:
    case MSG_VLAN_DP:
    case MSG_IPv4_DP:
    case MSG_IPv6_DP:
      return 2;

    case MSG_SATA_DP:
    case MSG_ATAPI_DP:
    case MSG_UFS_DP:
    case MSG_NVME_NAMESPACE_DP:
      return 4;

    case MSG_USB_DP:
      return 6;
  }

  if (StrCmp (BootOption->Description, (CHAR16 *)PcdGetPtr (PcdShellFileDesc)) == 0) {
    if (PcdGetBool (PcdBootToShellOnly)) {
      return 0;
    }

    return 9;
  }

  if (StrCmp (BootOption->Description, UEFI_HARD_DRIVE_NAME) == 0) {
    return 8;
  }

  return 100;
}

/**
  Returns the priority number while giving HDD, DVD, and NVME highest priority.

  @param[in] BootOption Load option to get the priority value for
  @retval
    OptionType                 EFI
    ------------------------------------
    NVME, DVD, HDD              2
    PXE                         4
    USB                         6
    HDD                         8
    EFI Shell                   9
    Others                      100
**/
UINTN
HddBootOptionPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  //
  // EFI boot options
  //
  switch (BootOptionType (BootOption->FilePath)) {
    case MSG_SATA_DP:
    case MSG_UFS_DP:
    case MSG_NVME_NAMESPACE_DP:
      return 2;

    case MSG_ATAPI_DP:
      return 3;

    case MSG_MAC_ADDR_DP:
    case MSG_VLAN_DP:
    case MSG_IPv4_DP:
    case MSG_IPv6_DP:
      return 4;

    case MSG_USB_DP:
      return 6;
  }

  if (StrCmp (BootOption->Description, (CHAR16 *)PcdGetPtr (PcdShellFileDesc)) == 0) {
    if (PcdGetBool (PcdBootToShellOnly)) {
      return 0;
    }

    return 9;
  }

  if (StrCmp (BootOption->Description, UEFI_HARD_DRIVE_NAME) == 0) {
    return 2;
  }

  return 100;
}

/**
  Compares boot priorities of two boot options.

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @retval           The difference between the Left and Right
                    boot options
 **/
INTN
EFIAPI
CompareBootOption (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  return BootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Left) -
         BootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Right);
}

/**
   Compares boot priorities of two boot options, while giving PXE the highest priority.

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @retval           The difference between the Left and Right
                    boot options
 **/
INTN
EFIAPI
CompareBootOptionPxePriority (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  return PxeBootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Left) -
         PxeBootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Right);
}

/**
   Compares boot priorities of two boot options, while giving HDD the highest priority.

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @retval           The difference between the Left and Right
                    boot options
 **/
INTN
EFIAPI
CompareBootOptionHddPriority (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  return HddBootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Left) -
         HddBootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Right);
}
