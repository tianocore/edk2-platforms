/** @file
  This library registers the BootOptionPriorityProtocol, if necessary.
  Searches through PCIE devices to find the LOM to set as default PXE boot.

  Copyright (C) 2024 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AmdIanaId.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>
#include "AmdBdsBootConfig.h"

// Generic protocol global variables
EFI_HANDLE                                   mBoardBdsHandle = NULL;
AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL  mBootOptionPriorityProtocol;
EFI_DEVICE_PATH_PROTOCOL                     *mLomDevicePath;

// variables for LOM override
EFI_DEVICE_PATH_PROTOCOL  *mLomDevicePath;

// variables for specific NIC override
BOOLEAN                   mIsDhcpConfigured  = FALSE;
BOOLEAN                   mRequestedNicFound = FALSE;
EFI_IP_ADDRESS            mRequestedSubnetId;
EFI_DEVICE_PATH_PROTOCOL  *mNicDevicePath;

/**
  Returns the boot option type of a device.

  @param[in] DevicePath         The DevicePath whose boot option type is
                                to be returned.
  @retval MAX_UINT8             Device type not found.
  @retval < MAX_UINT8           Device type found.
**/
UINT8
EFIAPI
AmdBootOptionType (
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

  return MAX_UINT8;
}

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
  UINTN  PartialSize;
  UINTN  FullSize;

  // Size includes end of device path node, don't want this to be included in comparison
  if (GetDevicePathSize (PartialDevicePath) > sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    PartialSize = (GetDevicePathSize (PartialDevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL));
  } else {
    PartialSize = 0;
  }

  if (GetDevicePathSize (FullDevicePath) > sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    FullSize = (GetDevicePathSize (FullDevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL));
  } else {
    FullSize = 0;
  }

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
INTN
PlatformBootOptionPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  //
  // EFI boot options
  //
  switch (AmdBootOptionType (BootOption->FilePath)) {
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
  Returns the priority number, giving the NIC with specified subnet address
  highest priority.

  @param[in] BootOption   Load option

  @retval
    OptionType                 EFI
    ------------------------------------
    NIC                         0
    PXE                         2
    DVD                         4
    USB                         6
    NVME                        7
    HDD                         8
    EFI Shell                   9
    Others                      100
**/
INTN
PlatformBootOptionNicPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  // highest priority for NIC
  if (StartsWithDevicePath (mNicDevicePath, BootOption->FilePath) &&
      (AmdBootOptionType (BootOption->FilePath) == MSG_IPv4_DP))
  {
    return 0;
  }

  //
  // EFI boot options
  //
  switch (AmdBootOptionType (BootOption->FilePath)) {
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
INTN
PlatformBootOptionLomPriority (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  // highest priority for LOM
  if (StartsWithDevicePath (mLomDevicePath, BootOption->FilePath) &&
      (AmdBootOptionType (BootOption->FilePath) == MSG_IPv4_DP))
  {
    return 0;
  }

  //
  // EFI boot options
  //
  switch (AmdBootOptionType (BootOption->FilePath)) {
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
  INTN  LeftPrio;
  INTN  RightPrio;

  LeftPrio  = (INTN)PlatformBootOptionLomPriority ((CONST EFI_BOOT_MANAGER_LOAD_OPTION *)Left);
  RightPrio = (INTN)PlatformBootOptionLomPriority ((CONST EFI_BOOT_MANAGER_LOAD_OPTION *)Right);

  return (LeftPrio - RightPrio);
}

/**
   Compares boot priorities of two boot options while giving
   a specific NIC device path determined via IPMI the highest priority.
   This function configures DHCP for all detected PXE boot options when it
   is first called.

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @return           The difference between the Left and Right
                    boot options
 **/
INTN
EFIAPI
CompareBootOptionPlatformPriorityNic (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  EFI_STATUS  Status;

  // initialize DHCP if necessary
  if (!mIsDhcpConfigured) {
    mIsDhcpConfigured = TRUE;
    // get device path for NIC with certain subnet ID
    Status = GetNicDevicePath (mRequestedSubnetId, &mNicDevicePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "Error locating NIC device path with subnetID : %x.%x.%x.%x\n",
        mRequestedSubnetId.v4.Addr[0],
        mRequestedSubnetId.v4.Addr[1],
        mRequestedSubnetId.v4.Addr[2],
        mRequestedSubnetId.v4.Addr[3]
        ));
      mRequestedNicFound = FALSE;
    } else {
      DEBUG ((
        DEBUG_INFO,
        "Found requested NIC device path with subnetID : %x.%x.%x.%x\n",
        mRequestedSubnetId.v4.Addr[0],
        mRequestedSubnetId.v4.Addr[1],
        mRequestedSubnetId.v4.Addr[2],
        mRequestedSubnetId.v4.Addr[3]
        ));

      mRequestedNicFound = TRUE;
    }
  }

  if (mRequestedNicFound) {
    return ((INTN)PlatformBootOptionNicPriority ((EFI_BOOT_MANAGER_LOAD_OPTION CONST *)Left)) -
           ((INTN)PlatformBootOptionNicPriority ((EFI_BOOT_MANAGER_LOAD_OPTION CONST *)Right));
  } else {
    return ((INTN)PlatformBootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION CONST *)Left)) -
           ((INTN)PlatformBootOptionPriority ((EFI_BOOT_MANAGER_LOAD_OPTION CONST *)Right));
  }
}

