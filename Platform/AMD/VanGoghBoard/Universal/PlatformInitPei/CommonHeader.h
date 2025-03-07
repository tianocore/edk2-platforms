/** @file
  Implements CommonHeader.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2016，Intel Corporation. All rights reserved.<BR>
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
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PciExpressLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MtrrLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/SmramMemoryReserve.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/AmdMemoryInfoHob.h>
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

/**

  This function set different memory range cache attribute.

  @param  PeiServices         Pointer to the PEI Service Table.

**/
EFI_STATUS
EFIAPI
SetPeiCacheMode (
  IN  CONST EFI_PEI_SERVICES  **PeiServices
  );

/**

  Waits for at least the given number of microseconds.

  @param PeiServices     General purpose services available to every PEIM.
  @param This            PPI instance structure.
  @param Microseconds    Desired length of time to wait.

  @retval EFI_SUCCESS    If the desired amount of time was passed.

*/
EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI  *This,
  IN UINTN                    Microseconds
  );

/**
  Peform the boot mode determination logic

  @param  PeiServices General purpose services available to every PEIM.
  @param  BootMode The detected boot mode.

  @retval EFI_SUCCESS if the boot mode could be set
**/
EFI_STATUS
EFIAPI
UpdateBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN       EFI_BOOT_MODE     *BootMode
  );

/**

  This function returns the different avaiable memory length.

  @param  PeiServices         Pointer to the PEI Service Table.
  @param  LowMemoryLength     Avaiable memory length below 4G address.
  @param  HighMemoryLength    Avaiable memory length above 4G address.
  @param  GraphicMemoryBase   Avaiable UMA base address.
  @param  GraphicMemoryLength Avaiable UMA length.

  @retval  LowMemoryLength     Avaiable memory length below 4G address.
  @retval  HighMemoryLength    Avaiable memory length above 4G address.
  @retval  GraphicMemoryBase   Avaiable UMA base address.
  @retval  GraphicMemoryLength Avaiable UMA length.

**/
EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT64                  *LowMemoryLength,
  OUT UINT64                  *HighMemoryLength,
  OUT UINT64                  *GraphicMemoryBase OPTIONAL,
  OUT UINT64                  *GraphicMemoryLength OPTIONAL
  );

/**

  This function returns the memory ranges to be enabled, along with information
  describing how the range should be used.

  @param  MemoryMap     Buffer to record details of the memory ranges.
  @param  NumRanges     On input, this contains the maximum number of memory ranges that can be described
                        in the MemoryMap buffer.

  @retval MemoryMap     The buffer will be filled in
  @retval NumRanges     will contain the actual number of memory ranges that are to be anabled.
  @retval EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
GetAvailableMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges
  );

/**

  This function returns the memory ranges to be reserved, along with information
  describing how the range should be used.

  @param  MemoryMap     Buffer to record details of the memory ranges.
  @param  NumRanges     On input, this contains the maximum number of memory ranges that can be described
                        in the MemoryMap buffer.

  @retval MemoryMap     The buffer will be filled in
  @retval NumRanges     will contain the actual number of memory ranges that are to be reserved.
  @retval EFI_SUCCESS   The function completed successfully.

**/
EFI_STATUS
GetReservedMemoryRanges (
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE  *MemoryMap,
  IN OUT UINT8                                  *NumRanges
  );

/**

  Callback function after Memory discovered.

  @param  PeiServices       PEI Services table.
  @param  NotifyDescriptor  Notify Descriptor.
  @param  Ppi               Ppi

  @return EFI_STATUS        If the function completed successfully.

**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**

  Callback function after Memory Info Hob Installed.

  @param  PeiServices       PEI Services table.
  @param  NotifyDescriptor  Notify Descriptor.
  @param  Ppi               Ppi

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
MemoryInfoHobPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Get sleep type after wakeup

  @param PeiServices       Pointer to the PEI Service Table.
  @param SleepType         Sleep type to be returned.

  @retval TRUE              A wake event occured without power failure.
  @retval FALSE             Power failure occured or not a wakeup.

**/
BOOLEAN
GetSleepTypeAfterWakeup (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  OUT UINT16                  *SleepType
  );

#endif
