/** @file
  AMD SMBIOS Type 42 Record.

  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "SmbiosCommon.h"

typedef enum {
  HostInterfaceTypeReserved0 = 0x00,
  HostInterfaceTypeReserved1 = 0x01,
  HostInterfaceTypeKCS       = 0x02,
  HostInterfaceType8250      = 0x03,
  HostInterfaceType16450     = 0x04,
  HostInterfaceType16550     = 0x05,
  HostInterfaceType16650     = 0x06,
  HostInterfaceType16750     = 0x07,
  HostInterfaceType16850     = 0x08
} HOST_INTERFACE_TYPE;

typedef struct {
  UINT8     ProtocolType;
  UINT8     ProtocolTypeSpecificDataLength;
  UINT32    ProtocolTypeSpecificData;
} SMBIOS_TYPE42_PROTOCOL_RECORD_DATA_FORMAT;

typedef struct {
  SMBIOS_TABLE_TYPE42                          SmbiosType42;
  UINT8                                        NumberOfProtocolRecords;
  SMBIOS_TYPE42_PROTOCOL_RECORD_DATA_FORMAT    ProtocolRecords[1];
} SMBIOS_TABLE_TYPE42_STRUCTURE;

/**
  Management Controller Host Interface (Type 42).

  @param[in]  Smbios  The EFI_SMBIOS_PROTOCOL protocol instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
**/
EFI_STATUS
EFIAPI
HostInterface (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                     Status;
  EFI_SMBIOS_HANDLE              SmbiosHandle;
  SMBIOS_TABLE_TYPE42_STRUCTURE  *SmbiosRecord;

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE42_STRUCTURE) + 1 + 1);
  if (SmbiosRecord == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  SmbiosRecord->SmbiosType42.Hdr.Type   = EFI_SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE;
  SmbiosRecord->SmbiosType42.Hdr.Length = sizeof (SMBIOS_TABLE_TYPE42_STRUCTURE);
  SmbiosRecord->SmbiosType42.Hdr.Handle = 0xFF;

  switch (FixedPcdGet8 (PcdIpmiInterfaceType)) {
    case IPMIDeviceInfoInterfaceTypeKCS:
      SmbiosRecord->SmbiosType42.InterfaceType = HostInterfaceTypeKCS;
      break;
    default:
      SmbiosRecord->SmbiosType42.InterfaceType = MCHostInterfaceTypeOemDefined;
      break;
  }

  SmbiosRecord->SmbiosType42.InterfaceTypeSpecificDataLength      = sizeof (SmbiosRecord->SmbiosType42.InterfaceTypeSpecificData);
  SmbiosRecord->SmbiosType42.InterfaceTypeSpecificData[0]         = 0xFF;
  SmbiosRecord->NumberOfProtocolRecords                           = 0x01;
  SmbiosRecord->ProtocolRecords[0].ProtocolType                   = MCHostInterfaceProtocolTypeIPMI;
  SmbiosRecord->ProtocolRecords[0].ProtocolTypeSpecificDataLength = sizeof (SmbiosRecord->ProtocolRecords[0].ProtocolTypeSpecificData);
  SmbiosRecord->ProtocolRecords[0].ProtocolTypeSpecificData       = 0xFF;

  //
  // Now we have got the full smbios record,
  // call smbios protocol to add this record.
  //
  Status = AddCommonSmbiosRecord (
             Smbios,
             &SmbiosHandle,
             (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
             );
  FreePool (SmbiosRecord);

  return Status;
}
