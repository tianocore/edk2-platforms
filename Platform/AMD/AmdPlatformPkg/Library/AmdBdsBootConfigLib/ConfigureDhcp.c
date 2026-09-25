/** @file
  This file configures DHCP for all PXE capable devices.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Protocol/PlatformBootManager.h>
#include <Protocol/Ip4Config2.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include "AmdBdsBootConfig.h"

EFI_BOOT_MANAGER_LOAD_OPTION  *mPxeBootOptionPtr       = NULL;
UINTN                         mNumberOfPxeBootOptions  = 0;
EFI_HANDLE                    mBootOptionNicHandle     = NULL;
EFI_IP4_CONFIG2_PROTOCOL      *mNicIpv4Config2Protocol = NULL;

/**
  This function retrieves the interface information for the IVP4 Configuration protocol.

  @param[out] InterfaceInformation  IPV4 configuration data

  @retval EFI_SUCCESS             Interface information successfully retrieved
  @retval EFI_OUT_OF_RESOURCES    Insufficient system memory
**/
EFI_STATUS
GetNicIpv4InterfaceInfo (
  OUT EFI_IP4_CONFIG2_INTERFACE_INFO  **InterfaceInformation
  )
{
  EFI_STATUS                      Status;
  UINTN                           DataSize;
  EFI_IP4_CONFIG2_INTERFACE_INFO  *InterfaceInfo;

  // Get the interface information size.
  DataSize      = 0;
  InterfaceInfo = NULL;
  Status        = mNicIpv4Config2Protocol->GetData (
                                             mNicIpv4Config2Protocol,
                                             Ip4Config2DataTypeInterfaceInfo,
                                             &DataSize,
                                             NULL
                                             );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  InterfaceInfo = AllocateZeroPool (DataSize);
  if (InterfaceInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the interface info.
  Status = mNicIpv4Config2Protocol->GetData (
                                      mNicIpv4Config2Protocol,
                                      Ip4Config2DataTypeInterfaceInfo,
                                      &DataSize,
                                      InterfaceInfo
                                      );
  if (!EFI_ERROR (Status)) {
    *InterfaceInformation = InterfaceInfo;
  } else {
    FreePool (InterfaceInfo);
    *InterfaceInformation = NULL;
  }

  return Status;
}

/**
  This function configures DHCP on the FileDevicePath parameter, and returns its
  subnet ID.

  @param[in]    FileDevicePath         Device path to configure DHCP
  @param[out]   SubnetId               Subnet IP address

  @retval EFI_SUCCESS                 DHCP configured, valid IP address information found
  @retval others                      DHCP could not be configured properly
**/
EFI_STATUS
RetrieveSubnetId (
  IN EFI_DEVICE_PATH_PROTOCOL  *FileDevicePath,
  OUT EFI_IP_ADDRESS           *SubnetId
  )
{
  EFI_STATUS                      Status;
  EFI_IP_ADDRESS                  SubnetMask;
  EFI_IP4_CONFIG2_POLICY          Policy;
  EFI_IP4_CONFIG2_INTERFACE_INFO  *InterfaceInfo;
  UINTN                           TryTimes;
  EFI_TPL                         OldTpl;
  CHAR16                          *DevicePathStr;
  UINT8                           *ZeroBuff;

  InterfaceInfo = NULL;
  Status        = GetNicIpv4InterfaceInfo (&InterfaceInfo);
  if (EFI_ERROR (Status) || (InterfaceInfo == NULL)) {
    DEBUG ((DEBUG_ERROR, "Get mode data is failed.\n"));
    return Status;
  }

  DevicePathStr = ConvertDevicePathToText (FileDevicePath, FALSE, FALSE);
  ZeroBuff      = AllocateZeroPool (4);
  if (ZeroBuff == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((CompareMem (&InterfaceInfo->StationAddress.Addr[0], ZeroBuff, 4) != 0) ||
      (CompareMem (&InterfaceInfo->SubnetMask.Addr[0], ZeroBuff, 4) != 0))
  {
    // Set the interface IPv4 policy to DHCP.
    Policy = Ip4Config2PolicyDhcp;
    Status = mNicIpv4Config2Protocol->SetData (
                                        mNicIpv4Config2Protocol,
                                        Ip4Config2DataTypePolicy,
                                        sizeof (EFI_IP4_CONFIG2_POLICY),
                                        &Policy
                                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Set IPv4 policy error.\n"));
      goto Error;
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    DEBUG ((DEBUG_INFO, "Current TPL %d.\n", OldTpl));
    gBS->RestoreTPL (TPL_APPLICATION);

    DEBUG ((DEBUG_INFO, "Waiting for DHCP returns the IP information...\n"));
    TryTimes = PcdGet8 (PcdAmdGetIpInfoRetryCount);

    do {
      gBS->Stall (100 * 1000); // Each stall for 100ms
      FreePool (InterfaceInfo);
      InterfaceInfo = NULL;

      if (TryTimes == 0) {
        DEBUG ((DEBUG_ERROR, "Failed to get the valid subnet ID on Device Path - %s\n", DevicePathStr));
        goto Error;
      }

      TryTimes--;
      Status = GetNicIpv4InterfaceInfo (&InterfaceInfo);
      if (EFI_ERROR (Status) || (InterfaceInfo == NULL)) {
        DEBUG ((DEBUG_ERROR, "Get mode data is failed.\n"));
        continue;
      }
    } while (CompareMem (&InterfaceInfo->StationAddress.Addr[0], ZeroBuff, 4) != 0 ||
             CompareMem (&InterfaceInfo->SubnetMask.Addr[0], ZeroBuff, 4) != 0);

    gBS->RaiseTPL (OldTpl);
  }

  IP4_COPY_ADDRESS (&SubnetMask.Addr, &InterfaceInfo->SubnetMask);
  SubnetId->v4.Addr[0] = InterfaceInfo->StationAddress.Addr[0] & SubnetMask.v4.Addr[0];
  SubnetId->v4.Addr[1] = InterfaceInfo->StationAddress.Addr[1] & SubnetMask.v4.Addr[1];
  SubnetId->v4.Addr[2] = InterfaceInfo->StationAddress.Addr[2] & SubnetMask.v4.Addr[2];
  SubnetId->v4.Addr[3] = InterfaceInfo->StationAddress.Addr[3] & SubnetMask.v4.Addr[3];

  DEBUG ((
    DEBUG_INFO,
    "Subnet ID: %d.%d.%d.%d on Device Path - %s\n",
    SubnetId->v4.Addr[0],
    SubnetId->v4.Addr[1],
    SubnetId->v4.Addr[2],
    SubnetId->v4.Addr[3],
    DevicePathStr
    ));

  Status = EFI_SUCCESS;

Error:
  FreePool (DevicePathStr);
  FreePool (ZeroBuff);
  if (InterfaceInfo != NULL) {
    FreePool (InterfaceInfo);
  }

  return Status;
}

/**
  This function handles the IPv4 protocol, and searches for the Subnet IP address
  corresponding to the passed device path.

  @param[in]   FileDevicePath         Device path to configure DHCP, and compare it Subnet ID
  @param[out]  FoundNicSubnetIp       Subnet IP address for newly configured DHCP device

  @retval EFI_SUCCESS       Subnet IP address successfully obtained
  @retval others            Subnet IP address could not be retrieved, or Ipv4 protocol
                            could not be located
**/
EFI_STATUS
GetSubnetIdFromBootOptionNicHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FileDevicePath,
  OUT EFI_IP_ADDRESS            *FoundNicSubnetIp
  )
{
  EFI_STATUS  Status;

  //
  // Locate Ip4Config2
  //
  Status = gBS->HandleProtocol (
                  mBootOptionNicHandle,
                  &gEfiIp4Config2ProtocolGuid,
                  (VOID **)&mNicIpv4Config2Protocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate IPv4 Config2 protocol - %r.\n", __func__, Status));
    return Status;
  }

  // Get Subnet ID assigned to this NIC.
  Status = RetrieveSubnetId (FileDevicePath, FoundNicSubnetIp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get Subnet ID on this NIC handle\n"));
  }

  return Status;
}

/**
  This function identifies the device path node with the specified subtype

  @param[in]   ThisDevicePath                 The EFI device path
  @param[in]   DevicePathNodeSubtypeToSearch  Device path subtype to search
  @param[out]  MatchedDevicePathNode          Pointer to retrieve the pointer
                                              of matched device path node.

  @retval EFI_DEVICE_PATH_PROTOCOL  Device path subtype found, the
                                    remaining device path is returned.
  @retval NULL                      Device path subtype is not found
**/
EFI_DEVICE_PATH_PROTOCOL *
IdentifyMessageDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL  *ThisDevicePath,
  IN  UINT8                     DevicePathNodeSubtypeToSearch,
  OUT EFI_DEVICE_PATH_PROTOCOL  **MatchedDevicePathNode        OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  if (MatchedDevicePathNode != NULL) {
    *MatchedDevicePathNode = NULL;
  }

  DevicePath = ThisDevicePath;

  while (TRUE) {
    if (IsDevicePathEnd (DevicePath)) {
      break;
    }

    if ((DevicePath->Type == MESSAGING_DEVICE_PATH) && (DevicePath->SubType == DevicePathNodeSubtypeToSearch)) {
      if (MatchedDevicePathNode != NULL) {
        *MatchedDevicePathNode = DevicePath;
      }

      DevicePath = NextDevicePathNode (DevicePath);  // Advance to next device path protocol as
                                                     // Remaining device path.
      return DevicePath;
    }

    DevicePath = NextDevicePathNode (DevicePath); // Advance to next device path protocol.
  }

  return NULL;
}

/**
  This function locates the EFI HANDLE's that have MTFTP installed, and searches for
  the handle that also has a matching MAC address thats in BootOptionDevicePath.

  @param[in]   BootOptionDevicePath       Device path to search for the physical handle
  @param[out]  NicHandle                  The EFI_HANDLE corresponding to the BootOptionDevicePath

  @retval EFI_SUCCESS     The EFI_HANDLE correspnding to BootOptionDevicePath was found
  @retval EFI_NOT_FOUND   The handle was not found
**/
EFI_STATUS
IdentifyPxeBootOptionPhysicalNicHandle (
  IN   EFI_DEVICE_PATH_PROTOCOL  *BootOptionDevicePath,
  OUT  EFI_HANDLE                *NicHandle
  )
{
  EFI_STATUS                Status;
  UINTN                     BufferSize;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  CHAR16                    *DevicePathStr;
  EFI_DEVICE_PATH_PROTOCOL  *BootOptionMacAddressDp;
  EFI_DEVICE_PATH_PROTOCOL  *TargetMacAddressDp;
  VOID                      **DummyInterface;

  *NicHandle   = NULL;
  BufferSize   = 0;
  HandleBuffer = NULL;

  // Get the MAC address of Boot Option
  IdentifyMessageDevicePathNode (
    BootOptionDevicePath,
    MSG_MAC_ADDR_DP,
    &BootOptionMacAddressDp
    );
  DevicePathStr = ConvertDevicePathToText (BootOptionMacAddressDp, FALSE, FALSE);
  DEBUG ((DEBUG_INFO, "  Boot Option MAC address Device Path - %s.\n", DevicePathStr));
  FreePool (DevicePathStr);

  // Get all DHCP service binding protocol.
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  NULL,
                  &BufferSize,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "  Failed to locate all DHCP service binding protocols.\n"));
    FreePool (HandleBuffer);
    return Status;
  }

  for (Index = 0; Index < BufferSize/sizeof (EFI_HANDLE); Index++) {
    // Check if the handle also has MTFTP service binding protocol installed.
    Status = gBS->HandleProtocol (
                    *(HandleBuffer + Index),
                    &gEfiMtftp4ServiceBindingProtocolGuid,
                    (VOID **)&DummyInterface
                    );
    if (EFI_ERROR (Status)) {
      continue;
    } else {
      DEBUG ((DEBUG_INFO, "  MTFTP service binding protocol is found on handle %x.\n", *(HandleBuffer + Index)));
    }

    // Get the device path protocol from this handle.
    Status = gBS->HandleProtocol (
                    *(HandleBuffer + Index),
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&TargetMacAddressDp
                    );
    if (EFI_ERROR (Status)) {
      continue;
    } else {
      DevicePathStr = ConvertDevicePathToText (TargetMacAddressDp, FALSE, FALSE);
      DEBUG ((DEBUG_INFO, "  Device Path on the handle has DHCP and MTFTP service binding protocol - %s.\n", DevicePathStr));
      FreePool (DevicePathStr);
    }

    // Get the MAC address of the handle with DHCP and MTFTP service binding protocol installed.
    IdentifyMessageDevicePathNode (
      TargetMacAddressDp,
      MSG_MAC_ADDR_DP,
      &TargetMacAddressDp
      );
    if (TargetMacAddressDp == NULL) {
      continue;
    }

    DevicePathStr = ConvertDevicePathToText (TargetMacAddressDp, FALSE, FALSE);
    DEBUG ((DEBUG_INFO, "  MAC address Device Path - %s.\n", DevicePathStr));
    FreePool (DevicePathStr);
    if (CompareMem (
          (VOID *)&((MAC_ADDR_DEVICE_PATH *)((VOID *)TargetMacAddressDp))->MacAddress,
          (VOID *)&((MAC_ADDR_DEVICE_PATH *)((VOID *)BootOptionMacAddressDp))->MacAddress,
          6
          ) == 0)
    {
      DEBUG ((DEBUG_INFO, "  EFI handle %x is the physical NIC that supported for PXE boot.\n", *(HandleBuffer + Index)));
      *NicHandle = *(HandleBuffer + Index);
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "  No matched EFI handle with DHCP and MTFTP service binding protocol installed.\n"));
  return EFI_NOT_FOUND;
}

