/** @file
  Process IPMI bootdev command and force the UEFI to boot with selected option.

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/AmpereEventAfterConsole.h>
#include <Guid/AmpereVariableGuid.h>
#include <Guid/GlobalVariable.h>
#include <IndustryStandard/Ipmi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePathToText.h>

#define BBS_TYPE_OS_HARDDRIVE          0xFD
#define BBS_TYPE_MENU                  0xFE
#define LAST_BOOT_ORDER_VARIABLE_NAME  L"LastBootOrder"

//
// This variable is used to inidicate the persistent boot to UiApp.
//   0: Disable persistent boot to UiApp
//   1: Enable persistent boot to UiApp
//
#define FORCE_UIAPP_VARIABLE_NAME  L"ForceUiApp"

/**
  Get BBS Type from a messaging device path.

  @param[in] DevicePath       Current node of the Messaging device path.

  @retval UINT16              Return BBS Type.

**/
UINT16
GetBBSTypeFromMessagingDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Node
  )
{
  UINT16              Result;
  VENDOR_DEVICE_PATH  *VendorDevicePathNode;

  ASSERT (Node != NULL);

  Result = BBS_TYPE_UNKNOWN;

  switch (DevicePathSubType (Node)) {
    case MSG_MAC_ADDR_DP:
      Result = BBS_TYPE_EMBEDDED_NETWORK;
      break;

    case MSG_USB_DP:
      Result = BBS_TYPE_FLOPPY;
      break;

    case MSG_SATA_DP:
    case MSG_NVME_NAMESPACE_DP:
      Result = BBS_TYPE_HARDDRIVE;
      break;

    case MSG_VENDOR_DP:
      VendorDevicePathNode = (VENDOR_DEVICE_PATH *)Node;
      if (&VendorDevicePathNode->Guid != NULL) {
        if (CompareGuid (&VendorDevicePathNode->Guid, &((EFI_GUID)DEVICE_PATH_MESSAGING_SAS))) {
          Result = BBS_TYPE_HARDDRIVE;
        }
      }

      break;

    default:
      break;
  }

  return Result;
}

/**
  Get BBS Type from a Media device path.

  @param[in] Node          Current node of the Media device path.

  @retval UINT16           Return BBS Type.

**/
UINT16
GetBBSTypeFromMediaDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Node
  )
{
  UINT16                             Result;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvDevicePathNode;

  ASSERT (Node != NULL);

  Result = BBS_TYPE_UNKNOWN;

  switch (DevicePathSubType (Node)) {
    case MEDIA_CDROM_DP:
      Result = BBS_TYPE_CDROM;
      break;

    case MEDIA_PIWG_FW_FILE_DP:
      FvDevicePathNode = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)Node;
      if (&FvDevicePathNode->FvFileName != NULL) {
        if (CompareGuid (&FvDevicePathNode->FvFileName, PcdGetPtr (PcdBootManagerMenuFile))) {
          Result = BBS_TYPE_MENU;
        }
      }

      break;

    default:
      break;
  }

  return Result;
}

/**
  Get BBS type of a boot option from its option number.

  @param[in]  OptionNumber        Option number of the Boot option.

  @retval UINT8                   Return BBS type.

**/
UINT8
GetBBSType (
  UINT16  OptionNumber
  )
{
  CHAR16                        OptionName[sizeof ("Boot####")];
  EFI_BOOT_MANAGER_LOAD_OPTION  Option;
  EFI_DEVICE_PATH_PROTOCOL      *StartOfMediaDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *StartOfMessagingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *Node;
  EFI_STATUS                    Status;
  UINT16                        Result;

  StartOfMediaDevicePath     = NULL;
  StartOfMessagingDevicePath = NULL;

  UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionNumber);
  Status = EfiBootManagerVariableToLoadOption (OptionName, &Option);
  if (EFI_ERROR (Status)) {
    return BBS_TYPE_UNKNOWN;
  }

  DEBUG_CODE (
    CHAR16 *Str = ConvertDevicePathToText (Option.FilePath, TRUE, TRUE);
    if (Str != NULL) {
    DEBUG ((DEBUG_INFO, "Boot0x%04x: %s\n", OptionNumber, Str));
    FreePool (Str);
  }

    );

  Node = Option.FilePath;
  while (!IsDevicePathEnd (Node)) {
    if (  (DevicePathType (Node) == MESSAGING_DEVICE_PATH)
       && (StartOfMessagingDevicePath == NULL))
    {
      StartOfMessagingDevicePath = Node;
    }

    if (  (DevicePathType (Node) == MEDIA_DEVICE_PATH)
       && (StartOfMediaDevicePath == NULL))
    {
      StartOfMediaDevicePath = Node;
      break;
    }

    Node = NextDevicePathNode (Node);
  }

  Result = BBS_TYPE_UNKNOWN;

  if (StartOfMediaDevicePath != NULL) {
    while (!IsDevicePathEnd (Node)) {
      Result = GetBBSTypeFromMediaDevicePath (Node);
      if (Result != BBS_TYPE_UNKNOWN) {
        return Result;
      }

      Node = NextDevicePathNode (Node);
    }
  }

  if (StartOfMessagingDevicePath != NULL) {
    while (!IsDevicePathEnd (StartOfMessagingDevicePath)) {
      Result = GetBBSTypeFromMessagingDevicePath (StartOfMessagingDevicePath);
      if (Result != BBS_TYPE_UNKNOWN) {
        return Result;
      }

      StartOfMessagingDevicePath = NextDevicePathNode (StartOfMessagingDevicePath);
    }
  }

  return Result;
}

