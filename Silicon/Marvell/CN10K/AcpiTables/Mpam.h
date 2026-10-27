/** @file

  Header file for the ACPI Memory System Resource Partitioning And Monitoring (MPAM) tables.

  Copyright (c) 2023-2026 Marvell. All rights Reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <IndustryStandard/Acpi.h>

///
/// Location Types
///
#define EFI_ACPI_MPAM_LOCATION_TYPE_PROCESSOR_CACHE  0x00
#define EFI_ACPI_MPAM_LOCATION_TYPE_MEMORY           0x01
#define EFI_ACPI_MPAM_LOCATION_TYPE_SMMU             0x02
#define EFI_ACPI_MPAM_LOCATION_TYPE_MEMORY_CACHE     0x03
#define EFI_ACPI_MPAM_LOCATION_TYPE_ACPI_DEVICE      0x04

///
/// Interrupt Flags
///
#define EFI_ACPI_MPAM_LEVEL_TRIG_INTERRUPT_MODE  0x0
#define EFI_ACPI_MPAM_EDGE_TRIG_INTERRUPT_MODE   0x1
#define EFI_ACPI_MPAM_WIRED_INTERRUPT_TYPE       0x0

///
/// Interface Type
///
#define EFI_ACPI_MPAM_INTERFACE_TYPE_MMIO  0x00
#define EFI_ACPI_MPAM_INTERFACE_TYPE_PCC   0x0a

#pragma pack(1)

///
/// MPAM Revision (as defined in ACPI 6.4 spec.)
///
#define EFI_ACPI_6_4_MEMORY_SYSTEM_RESOURCE_PARTITIONING_MONITORING_TABLE_REVISION  0x01

///
/// "MPAM" Memory System Resource Partitioning And Monitoring Table
///
#define EFI_ACPI_6_4_MEMORY_SYSTEM_RESOURCE_PARTITIONING_MONITORING_TABLE_STRUCTURE_SIGNATURE  SIGNATURE_32('M', 'P', 'A', 'M')

///
/// Processor cache Locator descriptor
///
typedef struct {
  UINT64    CacheReference;
  UINT32    Reserved;
} EFI_ACPI_MPAM_RESOURCE_CACHE_LOCATOR;

///
/// Memory Locator descriptor
///
typedef struct {
  UINT64    ProximityDomain;
  UINT32    Reserved;
} EFI_ACPI_MPAM_RESOURCE_MEMORY_LOCATOR;

///
/// Locator structure that is used for determining where resource is situated.
///
typedef union {
  EFI_ACPI_MPAM_RESOURCE_CACHE_LOCATOR     CacheLocator;
  EFI_ACPI_MPAM_RESOURCE_MEMORY_LOCATOR    MemoryLocator;
} EFI_ACPI_MPAM_RESOURCE_LOCATOR;

///
/// Functional Dependency Structure
///
typedef struct {
  UINT32    Producer;
  UINT32    Reserved1;
} EFI_ACPI_6_4_MPAM_FUNC_DEP_NODE;

///
/// Resource Node Structure
///
typedef struct {
  UINT32                            Identifier;
  UINT8                             RisIndex;
  UINT16                            Reserved1;
  UINT8                             LocatorType;
  EFI_ACPI_MPAM_RESOURCE_LOCATOR    Locator;
  UINT32                            NumFuncDep;
} EFI_ACPI_6_4_MPAM_RESOURCE_NODE;

///
/// Memory System Controller Node Structure
///
typedef struct {
  UINT16    Length;
  UINT8     InterfaceType;
  UINT8     Reserved;
  UINT32    Identifier;
  UINT64    BaseAddress;
  UINT32    MmioSize;
  UINT32    OverflowInterrupt;
  UINT32    OverflowInterruptFlags;
  UINT32    Reserved1;
  UINT32    OverflowInterruptAff;
  UINT32    ErrorInterrupt;
  UINT32    ErrorInterruptFlags;
  UINT32    Reserved2;
  UINT32    ErrorInterruptAff;
  UINT32    MaxNRdyUsec;
  UINT64    LinkedDeviceHwId;
  UINT32    LinkedDeviceInstanceHwId;
  UINT32    NumResourceNodes;
} EFI_ACPI_6_4_MPAM_MSC_NODE;

//
/// Memory System Resource Partitioning and Monitoring Table (MPAM)
///
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
} EFI_ACPI_6_4_MEMORY_SYSTEM_RESOURCE_PARTITIONING_MONITORING_TABLE_HEADER;

#pragma pack()
