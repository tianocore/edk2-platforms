/** @file

  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <MorelloPlatform.h>

// The total number of descriptors, including the final "end-of-table" descriptor.
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  16

STATIC CONST CHAR8  *gTblAttrDesc[] = {
  "UNCACHED_UNBUFFERED          ",
  "NONSECURE_UNCACHED_UNBUFFERED",
  "WRITE_BACK                   ",
  "NONSECURE_WRITE_BACK         ",
  "WB_NONSHAREABLE              ",
  "NONSECURE_WB_NONSHAREABLE    ",
  "WRITE_THROUGH                ",
  "NONSECURE_WRITE_THROUGH      ",
  "DEVICE                       ",
  "NONSECURE_DEVICE             "
};

#define LOG_MEM(desc)  DEBUG ((                                             \
                        DEBUG_ERROR,                                        \
                        desc,                                               \
                        VirtualMemoryTable[Index].PhysicalBase,             \
                        (VirtualMemoryTable[Index].PhysicalBase +           \
                         VirtualMemoryTable[Index].Length - 1),             \
                        VirtualMemoryTable[Index].Length,                   \
                        gTblAttrDesc[VirtualMemoryTable[Index].Attributes]  \
                        ));

/**
  Returns the Virtual Memory Map of the platform.

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on your platform.

  @param[out] VirtualMemoryMap Array of ARM_MEMORY_REGION_DESCRIPTOR describing
                               a Physical-to-Virtual Memory mapping. This array
                               must be ended by a zero-filled entry.
**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  CONST MORELLO_PLAT_INFO_SOC   *PlatInfo;
  EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttributes;
  EFI_STATUS                    Status;
  UINT64                        DramBlock2Size;
  UINTN                         Index;

  Status = PeiServicesLocatePpi (
             &gArmMorelloSocPlatformInfoDescriptorPpiGuid,
             0,
             NULL,
             (VOID **)&PlatInfo
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]: failed to locate gArmMorelloSocPlatformInfoDescriptorPpiGuid - %r\n",
      gEfiCallerBaseName,
      Status
      ));
    return;
  }

  Index          = 0;
  DramBlock2Size = 0;

  if (PlatInfo->LocalDdrSize > MORELLO_DRAM_BLOCK1_SIZE) {
    DramBlock2Size = PlatInfo->LocalDdrSize - MORELLO_DRAM_BLOCK1_SIZE;
  }

  if (DramBlock2Size != 0) {
    ResourceAttributes =
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED;

    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      ResourceAttributes,
      FixedPcdGet64 (PcdDramBlock2Base),
      DramBlock2Size
      );
  }

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePool (
                         sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );
  if (VirtualMemoryTable == NULL) {
    return;
  }

  DEBUG ((
    DEBUG_ERROR,
    " Memory Map\n----------------------------------------------------------\n"
    ));
  DEBUG ((
    DEBUG_ERROR,
    "Description                     :        START       -        END         " \
    "[        SIZE        ] {              ATTR             }\n"
    ));

  // SubSystem Peripherals - Generic Watchdog
  VirtualMemoryTable[Index].PhysicalBase = MORELLO_GENERIC_WDOG_BASE;
  VirtualMemoryTable[Index].VirtualBase  = MORELLO_GENERIC_WDOG_BASE;
  VirtualMemoryTable[Index].Length       = MORELLO_GENERIC_WDOG_SZ;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("Generic Watchdog                : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // SubSystem Peripherals - GIC-600
  VirtualMemoryTable[++Index].PhysicalBase = MORELLO_GIC_BASE;
  VirtualMemoryTable[Index].VirtualBase    = MORELLO_GIC_BASE;
  VirtualMemoryTable[Index].Length         = MORELLO_GIC_SZ;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("GIC-600                         : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // SubSystem Peripherals - GICR-600
  VirtualMemoryTable[++Index].PhysicalBase = MORELLO_GICR_BASE;
  VirtualMemoryTable[Index].VirtualBase    = MORELLO_GICR_BASE;
  VirtualMemoryTable[Index].Length         = MORELLO_GICR_SZ;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("GICR-600                        : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // SubSystem non-secure SRAM
  VirtualMemoryTable[++Index].PhysicalBase = MORELLO_NON_SECURE_SRAM_BASE;
  VirtualMemoryTable[Index].VirtualBase    = MORELLO_NON_SECURE_SRAM_BASE;
  VirtualMemoryTable[Index].Length         = MORELLO_NON_SECURE_SRAM_SZ;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
  LOG_MEM ("non-secure SRAM                 : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // SubSystem Pheripherals - UART0
  VirtualMemoryTable[++Index].PhysicalBase = MORELLO_UART0_BASE;
  VirtualMemoryTable[Index].VirtualBase    = MORELLO_UART0_BASE;
  VirtualMemoryTable[Index].Length         = MORELLO_UART0_SZ;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("UART0                           : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // DDR Primary
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].Length         = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  LOG_MEM ("DDR Primary                     : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // DDR Secondary
  if (DramBlock2Size != 0) {
    VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdDramBlock2Base);
    VirtualMemoryTable[Index].VirtualBase    = PcdGet64 (PcdDramBlock2Base);
    VirtualMemoryTable[Index].Length         = DramBlock2Size;
    VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
    LOG_MEM ("DDR Secondary                   : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");
  }

  // AP QSPI flash device
  VirtualMemoryTable[++Index].PhysicalBase = MORELLO_AP_QSPI_AHB_BASE;
  VirtualMemoryTable[Index].VirtualBase    = MORELLO_AP_QSPI_AHB_BASE;
  VirtualMemoryTable[Index].Length         = MORELLO_AP_QSPI_AHB_SZ;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("AP QSPI flash device            : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // Expansion Peripherals
  VirtualMemoryTable[++Index].PhysicalBase = MORELLO_AXI_EXPANSION_PERIPHERAL_BASE;
  VirtualMemoryTable[Index].VirtualBase    = MORELLO_AXI_EXPANSION_PERIPHERAL_BASE;
  VirtualMemoryTable[Index].Length         = MORELLO_AXI_EXPANSION_PERIPHERAL_SZ;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("Expansion Peripherals           : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // PCIe ECAM Configuration Space
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdPciExpressBaseAddress);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet64 (PcdPciExpressBaseAddress);
  VirtualMemoryTable[Index].Length         = (FixedPcdGet32 (PcdPciBusMax) -
                                              FixedPcdGet32 (PcdPciBusMin) + 1) *
                                             SIZE_1MB;
  VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("PCIe ECAM Region                : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // PCIe MMIO32 Memory Space
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet32 (PcdPciMmio32Base);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet32 (PcdPciMmio32Base);
  VirtualMemoryTable[Index].Length         = (PcdGet32 (PcdPciMmio32Size) +
                                              FixedPcdGet32 (PcdPciIoSize));
  VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("PCIe MMIO32 & IO Region         : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // PCIe MMIO64 Memory Space
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdPciMmio64Base);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet64 (PcdPciMmio64Base);
  VirtualMemoryTable[Index].Length         = PcdGet64 (PcdPciMmio64Size);
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("PCIe MMIO64 Region              : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // CCIX ECAM Configuration Space
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdCcixExpressBaseAddress);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet64 (PcdCcixExpressBaseAddress);
  VirtualMemoryTable[Index].Length         = (FixedPcdGet32 (PcdCcixBusMax) -
                                              FixedPcdGet32 (PcdCcixBusMin) + 1) *
                                             SIZE_1MB;
  VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("CCIX ECAM Region                : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // CCIX MMIO32 Memory Space
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet32 (PcdCcixMmio32Base);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet32 (PcdCcixMmio32Base);
  VirtualMemoryTable[Index].Length         = (PcdGet32 (PcdCcixMmio32Size) +
                                              PcdGet32 (PcdCcixIoSize));
  VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("CCIX MMIO32 & IO Region         : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // CCIX MMIO64 Memory Space
  VirtualMemoryTable[++Index].PhysicalBase = PcdGet64 (PcdCcixMmio64Base);
  VirtualMemoryTable[Index].VirtualBase    = PcdGet64 (PcdCcixMmio64Base);
  VirtualMemoryTable[Index].Length         = PcdGet64 (PcdCcixMmio64Size);
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  LOG_MEM ("CCIX MMIO64 Region              : 0x%016lx - 0x%016lx [ 0x%016lx ] { %a }\n");

  // End of Table
  VirtualMemoryTable[++Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase    = 0;
  VirtualMemoryTable[Index].Length         = 0;
  VirtualMemoryTable[Index].Attributes     = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT ((Index) < MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);
  DEBUG ((DEBUG_INIT, "Virtual Memory Table setup complete.\n"));

  *VirtualMemoryMap = VirtualMemoryTable;
}
