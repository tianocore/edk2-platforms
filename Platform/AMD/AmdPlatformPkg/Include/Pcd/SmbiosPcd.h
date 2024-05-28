/** @file
  Miscellaneous smbios data structures.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AMD_SMBIOS_PCD_H_
#define AMD_SMBIOS_PCD_H_

#include <IndustryStandard/SmBios.h>
#include <Uefi.h>

#define AMD_SMBIOS_TYPE8_MAX_PORT_CONNETORS    16
#define AMD_SMBIOS_TYPE41_MAX_ONBOARD_DEVICES  16

typedef struct {
  CHAR8    IntDesignatorStr[SMBIOS_STRING_MAX_LENGTH];
  CHAR8    ExtDesignatorStr[SMBIOS_STRING_MAX_LENGTH];
} PORT_CONNECTOR_STR;

//
// AMD SMBIOS type 8 record structure.
//
typedef struct {
  SMBIOS_TABLE_TYPE8    Type8Data;
  PORT_CONNECTOR_STR    DesinatorStr;
} SMBIOS_PORT_CONNECTOR_RECORD;

//
// AMD SMBIOS type 8 record structure array.
//
typedef struct {
  SMBIOS_PORT_CONNECTOR_RECORD    SmbiosPortConnectorRecords[AMD_SMBIOS_TYPE8_MAX_PORT_CONNETORS];
} SMBIOS_PORT_CONNECTOR_RECORD_ARRAY;

//
// AMD SMBIOS type 41 record structure
//
typedef struct {
  SMBIOS_TABLE_STRING    ReferenceDesignation;
  UINT8                  DeviceType;
  UINT8                  DeviceEnabled;
  UINT8                  DeviceTypeInstance;
  UINT16                 VendorId;
  UINT16                 DeviceId;
  CHAR8                  RefDesignationStr[SMBIOS_STRING_MAX_LENGTH];
} SMBIOS_ONBOARD_DEV_EXT_INFO_RECORD;

//
// AMD SMBIOS type 41 record structure array.
//
typedef struct {
  SMBIOS_ONBOARD_DEV_EXT_INFO_RECORD    SmbiosOnboardDevExtInfos[AMD_SMBIOS_TYPE41_MAX_ONBOARD_DEVICES];
} SMBIOS_ONBOARD_DEV_EXT_INFO_ARRAY;

#endif // AMD_SMBIOS_PCD_H_
