/** @file
  Header file internal to AmdBdsBootConfigLib.

  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#pragma once

#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/AmdBootOptionPriorityProtocol.h>

#define UEFI_HARD_DRIVE_NAME  L"UEFI Hard Drive"

/**
  Find the Lan-On-Motherboard device path.

  @param[out]   LomDevicePath         DevicePath of the LOM device. NULL if the LOM
                                      is not found
  @retval       EFI NOT_FOUND         LOM device path is not found
  @retval       EFI_SUCCESS           LOM device path found
**/
EFI_STATUS
EFIAPI
GetLomDevicePath (
  OUT EFI_DEVICE_PATH_PROTOCOL  **LomDevicePath
  );

/**
  This function searches for the NIC device path that has subnet IP address specified
  in NicAddress. Configures DHCP to retrieve subnet address for all PXE enabled
  boot options. Caller is responsible for freeing *NicDevicePath.
  @param[in]  NicAddress                   Subnet IP address for the NIC to search for.
  @param[in, out] NicDevicePath            The NIC device path that has the subnet IP address
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
  IN      EFI_IP_ADDRESS            NicAddress,
  IN OUT  EFI_DEVICE_PATH_PROTOCOL  **NicDevicePath
  );

/**
  Returns the boot option type of a device.

  @param[in] DevicePath         The path of device whose boot option type
                                should be returned.
  @retval MAX_UINT8             Device type not found.
  @retval < MAX_UINT8           Device type found.
**/
UINT8
EFIAPI
AmdBootOptionType (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );
