/** @file
  Implements PciPlatform.c
  Registers onboard PCI ROMs with PCI.IO

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

#include "PciPlatform.h"

PCI_OPTION_ROM_TABLE  mPciOptionRomTable[] = {
  { ONBOARD_SPH_VIDEO_OPTION_ROM_FILE_GUID, 0x1002, 0x1435 },
  { NULL_ROM_FILE_GUID,                     0xffff, 0xffff }
};

EFI_PCI_PLATFORM_PROTOCOL  mPciPlatform = {
  PhaseNotify,
  PlatformPrepController,
  GetPlatformPolicy,
  GetPciRom
};

EFI_HANDLE  mPciPlatformHandle = NULL;
EFI_HANDLE  mImageHandle       = NULL;

EFI_STATUS
EFIAPI
PhaseNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                     *This,
  IN  EFI_HANDLE                                    HostBridge,
  IN  EFI_HANDLE                                    RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS   PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE               ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}

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
  )
{
  *PciPolicy |= EFI_RESERVE_VGA_IO_ALIAS;
  return EFI_SUCCESS;
}

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
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
  IN        EFI_HANDLE                 PciHandle,
  OUT       VOID                       **RomImage,
  OUT       UINTN                      *RomSize
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINTN                Segment;
  UINTN                Bus;
  UINTN                Device;
  UINTN                Function;
  UINT16               VendorId;
  UINT16               DeviceId;
  UINTN                TableIndex;

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0, 1, &VendorId);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 2, 1, &DeviceId);

  //
  // Loop through table of video option rom descriptions
  //
  for (TableIndex = 0; mPciOptionRomTable[TableIndex].VendorId != 0xffff; TableIndex++) {
    //
    // See if the PCI device specified by PciHandle matches at device in mPciOptionRomTable
    //
    if ((VendorId != mPciOptionRomTable[TableIndex].VendorId) ||
        (DeviceId != mPciOptionRomTable[TableIndex].DeviceId))
    {
      continue;
    }

    Status = GetSectionFromAnyFv (
               &mPciOptionRomTable[TableIndex].FileName,
               EFI_SECTION_RAW,
               0,
               RomImage,
               RomSize
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_STATUS return status of InstallProtocolInterface.

**/
EFI_STATUS
EFIAPI
PciPlatformDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mImageHandle = ImageHandle;

  //
  // Install on a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &mPciPlatformHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatform
                  );

  return Status;
}
