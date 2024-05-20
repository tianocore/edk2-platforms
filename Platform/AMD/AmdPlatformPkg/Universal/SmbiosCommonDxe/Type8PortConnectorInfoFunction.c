/** @file
  AMD SMBIOS Type 8 Record

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Pcd/SmbiosPcd.h>
#include "SmbiosCommon.h"

/**
  This function adds port connector information smbios record (Type 8).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
PortConnectorInfoFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  )
{
  EFI_STATUS                    Status;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  SMBIOS_TABLE_TYPE8            *SmbiosRecord;
  SMBIOS_PORT_CONNECTOR_RECORD  *PortConnRecord;
  UINT8                         PortIdx;
  UINT8                         NumberOfPortConnector;
  UINTN                         StringOffset;
  CHAR8                         *IntPortConDesStr;
  UINTN                         IntPortConDesStrLen;
  CHAR8                         *ExtPortConDesStr;
  UINTN                         ExtPortConDesStrLen;

  if (Smbios == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get the total number of port connectors.
  NumberOfPortConnector = PcdGet8 (PcdAmdSmbiosType8Number);
  DEBUG ((DEBUG_INFO, "%a: Total number of AMD SMBIOS type8 PCD structure %d.\n", __func__, NumberOfPortConnector));
  PortConnRecord = (SMBIOS_PORT_CONNECTOR_RECORD *)PcdGetPtr (PcdAmdSmbiosType8);

  if (NumberOfPortConnector == 0) {
    DEBUG ((DEBUG_INFO, "No port connectors found.\n"));
    return EFI_NOT_FOUND;
  }

  // Generate type8 smbios record for each connector and add it to Smbios table.
  for (PortIdx = 0; PortIdx < NumberOfPortConnector; PortIdx++) {
    DEBUG ((DEBUG_MANAGEABILITY, "Port %d:\n", PortIdx));
    // Check whether Port connector designator strings are present or not.
    if (PortConnRecord->Type8Data.InternalReferenceDesignator != 0) {
      IntPortConDesStr    = PortConnRecord->DesinatorStr.IntDesignatorStr;
      IntPortConDesStrLen = AsciiStrLen (IntPortConDesStr) + 1;
      DEBUG ((DEBUG_MANAGEABILITY, "-- DesinatorStr.IntDesignatorStr = %a\n", IntPortConDesStr));
    } else {
      IntPortConDesStr    = NULL;
      IntPortConDesStrLen = 0;
    }

    if (PortConnRecord->Type8Data.ExternalReferenceDesignator != 0) {
      ExtPortConDesStr    = PortConnRecord->DesinatorStr.ExtDesignatorStr;
      ExtPortConDesStrLen = AsciiStrLen (ExtPortConDesStr) + 1;
      DEBUG ((DEBUG_MANAGEABILITY, "-- DesinatorStr.ExtDesignatorStr = %a\n", ExtPortConDesStr));
    } else {
      ExtPortConDesStr    = NULL;
      ExtPortConDesStrLen = 0;
    }

    SmbiosRecord = NULL;
    SmbiosRecord = AllocateZeroPool (
                     sizeof (SMBIOS_TABLE_TYPE8) + IntPortConDesStrLen + ExtPortConDesStrLen + 1
                     );

    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    } else {
      SmbiosRecord->Hdr.Type   = SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION;
      SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE8);
      SmbiosRecord->Hdr.Handle = 0;

      SmbiosRecord->InternalReferenceDesignator =
        PortConnRecord->Type8Data.InternalReferenceDesignator;
      SmbiosRecord->InternalConnectorType =
        PortConnRecord->Type8Data.InternalConnectorType;
      SmbiosRecord->ExternalReferenceDesignator =
        PortConnRecord->Type8Data.ExternalReferenceDesignator;
      SmbiosRecord->ExternalConnectorType =
        PortConnRecord->Type8Data.ExternalConnectorType;
      SmbiosRecord->PortType =
        PortConnRecord->Type8Data.PortType;
      DEBUG ((DEBUG_MANAGEABILITY, " - InternalReferenceDesignator = %d\n", SmbiosRecord->InternalReferenceDesignator));
      DEBUG ((DEBUG_MANAGEABILITY, " - InternalConnectorType = %d\n", SmbiosRecord->InternalConnectorType));
      DEBUG ((DEBUG_MANAGEABILITY, " - ExternalReferenceDesignator = %d\n", SmbiosRecord->ExternalReferenceDesignator));
      DEBUG ((DEBUG_MANAGEABILITY, " - ExternalConnectorType = %d\n", SmbiosRecord->ExternalConnectorType));
      DEBUG ((DEBUG_MANAGEABILITY, " - PortType = %d\n", SmbiosRecord->PortType));

      // Add strings to bottom of data block
      StringOffset = SmbiosRecord->Hdr.Length;
      if (IntPortConDesStr != NULL) {
        CopyMem (
          (UINT8 *)SmbiosRecord + StringOffset,
          IntPortConDesStr,
          IntPortConDesStrLen
          );
        StringOffset += IntPortConDesStrLen;
      }

      if (ExtPortConDesStr != NULL) {
        CopyMem (
          (UINT8 *)SmbiosRecord + StringOffset,
          ExtPortConDesStr,
          ExtPortConDesStrLen
          );
      }

      Status = AddCommonSmbiosRecord (
                 Smbios,
                 &SmbiosHandle,
                 (EFI_SMBIOS_TABLE_HEADER *)SmbiosRecord
                 );
      FreePool (SmbiosRecord);
    }

    PortConnRecord++;
  }

  return Status;
}
