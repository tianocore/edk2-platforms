/** @file
  Functions for handling IPMI boot overrides.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/AmdBoardBdsHookLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/Ipmi.h>

#include "BoardBdsHook.h"

// A GUID to be added as the OptionalData field for boot options to identify them as being
// created by this library to handle IPMI boot overrides.
EFI_GUID  mAmdIpmiOverrideBootOptionGuid = {
  0x17a3d4cf, 0x89e5, 0x4f92, { 0xae, 0xed, 0xda, 0x97, 0xbc, 0xd2, 0xd2, 0x0a }
};

/**
  Check if the given boot option is a match for the current IPMI boot override setting.

  @param[in] BootOption              Pointer to the boot option to check.
  @param[in] CurrentIpmiOverride     The IPMI override to be used for the current boot.

  @retval  TRUE      Boot option is a match for the current boot override setting.
  @retval  FALSE     Not a match.
**/
BOOLEAN
BootOptionMatch (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption,
  IN UINT8                         CurrentIpmiOverride
  )
{
  UINT8  CurrentBootOptionType;

  if (PcdGetBool (PcdBootToShellOnly)) {
    return (StrCmp (BootOption->Description, (CONST CHAR16 *)PcdGetPtr (PcdShellFileDesc)) == 0);
  }

  CurrentBootOptionType = BootOptionType (BootOption->FilePath);

  switch (CurrentIpmiOverride) {
    case IPMI_BOOT_DEVICE_SELECTOR_HARDDRIVE:
      switch (CurrentBootOptionType) {
        case MSG_SATA_DP:
        case MSG_UFS_DP:
        case MSG_NVME_NAMESPACE_DP:
          return TRUE;
        default:
          return (StrCmp (BootOption->Description, UEFI_HARD_DRIVE_NAME) == 0);
      }

    case IPMI_BOOT_DEVICE_SELECTOR_PXE:
      switch (CurrentBootOptionType) {
        case MSG_MAC_ADDR_DP:
        case MSG_VLAN_DP:
        case MSG_IPv4_DP:
        case MSG_IPv6_DP:
          return TRUE;
        default:
          return FALSE;
      }

    case IPMI_BOOT_DEVICE_SELECTOR_FLOPPY:
      switch (CurrentBootOptionType) {
        case MSG_USB_DP:
        case MSG_USB_CLASS_DP:
          return TRUE;
        default:
          return FALSE;
      }

    case IPMI_BOOT_DEVICE_SELECTOR_BIOS_SETUP:
      return (StrCmp (BootOption->Description, ENTER_SETUP_STR) == 0);

    default:
      return FALSE;
  }
}

/**
  Check if the given boot option was created by this library as an IPMI boot override option.

  @param[in] BootOption     Pointer to the boot option to check.

  @retval  TRUE      Boot option was created by this library as an IPMI override option.
  @retval  FALSE     Not an IPMI override option.
**/
BOOLEAN
IsIpmiOverrideBootOption (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  return (BootOption->OptionalDataSize == sizeof (EFI_GUID)) &&
         CompareGuid ((EFI_GUID *)BootOption->OptionalData, &mAmdIpmiOverrideBootOptionGuid);
}

/**
  Delete any boot options created by this library as an IPMI boot override option.

  @param[in] BootOptions         The list of boot options returned by BootManager.
  @param[in] BootOptionCount     The number of elements in BootOptions.
**/
VOID
RemoveOverrideOptions (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  IN UINTN                         BootOptionCount
  )
{
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *CurrentBootOption;

  for (Index = 0; Index < BootOptionCount; Index++) {
    CurrentBootOption = &BootOptions[Index];
    if (IsIpmiOverrideBootOption (CurrentBootOption)) {
      EfiBootManagerDeleteLoadOptionVariable (CurrentBootOption->OptionNumber, LoadOptionTypeBoot);
    }
  }
}

