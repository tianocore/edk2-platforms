/** @file
  Implements CommonHeader.h

  Copyright (c) 2013 - 2016 Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef COMMON_HEADER_H_
#define COMMON_HEADER_H_

#include <PiPei.h>
#include <Ppi/Stall.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/Capsule.h>
#include <Library/IoLib.h>
#include <Guid/DebugMask.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
// #include <Fch.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PciExpressLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MtrrLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/SmramMemoryReserve.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/AcpiS3Context.h>
#include "MemoryInstall.h"

#define   B_SLP_TYPE                        (BIT10 + BIT11 + BIT12)
#define     V_SLP_TYPE_S0                   (0 << 10)
#define     V_SLP_TYPE_S1                   (1 << 10)
#define     V_SLP_TYPE_S3                   (3 << 10)
#define     V_SLP_TYPE_S4                   (4 << 10)
#define     V_SLP_TYPE_S5                   (5 << 10)
#define   B_ACPI_SLP_EN                     BIT13
#define     V_ACPI_SLP_EN                   BIT13
#define   SPI_BASE                          0xFEC10000ul
#define   EFI_CPUID_EXTENDED_FUNCTION       0x80000000
#define   EFI_CPUID_VIRT_PHYS_ADDRESS_SIZE  0x80000008

#define ACPI_MMIO_BASE         0xFED80000ul
#define SMI_BASE               0x200                     // DWORD
#define FCH_PMIOA_REG60        0x60                      // AcpiPm1EvtBlk
#define FCH_PMIOA_REG62        0x62                      // AcpiPm1CntBlk
#define FCH_PMIOA_REG64        0x64                      // AcpiPmTmrBlk
#define PMIO_BASE              0x300                     // DWORD
#define FCH_SMI_REGA0          0xA0
#define FCH_SMI_REGC4          0xC4
#define R_FCH_ACPI_PM_CONTROL  0x04

EFI_STATUS
GetAvailableMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges,
  IN VOID                                       *FspHobList
  );

EFI_STATUS
GetReservedMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges,
  IN VOID                                       *FspHobList
  );

EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT64                  *LowMemoryLength,
  OUT UINT64                  *HighMemoryLength,
  OUT UINT64                  *GraphicMemoryBase OPTIONAL,
  OUT UINT64                  *GraphicMemoryLength OPTIONAL,
  IN VOID                     *FspHobList
  );

EFI_STATUS
EFIAPI
SetPeiCacheMode (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  VOID                    *FspHobList
  );

#endif
