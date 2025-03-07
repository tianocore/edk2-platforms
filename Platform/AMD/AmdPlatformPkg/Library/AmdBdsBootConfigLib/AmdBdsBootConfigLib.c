/** @file
  This library registers the BootOptionPriorityProtocol, if necessary. Searches
  through PCIE devices to find the LOM to set as default PXE boot.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent<BR>

**/

#include <Library/DebugLib.h>
#include <Library/AmdBoardBdsHookLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include "AmdBdsBootConfig.h"

EFI_HANDLE                                   mBoardBdsHandle = NULL;
AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL  mBootOptionPriorityProtocol;
EFI_DEVICE_PATH_PROTOCOL                     *mLomDevicePath;

/**
  Compares two device paths to see if FullDevicePath starts with PartialDevicePath.

  @param[in] PartialDevicePath  Partial device path pointer.
  @param[in] FullDevicePath     Complete device path pointer

  @retval TRUE    PartialDevicePath was found in FullDevicePath.
  @retval FALSE   PartialDevicePath was not found in FullDevicePath.

**/
BOOLEAN
StartsWithDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *PartialDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath
  )
{
  INTN  PartialSize;
  INTN  FullSize;

  // Size includes end of device path node, don't want this to be included in comparison
  PartialSize = (INTN)GetDevicePathSize (PartialDevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL);
  FullSize    = (INTN)GetDevicePathSize (FullDevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL);

  if ((PartialSize <= 0) || (FullSize <= 0)) {
    return FALSE;
  }

  if (CompareMem (PartialDevicePath, FullDevicePath, PartialSize) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Returns the priority number.

  @param[in] BootOption   Load option
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
PlatformBootOptionPriority (
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
  Returns the priority number, giving the LOM device highest priority.

  @param[in] BootOption   Load option
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
PlatformBootOptionLomPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  // highest priority for LOM
  if (StartsWithDevicePath (mLomDevicePath, BootOption->FilePath) &&
      (BootOptionType (BootOption->FilePath) == MSG_IPv4_DP))
  {
    return 0;
  }

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
   Compares boot priorities of two boot options while giving
   the LOM device the highest priority.

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @return           The difference between the Left and Right
                    boot options
 **/
INTN
EFIAPI
CompareBootOptionPlatformPriorityLom (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  return PlatformBootOptionLomPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Left) -
         PlatformBootOptionLomPriority ((EFI_BOOT_MANAGER_LOAD_OPTION *)Right);
}

/**
  PciEnumerationComplete Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
OnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  // Handle LOM Device path here
  Status = GetLomDevicePath (&mLomDevicePath);
  DEBUG ((DEBUG_INFO, "Searching for LOM device path, Status = %r\n", Status));

  if (!EFI_ERROR (Status)) {
    mBootOptionPriorityProtocol.IpmiBootDeviceSelectorType = IPMI_BOOT_DEVICE_SELECTOR_PXE;
    mBootOptionPriorityProtocol.Compare                    = CompareBootOptionPlatformPriorityLom;
    // Install Boot Option Priority Protocol here
    Status = gBS->InstallProtocolInterface (
                    &mBoardBdsHandle,
                    &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mBootOptionPriorityProtocol
                    );
    DEBUG ((DEBUG_INFO, "Installing gBoardBdsBootOptionPriorityProtocolGuid, Status = %r\n", Status));
  }
}

/**
  Constructor function for AmdBdsBootConfig. Register PcieEnumerationComplete
  Callback to handle IPMI seelctor choice and
  BootOptionPriorityProtocol installation

  @param[in]  ImageHandle     Handle for the image of this driver
  @param[in]  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

**/
EFI_STATUS
EFIAPI
AmdBdsBootConfigConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT  ProtocolNotifyEvent;
  VOID       *Registration;

  ProtocolNotifyEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiPciEnumerationCompleteProtocolGuid,
                          TPL_CALLBACK,
                          OnPciEnumerationComplete,
                          NULL,
                          &Registration
                          );

  if (ProtocolNotifyEvent == NULL) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
