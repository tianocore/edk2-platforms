/** @file
  Shared declarations for the Marvell CN10K SMBIOS platform driver.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PLATFORM_DXE_H_
#define SMBIOS_PLATFORM_DXE_H_

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Library/PcdLib.h>

//
// Number of DIMMs that can be attached to a single node. Each DSS drives one
// DIMM, so this is the same as the number of DSSs.
//
#define CN10K_SMBIOS_MAX_DIMMS  FixedPcdGet8 (PcdDssPerNode)

//
// SPD byte 2 (Key Byte / DRAM Device Type) values handled by this driver.
//
#define CN10K_SPD_MEMORY_TYPE_DDR4  0x0C
#define CN10K_SPD_MEMORY_TYPE_DDR5  0x12

//
// Product IDs reported in the SMMU IIDR register.
//
typedef enum {
  Cn10kaProductId  = 0xB9,
  Cnf10kaProductId = 0xBA,
  Cnf10kbProductId = 0xBC,
  Cn10kbProductId  = 0xBD
} CN10K_PRODUCT_ID;

typedef enum {
  DimmStatusNotPresent,
  DimmStatusEmpty,
  DimmStatusAvailable
} DIMM_STATUS;

typedef struct _DIMM_CFG {
  DIMM_STATUS    Status;
  UINT8          Type;
  UINT8          SubType;
  UINT8          Rank;
  UINT16         Mfg;
  UINTN          SizeMb;
  UINTN          Speed;
  UINT16         TotalWidth;
  UINT8          DataWidth;
  CHAR8          Serial[9];  // DDR4/DDR5: Serial is build out of 4 2 digit numbers -> 9 (inc NULL)
  CHAR8          PartNo[31]; // 30 bytes + NULL for DDR5. Might be extended for new types of memory
} DIMM_CFG;

typedef struct _NODE_CFG {
  DIMM_CFG    DimmCfg[CN10K_SMBIOS_MAX_DIMMS];
  UINT64      RamStart;
  UINT64      RamSize;
  UINT64      CPUSpeed;
  UINT8       CPUType;
  CHAR8       CPUPartNumber[32]; // e.g. MV-CN10670A0-CF450AA-AAP-ES
  CHAR8       CPUVersion[4];     // e.g. A0
  CHAR8       CPUSerial[32];
} NODE_CFG;

//
// State shared between the driver entry point and the per-type installers.
//
typedef struct _CN10K_SMBIOS_CONTEXT {
  NODE_CFG             *NodeConfig;
  UINT64               DdrSpeed;
  CHAR8                ProductName[32];
  CHAR8                SystemSerialNumber[32];
  CHAR8                SystemRevision[32];
  CHAR8                BiosVer[64];
  CHAR8                ReleaseDate[11];
  UINT64               LlcSizeInBytes;
  EFI_SMBIOS_HANDLE    ChassisHandle;
  EFI_SMBIOS_HANDLE    PmaHandle;
  EFI_SMBIOS_HANDLE    MamaHandle;
  EFI_SMBIOS_HANDLE    L1DHandle;
  EFI_SMBIOS_HANDLE    L1IHandle;
  EFI_SMBIOS_HANDLE    L2Handle;
  EFI_SMBIOS_HANDLE    L3Handle;
  EFI_SMBIOS_HANDLE    DimmHandles[CN10K_SMBIOS_MAX_DIMMS];
} CN10K_SMBIOS_CONTEXT;

#define SMBIOS_SET_STRING(Strings, Template, Field, Value)                    \
  (Strings)[(Template)->Field - 1] =                                          \
    ((Value) != NULL && *(Value) != '\0') ? (Value) : "Unknown"

/**
  Return whether the given product ID is a supported CN10K/CNF10K variant.

  @param[in]  CpuType  Product ID read from the SMMU IIDR register.

  @retval TRUE   The product ID is supported.
  @retval FALSE  The product ID is not supported.
**/
BOOLEAN
CpuTypeIsSupported (
  IN UINT8  CpuType
  );

/**
  Add an SMBIOS record and optionally return the handle assigned to it.

  @param[in]  Smbios       SMBIOS protocol instance.
  @param[in]  Entry        Fixed SMBIOS structure to add.
  @param[in]  StringArray  NULL terminated string array, may be NULL.
  @param[out] Handle       If not NULL, set to the handle of the added record.

  @retval EFI_SUCCESS      The record was added.
  @retval Others           The record could not be added.
**/
EFI_STATUS
SmbiosPlatformAddEntry (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios,
  IN  SMBIOS_STRUCTURE     *Entry,
  IN  CHAR8                **StringArray  OPTIONAL,
  OUT EFI_SMBIOS_HANDLE    *Handle        OPTIONAL
  );

/**
  Set all cache size fields of a Type 7 record from a size in bytes.

  @param[in,out] Cache        Type 7 record to update.
  @param[in]     SizeInBytes  Cache size in bytes.
**/
VOID
SmbiosSetCacheSize (
  IN OUT SMBIOS_TABLE_TYPE7  *Cache,
  IN     UINT64              SizeInBytes
  );

/**
  Read the LLC size from the device tree.

  @param[out] CacheSize  Set to the L3 cache size in bytes.

  @retval EFI_SUCCESS    The cache size was found.
  @retval Others         The device tree could not be queried.
**/
EFI_STATUS
GetLlcSizeOutOfDeviceTree (
  OUT UINT64  *CacheSize
  );

EFI_STATUS
InstallType0BiosInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType1SystemInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType2BaseboardInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType3SystemEnclosure (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType4ProcessorInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType7CacheInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType8PortConnectorInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType9SystemSlots (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType11OemStrings (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType12SystemConfigurationOptions (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType16PhysicalMemoryArray (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType17MemoryDevice (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType19MemoryArrayMappedAddress (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType20MemoryDeviceMappedAddress (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType23SystemReset (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

EFI_STATUS
InstallType32SystemBootInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  );

#endif // SMBIOS_PLATFORM_DXE_H_
