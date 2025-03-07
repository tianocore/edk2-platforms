/** @file
  Implements PciPlatform.h
  This code supports a the private implementation
  of the Legacy BIOS Platform protocol

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_PLATFORM_H_
#define PCI_PLATFORM_H_

#include <IndustryStandard/Pci.h>
#include <Library/PcdLib.h>
//
// Global variables for Option ROMs
//
#define NULL_ROM_FILE_GUID \
{ 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}

#define ONBOARD_SPH_VIDEO_OPTION_ROM_FILE_GUID \
{ 0xE7D31EB4, 0x90F3, 0x4A14, {0x8A, 0x28, 0x48, 0xD0, 0x47, 0x42, 0xF8, 0xE1 }}

typedef struct {
  EFI_GUID    FileName;
  UINT16      VendorId;
  UINT16      DeviceId;
} PCI_OPTION_ROM_TABLE;

EFI_STATUS
EFIAPI
PhaseNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  );

EFI_STATUS
EFIAPI
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                     *This,
  IN  EFI_HANDLE                                    HostBridge,
  IN  EFI_HANDLE                                    RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS   PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE               ChipsetPhase
  );

/**
  Get PlatformPolicy for VGA IO ALIAS

  @param This       Protocol instance pointer.
  @param PciPolicy  PCI Platform Policy.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
GetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
  OUT       EFI_PCI_PLATFORM_POLICY    *PciPolicy
  );

/**
  Return a PCI ROM image for the onboard device represented by PciHandle

  @param This       Protocol instance pointer.
  @param PciHandle  PCI device to return the ROM image for.
  @param RomImage   PCI Rom Image for onboard device
  @param RomSize    Size of RomImage in bytes

  @retval EFI_SUCCESS   - RomImage is valid
  @retval EFI_NOT_FOUND - No RomImage

**/
EFI_STATUS
EFIAPI
GetPciRom (
  IN CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
  IN       EFI_HANDLE                 PciHandle,
  OUT      VOID                       **RomImage,
  OUT      UINTN                      *RomSize
  );

#endif
