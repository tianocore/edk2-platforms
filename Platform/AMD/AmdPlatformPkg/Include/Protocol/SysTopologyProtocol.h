/*****************************************************************************
 *
 * Copyright (C) 2016-2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 *******************************************************************************
 */

#pragma once

// 9af2c1de-115a-49f0-a7c1-276c21a44e53
#define EFI_SYS_TOPOLOGY_PROTOCOL_GUID \
  { \
    0x9af2c1de, 0x115a, 0x49f0, {0xa7, 0xc1, 0x27, 0x6c, 0x21, 0xa4, 0x4e, 0x53} \
  }

typedef struct _SYS_TOPOLOGY_HEADER {
  UINT32    Signature;
  UINT8     CheckSum;
  UINT8     Version;
  UINT32    ReportSize;
  UINT16    EntryCount;
  UINT8     Reserved[4];
} SYS_TOPOLOGY_HEADER;

typedef struct _SYS_TOPOLOGY_ENTRY_HEADER {
  UINT8     Type;
  UINT8     Version;
  UINT16    Length;
} SYS_TOPOLOGY_ENTRY_HEADER;

// *******************************************************
// Entry Structure Definitions
// *******************************************************

typedef struct _SYS_TOPOLOGY_PCIE {
  UINT8     Type;
  UINT8     Version;
  UINT16    Length;
  UINT8     Class;
  UINT8     Subclass;
  UINT8     ProgIf;
  UINT8     DeviceType;
  UINT16    Vid;
  UINT16    Did;
  UINT16    Status;
  UINT8     Segment;
  UINT8     Bus;
  UINT8     Device;
  UINT8     Function;
  UINT8     PcieGen;
  UINT8     MaxPcieGen;
  UINT8     LanesUsed;
  UINT8     MaxLanes;
  UINT32    SerialNumberUpper;
  UINT32    SerialNumberLower;
  UINT8     Reserved[2];
} SYS_TOPOLOGY_PCIE;

typedef enum {
  SATA_STORAGE,
  NVME_STORAGE
} MEDIA_TYPE;

typedef struct _SYS_TOPOLOGY_STORAGE {
  UINT8     Type;
  UINT8     Version;
  UINT16    Length;
  UINT64    CapacityBytes;
  UINT32    BlockSizeBytes;
  UINT8     MediaType;
  UINT16    ModelName;
  UINT16    SerialNumber;
  UINT16    FirmwareVersion;
  UINT8     Reserved[11];
  CHAR8     Strings[1];
} SYS_TOPOLOGY_STORAGE;

typedef struct {
  SYS_TOPOLOGY_ENTRY_HEADER    Header;
  UINT32                       Size;          // Redfish CapacityMiB:3200
  UINT32                       Speed;         // Redfish OperatingSpeedMhz
  CHAR8                        *DeviceLocator;
  CHAR8                        *BankLocator;
  CHAR8                        *Manufacturer;
  CHAR8                        *SerialNumber;
  CHAR8                        *PartNumber;
  CHAR8                        *BaseModuleType;
  CHAR8                        *MemoryType;   // Redfish MemoryDeviceType:DDR5
  CHAR8                        *MemoryDeviceType;
  CHAR8                        *FirmwareVersion;
  UINT32                       DataWidth;
  UINT32                       BusWidth;
  UINT32                       RankCount;
  CHAR8                        *ErrorCorrectionType;
  CHAR8                        *Pmic0ManufacturerId;
  CHAR8                        *RcdManufacturerId;
} SYS_TOPOLOGY_MEMORY;

typedef struct {
  SYS_TOPOLOGY_ENTRY_HEADER    Header;
  CHAR8                        *BiosVersion;
} SYS_TOPOLOGY_BIOS;

// *******************************************************
// Context structures to contain dynamic length entries
// *******************************************************

// Generic resizable entry context
typedef struct _SYS_TOPOLOGY_ENTRY_CONTEXT {
  UINT32                       MemoryAllocated;
  SYS_TOPOLOGY_ENTRY_HEADER    *Header;
} SYS_TOPOLOGY_ENTRY_CONTEXT;

// Storage context
typedef struct _SYS_TOPOLOGY_STORAGE_CONTEXT {
  UINT32                  MemoryAllocated;
  SYS_TOPOLOGY_STORAGE    *Storage;
} SYS_TOPOLOGY_STORAGE_CONTEXT;

typedef struct _SYS_TOPOLOGY_REPORT_CONTEXT {
  UINT32                 MemoryAllocated;
  SYS_TOPOLOGY_HEADER    *ReportHeader;
} SYS_TOPOLOGY_REPORT_CONTEXT;

typedef struct _EFI_SYS_TOPOLOGY_PROTOCOL EFI_SYS_TOPOLOGY_PROTOCOL;

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_CREATE_REPORT)(
  IN CONST EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT **Report
  );

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_FREE_REPORT)(
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT **Report
  );

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_PCIE_DEVICES)(
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT *Report
  );

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_STORAGE_DEVICES)(
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT *Report
  );

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_CXL_DEVICES)(
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT *Report
  );

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_MEMORY_DEVICES)(
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT *Report
  );

typedef
  EFI_STATUS
(EFIAPI *EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_BIOS_Version)(
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT *Report
  );

struct _EFI_SYS_TOPOLOGY_PROTOCOL {
  EFI_SYS_TOPOLOGY_PROTOCOL_CREATE_REPORT              CreateReport;
  EFI_SYS_TOPOLOGY_PROTOCOL_FREE_REPORT                FreeReport;
  EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_PCIE_DEVICES       CollectPcieDevices;
  EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_STORAGE_DEVICES    CollectStorageDevices;
  EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_CXL_DEVICES        CollectCxlDevices;
  EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_MEMORY_DEVICES     CollectMemoryDevices;
  EFI_SYS_TOPOLOGY_PROTOCOL_COLLECT_BIOS_Version       CollectBiosVersion;
};

extern EFI_GUID  gEfiSysTopologyProtocolGuid;
