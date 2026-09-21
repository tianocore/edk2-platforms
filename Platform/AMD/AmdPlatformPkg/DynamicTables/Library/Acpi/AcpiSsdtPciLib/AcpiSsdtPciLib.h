/** @file

  ACPI SSDT PCI Library internal header file.

  Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#pragma once

#include <Library/AmdPlatformSocLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Uefi/UefiBaseType.h>

/**
  Attach USB ASL template scope to root node.

  This function parses the AcpiSsdtUsb.asl template, finds the \_SB scope,
  detaches it from the template, and attaches it to the provided root node.

  @param[in,out]  RootNode  - AML root node to attach the USB scope to

  @retval     EFI_SUCCESS           USB scope successfully attached.
  @retval     EFI_NOT_FOUND         \_SB scope not found in template.
  @retval     Other                 Error from AML library.
**/
EFI_STATUS
EFIAPI
AttachUsbAslTemplate (
  IN OUT  AML_ROOT_NODE_HANDLE  RootNode
  );

/**
  Check if a USB host controller is connected to the specified root port.

  This function reads the secondary bus number from the root port and checks
  if the device at Device 0 on that bus is a USB host controller (Class Code 0x0C03).
  In PCIe topology, a device directly connected to a root port is always at Device 0.

  @param[in]      RootBridge  - Root Bridge instance containing the root port
  @param[in]      RPIndex     - Index of the root port to check
  @param[in,out]  DeviceNode  - AML device node for the root port (for creating child devices)

  @retval     EFI_SUCCESS           USB host controller found.
  @retval     EFI_NOT_FOUND         No USB host controller found on this root port.
  @retval     EFI_INVALID_PARAMETER Invalid parameters provided.
**/
EFI_STATUS
EFIAPI
DetectUsbHostController (
  IN      AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge,
  IN      UINTN                                RPIndex,
  IN OUT  AML_OBJECT_NODE_HANDLE               DeviceNode
  );

/**
  Install the ACPI SSDT System Device table.

  This function takes the pre-compiled AML bytecode from AcpiSsdtSysDev.asl,
  updates the table header with platform-specific OEM information, and installs
  it as a Secondary System Description Table (SSDT) via the ACPI Table Protocol.

  @retval EFI_SUCCESS           The SSDT SysDev table was installed successfully.
  @retval EFI_NOT_FOUND         The ACPI Table Protocol was not found.
  @retval EFI_INVALID_PARAMETER The compiled AML table has an unexpected signature.
  @retval Others                Failed to install the ACPI table.
**/
EFI_STATUS
EFIAPI
InstallAcpiSsdtSysDevTable (
  VOID
  );

/**
  Add GPE _Lxx notify methods for USB wake support.

  This function creates the _GPE scope and adds _L0B and _L10 methods
  that send wake notifications to USB host controllers. It uses the
  device paths that were saved when USB controllers were detected.

  @param[in]  ScopeNode  - Parent scope node (typically root) to attach _GPE scope

  @retval     EFI_SUCCESS           GPE methods added successfully.
  @retval     EFI_NOT_FOUND         No USB devices were detected.
  @retval     EFI_INVALID_PARAMETER Invalid parameters provided.
  @retval     Other                 Error from AML library.
**/
EFI_STATUS
EFIAPI
AddGpeLxxNotifyMethod (
  IN  AML_OBJECT_NODE_HANDLE  ScopeNode
  );