/**
  Update the boot order according to the current IPMI boot override setting.
  If the current setting is not None, creates an override boot option for any options matching the
  current setting to add at the front of the boot order.
  Otherwise, clears any invalid override options which may have been created in the previous boot.

  @param[in] BootOptions                 The list of boot options returned by BootManager.
  @param[in] BootOptionCount             The number of elements in BootOptions.
  @param[in] CurrentIpmiBootOverride     The IPMI override to be used for the current boot.

  @retval  EFI_SUCCESS         Boot order updated successfully, or not necessary.
  @retval  EFI_UNSUPPORTED     Attempted to override boot to an unsupported boot option.
  @retval  Other errors        Failed to update the boot order.
**/
EFI_STATUS
UpdateBootOrder (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  IN UINTN                         BootOptionCount,
  IN UINT8                         CurrentIpmiBootOverride
  )
{
  EFI_STATUS                                   Status;
  UINTN                                        Index;
  AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL  *BootOptionPriorityProtocol;
  UINTN                                        BootPriorityCount;
  EFI_HANDLE                                   *BootPriorityHandles;
  EFI_BOOT_MANAGER_LOAD_OPTION                 *CurrentBootOption;
  EFI_BOOT_MANAGER_LOAD_OPTION                 OverrideBootOption;
  UINTN                                        OverrideBootOptionIndex;

  // If the current override is None, check to delete any existing override options which are
  // no longer valid. Otherwise, no boot order update needed.
  if (CurrentIpmiBootOverride == IPMI_BOOT_DEVICE_SELECTOR_NO_OVERRIDE) {
    RemoveOverrideOptions (BootOptions, BootOptionCount);
    return EFI_SUCCESS;
  }

  // Use the platform specific boot priority override if found.
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                  NULL,
                  &BootPriorityCount,
                  &BootPriorityHandles
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < BootPriorityCount; Index++) {
      Status = gBS->HandleProtocol (
                      BootPriorityHandles[Index],
                      &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                      (VOID **)&BootOptionPriorityProtocol
                      );
      if (!EFI_ERROR (Status) &&
          (BootOptionPriorityProtocol->IpmiBootDeviceSelectorType == CurrentIpmiBootOverride))
      {
        DEBUG ((DEBUG_INFO, "Valid BootOptionPriority override detected\n"));
        RemoveOverrideOptions (BootOptions, BootOptionCount);
        EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, BootOptionPriorityProtocol->Compare);
        gBS->FreePool (BootPriorityHandles);
        return EFI_SUCCESS;
      }
    }

    gBS->FreePool (BootPriorityHandles);
  }

  OverrideBootOptionIndex = 0;

  for (Index = 0; Index < BootOptionCount; Index++) {
    CurrentBootOption = &BootOptions[Index];
    if (!IsIpmiOverrideBootOption (CurrentBootOption) && BootOptionMatch (CurrentBootOption, CurrentIpmiBootOverride)) {
      // If this boot option is a match for the selected IPMI boot override setting, initialize a new temporary hidden boot
      // option for it which can be moved to the front of the boot order.
      Status = EfiBootManagerInitializeLoadOption (
                 &OverrideBootOption,
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 LOAD_OPTION_CATEGORY_BOOT | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN,
                 CurrentBootOption->Description,
                 CurrentBootOption->FilePath,
                 (UINT8 *)&mAmdIpmiOverrideBootOptionGuid,
                 sizeof (EFI_GUID)
                 );

      if (Status == EFI_INVALID_PARAMETER) {
        DEBUG ((DEBUG_INFO, "Failed to initialize an IPMI boot override option for Boot%04x\n", CurrentBootOption->OptionNumber));
        continue;
      }

      // Check if this override boot option is already present from the previous boot and can be reused. Otherwise, add it.
      if (EfiBootManagerFindLoadOption (&OverrideBootOption, BootOptions, BootOptionCount) == -1) {
        Status = EfiBootManagerAddLoadOptionVariable (&OverrideBootOption, OverrideBootOptionIndex);
        if (EFI_ERROR (Status)) {
          EfiBootManagerFreeLoadOption (&OverrideBootOption);
          return Status;
        }

        OverrideBootOptionIndex++;
      }

      EfiBootManagerFreeLoadOption (&OverrideBootOption);
    } else if (IsIpmiOverrideBootOption (CurrentBootOption) && !BootOptionMatch (CurrentBootOption, CurrentIpmiBootOverride)) {
      // Remove any invalid override boot options which may have been added in the previous boot.
      EfiBootManagerDeleteLoadOptionVariable (CurrentBootOption->OptionNumber, LoadOptionTypeBoot);
    }
  }

  return EFI_SUCCESS;
}

