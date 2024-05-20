/** @file
  AMD SMBIOS Type 38 Record

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "SmbiosCommon.h"

/**
  This function updates IPMI Device information changes to the contents of the
  Table Type 38.

  @param[in]  Smbios  The EFI_SMBIOS_PROTOCOL protocol instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
**/
EFI_STATUS
EFIAPI
IpmiDeviceInformation (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  SMBIOS_TABLE_TYPE38  *SmbiosRecord;

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE38) + 1 + 1);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_IPMI_DEVICE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE38);
  SmbiosRecord->Hdr.Handle = 0;

  switch (FixedPcdGet8 (PcdIpmiInterfaceType)) {
    case IPMIDeviceInfoInterfaceTypeKCS:
      SmbiosRecord->InterfaceType             = IPMIDeviceInfoInterfaceTypeKCS;
      SmbiosRecord->IPMISpecificationRevision = 0x20; // IPMI v2.0
      SmbiosRecord->I2CSlaveAddress           = 0x00; // not used in KCS interface
      SmbiosRecord->NVStorageDeviceAddress    = 0xFF;
      // KCS port number base and set LSB bit 1 to mark IO ADDRESS space
      SmbiosRecord->BaseAddress                       = FixedPcdGet16 (PcdIpmiKcsIoBaseAddress) | 0x1;
      SmbiosRecord->BaseAddressModifier_InterruptInfo = 0x00;
      SmbiosRecord->InterruptNumber                   = 0x00;
      //
      // Now we have got the full smbios record,
      // call smbios protocol to add this record.
      //
      Status = AddCommonSmbiosRecord (
                 Smbios,
                 &SmbiosHandle,
                 (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
                 );
      break;
    default:
      // Do not add table
      Status = EFI_UNSUPPORTED;
      break;
  }

  FreePool (SmbiosRecord);
  return Status;
}