/**
  Build a new BootOrder base on an existed one with the head includes all options of the same selected type.

  @param[in]  BootType            BBS Type to find for
  @param[in]  BootOrder           Pointer to the array of an existed BootOrder.
  @param[in]  BootOrderSize       Size of the existed BootOrder.

  @retval UINT16 *                Pointer to the new BootOrder array.
  @retval NULL                    Can't build the new BootOrder or current BootOrder is good enough.

**/
UINT16 *
BuildBootOrder (
  IN UINT8   BootType,
  IN UINT16  *BootOrder,
  IN UINTN   BootOrderSize
  )
{
  UINT16  *NewBootOrder;
  UINT16  *SelectOptions;
  UINT16  *RemainOptions;
  UINT8   Temp;
  UINTN   Index;
  UINTN   NewCnt;
  UINTN   RemainCnt;
  UINTN   SelectCnt;

  ASSERT (BootOrder != NULL);

  NewBootOrder  = AllocatePool (BootOrderSize);
  SelectOptions = AllocatePool (BootOrderSize);
  RemainOptions = AllocatePool (BootOrderSize);
  if (  (NewBootOrder == NULL)
     || (SelectOptions == NULL)
     || (RemainOptions == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Out of resources", __func__));
    goto Exit;
  }

  NewCnt    = 0;
  SelectCnt = 0;
  RemainCnt = 0;

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    Temp = GetBBSType (BootOrder[Index]);
    //
    // Hard drive with OS is a special type of hard drive boot option.
    // Treat it with higher priority than normal hard drive.
    //
    if ((BootType == BBS_TYPE_HARDDRIVE) && (Temp == BBS_TYPE_OS_HARDDRIVE)) {
      NewBootOrder[NewCnt++] = BootOrder[Index];
    } else if (Temp == BootType) {
      SelectOptions[SelectCnt++] = BootOrder[Index];
    } else {
      RemainOptions[RemainCnt++] = BootOrder[Index];
    }
  }

  CopyMem (&SelectOptions[SelectCnt], RemainOptions, RemainCnt * sizeof (UINT16));
  CopyMem (&NewBootOrder[NewCnt], SelectOptions, (SelectCnt + RemainCnt) * sizeof (UINT16));

  if (CompareMem (NewBootOrder, BootOrder, BootOrderSize) != 0) {
    FreePool (SelectOptions);
    FreePool (RemainOptions);
    return NewBootOrder;
  }

Exit:
  if (SelectOptions != NULL) {
    FreePool (SelectOptions);
  }

  if (RemainOptions != NULL) {
    FreePool (RemainOptions);
  }

  if (NewBootOrder != NULL) {
    FreePool (NewBootOrder);
  }

  return NULL;
}

/**
  Convert IPMI device selector to BBS Type.

  @param[in]  DeviceSelector      Input device selector.

  @retval UINT8                   Return BBS Type.

**/
UINT8
DeviceSelectorToBBSType (
  UINT8  DeviceSelector
  )
{
  switch (DeviceSelector) {
    case IPMI_BOOT_DEVICE_SELECTOR_PXE:
      return BBS_TYPE_EMBEDDED_NETWORK;

    case IPMI_BOOT_DEVICE_SELECTOR_HARDDRIVE:
      return BBS_TYPE_HARDDRIVE;

    case IPMI_BOOT_DEVICE_SELECTOR_CD_DVD:
      return BBS_TYPE_CDROM;

    case IPMI_BOOT_DEVICE_SELECTOR_BIOS_SETUP:
      return BBS_TYPE_MENU;

    case IPMI_BOOT_DEVICE_SELECTOR_FLOPPY:
      return BBS_TYPE_FLOPPY;

    default:
      return BBS_TYPE_UNKNOWN;
  }
}