/**
  This function installs the BoardBdsBootOptionPriorityProtocol to boot
  from the Lan-On-Motherboard.

  @param[in] VOID

  @retval EFI_SUCCESS BoardBdsBootOptionPriorityProtocol installed successfully
  @retval others      BoardBdsBootOptionPriorityProtocol could not be installed, or
                      the LOM could not be found
**/
EFI_STATUS
EFIAPI
LomPxeOverride (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = GetLomDevicePath (&mLomDevicePath);
  DEBUG ((DEBUG_INFO, "Searching for LOM device path, Status = %r\n", Status));

  if (!EFI_ERROR (Status)) {
    mBootOptionPriorityProtocol.IpmiBootDeviceSelectorType = IPMI_BOOT_DEVICE_SELECTOR_PXE;
    mBootOptionPriorityProtocol.Compare                    = CompareBootOptionPlatformPriorityLom;
    Status                                                 = gBS->InstallProtocolInterface (
                                                                    &mBoardBdsHandle,
                                                                    &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                                                                    EFI_NATIVE_INTERFACE,
                                                                    &mBootOptionPriorityProtocol
                                                                    );
  }

  return Status;
}

/**
  This function installs the BoardBdsBootOptionPriorityProtocol to boot
  from a specific NIC with specified Subnet IP address found in IPMI
  boot initiator mailbox.

  @param[in] VOID

  @retval EFI_SUCCESS BoardBdsBootOptionPriorityProtocol installed successfully
  @retval others      BoardBdsBootOptionPriorityProtocol could not be installed
**/
EFI_STATUS
EFIAPI
NicPxeOverride (
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "Overriding PXE boot with NIC device path\n"));

  mBootOptionPriorityProtocol.IpmiBootDeviceSelectorType = IPMI_BOOT_DEVICE_SELECTOR_PXE;
  mBootOptionPriorityProtocol.Compare                    = CompareBootOptionPlatformPriorityNic;
  Status                                                 = gBS->InstallProtocolInterface (
                                                                  &mBoardBdsHandle,
                                                                  &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                                                                  EFI_NATIVE_INTERFACE,
                                                                  &mBootOptionPriorityProtocol
                                                                  );

  return Status;
}

/**
  PciEnumerationComplete Protocol notification event handler. Reads IPMI
  boot initiator mailbox and if there is valid data there, then prioritize PXE
  boot option that has matching DHCP subnet address. Otherwise override the PXE
  boot option with the LOM.

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
  UINT8       *IpmiMailboxReadBuffer;
  UINT8       *IpmiMailboxWriteBuffer;
  UINT32      ReadIanaId;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - entry\n", __FUNCTION__));

  IpmiMailboxWriteBuffer = AllocateZeroPool (IPMI_BOOT_INITIATOR_MAILBOX_BLOCK_DATA_SIZE);
  Status                 = IpmiReadBootInitiatorMailbox (0, &IpmiMailboxReadBuffer);
  if (EFI_ERROR (Status) || (IpmiMailboxReadBuffer == NULL)) {
    FreePool (IpmiMailboxWriteBuffer);
    return;
  }

  ReadIanaId = (((UINT32)IpmiMailboxReadBuffer[2] << 16)) | (((UINT32)IpmiMailboxReadBuffer[1]) << 8) | ((UINT32)IpmiMailboxReadBuffer[0]);
  // check for AMD IANA number and valid boot override value
  if ((ReadIanaId == AMD_IANA_ID) && (IpmiMailboxReadBuffer[3] == 1)) {
    IpmiWriteBootInitiatorMailbox (0, IpmiMailboxWriteBuffer, IPMI_BOOT_INITIATOR_MAILBOX_BLOCK_DATA_SIZE); // clear boot override data
    mRequestedSubnetId.v4.Addr[0] = IpmiMailboxReadBuffer[4];
    mRequestedSubnetId.v4.Addr[1] = IpmiMailboxReadBuffer[5];
    mRequestedSubnetId.v4.Addr[2] = IpmiMailboxReadBuffer[6];
    mRequestedSubnetId.v4.Addr[3] = IpmiMailboxReadBuffer[7];
    NicPxeOverride ();
  } else {
    LomPxeOverride ();
  }

  if (IpmiMailboxReadBuffer != NULL) {
    FreePool (IpmiMailboxReadBuffer);
  }

  FreePool (IpmiMailboxWriteBuffer);
}

/**
  Constructor function for AmdBdsBootConfig. Register PcieEnumerationComplete
  Callback to handle IPMI selector choice and
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
