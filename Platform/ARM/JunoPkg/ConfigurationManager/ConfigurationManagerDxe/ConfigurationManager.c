/** @file
  Configuration Manager Dxe

  Copyright (c) 2017 - 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/DynamicTablesScmiInfoLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "ArmPlatform.h"
#include "ConfigurationManager.h"
#include "Platform.h"

/** The platform configuration repository information.
*/
STATIC
EDKII_PLATFORM_REPOSITORY_INFO ArmJunoPlatformRepositoryInfo = {
  /// Configuration Manager information
  { CONFIGURATION_MANAGER_REVISION, CFG_MGR_OEM_ID },

  // ACPI Table List
  {
    // FADT Table
    {
      EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
      NULL
    },
    // GTDT Table
    {
      EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdGtdt),
      NULL
    },
    // MADT Table
    {
      EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
      NULL
    },
    // SPCR Table
    {
      EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpcr),
      NULL
    },
    // DSDT Table
    {
      EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDsdt),
      (EFI_ACPI_DESCRIPTION_HEADER*)dsdt_aml_code
    },
    // DBG2 Table
    {
      EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE,
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
      NULL
    },
    // SSDT Table describing the Juno USB
    {
      EFI_ACPI_6_2_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdt),
      (EFI_ACPI_DESCRIPTION_HEADER*)ssdtjunousb_aml_code
    },
    // PPTT Table
    {
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdPptt),
      NULL
    },
    // SSDT Table (Cpu topology)
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCpuTopology),
      NULL,
      SIGNATURE_64 ('C','P','U','-','T','O','P','O')
    },
    /* PCI MCFG Table
       PCIe is only available on Juno R1 and R2.
       Add the PCI table entries at the end of the table so that
       we can easily disable PCIe for Juno R0
    */
    {
      EFI_ACPI_6_2_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
      NULL
    },
    // SSDT table describing the PCI root complex
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtPciExpress),
      NULL,
      SIGNATURE_64 ('S','S','D','T','-','P','C','I')
    },
  },

  // Boot architecture information
  { EFI_ACPI_6_2_ARM_PSCI_COMPLIANT },      // BootArchFlags

  // Power management profile information
  { EFI_ACPI_6_2_PM_PROFILE_MOBILE },       // PowerManagement Profile

  /* GIC CPU Interface information
     GIC_ENTRY (CPUInterfaceNumber, Mpidr, PmuIrq, VGicIrq, EnergyEfficiency)
  */
  {
    GICC_ENTRY (0, GET_MPID (0, 0), 34, 25, 1, REFERENCE_TOKEN (PsdInfo[PSD_BIG_DOMAIN_ID])),
    GICC_ENTRY (1, GET_MPID (0, 1), 38, 25, 1, REFERENCE_TOKEN (PsdInfo[PSD_BIG_DOMAIN_ID])),
    GICC_ENTRY (2, GET_MPID (1, 0), 50, 25, 0, REFERENCE_TOKEN (PsdInfo[PSD_LITTLE_DOMAIN_ID])),
    GICC_ENTRY (3, GET_MPID (1, 1), 54, 25, 0, REFERENCE_TOKEN (PsdInfo[PSD_LITTLE_DOMAIN_ID])),
    GICC_ENTRY (4, GET_MPID (1, 2), 58, 25, 0, REFERENCE_TOKEN (PsdInfo[PSD_LITTLE_DOMAIN_ID])),
    GICC_ENTRY (5, GET_MPID (1, 3), 62, 25, 0, REFERENCE_TOKEN (PsdInfo[PSD_LITTLE_DOMAIN_ID])),
  },

  // GIC Distributor Info
  {
    FixedPcdGet64 (PcdGicDistributorBase),  // UINT64  PhysicalBaseAddress
    0,                                      // UINT32  SystemVectorBase
    2                                       // UINT8   GicVersion
  },

  // Generic Timer Info
  {
    // The physical base address for the counter control frame
    JUNO_SYSTEM_TIMER_BASE_ADDRESS,
    // The physical base address for the counter read frame
    JUNO_CNT_READ_BASE_ADDRESS,
    // The secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerSecIntrNum),
    // The secure PL1 timer flags
    JUNO_GTDT_GTIMER_FLAGS,
    // The non-secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerIntrNum),
    // The non-secure PL1 timer flags
    JUNO_GTDT_GTIMER_FLAGS,
    // The virtual timer interrupt
    FixedPcdGet32 (PcdArmArchTimerVirtIntrNum),
    // The virtual timer flags
    JUNO_GTDT_GTIMER_FLAGS,
    // The non-secure PL2 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerHypIntrNum),
    // The non-secure PL2 timer flags
    JUNO_GTDT_GTIMER_FLAGS
  },

  // Generic Timer Block Information
  {
    {
      // The physical base address for the GT Block Timer structure
      JUNO_GT_BLOCK_CTL_BASE,
      // The number of timer frames implemented in the GT Block
      JUNO_TIMER_FRAMES_COUNT,
      // Reference token for the GT Block timer frame list
      (CM_OBJECT_TOKEN)((UINT8*)&ArmJunoPlatformRepositoryInfo +
        OFFSET_OF (EDKII_PLATFORM_REPOSITORY_INFO, GTBlock0TimerInfo))
    }
  },

  // GT Block Timer Frames
  {
    // Frame 0
    {
      0,                                 // UINT8   FrameNumber
      JUNO_GT_BLOCK_FRAME0_CTL_BASE,     // UINT64  PhysicalAddressCntBase
      JUNO_GT_BLOCK_FRAME0_CTL_EL0_BASE, // UINT64  PhysicalAddressCntEL0Base
      JUNO_GT_BLOCK_FRAME0_GSIV,         // UINT32  PhysicalTimerGSIV
      JUNO_GTX_TIMER_FLAGS,              // UINT32  PhysicalTimerFlags
      0,                                 // UINT32  VirtualTimerGSIV
      0,                                 // UINT32  VirtualTimerFlags
      JUNO_GTX_COMMON_FLAGS_S            // UINT32  CommonFlags
    },
    // Frame 1
    {
      1,                                 // UINT8   FrameNumber
      JUNO_GT_BLOCK_FRAME1_CTL_BASE,     // UINT64  PhysicalAddressCntBase
      JUNO_GT_BLOCK_FRAME1_CTL_EL0_BASE, // UINT64  PhysicalAddressCntEL0Base
      JUNO_GT_BLOCK_FRAME1_GSIV,         // UINT32  PhysicalTimerGSIV
      JUNO_GTX_TIMER_FLAGS,              // UINT32  PhysicalTimerFlags
      0,                                 // UINT32  VirtualTimerGSIV
      0,                                 // UINT32  VirtualTimerFlags
      JUNO_GTX_COMMON_FLAGS_NS           // UINT32  CommonFlags
    },
  },

  // Watchdog Info
  {
    // The physical base address of the SBSA Watchdog control frame
    FixedPcdGet64 (PcdGenericWatchdogControlBase),
    // The physical base address of the SBSA Watchdog refresh frame
    FixedPcdGet64 (PcdGenericWatchdogRefreshBase),
    // The watchdog interrupt
    FixedPcdGet32 (PcdGenericWatchdogEl2IntrNum),
    // The watchdog flags
    JUNO_SBSA_WATCHDOG_FLAGS
  },

  // SPCR Serial Port
  {
    FixedPcdGet64 (PcdSerialRegisterBase),            // BaseAddress
    FixedPcdGet32 (PL011UartInterrupt),               // Interrupt
    FixedPcdGet64 (PcdUartDefaultBaudRate),           // BaudRate
    FixedPcdGet32 (PL011UartClkInHz),                 // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART  // Port subtype
  },
  // Debug Serial Port
  {
    FixedPcdGet64 (PcdSerialDbgRegisterBase),         // BaseAddress
    38,                                               // Interrupt
    FixedPcdGet64 (PcdSerialDbgUartBaudRate),         // BaudRate
    FixedPcdGet32 (PcdSerialDbgUartClkInHz),          // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART  // Port subtype
  },

  // PCI Configuration Space Info
  {
    // The physical base address for the PCI segment
    FixedPcdGet64 (PcdPciExpressBaseAddress),
    // The PCI segment group number
    0,
    // The start bus number
    FixedPcdGet32 (PcdPciBusMin),
    // The end bus number
    FixedPcdGet32 (PcdPciBusMax),
    // AddressMapToken
    REFERENCE_TOKEN (PciAddressMapRef),
    // InterruptMapToken
    REFERENCE_TOKEN (PciInterruptMapRef)
  },

  // PCI address-range mapping references
  {
    { REFERENCE_TOKEN (PciAddressMapInfo[0]) },
    { REFERENCE_TOKEN (PciAddressMapInfo[1]) },
    { REFERENCE_TOKEN (PciAddressMapInfo[2]) }
  },
  // PCI address-range mapping information
  {
    { // PciAddressMapInfo[0] -> 32-bit BAR Window
      PCI_SS_M32,    // SpaceCode
      0x50000000,    // PciAddress
      0x50000000,    // CpuAddress
      0x08000000     // AddressSize
    },
    { // PciAddressMapInfo[1] -> 64-bit BAR Window
      PCI_SS_M64,    // SpaceCode
      0x4000000000,  // PciAddress
      0x4000000000,  // CpuAddress
      0x0100000000   // AddressSize
    },
    { // PciAddressMapInfo[2] -> IO BAR Window
      PCI_SS_IO,     // SpaceCode
      0x00000000,    // PciAddress
      0x5f800000,    // CpuAddress
      0x00800000     // AddressSize
    },
  },

  // PCI device legacy interrupts mapping information
  {
    { REFERENCE_TOKEN (PciInterruptMapInfo[0]) },
    { REFERENCE_TOKEN (PciInterruptMapInfo[1]) },
    { REFERENCE_TOKEN (PciInterruptMapInfo[2]) },
    { REFERENCE_TOKEN (PciInterruptMapInfo[3]) }
  },
  // PCI device legacy interrupts mapping information
  {
    { // PciInterruptMapInfo[0] -> Device 0, INTA
      0,   // PciBus
      0,   // PciDevice
      0,   // PciInterrupt
      {
        168, // Interrupt
        0x0  // Flags
      }
    },
    { // PciInterruptMapInfo[1] -> Device 0, INTB
      0,   // PciBus
      0,   // PciDevice
      1,   // PciInterrupt
      {
        169, // Interrupt
        0x0  // Flags
      }
    },
    { // PciInterruptMapInfo[2] -> Device 0, INTC
      0,   // PciBus
      0,   // PciDevice
      2,   // PciInterrupt
      {
        170, // Interrupt
        0x0  // Flags
      }
    },
    { // PciInterruptMapInfo[3] -> Device 0, INTD
      0,   // PciBus
      0,   // PciDevice
      3,   // PciInterrupt
      {
        171, // Interrupt
        0x0  // Flags
      }
    },
  },

  // GIC Msi Frame Info
  {
    // The GIC MSI Frame ID
    0,
    // The Physical base address for the MSI Frame.
    ARM_JUNO_GIV2M_MSI_BASE,
    /* The GIC MSI Frame flags
       as described by the GIC MSI frame
       structure in the ACPI Specification.
    */
    0,
    // SPI Count used by this frame.
    127,
    // SPI Base used by this frame.
    224
  },

  // Processor Hierarchy Nodes
  {
    // Package
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[0]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      CM_NULL_TOKEN,
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      0,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      CM_NULL_TOKEN,
      // CM_OBJECT_TOKEN  LpiToken
      CM_NULL_TOKEN
    },
    // 'big' cluster
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[1]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[0]), // -> Package
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      BIG_CLUSTER_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (BigClusterResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (ClustersLpiRef)
    },
    // 'LITTLE' cluster
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[2]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[0]), // -> Package
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      LITTLE_CLUSTER_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (LittleClusterResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (ClustersLpiRef)
    },
    // Two 'big' cores
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[3]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> 'big' cluster
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[0]),
      // UINT32  NoOfPrivateResources
      BIG_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (BigCoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[4]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> 'big' cluster
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[1]),
      // UINT32  NoOfPrivateResources
      BIG_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (BigCoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    // Four 'LITTLE' cores
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[5]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> 'LITTLE' cluster
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[2]),
      // UINT32  NoOfPrivateResources
      LITTLE_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (LittleCoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[6]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> 'LITTLE' cluster
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[3]),
      // UINT32  NoOfPrivateResources
      LITTLE_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (LittleCoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[7]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> 'LITTLE' cluster
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[4]),
      // UINT32  NoOfPrivateResources
      LITTLE_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (LittleCoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[8]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> 'LITTLE' cluster
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[5]),
      // UINT32  NoOfPrivateResources
      LITTLE_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (LittleCoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    }
  },

  // Cache information
  {
    // 'big' cluster's L2 cache
    {
      REFERENCE_TOKEN (CacheInfo[0]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x200000,                        // UINT32  Size
      2048,                            // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'big' core's L1 instruction cache
    {
      REFERENCE_TOKEN (CacheInfo[1]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0xc000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      3,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'big' core's L1 data cache
    {
      REFERENCE_TOKEN (CacheInfo[2]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'LITTLE' cluster's L2 cache
    {
      REFERENCE_TOKEN (CacheInfo[3]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x100000,                        // UINT32  Size
      1024,                            // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'LITTLE' core's L1 instruction cache
    {
      REFERENCE_TOKEN (CacheInfo[4]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'LITTLE' core's L1 data cache
    {
      REFERENCE_TOKEN (CacheInfo[5]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      128,                             // UINT32  NumberOfSets
      4,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    }
  },
  // Resources private to the 'big' cluster (shared among cores)
  {
    { REFERENCE_TOKEN (CacheInfo[0]) }  // -> 'big' cluster's L2 cache
  },
  // Resources private to each individual 'big' core instance
  {
    { REFERENCE_TOKEN (CacheInfo[1]) }, // -> 'big' core's L1 I-cache
    { REFERENCE_TOKEN (CacheInfo[2]) }  // -> 'big' core's L1 D-cache
  },
  // Resources private to the 'LITTLE' cluster (shared among cores)
  {
    { REFERENCE_TOKEN (CacheInfo[3]) }  // -> 'LITTLE' cluster's L2 cache
  },
  // Resources private to each individual 'LITTLE' core instance
  {
    { REFERENCE_TOKEN (CacheInfo[4]) }, // -> 'LITTLE' core's L1 I-cache
    { REFERENCE_TOKEN (CacheInfo[5]) }  // -> 'LITTLE' core's L1 D-cache
  },

  // Low Power Idle state information (LPI) for all cores/clusters
  {
    { // LpiInfo[0] -> Clusters CluPwrDn
      2500,         // MinResidency
      1150,         // WorstCaseWakeLatency
      1,            // Flags
      1,            // ArchFlags
      100,          // ResCntFreq
      0,            // EnableParentState
      TRUE,         // IsInteger
      0x01000000,   // IntegerEntryMethod
      // RegisterEntryMethod (NULL, use IntegerEntryMethod)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      // ResidencyCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      // UsageCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      "CluPwrDn" // StateName
    },
    // LpiInfo[1] -> Cores WFI
    {
      1,            // MinResidency
      1,            // WorstCaseWakeLatency
      1,            // Flags
      0,            // ArchFlags
      100,          // ResCntFreq
      0,            // EnableParentState
      FALSE,        // IsInteger
      0,            // IntegerEntryMethod (0, use RegisterEntryMethod)
      // RegisterEntryMethod
      {
        EFI_ACPI_6_3_FUNCTIONAL_FIXED_HARDWARE, // AddressSpaceId
        0x20, // RegisterBitWidth
        0x00, // RegisterBitOffset
        0x03, // AccessSize
        0xFFFFFFFF // Address
      },
      // ResidencyCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      // UsageCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      "WFI" // StateName
    },
    // LpiInfo[2] -> Cores CorePwrDn
    {
      150,          // MinResidency
      350,          // WorstCaseWakeLatency
      1,            // Flags
      1,            // ArchFlags
      100,          // ResCntFreq
      1,            // EnableParentState
      FALSE,        // IsInteger
      0,            // IntegerEntryMethod (0, use RegisterEntryMethod)
      // RegisterEntryMethod
      {
          EFI_ACPI_6_3_FUNCTIONAL_FIXED_HARDWARE, // AddressSpaceId
          0x20, // RegisterBitWidth
          0x00, // RegisterBitOffset
          0x03, // AccessSize
          0x00010000 // Address
      },
      // ResidencyCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      // UsageCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY, 0, 0, 0, 0 },
      "CorePwrDn" // StateName
    },
  },
  // Cluster Low Power Idle state references (LPI)
  {
    { REFERENCE_TOKEN (LpiInfo[0]) }
  },
  // Cores Low Power Idle state references (LPI)
  {
    { REFERENCE_TOKEN (LpiInfo[1]) },
    { REFERENCE_TOKEN (LpiInfo[2]) },
  },
  { // Power domains
    { // 0: big cores
      // Revision
      EFI_ACPI_6_5_AML_PSD_REVISION,
      // Domain
      PSD_BIG_DOMAIN_ID,
      // CoordType
      ACPI_AML_COORD_TYPE_SW_ANY,
      // NumProc
      2,
    },
    { // 1: little cores
      // Revision
      EFI_ACPI_6_5_AML_PSD_REVISION,
      // Domain
      PSD_LITTLE_DOMAIN_ID,
      // CoordType
      ACPI_AML_COORD_TYPE_SW_ANY,
      // NumProc
      4,
    },
  },
};

/** A helper function for returning the Configuration Manager Objects.

  @param [in]       CmObjectId     The Configuration Manager Object ID.
  @param [in]       Object         Pointer to the Object(s).
  @param [in]       ObjectSize     Total size of the Object(s).
  @param [in]       ObjectCount    Number of Objects.
  @param [in, out]  CmObjectDesc   Pointer to the Configuration Manager Object
                                   descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
**/
STATIC
EFI_STATUS
EFIAPI
HandleCmObject (
  IN  CONST CM_OBJECT_ID                CmObjectId,
  IN        VOID                *       Object,
  IN  CONST UINTN                       ObjectSize,
  IN  CONST UINTN                       ObjectCount,
  IN  OUT   CM_OBJ_DESCRIPTOR   * CONST CmObjectDesc
  )
{
  CmObjectDesc->ObjectId = CmObjectId;
  CmObjectDesc->Size = ObjectSize;
  CmObjectDesc->Data = (VOID*)Object;
  CmObjectDesc->Count = ObjectCount;
  DEBUG ((
    DEBUG_INFO,
    "INFO: CmObjectId = %x, Ptr = 0x%p, Size = %d, Count = %d\n",
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return EFI_SUCCESS;
}

/** A helper function for returning the Configuration Manager Objects that
    match the token.

  @param [in]  This               Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId         The Configuration Manager Object ID.
  @param [in]  Object             Pointer to the Object(s).
  @param [in]  ObjectSize         Total size of the Object(s).
  @param [in]  ObjectCount        Number of Objects.
  @param [in]  Token              A token identifying the object.
  @param [in]  HandlerProc        A handler function to search the object
                                  referenced by the token.
  @param [in, out]  CmObjectDesc  Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
STATIC
EFI_STATUS
EFIAPI
HandleCmObjectRefByToken (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN        VOID                                  *       Object,
  IN  CONST UINTN                                         ObjectSize,
  IN  CONST UINTN                                         ObjectCount,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  CONST CM_OBJECT_HANDLER_PROC                        HandlerProc,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObjectDesc
  )
{
  EFI_STATUS  Status;
  CmObjectDesc->ObjectId = CmObjectId;
  if (Token == CM_NULL_TOKEN) {
    CmObjectDesc->Size = ObjectSize;
    CmObjectDesc->Data = (VOID*)Object;
    CmObjectDesc->Count = ObjectCount;
    Status = EFI_SUCCESS;
  } else {
    Status = HandlerProc (This, CmObjectId, Token, CmObjectDesc);
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Token = 0x%p, CmObjectId = %x, Ptr = 0x%p, Size = %d, Count = %d\n",
    (VOID*)Token,
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return Status;
}

/** A helper function for returning Configuration Manager Object(s) referenced
    by token when the entire platform repository is in scope and the
    CM_NULL_TOKEN value is not allowed.

  @param [in]  This               Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId         The Configuration Manager Object ID.
  @param [in]  Token              A token identifying the object.
  @param [in]  HandlerProc        A handler function to search the object(s)
                                  referenced by the token.
  @param [in, out]  CmObjectDesc  Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
STATIC
EFI_STATUS
EFIAPI
HandleCmObjectSearchPlatformRepo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  CONST CM_OBJECT_HANDLER_PROC                        HandlerProc,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObjectDesc
  )
{
  EFI_STATUS  Status;
  CmObjectDesc->ObjectId = CmObjectId;
  if (Token == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: CM_NULL_TOKEN value is not allowed when searching"
      " the entire platform repository.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = HandlerProc (This, CmObjectId, Token, CmObjectDesc);
  DEBUG ((
    DEBUG_INFO,
    "INFO: Token = 0x%p, CmObjectId = %x, Ptr = 0x%p, Size = %d, Count = %d\n",
    (VOID*)Token,
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return Status;
}

/** Clear Cpc information.

  If populating _CPC information fails, remove GicC tokens pointing
  to Cpc CmObj to avoid creating corrupted _CPC objects.

  @param [in] PlatformRepo   Platfom Info repository.

  @retval EFI_SUCCESS   Success.
**/
STATIC
EFI_STATUS
EFIAPI
ClearCpcInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  )
{
  CM_ARM_GICC_INFO  *GicCInfo;

  GicCInfo = (CM_ARM_GICC_INFO*)&PlatformRepo->GicCInfo;

  GicCInfo[0].CpcToken = CM_NULL_TOKEN;
  GicCInfo[1].CpcToken = CM_NULL_TOKEN;
  GicCInfo[2].CpcToken = CM_NULL_TOKEN;
  GicCInfo[3].CpcToken = CM_NULL_TOKEN;
  GicCInfo[4].CpcToken = CM_NULL_TOKEN;
  GicCInfo[5].CpcToken = CM_NULL_TOKEN;

  return EFI_SUCCESS;
}

/** Use the SCMI protocol to populate CPC objects dynamically.

  @param [in] PlatformRepo   Platfom Info repository.
  @param [in] DomainId       Id of the DVFS domain to probe.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         Not supported.
  @retval !(EFI_SUCCESS)          An error occured.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateCpcInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo,
  IN  UINT32                      DomainId
  )
{
  EFI_STATUS          Status;
  CM_ARM_GICC_INFO    *GicCInfo;
  AML_CPC_INFO        *CpcInfo;

  if ((PlatformRepo == NULL)  ||
      ((DomainId != PSD_BIG_DOMAIN_ID) &&
       (DomainId != PSD_LITTLE_DOMAIN_ID))) {
    Status = EFI_INVALID_PARAMETER;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  CpcInfo = &PlatformRepo->CpcInfo[DomainId];
  GicCInfo = (CM_ARM_GICC_INFO*)&PlatformRepo->GicCInfo;

  Status = DynamicTablesScmiInfoGetFastChannel (
              PlatformRepo->PsdInfo[DomainId].Domain,
              CpcInfo
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* CPPC must advertise performances on a 'continuous, abstract, unit-less
     performance scale', i.e. CPU performances on an asymmetric platform
     nust be represented on a unified scale.
     CPU performance values are obtained from SCP through SCMI and advertised
     to the OS via the _CPC objects. SCP currently maps performance requests
     to frequency requests.
     Thus, SCP must be modified to  advertise (and correctly handle)
     performance values on a unified scale.

     Check that SCP is using a unified scale by checking that the advertised
     lowest/nominal frequencies are not the default ones.
   */
  if (((DomainId == PSD_BIG_DOMAIN_ID) &&
       (CpcInfo->LowestPerformanceInteger == 600000000) &&
       (CpcInfo->NominalPerformanceInteger == 1000000000)) ||
      ((DomainId == PSD_LITTLE_DOMAIN_ID) &&
       (CpcInfo->LowestPerformanceInteger == 450000000) &&
       (CpcInfo->NominalPerformanceInteger == 800000000))) {
    return EFI_UNSUPPORTED;
  }

  // Juno R2's lowest/nominal frequencies.
  // Nominal frequency != Highest frequency.
  if (DomainId == PSD_BIG_DOMAIN_ID) {
    CpcInfo->LowestFrequencyInteger = 600;
    CpcInfo->NominalFrequencyInteger = 1000;
  } else {
    CpcInfo->LowestFrequencyInteger = 450;
    CpcInfo->NominalFrequencyInteger = 800;
  }

  // The mapping Psd -> CPUs is available here.
  if (DomainId == PSD_BIG_DOMAIN_ID) {
    GicCInfo[0].CpcToken = (CM_OBJECT_TOKEN)CpcInfo;
    GicCInfo[1].CpcToken = (CM_OBJECT_TOKEN)CpcInfo;
  } else {
    GicCInfo[2].CpcToken = (CM_OBJECT_TOKEN)CpcInfo;
    GicCInfo[3].CpcToken = (CM_OBJECT_TOKEN)CpcInfo;
    GicCInfo[4].CpcToken = (CM_OBJECT_TOKEN)CpcInfo;
    GicCInfo[5].CpcToken = (CM_OBJECT_TOKEN)CpcInfo;
  }

  /*
    Arm advises to use FFH to the following registers which uses AMU counters:
     - ReferencePerformanceCounterRegister
     - DeliveredPerformanceCounterRegister
    Cf. Arm Functional Fixed Hardware Specification
    s3.2 Performance management and Collaborative Processor Performance Control

    AMU is not supported by the Juno, so clear these registers.
   */
  CpcInfo->ReferencePerformanceCounterRegister.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  CpcInfo->ReferencePerformanceCounterRegister.RegisterBitWidth = 0;
  CpcInfo->ReferencePerformanceCounterRegister.RegisterBitOffset = 0;
  CpcInfo->ReferencePerformanceCounterRegister.AccessSize = 0;
  CpcInfo->ReferencePerformanceCounterRegister.Address = 0;

  CpcInfo->DeliveredPerformanceCounterRegister.AddressSpaceId = EFI_ACPI_6_5_SYSTEM_MEMORY;
  CpcInfo->DeliveredPerformanceCounterRegister.RegisterBitWidth = 0;
  CpcInfo->DeliveredPerformanceCounterRegister.RegisterBitOffset = 0;
  CpcInfo->DeliveredPerformanceCounterRegister.AccessSize = 0;
  CpcInfo->DeliveredPerformanceCounterRegister.Address = 0;

  return Status;
}

/** Iterate over the PSD Domains and try to populate the Cpc objects.

  @param [in] PlatformRepo   Platfom Info repository.
**/
STATIC
VOID
EFIAPI
PopulateCpcObjects (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  BOOLEAN     CpcFailed;

  CpcFailed = FALSE;
  for (Index = 0; Index < PSD_DOMAIN_COUNT; Index++) {
    Status = PopulateCpcInfo (PlatformRepo, Index);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "WARN: Could not populate _CPC.\n"));
      CpcFailed = TRUE;
      break;
    }
  }

  if (CpcFailed) {
    // _CPC information is not mandatory and SCP might not support some
    // SCMI requests. Failing should not prevent from booting.
    ClearCpcInfo (PlatformRepo);
  }
}

/** Initialize the platform configuration repository.

  @param [in]  This        Pointer to the Configuration Manager Protocol.

  @retval EFI_SUCCESS   Success
**/
STATIC
EFI_STATUS
EFIAPI
InitializePlatformRepository (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;

  PlatformRepo = This->PlatRepoInfo;

  GetJunoRevision (PlatformRepo->JunoRevision);
  DEBUG ((DEBUG_INFO, "Juno Rev = 0x%x\n", PlatformRepo->JunoRevision));

  ///
  /// 1.
  /// _CPC was only tested on Juno R2, so only enable support for this version.
  ///
  /// 2.
  /// Some _CPC registers cannot be populated for the Juno:
  /// - PerformanceLimitedRegister
  /// - ReferencePerformanceCounterRegister
  /// - DeliveredPerformanceCounterRegister
  /// Only build _CPC objects if relaxation regarding these registers
  /// is allowed.
  if ((PlatformRepo->JunoRevision == JUNO_REVISION_R2) &&
      (PcdGet64(PcdDevelopmentPlatformRelaxations) & BIT0)) {
    PopulateCpcObjects (PlatformRepo);
  }

  return EFI_SUCCESS;
}

/** Return a GT Block timer frame info list.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       A token for identifying the object
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetGTBlockTimerFrameInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if (Token != (CM_OBJECT_TOKEN)&PlatformRepo->GTBlock0TimerInfo) {
    return EFI_NOT_FOUND;
  }

  CmObject->ObjectId = CmObjectId;
  CmObject->Size = sizeof (PlatformRepo->GTBlock0TimerInfo);
  CmObject->Data = (VOID*)&PlatformRepo->GTBlock0TimerInfo;
  CmObject->Count = sizeof (PlatformRepo->GTBlock0TimerInfo) /
                      sizeof (PlatformRepo->GTBlock0TimerInfo[0]);
  return EFI_SUCCESS;
}

/** Return GIC CPU Interface Info.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARM_GICC_INFO object.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetGicCInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TotalObjCount;
  UINT32                            ObjIndex;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  TotalObjCount = ARRAY_SIZE (PlatformRepo->GicCInfo);

  for (ObjIndex = 0; ObjIndex < TotalObjCount; ObjIndex++) {
    if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->GicCInfo[ObjIndex]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->GicCInfo[ObjIndex]);
      CmObject->Data = (VOID*)&PlatformRepo->GicCInfo[ObjIndex];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return Lpi State Info.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARCH_COMMON_LPI_INFO object.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetLpiInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TotalObjCount;
  UINT32                            ObjIndex;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  TotalObjCount = ARRAY_SIZE (PlatformRepo->LpiInfo);

  for (ObjIndex = 0; ObjIndex < TotalObjCount; ObjIndex++) {
    if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->LpiInfo[ObjIndex]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->LpiInfo[ObjIndex]);
      CmObject->Data = (VOID*)&PlatformRepo->LpiInfo[ObjIndex];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/** Return PCI address-range mapping Info.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO object.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetPciAddressMapInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TotalObjCount;
  UINT32                            ObjIndex;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  TotalObjCount = ARRAY_SIZE (PlatformRepo->PciAddressMapInfo);

  for (ObjIndex = 0; ObjIndex < TotalObjCount; ObjIndex++) {
    if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PciAddressMapInfo[ObjIndex]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->PciAddressMapInfo[ObjIndex]);
      CmObject->Data = (VOID*)&PlatformRepo->PciAddressMapInfo[ObjIndex];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return PCI device legacy interrupt mapping Info.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO object.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetPciInterruptMapInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TotalObjCount;
  UINT32                            ObjIndex;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  TotalObjCount = ARRAY_SIZE (PlatformRepo->PciInterruptMapInfo);

  for (ObjIndex = 0; ObjIndex < TotalObjCount; ObjIndex++) {
    if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PciInterruptMapInfo[ObjIndex]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->PciInterruptMapInfo[ObjIndex]);
      CmObject->Data = (VOID*)&PlatformRepo->PciInterruptMapInfo[ObjIndex];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return Psd Info.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARCH_COMMON_PSD_INFO object.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetPsdInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TotalObjCount;
  UINT32                            ObjIndex;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  TotalObjCount = ARRAY_SIZE (PlatformRepo->PsdInfo);

  for (ObjIndex = 0; ObjIndex < TotalObjCount; ObjIndex++) {
    if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PsdInfo[ObjIndex]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->PsdInfo[ObjIndex]);
      CmObject->Data = (VOID*)&PlatformRepo->PsdInfo[ObjIndex];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return Cpc Info.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARCH_COMMON_CPC_INFO object.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetCpcInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TotalObjCount;
  UINT32                            ObjIndex;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  TotalObjCount = ARRAY_SIZE (PlatformRepo->CpcInfo);

  for (ObjIndex = 0; ObjIndex < TotalObjCount; ObjIndex++) {
    if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->CpcInfo[ObjIndex]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->CpcInfo[ObjIndex]);
      CmObject->Data = (VOID*)&PlatformRepo->CpcInfo[ObjIndex];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return a list of Configuration Manager object references pointed to by the
    given input token.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARCH_COMMON_OBJ_REF list.
  @param [in, out] CmObject       Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetCmObjRefs (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->BigClusterResources) {
    CmObject->Size = sizeof (PlatformRepo->BigClusterResources);
    CmObject->Data = (VOID*)&PlatformRepo->BigClusterResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->BigClusterResources);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->BigCoreResources) {
    CmObject->Size = sizeof (PlatformRepo->BigCoreResources);
    CmObject->Data = (VOID*)&PlatformRepo->BigCoreResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->BigCoreResources);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->LittleClusterResources) {
    CmObject->Size = sizeof (PlatformRepo->LittleClusterResources);
    CmObject->Data = (VOID*)&PlatformRepo->LittleClusterResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->LittleClusterResources);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->LittleCoreResources) {
    CmObject->Size = sizeof (PlatformRepo->LittleCoreResources);
    CmObject->Data = (VOID*)&PlatformRepo->LittleCoreResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->LittleCoreResources);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->ClustersLpiRef) {
    CmObject->Size = sizeof (PlatformRepo->ClustersLpiRef);
    CmObject->Data = (VOID*)&PlatformRepo->ClustersLpiRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->ClustersLpiRef);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->CoresLpiRef) {
    CmObject->Size = sizeof (PlatformRepo->CoresLpiRef);
    CmObject->Data = (VOID*)&PlatformRepo->CoresLpiRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->CoresLpiRef);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PciAddressMapRef) {
    CmObject->Size = sizeof (PlatformRepo->PciAddressMapRef);
    CmObject->Data = (VOID*)&PlatformRepo->PciAddressMapRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->PciAddressMapRef);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PciInterruptMapRef) {
    CmObject->Size = sizeof (PlatformRepo->PciInterruptMapRef);
    CmObject->Data = (VOID*)&PlatformRepo->PciInterruptMapRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->PciInterruptMapRef);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/** Return a standard namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetStandardNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EFI_STATUS                        Status;
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINT32                            TableCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EStdObjCfgMgrInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->CmInfo,
                 sizeof (PlatformRepo->CmInfo),
                 1,
                 CmObject
                 );
      break;

    case EStdObjAcpiTableList:
      TableCount = ARRAY_SIZE (PlatformRepo->CmAcpiTableList);
      if (PlatformRepo->JunoRevision == JUNO_REVISION_R0) {
        /* The last 2 tables in the ACPI table list enable PCIe support.
           Reduce the TableCount so that the PCIe specific ACPI
           tables are not installed on Juno R0
        */
        TableCount -= 2;
      }
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->CmAcpiTableList,
                 (sizeof (PlatformRepo->CmAcpiTableList[0]) * TableCount),
                 TableCount,
                 CmObject
                 );
      break;

    default: {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}


/** Return an Arch Common namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetArchCommonNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EFI_STATUS                        Status;
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EArchCommonObjPowerManagementProfileInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PmProfileInfo,
                 sizeof (PlatformRepo->PmProfileInfo),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjConsolePortInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SpcrSerialPort,
                 sizeof (PlatformRepo->SpcrSerialPort),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjSerialDebugPortInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->DbgSerialPort,
                 sizeof (PlatformRepo->DbgSerialPort),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjCmRef:
      Status = HandleCmObjectSearchPlatformRepo (
                 This,
                 CmObjectId,
                 Token,
                 GetCmObjRefs,
                 CmObject
                 );
      break;

    case EArchCommonObjPciConfigSpaceInfo:
      if (PlatformRepo->JunoRevision != JUNO_REVISION_R0) {
        Status = HandleCmObject (
                   CmObjectId,
                   &PlatformRepo->PciConfigInfo,
                   sizeof (PlatformRepo->PciConfigInfo),
                   1,
                   CmObject
                   );
      } else {
        // No PCIe on Juno R0.
        Status = EFI_NOT_FOUND;
      }
      break;

    case EArchCommonObjPciAddressMapInfo:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->PciAddressMapInfo,
                 sizeof (PlatformRepo->PciAddressMapInfo),
                 ARRAY_SIZE (PlatformRepo->PciAddressMapInfo),
                 Token,
                 GetPciAddressMapInfo,
                 CmObject
                 );
      break;

    case EArchCommonObjPciInterruptMapInfo:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->PciInterruptMapInfo,
                 sizeof (PlatformRepo->PciInterruptMapInfo),
                 ARRAY_SIZE (PlatformRepo->PciInterruptMapInfo),
                 Token,
                 GetPciInterruptMapInfo,
                 CmObject
                 );
      break;

    case EArchCommonObjLpiInfo:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 NULL,
                 0,
                 0,
                 Token,
                 GetLpiInfo,
                 CmObject
                 );
      break;

    case EArchCommonObjProcHierarchyInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->ProcHierarchyInfo,
                 sizeof (PlatformRepo->ProcHierarchyInfo),
                 ARRAY_SIZE (PlatformRepo->ProcHierarchyInfo),
                 CmObject
                 );
      break;

    case EArchCommonObjCacheInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->CacheInfo,
                 sizeof (PlatformRepo->CacheInfo),
                 ARRAY_SIZE (PlatformRepo->CacheInfo),
                 CmObject
                 );
      break;

    case EArchCommonObjCpcInfo:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->CpcInfo,
                 sizeof (PlatformRepo->CpcInfo),
                 ARRAY_SIZE (PlatformRepo->CpcInfo),
                 Token,
                 GetCpcInfo,
                 CmObject
                 );
      break;

    case EArchCommonObjPsdInfo:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->PsdInfo,
                 sizeof (PlatformRepo->PsdInfo),
                 ARRAY_SIZE (PlatformRepo->PsdInfo),
                 Token,
                 GetPsdInfo,
                 CmObject
                 );
      break;

    default: {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_INFO,
        "INFO: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  } //switch

  return Status;
}

/** Return an ARM namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetArmNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EFI_STATUS                        Status;
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EArmObjBootArchInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->BootArchInfo,
                 sizeof (PlatformRepo->BootArchInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjGenericTimerInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GenericTimerInfo,
                 sizeof (PlatformRepo->GenericTimerInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjPlatformGenericWatchdogInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->Watchdog,
                 sizeof (PlatformRepo->Watchdog),
                 1,
                 CmObject
                 );
      break;

    case EArmObjPlatformGTBlockInfo:
      if (PlatformRepo->JunoRevision == JUNO_REVISION_R0) {
        // Disable Memory Mapped Platform Timers for Juno R0
        // due to Juno Erratum 832219.
        Status = EFI_NOT_FOUND;
      } else {
        Status = HandleCmObject (
                   CmObjectId,
                   PlatformRepo->GTBlockInfo,
                   sizeof (PlatformRepo->GTBlockInfo),
                   ARRAY_SIZE (PlatformRepo->GTBlockInfo),
                   CmObject
                  );
      }
      break;

    case EArmObjGTBlockTimerFrameInfo:
      if (PlatformRepo->JunoRevision == JUNO_REVISION_R0) {
        // Disable Memory Mapped Platform Timers for Juno R0
        // due to Juno Erratum 832219.
        Status = EFI_NOT_FOUND;
      } else {
        Status = HandleCmObjectRefByToken (
                   This,
                   CmObjectId,
                   PlatformRepo->GTBlock0TimerInfo,
                   sizeof (PlatformRepo->GTBlock0TimerInfo),
                   ARRAY_SIZE (PlatformRepo->GTBlock0TimerInfo),
                   Token,
                   GetGTBlockTimerFrameInfo,
                   CmObject
                   );
      }
      break;

    case EArmObjGicCInfo:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->GicCInfo,
                 sizeof (PlatformRepo->GicCInfo),
                 ARRAY_SIZE (PlatformRepo->GicCInfo),
                 Token,
                 GetGicCInfo,
                 CmObject
                 );
      break;

    case EArmObjGicDInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GicDInfo,
                 sizeof (PlatformRepo->GicDInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjGicMsiFrameInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GicMsiFrameInfo,
                 sizeof (PlatformRepo->GicMsiFrameInfo),
                 1,
                 CmObject
                 );
      break;

    default: {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_INFO,
        "INFO: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }//switch

  return Status;
}

/** Return an OEM namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetOemNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    default: {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/** The GetObject function defines the interface implemented by the
    Configuration Manager Protocol for returning the Configuration
    Manager Objects.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
ArmJunoPlatformGetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_NAMESPACE_ID (CmObjectId)) {
    case EObjNameSpaceStandard:
      Status = GetStandardNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    case EObjNameSpaceArchCommon:
      Status = GetArchCommonNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    case EObjNameSpaceArm:
      Status = GetArmNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    case EObjNameSpaceOem:
      Status = GetOemNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    default: {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Unknown Namespace Object = 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/** The SetObject function defines the interface implemented by the
    Configuration Manager Protocol for updating the Configuration
    Manager Objects.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in]      CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the Object.

  @retval EFI_UNSUPPORTED  This operation is not supported.
**/
EFI_STATUS
EFIAPI
ArmJunoPlatformSetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  return EFI_UNSUPPORTED;
}

/** A structure describing the configuration manager protocol interface.
*/
STATIC
CONST
EDKII_CONFIGURATION_MANAGER_PROTOCOL ArmJunoPlatformConfigManagerProtocol = {
  CREATE_REVISION (1, 0),
  ArmJunoPlatformGetObject,
  ArmJunoPlatformSetObject,
  &ArmJunoPlatformRepositoryInfo
};

/**
  Entrypoint of Configuration Manager Dxe.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
ConfigurationManagerDxeInitialize (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE  * SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiConfigurationManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID*)&ArmJunoPlatformConfigManagerProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Install Configuration Manager Protocol." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Status = InitializePlatformRepository (
    &ArmJunoPlatformConfigManagerProtocol
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to initialize the Platform Configuration Repository." \
      " Status = %r\n",
      Status
      ));
  }

error_handler:
  return Status;
}