/**
  Check if there is a request from BMC for boot option override.

  @param[out]  BootDeviceSelector  Pointer to buffer to get overrided boot options.
  @param[out]  IsPersistent        Pointer to buffer for persistent boot option flag.

  @retval TRUE    IPMI Boot Options Override Request is valid.
  @retval FALSE   IPMI Boot Options Override Request is invalid.

**/
BOOLEAN
IsIpmiBootOptionsValid (
  UINT8    *BootDeviceSelector,
  BOOLEAN  *IsPersistent
  )
{
  EFI_STATUS                              Status;
  IPMI_GET_BOOT_OPTIONS_REQUEST           BootOptionsRequest;
  IPMI_GET_BOOT_OPTIONS_RESPONSE          *BootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4  *BootOptionsParameterData4;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5  *BootOptionsParameterData5;

  if ((BootDeviceSelector == NULL) || (IsPersistent == NULL)) {
    return FALSE;
  }

  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));

  //
  // Retrieve Boot Info Acknowledge from BMC.
  //
  BootOptionsResponse = AllocateZeroPool (sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST) + sizeof (IPMI_BOOT_OPTIONS_PARAMETERS));
  if (BootOptionsResponse == NULL) {
    return FALSE;
  }

  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK;

  Status = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  if (EFI_ERROR (Status)) {
    FreePool (BootOptionsResponse);
    return FALSE;
  }

  BootOptionsParameterData4 = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4 *)BootOptionsResponse->ParameterData;
  if (BootOptionsParameterData4->BootInitiatorAcknowledgeData != BOOT_OPTION_HANDLED_BY_BIOS) {
    FreePool (BootOptionsResponse);
    return FALSE;
  }

  //
  // Retrieve Boot Options parameter data
  //
  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));

  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;

  Status = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  if (EFI_ERROR (Status)) {
    FreePool (BootOptionsResponse);
    return FALSE;
  }

  BootOptionsParameterData5 = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)BootOptionsResponse->ParameterData;
  if (BootOptionsParameterData5->Data1.Bits.BootFlagValid != 0) {
    *IsPersistent       = (BootOptionsParameterData5->Data1.Bits.PersistentOptions != 0) ? TRUE : FALSE;
    *BootDeviceSelector = BootOptionsParameterData5->Data2.Bits.BootDeviceSelector;
    FreePool (BootOptionsResponse);
    return TRUE;
  }

  FreePool (BootOptionsResponse);
  return FALSE;
}

/**
  Acknowledge and clear the boot flags requested from BMC.

  @retval EFI_SUCCESS   The operation is completed sucessfully.
  @retval Others        An error when sending IPMI command.

**/
EFI_STATUS
ClearIpmiBootOptionsOverride (
  VOID
  )
{
  EFI_STATUS                              Status;
  IPMI_SET_BOOT_OPTIONS_REQUEST           *SetBootOptionsRequest;
  IPMI_SET_BOOT_OPTIONS_RESPONSE          SetBootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4  *BootOptionsParameterData;

  //
  // Set Boot Info Acknowledge to notify BMC that the Boot Flags has been handled by UEFI
  //
  ZeroMem (&SetBootOptionsResponse, sizeof (IPMI_SET_BOOT_OPTIONS_RESPONSE));

  SetBootOptionsRequest = AllocateZeroPool (sizeof (SetBootOptionsRequest) + sizeof (IPMI_BOOT_OPTIONS_PARAMETERS));
  if (SetBootOptionsRequest == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetBootOptionsRequest->ParameterValid.Bits.ParameterSelector    = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK;
  SetBootOptionsRequest->ParameterValid.Bits.MarkParameterInvalid = 0x0;

  BootOptionsParameterData = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4 *)&SetBootOptionsRequest->ParameterData;

  BootOptionsParameterData->WriteMask                    = BIT0;
  BootOptionsParameterData->BootInitiatorAcknowledgeData = 0;

  Status = IpmiSetSystemBootOptions (SetBootOptionsRequest, &SetBootOptionsResponse);
  if (EFI_ERROR (Status)) {
    FreePool (SetBootOptionsRequest);
    return Status;
  }

  //
  // Send command to clear BMC Boot Flags parameter
  //
  ZeroMem (&SetBootOptionsResponse, sizeof (IPMI_SET_BOOT_OPTIONS_RESPONSE));
  ZeroMem (SetBootOptionsRequest, sizeof (SetBootOptionsRequest) + sizeof (IPMI_BOOT_OPTIONS_PARAMETERS));

  SetBootOptionsRequest->ParameterValid.Bits.ParameterSelector    = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;
  SetBootOptionsRequest->ParameterValid.Bits.MarkParameterInvalid = 0x0;

  Status = IpmiSetSystemBootOptions (SetBootOptionsRequest, &SetBootOptionsResponse);

  FreePool (SetBootOptionsRequest);
  return Status;
}