/**
  This function searches through all boot options and identifies which support PXE. It places these options
  into a PXE specific array of boot options, mPxeBootOptionPtr.

  @param VOID
  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
IdentifyAllPxeBootOptions (
  VOID
  )
{
  UINTN                         Index;
  CHAR16                        *DevicePathStr;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionsCount;

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionsCount, LoadOptionTypeBoot);

  if ((BootOptions != NULL) && (BootOptionsCount != 0)) {
    // Allocate memory for Boot option number of Ipv4 PXE
    mPxeBootOptionPtr = AllocateZeroPool (BootOptionsCount * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
    if (mPxeBootOptionPtr == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough memory for mPxeBootOptionPtr array\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    BootOption = (EFI_BOOT_MANAGER_LOAD_OPTION *)BootOptions;
    for (Index = 0; Index < BootOptionsCount; Index++) {
      if (BootOption->FilePath == NULL) {
        DEBUG ((DEBUG_ERROR, "Unsure EFI device path of this boot option.\n"));
        ASSERT (FALSE);
      }

      DevicePathStr = ConvertDevicePathToText (BootOption->FilePath, FALSE, FALSE);
      if (DevicePathStr != NULL) {
        RemainingDevicePath = BootOption->FilePath;

        // Must have MAC device path
        RemainingDevicePath = IdentifyMessageDevicePathNode (RemainingDevicePath, MSG_MAC_ADDR_DP, NULL);
        if (RemainingDevicePath == NULL) {
          BootOption++;
          continue;
        }

        // Must have IPv4 device path
        RemainingDevicePath = IdentifyMessageDevicePathNode (RemainingDevicePath, MSG_IPv4_DP, NULL);
        if (RemainingDevicePath == NULL) {
          BootOption++;
          continue;
        }

        // Must not have URI device path
        RemainingDevicePath = IdentifyMessageDevicePathNode (RemainingDevicePath, MSG_URI_DP, NULL);
        if (RemainingDevicePath != NULL) {
          BootOption++;
          continue;
        }

        DEBUG ((DEBUG_INFO, "%s\n", BootOption->Description));
        DEBUG ((DEBUG_INFO, "  Boot Option Type   = %d\n", BootOption->OptionType));
        DEBUG ((DEBUG_INFO, "  Device Path        = %s\n", DevicePathStr));
        FreePool (DevicePathStr);

        //
        // Add to PXE boot option array
        //
        CopyMem (&mPxeBootOptionPtr[mNumberOfPxeBootOptions], BootOption, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
        mPxeBootOptionPtr[mNumberOfPxeBootOptions].FilePath = DuplicateDevicePath (BootOption->FilePath);
        mNumberOfPxeBootOptions++;
      } else {
        DEBUG ((DEBUG_ERROR, "DevicePathStr == NULL\n"));
      }

      BootOption++;
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionsCount);
  return EFI_SUCCESS;
}

/**
  This function searches for the NIC device path that has subnet IP address specified
  in NicAddress. Configures DHCP to retrieve subnet address for all PXE enabled
  boot options. Caller is responsible for freeing *NicDevicePath.

  @param[in] NicAddress                    Subnet IP address for the NIC to search for.
  @param[out] NicDevicePath                The NIC device path that has the subnet IP address
                                           specified in RequestedNicAddress

  @retval EFI_SUCCESS                      The requested NIC device path was found with subnet address
                                           RequestedNicAddress
  @retval EFI_OUT_OF_RESOURCES             Memory allocation failed.
  @retval EFI_INVALID_PARAMETER            NicDevicePath is NULL.
  @retval EFI_NOT_FOUND                    There were no PXE boot options, or NIC with subnet "NicAddress"
                                           was not found
**/
EFI_STATUS
EFIAPI
GetNicDevicePath (
  IN   EFI_IP_ADDRESS            NicAddress,
  OUT  EFI_DEVICE_PATH_PROTOCOL  **NicDevicePath
  )
{
  EFI_STATUS      Status;
  UINTN           Index;
  CHAR16          *DevicePathStr;
  EFI_IP_ADDRESS  FoundNicAddress;

  if (NicDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = IdentifyAllPxeBootOptions ();

  if ((mNumberOfPxeBootOptions == 0) || EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    goto End;
  }

  //
  // Connect device path to get the subnet ID.
  //
  for (Index = 0; Index < mNumberOfPxeBootOptions; Index++) {
    DEBUG ((DEBUG_INFO, "Connect PXE BootOption %d device path to get the subnet ID...\n", Index));
    Status = EfiBootManagerConnectDevicePath (mPxeBootOptionPtr[Index].FilePath, NULL);
    if (!EFI_ERROR (Status) || (Status == EFI_ALREADY_STARTED)) {
      Status = IdentifyPxeBootOptionPhysicalNicHandle (mPxeBootOptionPtr[Index].FilePath, &mBootOptionNicHandle);
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = GetSubnetIdFromBootOptionNicHandle (mPxeBootOptionPtr[Index].FilePath, &FoundNicAddress);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (CompareMem (NicAddress.v4.Addr, FoundNicAddress.v4.Addr, sizeof (FoundNicAddress.v4.Addr)) == 0) {
        // found matching Ip Address
        Status         = EFI_SUCCESS;
        *NicDevicePath = DuplicateDevicePath (mPxeBootOptionPtr[Index].FilePath);
        DevicePathStr  = ConvertDevicePathToText (*NicDevicePath, FALSE, FALSE);
        DEBUG ((DEBUG_INFO, "Found device path on specified subnet, Device path = %s\n", DevicePathStr));
        FreePool (DevicePathStr);
        break;
      } else {
        Status = EFI_NOT_FOUND;
      }
    }
  }

End:
  //
  // Clean up variables.
  //
  for (Index = 0; Index < mNumberOfPxeBootOptions; Index++) {
    FreePool (mPxeBootOptionPtr[Index].FilePath);
  }

  FreePool (mPxeBootOptionPtr);
  mNumberOfPxeBootOptions = 0;
  mBootOptionNicHandle    = NULL;

  return Status;
}
