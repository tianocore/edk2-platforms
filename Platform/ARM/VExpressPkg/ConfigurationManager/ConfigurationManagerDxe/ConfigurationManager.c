/** @file
  Configuration Manager Dxe

  Copyright (c) 2017 - 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
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
EDKII_PLATFORM_REPOSITORY_INFO  VExpressPlatRepositoryInfo = {
  /// Configuration Manager information
  { CONFIGURATION_MANAGER_REVISION, CFG_MGR_OEM_ID },

  // ACPI Table List
  {
    // FADT Table
    {
      EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
      NULL
    },
    // GTDT Table
    {
      EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdGtdt),
      NULL
    },
    // MADT Table
    {
      EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
      NULL
    },
    // SPCR Table
    {
      EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpcr),
      NULL
    },
    // DSDT Table
    {
      EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDsdt),
      (EFI_ACPI_DESCRIPTION_HEADER *)dsdt_aml_code
    },
    // DBG2 Table
    {
      EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
      NULL
    },
    // SSDT Cpu Hierarchy Table
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCpuTopology),
      NULL
    },
    // PPTT Table
    {
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdPptt),
      NULL
    },
    // Note: The last 3 tables in this list are for FVP RevC only.
    // IORT Table - FVP RevC
    {
      EFI_ACPI_6_3_IO_REMAPPING_TABLE_SIGNATURE,
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdIort),
      NULL
    },
    // PCI MCFG Table - FVP RevC
    {
      EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
      NULL
    },
    // SSDT table describing the PCI root complex - FVP RevC
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtPciExpress),
      NULL,
      SIGNATURE_64 ('S', 'S', 'D', 'T', '-', 'P', 'C', 'I')
    },
  },

  // Boot architecture information
  { EFI_ACPI_6_5_ARM_PSCI_COMPLIANT },              // BootArchFlags

 #ifdef HEADLESS_PLATFORM
  // Fixed feature flag information
  { EFI_ACPI_6_5_HEADLESS },                        // Fixed feature flags
 #endif

  // Power management profile information
  { EFI_ACPI_6_5_PM_PROFILE_ENTERPRISE_SERVER },    // PowerManagement Profile

  /* GIC CPU Interface information
     GIC_ENTRY (CPUInterfaceNumber, Mpidr, PmuIrq, VGicIrq, EnergyEfficiency)
     Note: The MPIDR is fixed up in InitializePlatformRepository() if the
           platform is FVP RevC.
  */
  {
    GICC_ENTRY (0, GET_MPID (0, 0), 92, 25, 0),
    GICC_ENTRY (1, GET_MPID (0, 1), 93, 25, 0),
    GICC_ENTRY (2, GET_MPID (0, 2), 94, 25, 0),
    GICC_ENTRY (3, GET_MPID (0, 3), 95, 25, 0),

    GICC_ENTRY (4, GET_MPID (1, 0), 96, 25, 0),
    GICC_ENTRY (5, GET_MPID (1, 1), 97, 25, 0),
    GICC_ENTRY (6, GET_MPID (1, 2), 98, 25, 0),
    GICC_ENTRY (7, GET_MPID (1, 3), 99, 25, 0)
  },

  // GIC Distributor Info
  {
    FixedPcdGet64 (PcdGicDistributorBase),  // UINT64  PhysicalBaseAddress
    0,                                      // UINT32  SystemVectorBase
    3                                       // UINT8   GicVersion
  },

  /// GIC Re-Distributor Info
  {
    // UINT64  DiscoveryRangeBaseAddress
    FixedPcdGet64 (PcdGicRedistributorsBase),
    // UINT32  DiscoveryRangeLength
    0x00200000
  },

  // Generic Timer Info
  {
    // The physical base address for the counter control frame
    FVP_SYSTEM_TIMER_BASE_ADDRESS,
    // The physical base address for the counter read frame
    FVP_CNT_READ_BASE_ADDRESS,
    // The secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerSecIntrNum),
    // The secure PL1 timer flags
    FVP_GTDT_GTIMER_FLAGS,
    // The non-secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerIntrNum),
    // The non-secure PL1 timer flags
    FVP_GTDT_GTIMER_FLAGS,
    // The virtual timer interrupt
    FixedPcdGet32 (PcdArmArchTimerVirtIntrNum),
    // The virtual timer flags
    FVP_GTDT_GTIMER_FLAGS,
    // The non-secure PL2 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerHypIntrNum),
    // The non-secure PL2 timer flags
    FVP_GTDT_GTIMER_FLAGS
  },

  // Generic Timer Block Information
  {
    {
      // The physical base address for the GT Block Timer structure
      FVP_GT_BLOCK_CTL_BASE,
      // The number of timer frames implemented in the GT Block
      FVP_TIMER_FRAMES_COUNT,
      // Reference token for the GT Block timer frame list
      REFERENCE_TOKEN (GTBlock0TimerInfo)
    }
  },

  // GT Block Timer Frames
  {
    // Frame 0
    {
      0,                                // UINT8   FrameNumber
      FVP_GT_BLOCK_FRAME0_CTL_BASE,     // UINT64  PhysicalAddressCntBase
      FVP_GT_BLOCK_FRAME0_CTL_EL0_BASE, // UINT64  PhysicalAddressCntEL0Base
      FVP_GT_BLOCK_FRAME0_GSIV,         // UINT32  PhysicalTimerGSIV
      FVP_GTX_TIMER_FLAGS,              // UINT32  PhysicalTimerFlags
      0,                                // UINT32  VirtualTimerGSIV
      0,                                // UINT32  VirtualTimerFlags
      FVP_GTX_COMMON_FLAGS_S            // UINT32  CommonFlags
    },
    // Frame 1
    {
      1,                                // UINT8   FrameNumber
      FVP_GT_BLOCK_FRAME1_CTL_BASE,     // UINT64  PhysicalAddressCntBase
      FVP_GT_BLOCK_FRAME1_CTL_EL0_BASE, // UINT64  PhysicalAddressCntEL0Base
      FVP_GT_BLOCK_FRAME1_GSIV,         // UINT32  PhysicalTimerGSIV
      FVP_GTX_TIMER_FLAGS,              // UINT32  PhysicalTimerFlags
      0,                                // UINT32  VirtualTimerGSIV
      0,                                // UINT32  VirtualTimerFlags
      FVP_GTX_COMMON_FLAGS_NS           // UINT32  CommonFlags
    },
  },

  // Watchdog Info
  {
    // The physical base address of the SBSA Watchdog control frame
    FixedPcdGet64 (PcdGenericWatchdogControlBase),
    // The physical base address of the SBSA Watchdog refresh frame
    FixedPcdGet64 (PcdGenericWatchdogRefreshBase),
    // The watchdog interrupt
    0,
    // The watchdog flags
    FVP_SBSA_WATCHDOG_FLAGS
  },

  // SPCR Serial Port
  {
    FixedPcdGet64 (PcdSerialRegisterBase),                    // BaseAddress
    FixedPcdGet32 (PL011UartInterrupt),                       // Interrupt
    FixedPcdGet64 (PcdUartDefaultBaudRate),                   // BaudRate
    FixedPcdGet32 (PL011UartClkInHz),                         // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART,  // Port subtype
    0x1000,                                                   // Address length
    EFI_ACPI_6_3_DWORD,                                       // Access size
  },
  // Debug Serial Port
  {
    FixedPcdGet64 (PcdSerialDbgRegisterBase),                 // BaseAddress
    38,                                                       // Interrupt
    FixedPcdGet64 (PcdSerialDbgUartBaudRate),                 // BaudRate
    FixedPcdGet32 (PcdSerialDbgUartClkInHz),                  // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART,  // Port subtype
    0x1000,                                                   // Address length
    EFI_ACPI_6_3_DWORD,                                       // Access size
  },

  // GIC ITS
  {
    // The GIC ITS ID.
    0,
    // The physical address for the Interrupt Translation Service
    0x2f020000
  },

  // SMMUv3 Node
  {
    // Reference token for this Iort node
    REFERENCE_TOKEN (SmmuV3Info),
    // Number of ID mappings
    1,
    // Reference token for the ID mapping array
    REFERENCE_TOKEN (DeviceIdMapping[0]),

    // SMMU Base Address
    FVP_REVC_SMMUV3_BASE,
    // SMMU flags
    EFI_ACPI_IORT_SMMUv3_FLAG_COHAC_OVERRIDE,
    // VATOS address
    0,
    // Model
    EFI_ACPI_IORT_SMMUv3_MODEL_GENERIC,
    // GSIV of the Event interrupt if SPI based
    0x6A,
    // PRI Interrupt if SPI based
    0x6B,
    // GERR interrupt if GSIV based
    0x6F,
    // Sync interrupt if GSIV based
    0x6D,

    // Proximity domain flag, ignored in this case
    0,
    // Index into the array of ID mapping, ignored as SMMU
    // control interrupts are GSIV based
    0
  },

  // ITS group node
  {
    // Reference token for this Iort node
    REFERENCE_TOKEN (ItsGroupInfo),
    // The number of ITS identifiers in the ITS node.
    1,
    // Reference token for the ITS identifier array
    REFERENCE_TOKEN (ItsIdentifierArray)
  },
  // ITS identifier array
  {
    {
      // The ITS Identifier
      0
    }
  },

  // Root Complex node info
  {
    // Reference token for this Iort node
    REFERENCE_TOKEN (RootComplexInfo),
    // Number of ID mappings
    1,
    // Reference token for the ID mapping array
    REFERENCE_TOKEN (DeviceIdMapping[1]),

    // Memory access properties : Cache coherent attributes
    EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA,
    // Memory access properties : Allocation hints
    0,
    // Memory access properties : Memory access flags
    0,
    // ATS attributes
    EFI_ACPI_IORT_ROOT_COMPLEX_ATS_UNSUPPORTED,
    // PCI segment number
    0,
    // Memory Address Size Limit
    64
  },

  // Array of Device ID mappings
  {
    /* Mapping When SMMUv3 is defined
       RootComplex -> SMMUv3 -> ITS Group
    */

    // SMMUv3 device ID mapping
    {
      // Input base
      0x0,
      // Number of input IDs
      0x0000FFFF,
      // Output Base
      0x0,
      // Output reference
      REFERENCE_TOKEN (ItsGroupInfo),
      // Flags
      0
    },
    // Device ID mapping for Root complex node
    {
      // Input base
      0x0,
      // Number of input IDs
      0x0000FFFF,
      // Output Base
      0x0,
      // Output reference token for the IORT node
      REFERENCE_TOKEN (SmmuV3Info),
      // Flags
      0
    }
  },

  // PCI Configuration Space Info
  {
    FixedPcdGet64 (PcdPciExpressBaseAddress),
    // PciSegmentGroupNumber
    0,
    FixedPcdGet32 (PcdPciBusMin),
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
    {             // PciAddressMapInfo[0] -> 32-bit BAR Window
      PCI_SS_M32, // SpaceCode
      0x50000000, // PciAddress
      0x50000000, // CpuAddress
      0x08000000  // AddressSize
    },
    {               // PciAddressMapInfo[1] -> 64-bit BAR Window
      PCI_SS_M64,   // SpaceCode
      0x4000000000, // PciAddress
      0x4000000000, // CpuAddress
      0x0100000000  // AddressSize
    },
    {             // PciAddressMapInfo[2] -> IO BAR Window
      PCI_SS_IO,  // SpaceCode
      0x00000000, // PciAddress
      0x5f800000, // CpuAddress
      0x00800000  // AddressSize
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
    {    // PciInterruptMapInfo[0] -> Device 0, INTA
      0, // PciBus
      0, // PciDevice
      0, // PciInterrupt
      {
        168, // Interrupt
        0x0  // Flags
      }
    },
    {    // PciInterruptMapInfo[1] -> Device 0, INTB
      0, // PciBus
      0, // PciDevice
      1, // PciInterrupt
      {
        169, // Interrupt
        0x0  // Flags
      }
    },
    {    // PciInterruptMapInfo[2] -> Device 0, INTC
      0, // PciBus
      0, // PciDevice
      2, // PciInterrupt
      {
        170, // Interrupt
        0x0  // Flags
      }
    },
    {    // PciInterruptMapInfo[3] -> Device 0, INTD
      0, // PciBus
      0, // PciDevice
      3, // PciInterrupt
      {
        171, // Interrupt
        0x0  // Flags
      }
    },
  },

  // Embedded Trace device info
  {
    ArmEtTypeEte
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
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
        ),
      // CM_OBJECT_TOKEN  ParentToken
      CM_NULL_TOKEN,
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      PACKAGE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (PackageResources),
      // CM_OBJECT_TOKEN  LpiToken
      CM_NULL_TOKEN
    },
    // Cluster 0
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[1]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
        ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[0]), // -> Package
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      CLUSTER0_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster0Resources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (ClustersLpiRef)
    },
    // Cluster 1
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[2]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
        ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[0]), // -> Package
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      CLUSTER1_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster1Resources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (ClustersLpiRef)
    },
    // Eight cores
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> Cluster 0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[0]),
      // UINT32  NoOfPrivateResources
      CLUSTER0_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster0CoreResources),
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> Cluster 0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[1]),
      // UINT32  NoOfPrivateResources
      CLUSTER0_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster0CoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> Cluster 0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[2]),
      // UINT32  NoOfPrivateResources
      CLUSTER0_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster0CoreResources),
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> Cluster 0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[3]),
      // UINT32  NoOfPrivateResources
      CLUSTER0_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster0CoreResources),
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
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> Cluster 1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[4]),
      // UINT32  NoOfPrivateResources
      CLUSTER1_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster1CoreResources),
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
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> Cluster 1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[5]),
      // UINT32  NoOfPrivateResources
      CLUSTER1_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster1CoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[9]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
        ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> Cluster 1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[6]),
      // UINT32  NoOfPrivateResources
      CLUSTER1_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster1CoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    },
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[10]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
        ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> Cluster 1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[7]),
      // UINT32  NoOfPrivateResources
      CLUSTER1_CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (Cluster1CoreResources),
      // CM_OBJECT_TOKEN  LpiToken
      REFERENCE_TOKEN (CoresLpiRef)
    }
  },

  // Cache information
  {
    // L3 cache
    {
      REFERENCE_TOKEN (CacheInfo[0]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x400000,                        // UINT32  Size
      4096,                            // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    },
    // L2 cache
    {
      REFERENCE_TOKEN (CacheInfo[1]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x80000,                         // UINT32  Size
      512,                             // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    },
    // L1 instruction cache
    {
      REFERENCE_TOKEN (CacheInfo[2]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    },
    // L1 data cache
    {
      REFERENCE_TOKEN (CacheInfo[3]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    },

    // L2 cache
    {
      REFERENCE_TOKEN (CacheInfo[4]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x80000,                         // UINT32  Size
      512,                             // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    },
    // L1 instruction cache
    {
      REFERENCE_TOKEN (CacheInfo[5]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    },
    // L1 data cache
    {
      REFERENCE_TOKEN (CacheInfo[6]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      0x8000,                          // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (
        // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
        ),
      64                               // UINT16  LineSize
    }
  },

  // Resources private to the package (shared among clusters)
  {
    { REFERENCE_TOKEN (CacheInfo[0]) }  // -> package's L3 cache
  },
  // Resources private to cluster 0 (shared among cores)
  {
    { REFERENCE_TOKEN (CacheInfo[1]) }  // -> cluster 1's L2 cache
  },
  // Resources private to each individual cluster 0 core instance
  {
    { REFERENCE_TOKEN (CacheInfo[2]) }, // -> cluster 0 core's L1 I-cache
    { REFERENCE_TOKEN (CacheInfo[3]) }  // -> cluster 0 core's L1 D-cache
  },
  // Resources private to cluster 1 (shared among cores)
  {
    { REFERENCE_TOKEN (CacheInfo[4]) }  // -> cluster 2's L2 cache
  },
  // Resources private to each individual cluster 1 core instance
  {
    { REFERENCE_TOKEN (CacheInfo[5]) }, // -> cluster 1 core's L1 I-cache
    { REFERENCE_TOKEN (CacheInfo[6]) }  // -> cluster 1 core's L1 D-cache
  },

  // Low Power Idle state information (LPI) for all cores/clusters
  // This structure currently contains dummy values
  {
    {             // LpiInfo[0] -> Clusters CluPwrDn
      2500,       // MinResidency
      1150,       // WorstCaseWakeLatency
      1,          // Flags
      1,          // ArchFlags
      100,        // ResCntFreq
      0,          // EnableParentState
      TRUE,       // IsInteger
      0x01000000, // IntegerEntryMethod
      // RegisterEntryMethod (NULL, use IntegerEntryMethod)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
      // ResidencyCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
      // UsageCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
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
        0x20,                                   // RegisterBitWidth
        0x00,                                   // RegisterBitOffset
        0x03,                                   // AccessSize
        0xFFFFFFFF                              // Address
      },
      // ResidencyCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
      // UsageCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
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
        0x20,                                   // RegisterBitWidth
        0x00,                                   // RegisterBitOffset
        0x03,                                   // AccessSize
        0x00010000                              // Address
      },
      // ResidencyCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
      // UsageCounterRegister (NULL)
      { EFI_ACPI_6_3_SYSTEM_MEMORY,     0, 0, 0, 0     },
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
  }
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
  IN        VOID                        *Object,
  IN  CONST UINTN                       ObjectSize,
  IN  CONST UINTN                       ObjectCount,
  IN  OUT   CM_OBJ_DESCRIPTOR   *CONST  CmObjectDesc
  )
{
  CmObjectDesc->ObjectId = CmObjectId;
  CmObjectDesc->Size     = ObjectSize;
  CmObjectDesc->Data     = (VOID *)Object;
  CmObjectDesc->Count    = ObjectCount;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN        VOID                                          *Object,
  IN  CONST UINTN                                         ObjectSize,
  IN  CONST UINTN                                         ObjectCount,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  CONST CM_OBJECT_HANDLER_PROC                        HandlerProc,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObjectDesc
  )
{
  EFI_STATUS  Status;

  CmObjectDesc->ObjectId = CmObjectId;
  if (Token == CM_NULL_TOKEN) {
    CmObjectDesc->Size  = ObjectSize;
    CmObjectDesc->Data  = (VOID *)Object;
    CmObjectDesc->Count = ObjectCount;
    Status              = EFI_SUCCESS;
  } else {
    Status = HandlerProc (This, CmObjectId, Token, CmObjectDesc);
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Token = 0x%p, CmObjectId = %x, Ptr = 0x%p, Size = %d, Count = %d\n",
    (VOID *)Token,
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  CONST CM_OBJECT_HANDLER_PROC                        HandlerProc,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObjectDesc
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
    (VOID *)Token,
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return Status;
}

/** Initialize the platform configuration repository.

  @param [in]  This        Pointer to the Configuration Manager Protocol.

  @retval
    EFI_SUCCESS   Success
**/
STATIC
EFI_STATUS
EFIAPI
InitializePlatformRepository (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINTN                           Index;
  UINT16                          TrbeInterrupt;
  CM_OBJECT_TOKEN                 EtToken;

  PlatformRepo = This->PlatRepoInfo;

  if (ArmHasGicV5SystemRegisters ()) {
    DEBUG ((DEBUG_ERROR, "ConfigurationManager: GICv5 not supported.\n"));
    return EFI_UNSUPPORTED;
  }

  PlatformRepo->SysId = MmioRead32 (ARM_VE_SYS_ID_REG);
  if ((PlatformRepo->SysId & ARM_FVP_SYS_ID_REV_MASK) ==
      ARM_FVP_BASE_REVC_REV)
  {
    // REVC affinity is shifted, update the MPIDR
    PlatformRepo->GicCInfo[0].MPIDR = GET_MPID_MT (0, 0, 0);
    PlatformRepo->GicCInfo[1].MPIDR = GET_MPID_MT (0, 1, 0);
    PlatformRepo->GicCInfo[2].MPIDR = GET_MPID_MT (0, 2, 0);
    PlatformRepo->GicCInfo[3].MPIDR = GET_MPID_MT (0, 3, 0);

    PlatformRepo->GicCInfo[4].MPIDR = GET_MPID_MT (1, 0, 0);
    PlatformRepo->GicCInfo[5].MPIDR = GET_MPID_MT (1, 1, 0);
    PlatformRepo->GicCInfo[6].MPIDR = GET_MPID_MT (1, 2, 0);
    PlatformRepo->GicCInfo[7].MPIDR = GET_MPID_MT (1, 3, 0);
  }

  TrbeInterrupt = 0;
  EtToken       = CM_NULL_TOKEN;

  // The ID_AA64DFR0_EL1.TraceBuffer field identifies support for FEAT_TRBE.
  if (ArmHasTrbe ()) {
    // TRBE Interrupt is PPI 15 on FVP model.
    TrbeInterrupt = 31;
  }

  // The ID_AA64DFR0_EL1.TraceVer field identifies the presence of FEAT_ETE.
  if (ArmHasEte ()) {
    EtToken = (CM_OBJECT_TOKEN)&PlatformRepo->EtInfo;
  }

  for (Index = 0; Index < PLAT_CPU_COUNT; Index++) {
    PlatformRepo->GicCInfo[Index].TrbeInterrupt = TrbeInterrupt;
    PlatformRepo->GicCInfo[Index].EtToken       = EtToken;
  }

  // Retrieve interrupts stored in PCDs
  PlatformRepo->Watchdog.TimerGSIV = PcdGet32 (PcdGenericWatchdogEl2IntrNum);

  return EFI_SUCCESS;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINT32                          TotalObjCount;
  UINT32                          ObjIndex;

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
      CmObject->Size     = sizeof (PlatformRepo->LpiInfo[ObjIndex]);
      CmObject->Data     = (VOID *)&PlatformRepo->LpiInfo[ObjIndex];
      CmObject->Count    = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

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
  CmObject->Size     = sizeof (PlatformRepo->GTBlock0TimerInfo);
  CmObject->Data     = (VOID *)&PlatformRepo->GTBlock0TimerInfo;
  CmObject->Count    = sizeof (PlatformRepo->GTBlock0TimerInfo) /
                       sizeof (PlatformRepo->GTBlock0TimerInfo[0]);
  return EFI_SUCCESS;
}

/** Return an ITS identifier array.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       A token for identifying the object
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
*/
EFI_STATUS
EFIAPI
GetItsIdentifierArray (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if (Token != (CM_OBJECT_TOKEN)&PlatformRepo->ItsIdentifierArray) {
    return EFI_NOT_FOUND;
  }

  CmObject->ObjectId = CmObjectId;
  CmObject->Size     = sizeof (PlatformRepo->ItsIdentifierArray);
  CmObject->Data     = (VOID *)&PlatformRepo->ItsIdentifierArray;
  CmObject->Count    = ARRAY_SIZE (PlatformRepo->ItsIdentifierArray);
  return EFI_SUCCESS;
}

/** Return a device Id mapping array.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       A token for identifying the object
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
*/
EFI_STATUS
EFIAPI
GetDeviceIdMappingArray (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if ((Token != (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[0]) &&
      (Token != (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[1]))
  {
    return EFI_NOT_FOUND;
  }

  CmObject->ObjectId = CmObjectId;
  CmObject->Size     = sizeof (CM_ARM_ID_MAPPING);
  CmObject->Data     = (VOID *)Token;
  CmObject->Count    = 1;
  return EFI_SUCCESS;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINT32                          TotalObjCount;
  UINT32                          ObjIndex;

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
      CmObject->Size     = sizeof (PlatformRepo->PciAddressMapInfo[ObjIndex]);
      CmObject->Data     = (VOID *)&PlatformRepo->PciAddressMapInfo[ObjIndex];
      CmObject->Count    = 1;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINT32                          TotalObjCount;
  UINT32                          ObjIndex;

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
      CmObject->Size     = sizeof (PlatformRepo->PciInterruptMapInfo[ObjIndex]);
      CmObject->Data     = (VOID *)&PlatformRepo->PciInterruptMapInfo[ObjIndex];
      CmObject->Count    = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINT32                          TotalObjCount;
  UINT32                          ObjIndex;

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
      CmObject->Size     = sizeof (PlatformRepo->GicCInfo[ObjIndex]);
      CmObject->Data     = (VOID *)&PlatformRepo->GicCInfo[ObjIndex];
      CmObject->Count    = 1;
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               SearchToken,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PciAddressMapRef) {
    CmObject->Size  = sizeof (PlatformRepo->PciAddressMapRef);
    CmObject->Data  = (VOID *)&PlatformRepo->PciAddressMapRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->PciAddressMapRef);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PciInterruptMapRef) {
    CmObject->Size  = sizeof (PlatformRepo->PciInterruptMapRef);
    CmObject->Data  = (VOID *)&PlatformRepo->PciInterruptMapRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->PciInterruptMapRef);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PackageResources) {
    CmObject->Size  = sizeof (PlatformRepo->PackageResources);
    CmObject->Data  = (VOID *)&PlatformRepo->PackageResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->PackageResources);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->Cluster0Resources) {
    CmObject->Size  = sizeof (PlatformRepo->Cluster0Resources);
    CmObject->Data  = (VOID *)&PlatformRepo->Cluster0Resources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->Cluster0Resources);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->Cluster0CoreResources) {
    CmObject->Size  = sizeof (PlatformRepo->Cluster0CoreResources);
    CmObject->Data  = (VOID *)&PlatformRepo->Cluster0CoreResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->Cluster0CoreResources);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->Cluster1Resources) {
    CmObject->Size  = sizeof (PlatformRepo->Cluster1Resources);
    CmObject->Data  = (VOID *)&PlatformRepo->Cluster1Resources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->Cluster1Resources);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->Cluster1CoreResources) {
    CmObject->Size  = sizeof (PlatformRepo->Cluster1CoreResources);
    CmObject->Data  = (VOID *)&PlatformRepo->Cluster1CoreResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->Cluster1CoreResources);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->ClustersLpiRef) {
    CmObject->Size  = sizeof (PlatformRepo->ClustersLpiRef);
    CmObject->Data  = (VOID *)&PlatformRepo->ClustersLpiRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->ClustersLpiRef);
    return EFI_SUCCESS;
  }

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->CoresLpiRef) {
    CmObject->Size  = sizeof (PlatformRepo->CoresLpiRef);
    CmObject->Data  = (VOID *)&PlatformRepo->CoresLpiRef;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->CoresLpiRef);
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINTN                           AcpiTableCount;

  Status = EFI_SUCCESS;
  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status         = EFI_NOT_FOUND;
  AcpiTableCount = ARRAY_SIZE (PlatformRepo->CmAcpiTableList);
  PlatformRepo   = This->PlatRepoInfo;

  if ((PlatformRepo->SysId & ARM_FVP_SYS_ID_REV_MASK) !=
      ARM_FVP_BASE_REVC_REV)
  {
    // The last 3 tables in the ACPI table list are for FVP RevC
    // Reduce the count by 3 if the platform is not FVP RevC
    AcpiTableCount -= 3;
  }

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
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->CmAcpiTableList,
                 sizeof (PlatformRepo->CmAcpiTableList),
                 AcpiTableCount,
                 CmObject
                 );
      break;

    default:
    {
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINTN                           PciConfigSpaceCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  if ((PlatformRepo->SysId & ARM_FVP_SYS_ID_REV_MASK) ==
      ARM_FVP_BASE_REVC_REV)
  {
    PciConfigSpaceCount = 1;
  } else {
    PciConfigSpaceCount = 0;
  }

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

 #ifdef HEADLESS_PLATFORM
    case EArchCommonObjFixedFeatureFlags:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FixedFeatureFlags,
                 sizeof (PlatformRepo->FixedFeatureFlags),
                 1,
                 CmObject
                 );
      break;
 #endif

    case EArchCommonObjPciConfigSpaceInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PciConfigInfo,
                 sizeof (PlatformRepo->PciConfigInfo),
                 PciConfigSpaceCount,
                 CmObject
                 );
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

    case EArchCommonObjCmRef:
      Status = HandleCmObjectSearchPlatformRepo (
                 This,
                 CmObjectId,
                 Token,
                 GetCmObjRefs,
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

    default:
    {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_INFO,
        "INFO: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  } // switch

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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINTN                           Smmuv3Count;
  UINTN                           ItsGroupCount;
  UINTN                           ItsIdentifierArrayCount;
  UINTN                           RootComplexCount;
  UINTN                           DeviceIdMappingArrayCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  if ((PlatformRepo->SysId & ARM_FVP_SYS_ID_REV_MASK) ==
      ARM_FVP_BASE_REVC_REV)
  {
    Smmuv3Count               = 1;
    ItsGroupCount             = 1;
    ItsIdentifierArrayCount   = ARRAY_SIZE (PlatformRepo->ItsIdentifierArray);
    RootComplexCount          = 1;
    DeviceIdMappingArrayCount = ARRAY_SIZE (PlatformRepo->DeviceIdMapping);
  } else {
    Smmuv3Count               = 0;
    ItsGroupCount             = 0;
    ItsIdentifierArrayCount   = 0;
    RootComplexCount          = 0;
    DeviceIdMappingArrayCount = 0;
  }

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
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->GTBlockInfo,
                 sizeof (PlatformRepo->GTBlockInfo),
                 ARRAY_SIZE (PlatformRepo->GTBlockInfo),
                 CmObject
                 );
      break;

    case EArmObjGTBlockTimerFrameInfo:
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

    case EArmObjGicRedistributorInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GicRedistInfo,
                 sizeof (PlatformRepo->GicRedistInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjGicItsInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GicItsInfo,
                 sizeof (PlatformRepo->GicItsInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjSmmuV3:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SmmuV3Info,
                 sizeof (PlatformRepo->SmmuV3Info),
                 Smmuv3Count,
                 CmObject
                 );
      break;

    case EArmObjItsGroup:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->ItsGroupInfo,
                 sizeof (PlatformRepo->ItsGroupInfo),
                 ItsGroupCount,
                 CmObject
                 );
      break;

    case EArmObjGicItsIdentifierArray:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->ItsIdentifierArray,
                 sizeof (PlatformRepo->ItsIdentifierArray),
                 ItsIdentifierArrayCount,
                 Token,
                 GetItsIdentifierArray,
                 CmObject
                 );
      break;

    case EArmObjRootComplex:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->RootComplexInfo,
                 sizeof (PlatformRepo->RootComplexInfo),
                 RootComplexCount,
                 CmObject
                 );
      break;

    case EArmObjIdMappingArray:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->DeviceIdMapping,
                 sizeof (PlatformRepo->DeviceIdMapping),
                 DeviceIdMappingArrayCount,
                 Token,
                 GetDeviceIdMappingArray,
                 CmObject
                 );
      break;

    case EArmObjEtInfo:
      if (Token == (CM_OBJECT_TOKEN)&PlatformRepo->EtInfo) {
        Status = HandleCmObject (
                   CmObjectId,
                   &PlatformRepo->EtInfo,
                   sizeof (PlatformRepo->EtInfo),
                   1,
                   CmObject
                   );
      }

      break;

    default:
    {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_INFO,
        "INFO: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }// switch

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
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration Manager
                                is less than the Object size for the requested
                                object.
**/
EFI_STATUS
EFIAPI
GetOemNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
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
    default:
    {
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
ArmVExpressPlatformGetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
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
    default:
    {
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
ArmVExpressPlatformSetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  return EFI_UNSUPPORTED;
}

/** A structure describing the configuration manager protocol interface.
*/
STATIC
CONST
EDKII_CONFIGURATION_MANAGER_PROTOCOL  VExpressPlatformConfigManagerProtocol = {
  CREATE_REVISION (1,           0),
  ArmVExpressPlatformGetObject,
  ArmVExpressPlatformSetObject,
  &VExpressPlatRepositoryInfo
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = InitializePlatformRepository (
             &VExpressPlatformConfigManagerProtocol
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to initialize the Platform Configuration Repository." \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiConfigurationManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&VExpressPlatformConfigManagerProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Install Configuration Manager Protocol." \
      " Status = %r\n",
      Status
      ));
  }

  return Status;
}