/**
  Handles possible IPMI boot overrides by modifying the current list of boot options.
  Uses sorting function installed in BootOptionPriorityProtocol if the protocol is installed
  and a valid IPMI override is detected.

  @retval  EFI_SUCCESS              Successfully updated the current boot order, or not necessary.
  @retval  EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval  Other errors             Failed to apply the override to the current boot order.
**/
EFI_STATUS
HandleIpmiBootOverride (
  VOID
  )
{
  EFI_STATUS                              Status;
  EFI_BOOT_MANAGER_LOAD_OPTION            *BootOptions;
  UINTN                                   BootOptionCount;
  IPMI_GET_BOOT_OPTIONS_REQUEST           BootOptionsRequest;
  IPMI_SET_BOOT_OPTIONS_RESPONSE          SetBootOptionsResponse;
  VOID                                    *GetBootOptionsBuffer;
  VOID                                    *SetBootOptionsBuffer;
  IPMI_GET_BOOT_OPTIONS_RESPONSE          *BootOptionsResponse;
  IPMI_SET_BOOT_OPTIONS_REQUEST           *SetBootOptionsRequest;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5  *BootOptionsParameterData;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5  *SetBootOptionsParameterData;
  UINT8                                   CurrentIpmiBootOverride;

  BootOptions     = NULL;
  BootOptionCount = 0;

  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));
  ZeroMem (&SetBootOptionsResponse, sizeof (IPMI_SET_BOOT_OPTIONS_RESPONSE));

  // Setup buffers
  GetBootOptionsBuffer = AllocateZeroPool (sizeof (BootOptionsResponse) + sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));
  if (GetBootOptionsBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetBootOptionsBuffer = AllocateZeroPool (sizeof (SetBootOptionsRequest) + sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));
  if (SetBootOptionsBuffer == NULL) {
    FreePool (GetBootOptionsBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  // Setup parameter data
  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;
  BootOptionsResponse                                         = (IPMI_GET_BOOT_OPTIONS_RESPONSE *)GetBootOptionsBuffer;
  Status                                                      = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  BootOptionsParameterData                                    = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)((VOID *)BootOptionsResponse->ParameterData);

  if (EFI_ERROR (Status)) {
    goto end;
  }

  // Setup SetBootOptions parameter data
  SetBootOptionsRequest       = (IPMI_SET_BOOT_OPTIONS_REQUEST *)SetBootOptionsBuffer;
  SetBootOptionsParameterData = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)((VOID *)SetBootOptionsRequest->ParameterData);

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  if (BootOptions == NULL) {
    goto end;
  }

  // If received valid IPMI data, then override boot option
  if (!BootOptionsResponse->ParameterValid.Bits.ParameterValid && BootOptionsParameterData->Data1.Bits.BootFlagValid) {
    CurrentIpmiBootOverride = BootOptionsParameterData->Data2.Bits.BootDeviceSelector;
    switch (CurrentIpmiBootOverride) {
      case IPMI_BOOT_DEVICE_SELECTOR_BIOS_SETUP:
        DEBUG ((DEBUG_INFO, "[Bds] BIOS Setup option override detected via IPMI\n"));
        break;
      case IPMI_BOOT_DEVICE_SELECTOR_PXE:
        DEBUG ((DEBUG_INFO, "[Bds] PXE option override detected via IPMI\n"));
        break;
      case IPMI_BOOT_DEVICE_SELECTOR_HARDDRIVE:
        DEBUG ((DEBUG_INFO, "[Bds] HDD option override detected via IPMI\n"));
        break;
      case IPMI_BOOT_DEVICE_SELECTOR_FLOPPY:
        DEBUG ((DEBUG_INFO, "[Bds] Floppy (USB) option override detected via IPMI\n"));
        break;
      default:
        break;
    }

    // If IPMI override not persistent, reset boot option to None and persistent to true
    if (!BootOptionsParameterData->Data1.Bits.PersistentOptions) {
      SetBootOptionsRequest->ParameterValid.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;
      CopyMem (SetBootOptionsParameterData, BootOptionsParameterData, sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));
      SetBootOptionsParameterData->Data1.Bits.PersistentOptions  = 1;                                     // persistent
      SetBootOptionsParameterData->Data2.Bits.BootDeviceSelector = IPMI_BOOT_DEVICE_SELECTOR_NO_OVERRIDE; // revert to no override
      Status                                                     = IpmiSetSystemBootOptions (SetBootOptionsRequest, &SetBootOptionsResponse);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a - Error when resetting IPMI boot override value back to None (nonpersistent boot)\n", __func__));
      }
    }
  } else {
    CurrentIpmiBootOverride = IPMI_BOOT_DEVICE_SELECTOR_NO_OVERRIDE;
  }

  Status = UpdateBootOrder (BootOptions, BootOptionCount, CurrentIpmiBootOverride);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a - Error updating the boot order: %r\n", __func__, Status));
  }

end:
  // Free buffers
  FreePool (GetBootOptionsBuffer);
  FreePool (SetBootOptionsBuffer);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  return Status;
}
