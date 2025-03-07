/*****************************************************************************
 * Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
*******************************************************************************/
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Protocol/PciEnumerationComplete.h>
#include <IndustryStandard/Ipmi.h>
#include <Bus/Pci/PciBusDxe/PciBus.h>
#include <Pcd/SmbiosPcd.h>
#include <Library/PciSegmentLib.h>
#include <Pcd/SmbiosPcd.h>
#include "SmbiosCommon.h"
#include <IndustryStandard/Ipmi.h>
#include <Library/BoardBdsHookLib.h>

EFI_HANDLE mBoardBdsHandle = NULL;
BOARD_BDS_BOOT_FROM_DEVICE_PATH_PROTOCOL  mBootDevicePathProtocol;

/**
  Find the Lan-On-Motherboard device path. Installs BOARD_BDS_BOOT_FROM_DEVICE_PATH_PROTOCOL
  with the LOM device path protocol

  @retval EFI NOT_FOUND         LOM device path is not found
  @retval EFI_SUCCESS           LOM device path found
**/
EFI_STATUS
EFIAPI
InstallLomDevicePath (
  )
{
  SMBIOS_ONBOARD_DEV_EXT_INFO_RECORD  *DevExtInfoRecord;
  EFI_STATUS                          Status;
  EFI_HANDLE                          *PciHandles;
  UINTN                               PciHandlesSize;
  UINTN                               Index;
  EFI_PCI_IO_PROTOCOL                 *PciProtocol;
  PCI_IO_DEVICE                       *PciIoDevice;
  UINT8                               NumberOfDevices;
  UINT8                               DevIdx;
  UINTN                               SegmentNumber;
  UINTN                               BusNumber;
  UINTN                               DeviceNumber;
  UINTN                               FunctionNumber;

  NumberOfDevices = PcdGet8 (PcdAmdSmbiosType41Number);
  DevExtInfoRecord = (SMBIOS_ONBOARD_DEV_EXT_INFO_RECORD *)PcdGetPtr (PcdAmdSmbiosType41);

  // No device entries found
  if (NumberOfDevices == 0) {
    DEBUG ((DEBUG_INFO, "No onboard devices found.\n"));
    return EFI_NOT_FOUND;
  }

  //search through present on board devices, look for onboard ethernet
  for (DevIdx = 0; DevIdx < NumberOfDevices; DevIdx++) {
    if (AsciiStrCmp(DevExtInfoRecord->RefDesignationStr, "Onboard Ethernet") == 0) {
        break;
    }
    DevExtInfoRecord++;
  }

  //edge case, no Onboard Ethernet designator
  if (AsciiStrCmp(DevExtInfoRecord->RefDesignationStr, "Onboard Ethernet") != 0) {
    DEBUG((DEBUG_INFO, "No Onboard ethernet SMBIOS designator found!\n"));
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &PciHandlesSize,
                  &PciHandles
                  );

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_INFO, "Can't locate gEfiPciIoProtocolGuid Protocol: Status = %r\n\n", Status));
    return Status;
  }

  for (Index = 0; Index < PciHandlesSize; Index++) {
    Status = gBS->HandleProtocol (
                    PciHandles[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciProtocol
                    );
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_INFO, "ERROR - Status = %r when locating PciIoProtocol\n", Status));
      continue;
    }

    PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS(PciProtocol);
    Status = PciIoDevice->PciIo.GetLocation(&PciIoDevice->PciIo, &SegmentNumber, &BusNumber, &DeviceNumber, &FunctionNumber);

    if ((PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS (SegmentNumber, BusNumber, DeviceNumber, FunctionNumber, 2)) == DevExtInfoRecord->DeviceId) &&
          (PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS (SegmentNumber, BusNumber, DeviceNumber,FunctionNumber, 0)) == DevExtInfoRecord->VendorId)) {
          //Making Lan0 default for systems with two LANs
          if (FunctionNumber == 0) {
            DEBUG((DEBUG_INFO, "Found Onboard Device with DeviceID=0x%X, VendorID=0x%X\n", DevExtInfoRecord->DeviceId, DevExtInfoRecord->VendorId));
            Status = EFI_SUCCESS;
            //install device path protocol here
            mBootDevicePathProtocol.Device = PciIoDevice->DevicePath;
            mBootDevicePathProtocol.IpmiBootDeviceSelectorType = IPMI_BOOT_DEVICE_SELECTOR_PXE;
            Status = gBS->InstallProtocolInterface (
                &mBoardBdsHandle,
                &gBoardBdsBootFromDevicePathProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &mBootDevicePathProtocol
                );
            if (!EFI_ERROR (Status)) {
                DEBUG((DEBUG_INFO, "BoardBdsBootFromDevicePathProtocol installed successfully\n"));
            }
            break;
          }
    }
  }

  return Status;
}