/**
  Handle IPMI bootdev request on AfterConsole Event notification.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
HandleIpmiBootOption (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  BOOLEAN     UpdateNeed;
  EFI_STATUS  Status;
  UINT16      *CurrBootOrder;
  UINT16      *NewBootOrder;
  UINT16      FirstBootOption;
  UINT8       BootType;
  UINT8       DeviceSelector;
  BOOLEAN     IsPersistent;
  UINT8       ForceUiApp;
  UINTN       CurrBootOrderSize;
  UINTN       DataSize;
  UINT64      OsIndication;

  OsIndication   = 0;
  NewBootOrder   = NULL;
  BootType       = 0xFF;
  DeviceSelector = 0xFF;
  IsPersistent   = FALSE;

  GetEfiGlobalVariable2 (EFI_BOOT_ORDER_VARIABLE_NAME, (VOID **)&CurrBootOrder, &CurrBootOrderSize);
  if (CurrBootOrder == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: BootOrder not found\n", __func__));
    return;
  }

  //
  // It is supposed that BootOrder must contain at least one boot option.
  //
  FirstBootOption = CurrBootOrder[0];

  //
  // Cache the FORCE_UIAPP_VARIABLE to reduce the accessing time to the NVRAM.
  //
  DataSize = sizeof (UINT8);
  Status   = gRT->GetVariable (
                    FORCE_UIAPP_VARIABLE_NAME,
                    &gAmpereVariableGuid,
                    NULL,
                    &DataSize,
                    &ForceUiApp
                    );
  if (EFI_ERROR (Status) || (DataSize != sizeof (UINT8))) {
    ForceUiApp = 0;
  }

  if (!IsIpmiBootOptionsValid (&DeviceSelector, &IsPersistent)) {
    goto Exit;
  }

  BootType = DeviceSelectorToBBSType (DeviceSelector);
  if (BootType == BBS_TYPE_UNKNOWN) {
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "IPMI Boot Type %d, Persistent %d\n", BootType, IsPersistent));

  NewBootOrder = BuildBootOrder (BootType, CurrBootOrder, CurrBootOrderSize);

  if (NewBootOrder != NULL) {
    //
    // Backup current BootOrder if this is not a persistent request.
    //
    if (!IsPersistent) {
      Status = gRT->SetVariable (
                      LAST_BOOT_ORDER_VARIABLE_NAME,
                      &gAmpereVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      CurrBootOrderSize,
                      CurrBootOrder
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to backup the BootOrder %r\n", __func__, Status));
        goto Exit;
      }
    }

    //
    // Update BootOrder with new value.
    //
    Status = gRT->SetVariable (
                    EFI_BOOT_ORDER_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                    | EFI_VARIABLE_NON_VOLATILE,
                    CurrBootOrderSize,
                    NewBootOrder
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Set BootOrder Variable %r\n", __func__, Status));
      goto Exit;
    }

    FirstBootOption = NewBootOrder[0];
  }

  UpdateNeed = FALSE;
  if (BootType == BBS_TYPE_MENU) {
    if (ForceUiApp == 0) {
      //
      // IPMI boot type is MENU, but FORCE_UIAPP_VARIABLE is not set.
      // Set ForceUiApp to allow boot to UiApp on this boot cycle.
      //
      ForceUiApp = 1;
      if (IsPersistent) {
        //
        // This is a persistent boot to UiApp request.
        // Update FORCE_UIAPP_VARIABLE to force the system to boot to UiApp
        // on next boot.
        //
        UpdateNeed = TRUE;
      }
    }
  } else if (ForceUiApp == 1) {
    //
    // IPMI boot type is not the UiApp, but FORCE_UIAPP_VARIABLE is set
    // Clear ForceUiApp to prevent the system from booting to UiApp on this boot cycle.
    //
    ForceUiApp = 0;
    if (IsPersistent) {
      //
      // This is a persistent request other than UiApp.
      // Update FORCE_UIAPP_VARIABLE to prevent the system from booting to UiApp
      // on next boot.
      //
      UpdateNeed = TRUE;
    }
  }

  if (UpdateNeed) {
    Status = gRT->SetVariable (
                    FORCE_UIAPP_VARIABLE_NAME,
                    &gAmpereVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (UINT8),
                    &ForceUiApp
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to set the Force UiApp variable - %r\n", __func__, Status));
    }
  }

  //
  // Acknowledge and clear the boot flags
  //
  Status = ClearIpmiBootOptionsOverride ();
  ASSERT_EFI_ERROR (Status);

Exit:
  //
  // UiApp was registered with HIDDEN attribute and will be ignored by BDS.
  // In order to boot to the UiApp, manually update the BootNext variable.
  //
  if ((ForceUiApp == 1) && (GetBBSType (FirstBootOption) == BBS_TYPE_MENU)) {
    DataSize = sizeof (UINT64);
    Status   = gRT->GetVariable (
                      EFI_OS_INDICATIONS_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      NULL,
                      &DataSize,
                      &OsIndication
                      );
    OsIndication |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
    Status        = gRT->SetVariable (
                           EFI_OS_INDICATIONS_VARIABLE_NAME,
                           &gEfiGlobalVariableGuid,
                           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                           sizeof (UINT64),
                           &OsIndication
                           );
    ASSERT_EFI_ERROR (Status);
  }

  if (NewBootOrder != NULL) {
    FreePool (NewBootOrder);
  }

  FreePool (CurrBootOrder);
}

/**
  Looking for the backup of previous BootOrder, then restore it with respect to current BootOrder.

  @retval EFI_SUCCESS       The function executed successfully.
  @retval Other             Some error occurs when saving the variables.

**/
EFI_STATUS
RestoreBootOrder (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      *CurrBootOrder;
  UINT16      *LastBootOrder;
  UINTN       CurrBootOrderSize;
  UINTN       LastBootOrderSize;

  CurrBootOrderSize = 0;
  LastBootOrderSize = 0;

  Status = GetVariable2 (
             LAST_BOOT_ORDER_VARIABLE_NAME,
             &gAmpereVariableGuid,
             (VOID **)&LastBootOrder,
             &LastBootOrderSize
             );
  if (LastBootOrder == NULL) {
    return EFI_SUCCESS;
  }

  //
  // On first boot, OS loader might add a new boot option to the head of the BootOrder.
  // If this is true, append that option to the tail of the LastBootOrder.
  //
  GetEfiGlobalVariable2 (EFI_BOOT_ORDER_VARIABLE_NAME, (VOID **)&CurrBootOrder, &CurrBootOrderSize);
  if (LastBootOrderSize == (CurrBootOrderSize - sizeof (UINT16))) {
    CurrBootOrder[CurrBootOrderSize / sizeof (UINT16) - 1] = CurrBootOrder[0];
    CopyMem (CurrBootOrder, LastBootOrder, LastBootOrderSize);
    FreePool (LastBootOrder);
    LastBootOrder     = CurrBootOrder;
    LastBootOrderSize = CurrBootOrderSize;
  } else {
    FreePool (CurrBootOrder);
  }

  Status = gRT->SetVariable (
                  EFI_BOOT_ORDER_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                  | EFI_VARIABLE_NON_VOLATILE,
                  LastBootOrderSize,
                  LastBootOrder
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to restore the BootOrder\n", __func__));
    goto Exit;
  }

  Status = gRT->SetVariable (
                  LAST_BOOT_ORDER_VARIABLE_NAME,
                  &gAmpereVariableGuid,
                  0,
                  0,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to erase the LastBootOrder\n", __func__));
    goto Exit;
  }

Exit:
  FreePool (LastBootOrder);

  return Status;
}

/**
  ReadyToBoot notification.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
RestoreBootOrderOnReadytoBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Restore BootOrder variable in normal condition.
  //
  Status = RestoreBootOrder ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when restoring boot order\n"));
  }
}

/**
  The Entry Point for IPMI Boot handler.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
IpmiBootEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   Event;
  EFI_STATUS  Status;

  Status = RestoreBootOrder ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when restoring boot order\n"));
    return Status;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  HandleIpmiBootOption,
                  NULL,
                  &gAmpereEventAfterConsoleGuid,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when creating an event callback for gAmpereEventAfterConsoleGuid\n"));
    return Status;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RestoreBootOrderOnReadytoBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when creating an event callback for gEfiEventReadyToBootGuid\n"));
  }

  return Status;
}
