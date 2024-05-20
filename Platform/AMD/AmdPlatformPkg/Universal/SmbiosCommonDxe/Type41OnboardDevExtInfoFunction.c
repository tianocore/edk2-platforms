/** @file
  AMD SMBIOS Type 41 Record

  Copyright (C) 2023 - 2024  Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Pcd/SmbiosPcd.h>
#include "SmbiosCommon.h"

#define MSR_MMIO_CFG_BASE  0xC0010058ul             // MMIO Configuration Base Address Register

/**
  This function adds onboard devices extended information smbios record (Type 41).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
OnboardDevExtInfoFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                          Status;
  EFI_SMBIOS_HANDLE                   SmbiosHandle;
  SMBIOS_TABLE_TYPE41                 *SmbiosRecord;
  SMBIOS_ONBOARD_DEV_EXT_INFO_RECORD  *DevExtInfoRecord;
  UINT8                               DevIdx;
  UINT8                               Idx;
  UINT8                               NumberOfDevices;
  UINTN                               StringOffset;
  CHAR8                               *RefDesStr;
  UINTN                               RefDesStrLen;
  UINT16                              SegmentNum;
  UINT8                               BusNum;
  UINT8                               DevNum;
  UINT8                               Functions;
  UINT8                               DeviceFound;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get the total number of onboard devices.
  NumberOfDevices = PcdGet8 (PcdAmdSmbiosType41Number);
  DEBUG ((DEBUG_INFO, "%a: Total number of AMD SMBIOS type41 PCD structure %d.\n", __func__, NumberOfDevices));
  DevExtInfoRecord = (SMBIOS_ONBOARD_DEV_EXT_INFO_RECORD *)PcdGetPtr (PcdAmdSmbiosType41);

  // No device entries found
  if (NumberOfDevices == 0) {
    DEBUG ((DEBUG_INFO, "No onboard devices found.\n"));
    return EFI_NOT_FOUND;
  }

  // Generate type41 smbios record for each device and add it to Smbios table.
  for (DevIdx = 0; DevIdx < NumberOfDevices; DevIdx++) {
    DEBUG ((DEBUG_MANAGEABILITY, "Device number %d:\n", DevIdx));
    // Check whether reference designation strings are present.
    if (DevExtInfoRecord->ReferenceDesignation != 0) {
      RefDesStr    = DevExtInfoRecord->RefDesignationStr;
      RefDesStrLen = AsciiStrLen (RefDesStr) + 1;
    } else {
      RefDesStr    = NULL;
      RefDesStrLen = 1;
    }

    DEBUG ((DEBUG_MANAGEABILITY, " - ReferenceDesignation = %d\n", DevExtInfoRecord->ReferenceDesignation));
    DEBUG ((DEBUG_MANAGEABILITY, " - DeviceType = %d\n", DevExtInfoRecord->DeviceType));
    DEBUG ((DEBUG_MANAGEABILITY, " - DeviceEnabled = %d\n", DevExtInfoRecord->DeviceEnabled));
    DEBUG ((DEBUG_MANAGEABILITY, " - DeviceTypeInstance = %d\n", DevExtInfoRecord->DeviceTypeInstance));
    DEBUG ((DEBUG_MANAGEABILITY, " - VendorId = %x\n", DevExtInfoRecord->VendorId));
    DEBUG ((DEBUG_MANAGEABILITY, " - DeviceId = %x\n", DevExtInfoRecord->DeviceId));
    DEBUG ((DEBUG_MANAGEABILITY, " - RefDesignationStr = %a\n", DevExtInfoRecord->RefDesignationStr));

    Status = GetBusDeviceInfo (
               &DevExtInfoRecord->VendorId,
               &DevExtInfoRecord->DeviceId,
               &DevExtInfoRecord->DeviceTypeInstance,
               &SegmentNum,
               &BusNum,
               &DevNum,
               &Functions,
               &DeviceFound
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Could not get SBDF details for idx %d\n", DevIdx));
      continue;
    }

    // Device not present
    if (DeviceFound == 0) {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "No onboard device found matching VendorId: %x DeviceId: %x\n",
        DevExtInfoRecord->VendorId,
        DevExtInfoRecord->DeviceId
        ));
      continue;
    }

    // Create one record for each function in a multi-function device
    for (Idx = 0; Idx <= 7; Idx++) {
      if ((Functions >> Idx) & 0x1) {
        SmbiosRecord = NULL;
        SmbiosRecord = AllocateZeroPool (
                         sizeof (SMBIOS_TABLE_TYPE41) + RefDesStrLen + 1
                         );
        if (SmbiosRecord == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          return Status;
        } else {
          SmbiosRecord->Hdr.Type             = SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION;
          SmbiosRecord->Hdr.Length           = sizeof (SMBIOS_TABLE_TYPE41);
          SmbiosRecord->Hdr.Handle           = 0;
          SmbiosRecord->ReferenceDesignation = DevExtInfoRecord->ReferenceDesignation;
          SmbiosRecord->DeviceType           = (DevExtInfoRecord->DeviceEnabled << 7) | DevExtInfoRecord->DeviceType;
          SmbiosRecord->DeviceTypeInstance   = DevExtInfoRecord->DeviceTypeInstance;
          SmbiosRecord->SegmentGroupNum      = SegmentNum;
          SmbiosRecord->BusNum               = BusNum;
          SmbiosRecord->DevFuncNum           = (DevNum << 3) + Idx;

          // Add strings to bottom of data block
          StringOffset = SmbiosRecord->Hdr.Length;
          CopyMem (
            (UINT8 *)SmbiosRecord + StringOffset,
            RefDesStr,
            RefDesStrLen
            );
          StringOffset += RefDesStrLen;

          Status = AddCommonSmbiosRecord (
                     Smbios,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
                     );
          FreePool (SmbiosRecord);
        }
      }
    }

    DevExtInfoRecord++;
  }

  return Status;
}

/**
  This function gets the Bus, Device and Segment number of a PCI device when Vendor ID, Device ID and instance
  are provided.

  @param[in]  VendorId        Vendor ID of the PCI device to be provided.
  @param[in]  DeviceId        Device ID of the PCI device to be provided
  @param[in]  Instance        Instance of the PCI device. If more than one devices with same vendor
                              and device ID is present, instance number is used.
  @param[out]  Segment        Segment number of the PCI device is assigned.
  @param[out]  Bus            Bus number of the PCI device is assigned.
  @param[out]  Device         Device number of the PCI device is assigned.
  @param[out]  Functions      Bits 0-7 of the Functions variable correspond to respective function numbers.
  @param[out]  DeviceFound    Set to 1 if the device is found.

  @retval EFI_SUCCESS         All parameters were valid.
**/
EFI_STATUS
EFIAPI
GetBusDeviceInfo (
  IN  UINT16  *VendorId,
  IN  UINT16  *DeviceId,
  IN  UINT8   *Instance,
  OUT UINT16  *Segment,
  OUT UINT8   *Bus,
  OUT UINT8   *Device,
  OUT UINT8   *Functions,
  OUT UINT8   *DeviceFound
  )
{
  UINT16  SegIdx;
  UINT8   BusIdx;
  UINT16  BusIdx16;
  UINT8   DevIdx;
  UINT8   FuncIdx;
  UINT8   InstanceCount;
  UINT16  MaxSegments;
  UINT8   BusRangeIdentifier;

  InstanceCount = *Instance;
  *DeviceFound  = 0;

  BusRangeIdentifier = (AsmReadMsr64 (MSR_MMIO_CFG_BASE) >> 2) & 0xF;
  if ( BusRangeIdentifier <= 0x8 ) {
    MaxSegments = 1;
  } else if ((BusRangeIdentifier >= 0x9) && (BusRangeIdentifier <= 0xF)) {
    MaxSegments = 1 << (BusRangeIdentifier - 0x8);
  }

  for (SegIdx = 0; SegIdx < MaxSegments; SegIdx++ ) {
    for (BusIdx16 = 0; BusIdx16 <= 255; BusIdx16++) {
      BusIdx = (UINT8)BusIdx16;
      for (DevIdx = 0; DevIdx < 32; DevIdx++) {
        if ((PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS (SegIdx, BusIdx, DevIdx, 0, 2)) == *DeviceId) &&
            (PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS (SegIdx, BusIdx, DevIdx, 0, 0)) == *VendorId))
        {
          *DeviceFound = 1;
          *Functions   = 0;
          if (InstanceCount > 1) {
            *DeviceFound = 0;
            InstanceCount--;
            continue;
          } else {
            *Bus        = BusIdx;
            *Device     = DevIdx;
            *Segment    = SegIdx;
            *Functions |= 1;
            for (FuncIdx = 1; FuncIdx < 8; FuncIdx++) {
              if ((PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS (SegIdx, BusIdx, DevIdx, FuncIdx, 2)) == *DeviceId) &&
                  (PciSegmentRead16 (PCI_SEGMENT_LIB_ADDRESS (SegIdx, BusIdx, DevIdx, FuncIdx, 0)) == *VendorId))
              {
                *Functions |= (1 << FuncIdx);
              }
            }

            return EFI_SUCCESS;
          }
        }
      }

      if (BusIdx == 255) {
        break;
      }
    }
  }

  return EFI_SUCCESS;
}
