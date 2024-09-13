/** @file
  Configuration Manager Dxe

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <NeoverseN1Soc.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "ConfigurationManager.h"
#include "N1SdpAcpiHeader.h"
#include "Platform.h"

extern struct EFI_ACPI_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE Hmat;

/** The platform configuration repository information.
*/
STATIC
EDKII_PLATFORM_REPOSITORY_INFO N1sdpRepositoryInfo = {
  // Configuration Manager information
  { CONFIGURATION_MANAGER_REVISION, CFG_MGR_OEM_ID },

  // ACPI Table List
  {
    // FADT Table
    {
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
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
      EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
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
      (EFI_ACPI_DESCRIPTION_HEADER*)dsdt_aml_code
    },
    // DBG2 Table
    {
      EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
      NULL
    },
    // PPTT Table
    {
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdPptt),
      NULL
    },
    // IORT Table
    {
      EFI_ACPI_6_3_IO_REMAPPING_TABLE_SIGNATURE,
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdIort),
      NULL
    },
    // PCI MCFG Table
    {
      EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
      NULL,
      SIGNATURE_64 ('A','R','M','N','1','S','D','P'),
      0x20181101
    },
    // SSDT table describing the PCI root complex
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdt),
      (EFI_ACPI_DESCRIPTION_HEADER*)ssdtpci_aml_code
    },
    // SRAT Table
    {
      EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE,
      EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSrat),
      NULL
    },
    // HMAT Table
    {
      EFI_ACPI_6_3_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE,
      EFI_ACPI_6_3_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdRaw),
      (EFI_ACPI_DESCRIPTION_HEADER*)&Hmat
    },
    // SSDT table describing the Remote Chip PCI root complex
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdt),
      (EFI_ACPI_DESCRIPTION_HEADER*)ssdtremotepci_aml_code
    },
  },

  // Boot architecture information
  { EFI_ACPI_6_3_ARM_PSCI_COMPLIANT },              // BootArchFlags

  // Fixed feature flag information
  { EFI_ACPI_6_3_HEADLESS },                        // Fixed feature flags

  // Power management profile information
  { EFI_ACPI_6_3_PM_PROFILE_ENTERPRISE_SERVER },    // PowerManagement Profile

  /* GIC CPU Interface information
     GICC_ENTRY (CPUInterfaceNumber, Mpidr, PmuIrq, VGicIrq, GicRedistBase,
                EnergyEfficiency, SpeIrq, ProximityDomain, ClockDomain)
  */
  {
    GICC_ENTRY (0, GET_MPID3 (0x0, 0x0, 0x0, 0x0), 23, 25, 0, 0, 21, 0, 0),
    GICC_ENTRY (1, GET_MPID3 (0x0, 0x0, 0x1, 0x0), 23, 25, 0, 0, 21, 0, 0),
    GICC_ENTRY (2, GET_MPID3 (0x0, 0x1, 0x0, 0x0), 23, 25, 0, 0, 21, 0, 0),
    GICC_ENTRY (3, GET_MPID3 (0x0, 0x1, 0x1, 0x0), 23, 25, 0, 0, 21, 0, 0),
    GICC_ENTRY (4, GET_MPID3 (0x1, 0x0, 0x0, 0x0), 23, 25, 0, 0, 21, 1, 0),
    GICC_ENTRY (5, GET_MPID3 (0x1, 0x0, 0x1, 0x0), 23, 25, 0, 0, 21, 1, 0),
    GICC_ENTRY (6, GET_MPID3 (0x1, 0x1, 0x0, 0x0), 23, 25, 0, 0, 21, 1, 0),
    GICC_ENTRY (7, GET_MPID3 (0x1, 0x1, 0x1, 0x0), 23, 25, 0, 0, 21, 1, 0),
  },

  // GIC Distributor Info
  {
    FixedPcdGet64 (PcdGicDistributorBase),  // UINT64  PhysicalBaseAddress
    0,                                      // UINT32  SystemVectorBase
    3                                       // UINT8   GicVersion
  },

  // GIC Re-Distributor Info
  {
    {
      // UINT64  DiscoveryRangeBaseAddress
      FixedPcdGet64 (PcdGicRedistributorsBase),
      // UINT32  DiscoveryRangeLength
      SIZE_1MB
    },
    {
      // UINT64  DiscoveryRangeBaseAddress
      FixedPcdGet64 (PcdGicRedistributorsBase) + (1ULL << 42),
      // UINT32  DiscoveryRangeLength
      SIZE_1MB
    },
  },

  // GIC ITS
  {
    // GIC ITS - CCIX TCU
    {
      // The GIC ITS ID.
      0,
      // The physical address for the Interrupt Translation Service
      0x30040000,
      //Proximity Domain
      0
    },
    // GIC ITS - PCIe TCU
    {
      // The GIC ITS ID.
      1,
      // The physical address for the Interrupt Translation Service
      0x30060000,
      //Proximity Domain
      0
    },
    // GIC ITS - CCIX RC
    {
      // The GIC ITS ID.
      2,
      // The physical address for the Interrupt Translation Service
      0x30080000,
      //Proximity Domain
      0
    },
    // GIC ITS - PCIe RC
    {
      // The GIC ITS ID.
      3,
      // The physical address for the Interrupt Translation Service
      0x300A0000,
      //Proximity Domain
      0
    },
    //Remote chip GIC ITS - PCIe TCU
    {
     Its_remote_smmu_pcie,
     0x40030060000,
     1
    },
    //Remote chip GIC ITS - PCIe RC
    {
     Its_remote_pcie,
     0x400300a0000,
     1
    },
  },

  // Generic Timer Info
  {
    // The physical base address for the counter control frame
    N1SDP_SYSTEM_TIMER_BASE_ADDRESS,
    // The physical base address for the counter read frame
    N1SDP_CNT_READ_BASE_ADDRESS,
    // The secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerSecIntrNum),
    // The secure PL1 timer flags
    N1SDP_GTDT_GTIMER_FLAGS,
    // The non-secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerIntrNum),
    // The non-secure PL1 timer flags
    N1SDP_GTDT_GTIMER_FLAGS,
    // The virtual timer interrupt
    FixedPcdGet32 (PcdArmArchTimerVirtIntrNum),
    // The virtual timer flags
    N1SDP_GTDT_GTIMER_FLAGS,
    // The non-secure PL2 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerHypIntrNum),
    // The non-secure PL2 timer flags
    N1SDP_GTDT_GTIMER_FLAGS
  },

  // Generic Timer Block Information
  {
    {
      // The physical base address for the GT Block Timer structure
      N1SDP_GT_BLOCK_CTL_BASE,
      // The number of timer frames implemented in the GT Block
      N1SDP_TIMER_FRAMES_COUNT,
      // Reference token for the GT Block timer frame list
      REFERENCE_TOKEN (GTBlock0TimerInfo)
    }
  },

  // GT Block Timer Frames
  {
    // Frame 0
    {
      0,                                  // UINT8   FrameNumber
      N1SDP_GT_BLOCK_FRAME0_CTL_BASE,     // UINT64  PhysicalAddressCntBase
      N1SDP_GT_BLOCK_FRAME0_CTL_EL0_BASE, // UINT64  PhysicalAddressCntEL0Base
      N1SDP_GT_BLOCK_FRAME0_GSIV,         // UINT32  PhysicalTimerGSIV
      N1SDP_GTX_TIMER_FLAGS,              // UINT32  PhysicalTimerFlags
      0,                                  // UINT32  VirtualTimerGSIV
      0,                                  // UINT32  VirtualTimerFlags
      N1SDP_GTX_COMMON_FLAGS_NS           // UINT32  CommonFlags
    },
    // Frame 1
    {
      1,                                  // UINT8   FrameNumber
      N1SDP_GT_BLOCK_FRAME1_CTL_BASE,     // UINT64  PhysicalAddressCntBase
      N1SDP_GT_BLOCK_FRAME1_CTL_EL0_BASE, // UINT64  PhysicalAddressCntEL0Base
      N1SDP_GT_BLOCK_FRAME1_GSIV,         // UINT32  PhysicalTimerGSIV
      N1SDP_GTX_TIMER_FLAGS,              // UINT32  PhysicalTimerFlags
      0,                                  // UINT32  VirtualTimerGSIV
      0,                                  // UINT32  VirtualTimerFlags
      N1SDP_GTX_COMMON_FLAGS_S            // UINT32  CommonFlags
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
    N1SDP_SBSA_WATCHDOG_FLAGS
  },

  // SPCR Serial Port
  {
    FixedPcdGet64 (PcdSerialRegisterBase),                  // BaseAddress
    FixedPcdGet32 (PL011UartInterrupt),                     // Interrupt
    FixedPcdGet64 (PcdUartDefaultBaudRate),                 // BaudRate
    FixedPcdGet32 (PL011UartClkInHz),                       // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART        // Port subtype
  },

  // Debug Serial Port
  {
    FixedPcdGet64 (PcdSerialDbgRegisterBase),               // BaseAddress
    0,                                                      // Interrupt -unused
    FixedPcdGet64 (PcdSerialDbgUartBaudRate),               // BaudRate
    FixedPcdGet32 (PcdSerialDbgUartClkInHz),                // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART        // Port subtype
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
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      CM_NULL_TOKEN,
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      SOC_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (SocResources)
    },
    // Cluster0
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
      CLUSTER_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (ClusterResources)
    },
    // Cluster1
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
      CLUSTER_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (ClusterResources)
    },
    // Cluster0 - Cpu0
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> 'cluster in Cluster0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[0]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Cluster0 - Cpu1
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> 'cluster in Cluster0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[1]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Cluster1 - Cpu0
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
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> 'cluster in Cluster1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[2]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Cluster1 - Cpu1
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
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> 'cluster in Cluster1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[3]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Slave chip hierarchy
    // Package
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[7]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      CM_NULL_TOKEN,
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      SOC_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (SocResources)
    },
    // Cluster0
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[8]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[7]), // -> Package
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      CLUSTER_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (ClusterResources)
    },
    // Cluster1
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[9]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[7]), // -> Package
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      CLUSTER_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (ClusterResources)
    },
    // Cluster0 - Cpu0
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
      REFERENCE_TOKEN (ProcHierarchyInfo[8]), // -> 'cluster in Cluster0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[4]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Cluster0 - Cpu1
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[11]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[8]), // -> 'cluster in Cluster0
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[5]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Cluster1 - Cpu0
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
      REFERENCE_TOKEN (ProcHierarchyInfo[9]), // -> 'cluster in Cluster1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[6]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
    // Cluster1 - Cpu1
    {
      // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (ProcHierarchyInfo[11]),
      // UINT32  Flags
      PROC_NODE_FLAGS (
        EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL,
        EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID,
        EFI_ACPI_6_3_PPTT_PROCESSOR_IS_NOT_THREAD,
        EFI_ACPI_6_3_PPTT_NODE_IS_LEAF,
        EFI_ACPI_6_3_PPTT_IMPLEMENTATION_NOT_IDENTICAL
      ),
      // CM_OBJECT_TOKEN  ParentToken
      REFERENCE_TOKEN (ProcHierarchyInfo[9]), // -> 'cluster in Cluster1
      // CM_OBJECT_TOKEN  AcpiIdObjectToken
      REFERENCE_TOKEN (GicCInfo[7]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
  },

  // Cache information
  {
    // 'cluster's L3 cache
    {
      REFERENCE_TOKEN (CacheInfo[0]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_1MB,                        // UINT32  Size
      2048,                            // UINT32  NumberOfSets
      8,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'core's L1 instruction cache
    {
      REFERENCE_TOKEN (CacheInfo[1]),  // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (CacheInfo[3]),  // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_64KB,                       // UINT32  Size
      256,                             // UINT32  NumberOfSets
      4,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // 'core's L1 data cache
    {
      REFERENCE_TOKEN (CacheInfo[2]),  // CM_OBJECT_TOKEN  Token
      REFERENCE_TOKEN (CacheInfo[3]),  // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_64KB,                       // UINT32  Size
      256,                             // UINT32  NumberOfSets
      4,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // cores's L2 cache
    {
      REFERENCE_TOKEN (CacheInfo[3]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_1MB,                        // UINT32  Size
      2048,                            // UINT32  NumberOfSets
      8,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // slc cache
    {
      REFERENCE_TOKEN (CacheInfo[4]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_8MB,                        // UINT32  Size
      8192,                            // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
  },

  // Resources private to the 'cluster (shared among cores) in Cluster
  {
    { REFERENCE_TOKEN (CacheInfo[0]) }  // -> 'cluster's L3 cache in Cluster
  },

  // Resources private to each individual 'core instance in Cluster
  {
    { REFERENCE_TOKEN (CacheInfo[1]) }, // -> 'core's L1 I-cache in Cluster
    { REFERENCE_TOKEN (CacheInfo[2]) }  // -> 'core's L1 D-cache in Cluster
  },

  // Resources private to the SoC
  {
    { REFERENCE_TOKEN (CacheInfo[4])   },  // -> slc for SoC
  },

  // ITS group node
  {
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (ItsGroupInfo[Its_smmu_ccix]),
      // The number of ITS identifiers in the ITS node.
      1,
      // Reference token for the ITS identifier array
      REFERENCE_TOKEN (ItsIdentifierArray[Its_smmu_pcie])
    },
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (ItsGroupInfo[Its_smmu_pcie]),
      // The number of ITS identifiers in the ITS node.
      1,
      // Reference token for the ITS identifier array
      REFERENCE_TOKEN (ItsIdentifierArray[Its_smmu_pcie])
    },
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (ItsGroupInfo[Its_ccix]),
      // The number of ITS identifiers in the ITS node.
      1,
      // Reference token for the ITS identifier array
      REFERENCE_TOKEN (ItsIdentifierArray[Its_ccix])
    },
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (ItsGroupInfo[Its_pcie]),
      // The number of ITS identifiers in the ITS node.
      1,
      // Reference token for the ITS identifier array
      REFERENCE_TOKEN (ItsIdentifierArray[Its_pcie])
    },
    //Remote Chip ITS
    {
      REFERENCE_TOKEN (ItsGroupInfo[Its_remote_smmu_pcie]),
      1,
      REFERENCE_TOKEN (ItsIdentifierArray[Its_remote_smmu_pcie])
    },
    {
      REFERENCE_TOKEN (ItsGroupInfo[Its_remote_pcie]),
      1,
      REFERENCE_TOKEN (ItsIdentifierArray[Its_remote_pcie])
    },
  },

  // ITS identifier array
  {
    {
      // The ITS Identifier - 0
      Its_smmu_ccix
    },
    {
      // The ITS Identifier - 1
      Its_smmu_pcie
    },
    {
      // The ITS Identifier - 2
      Its_ccix
    },
    {
      // The ITS Identifier - 3
      Its_pcie
    },
    {
      // The ITS Identifier - 4
      Its_remote_smmu_pcie
    },
    {
      // The ITS Identifier - 5
      Its_remote_pcie
    }
  },

  {
    // SMMUv3 Node
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (SmmuV3Info[Smmuv3info_pcie]),
      // Number of ID mappings
      2,
      // Reference token for the ID mapping array
      REFERENCE_TOKEN (DeviceIdMapping[Devicemapping_smmu_pcie][0]),
      // SMMU Base Address
      0x4F400000,
      // SMMU flags
      EFI_ACPI_IORT_SMMUv3_FLAG_COHAC_OVERRIDE,
      // VATOS address
      0,
      // Model
      EFI_ACPI_IORT_SMMUv3_MODEL_GENERIC,
      // GSIV of the Event interrupt if SPI based
      0x10B,
      // PRI Interrupt if SPI based
      0,
      // GERR interrupt if GSIV based
      0x10D,
      // Sync interrupt if GSIV based
      0x10C,
      // Proximity domain flag, ignored in this case
      0,
      // Index into the array of ID mapping
      1
    },
    // SMMUv3 Node
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (SmmuV3Info[Smmuv3info_ccix]),
      // Number of ID mappings
      2,
      // Reference token for the ID mapping array
      REFERENCE_TOKEN (DeviceIdMapping[Devicemapping_smmu_ccix][0]),
      // SMMU Base Address
      0x4F000000,
      // SMMU flags
      EFI_ACPI_IORT_SMMUv3_FLAG_COHAC_OVERRIDE,
      // VATOS address
      0,
      // Model
      EFI_ACPI_IORT_SMMUv3_MODEL_GENERIC,
      // GSIV of the Event interrupt if SPI based
      0x104,
      // PRI Interrupt if SPI based
      0,
      // GERR interrupt if GSIV based
      0x106,
      // Sync interrupt if GSIV based
      0x105,
      // Proximity domain flag, ignored in this case
      0,
      // Index into the array of ID mapping
      1
    },
    //Remote Chip SMMU V3 setting
    {
      REFERENCE_TOKEN (SmmuV3Info[Smmuv3info_remote_pcie]),
      2,
      REFERENCE_TOKEN (DeviceIdMapping[Devicemapping_remote_smmu_pcie][0]),
      0x4004f400000,
      EFI_ACPI_IORT_SMMUv3_FLAG_COHAC_OVERRIDE,
      0,
      EFI_ACPI_IORT_SMMUv3_MODEL_GENERIC,
      747,
      0,
      749,
      748,
      0,
      1
    }
  },

  {
    // Root Complex node info
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (RootComplexInfo[0]),
      // Number of ID mappings
      1,
      // Reference token for the ID mapping array
      REFERENCE_TOKEN (DeviceIdMapping[Devicemapping_pcie][0]),
      // Memory access properties : Cache coherent attributes
      EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA,
      // Memory access properties : Allocation hints
      0,
      // Memory access properties : Memory access flags
      0,
      // ATS attributes
      EFI_ACPI_IORT_ROOT_COMPLEX_ATS_SUPPORTED,
      // PCI segment number
      0,
      // Memory address size limit
      42
    },
      // Root Complex node info
    {
      // Reference token for this Iort node
      REFERENCE_TOKEN (RootComplexInfo[1]),
      // Number of ID mappings
      1,
      // Reference token for the ID mapping array
      REFERENCE_TOKEN (DeviceIdMapping[Devicemapping_pcie][1]),
      // Memory access properties : Cache coherent attributes
      EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA,
      // Memory access properties : Allocation hints
      0,
      // Memory access properties : Memory access flags
      0,
      // ATS attributes
      EFI_ACPI_IORT_ROOT_COMPLEX_ATS_SUPPORTED,
      // PCI segment number
      1,
      // Memory address size limit
      42
    },
     //Remote Chip Root Complex node Info
    {
      REFERENCE_TOKEN (RootComplexInfo[Root_remote_pcie]),
      1,
      REFERENCE_TOKEN (DeviceIdMapping[Devicemapping_remote_pcie][0]),
      EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA,
      0,
      0,
      EFI_ACPI_IORT_ROOT_COMPLEX_ATS_SUPPORTED,
      2,
      42
    }
  },

  // Array of Device ID mappings
  {
    // DeviceIdMapping[0][0] - [0][1]
    {
      // Mapping SMMUv3 -> ITS Group
      // SMMUv3 device ID mapping
      {
        // Input base
        0x0,
        // Number of input IDs
        0x0000FFFF,
        // Output Base
        0x0,
        // Output reference
        REFERENCE_TOKEN (ItsGroupInfo[Its_pcie]),
        // Flags
        0
      },
      // SMMUv3 device ID mapping
      {
        // Input base
        0x0,
        // Number of input IDs
        0x00000001,
        // Output Base
        0x0,
        // Output reference token for the IORT node
        REFERENCE_TOKEN (ItsGroupInfo[Its_smmu_pcie]),
        // Flags
        EFI_ACPI_IORT_ID_MAPPING_FLAGS_SINGLE
      }
    },
    // DeviceIdMapping[1][0] - [1][1]
    {
      // Mapping SMMUv3 -> ITS Group
      // SMMUv3 device ID mapping
      {
        // Input base
        0x0,
        // Number of input IDs
        0x0000FFFF,
        // Output Base
        0x0,
        // Output reference
        REFERENCE_TOKEN (ItsGroupInfo[Its_ccix]),
        // Flags
        0
      },
      // SMMUv3 device ID mapping
      {
        // Input base
        0x0,
        // Number of input IDs
        0x00000001,
        // Output Base
        0x0,
        // Output reference token for the IORT node
        REFERENCE_TOKEN (ItsGroupInfo[Its_smmu_ccix]),
        // Flags
        EFI_ACPI_IORT_ID_MAPPING_FLAGS_SINGLE
      }
    },
    // DeviceIdMapping[2][0] - [2][1]
    {
      // Mapping for  RootComplex -> SMMUv3
      // Device ID mapping for Root complex node
      {
        // Input base
        0x0,
        // Number of input IDs
        0x0000FFFF,
        // Output Base
        0x0,
        // Output reference
        REFERENCE_TOKEN (SmmuV3Info[Smmuv3info_pcie]),
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
        REFERENCE_TOKEN (SmmuV3Info[Smmuv3info_ccix]),
        // Flags
        0
      }
    },
      // Mapping of Remote Chip SMMUv3 -> ITS Group
    {
      {
        0x0,
        0x0000ffff,
        0x0,
        REFERENCE_TOKEN (ItsGroupInfo[Its_remote_pcie]),
        0
      },
      {
        0x0,
        0x00000001,
        0x0,
        REFERENCE_TOKEN (ItsGroupInfo[Its_remote_smmu_pcie]),
        EFI_ACPI_IORT_ID_MAPPING_FLAGS_SINGLE
      }
    },
      // Mapping for  Remote Chip RootComplex -> SMMUv3
    {
      {
        0x0,
        0x0000ffff,
        0x0,
        REFERENCE_TOKEN (SmmuV3Info[Smmuv3info_remote_pcie]),
        0
      }
    },
  },

  // PCI Configuration Space Info
  {
    // PCIe ECAM
    {
      FixedPcdGet64 (PcdPcieExpressBaseAddress),   // Base Address
      FixedPcdGet32 (PcdPcieSegmentNumber),        // Segment Group Number
      FixedPcdGet32 (PcdPcieBusMin),               // Start Bus Number
      FixedPcdGet32 (PcdPcieBusMax)                // End Bus Number
    },
    // CCIX ECAM
    {
      FixedPcdGet32 (PcdCcixExpressBaseAddress),   // Base Address
      FixedPcdGet32 (PcdCcixSegmentNumber),        // Segment Group Number
      FixedPcdGet32 (PcdCcixBusMin),               // Start Bus Number
      FixedPcdGet32 (PcdCcixBusMax)                // End Bus Number
    },
    //Remote Chip PCIe ECAM
    {
      FixedPcdGet64 (PcdRemotePcieBaseAddress),   // Base Address
      FixedPcdGet32 (PcdRemotePcieSegmentNumber),        // Segment Group Number
      FixedPcdGet32 (PcdRemotePcieBusMin),               // Start Bus Number
      FixedPcdGet32 (PcdRemotePcieBusMax)                // End Bus Number
    }
  },

  // Memory Affinity Info
  {
    {
      // Proximity domain to which memory range belongs
      0,
      //Base Address
      0x80000000,
      //Length
      0x80000000,
      //Flags
      EFI_ACPI_6_3_MEMORY_ENABLED
    },
    {
      // Proximity domain to which memory range belongs
      0,
      //Base Address
      0x8080000000,
      //Length is updated dynamically from SRAM
      0,
      //Flags
      EFI_ACPI_6_3_MEMORY_ENABLED
    }
  }

};

/** A helper function for returning the Configuration Manager Objects.
  @param [in]       CmObjectId     The Configuration Manager Object ID.
  @param [in]       Object         Pointer to the Object(s).
  @param [in]       ObjectSize     Total size of the Object(s).
  @param [in]       ObjectCount    Number of Objects.
  @param [in, out]  CmObjectDesc   Pointer to the Configuration Manager Object
                                   descriptor describing the requested Object.
  @retval EFI_SUCCESS              Success.
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
    CmObjectId,
    (VOID*)Token,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return Status;
}

/** Initialize the Platform Configuration Repository.
  @param [in]  PlatRepoInfo   Pointer to the Configuration Manager Protocol.
  @retval EFI_SUCCESS           Success
**/
STATIC
EFI_STATUS
EFIAPI
InitializePlatformRepository (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  * CONST PlatRepoInfo
  )
{
  UINT64                        Dram2Size;
  UINT64                        RemoteDdrSize;
  VOID                          *PlatInfoHob;

  PlatInfoHob = GetFirstGuidHob (&gArmNeoverseN1SocPlatformInfoDescriptorGuid);

  if (PlatInfoHob == NULL) {
    DEBUG ((DEBUG_ERROR, "Platform HOB is NULL\n"));
    return EFI_NOT_FOUND;
  }

  PlatRepoInfo->PlatInfo = (NEOVERSEN1SOC_PLAT_INFO *)GET_GUID_HOB_DATA (PlatInfoHob);

  RemoteDdrSize = 0;

  Dram2Size = ((PlatRepoInfo->PlatInfo->LocalDdrSize - 2) * SIZE_1GB);

  PlatRepoInfo->MemAffInfo[LOCAL_DDR_REGION2].Length = Dram2Size;

  if (PlatRepoInfo->PlatInfo->MultichipMode == 1) {
    RemoteDdrSize = ((UINT64)(PlatRepoInfo->PlatInfo->RemoteDdrSize - 2) * SIZE_1GB);

    // Update Remote DDR Region1
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION1].ProximityDomain = 1;
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION1]. \
      BaseAddress = FixedPcdGet64 (PcdExtMemorySpace)
                    + FixedPcdGet64 (PcdSystemMemoryBase);
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION1]. \
      Length = FixedPcdGet64 (PcdSystemMemorySize);
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION1]. \
      Flags = EFI_ACPI_6_3_MEMORY_ENABLED;

    // Update Remote DDR Region2
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION2]. \
      ProximityDomain = 1;
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION2]. \
      BaseAddress = FixedPcdGet64 (PcdExtMemorySpace)
                    + FixedPcdGet64 (PcdDramBlock2Base);
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION2]. \
      Length = RemoteDdrSize;
    PlatRepoInfo->MemAffInfo[REMOTE_DDR_REGION2]. \
      Flags = EFI_ACPI_6_3_MEMORY_ENABLED;
  }

  return EFI_SUCCESS;
}

/** Return a GT Block timer frame info list.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       A token for identifying the object
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
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
  CmObject->Count = ARRAY_SIZE (PlatformRepo->GTBlock0TimerInfo);
  return EFI_SUCCESS;
}

/** Return an ITS identifier array.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       A token for identifying the object
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetItsIdentifierArray (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINTN                             Count;
  UINTN                             Index;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  Count = ARRAY_SIZE (PlatformRepo->ItsIdentifierArray);

  for (Index = 0; Index < Count; Index++) {
    if (Token == (CM_OBJECT_TOKEN)&PlatformRepo->ItsIdentifierArray[Index]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->ItsIdentifierArray[0]);
      CmObject->Data = (VOID*)&PlatformRepo->ItsIdentifierArray[Index];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return an ITS group info.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       A token for identifying the object
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetItsGroupInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINTN                             Count;
  UINTN                             Index;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  Count = ARRAY_SIZE (PlatformRepo->ItsGroupInfo);

  for (Index = 0; Index < Count; Index++) {
    if (Token == (CM_OBJECT_TOKEN)&PlatformRepo->ItsGroupInfo[Index]) {
      CmObject->ObjectId = CmObjectId;
      CmObject->Size = sizeof (PlatformRepo->ItsGroupInfo[0]);
      CmObject->Data = (VOID*)&PlatformRepo->ItsGroupInfo[Index];
      CmObject->Count = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Return a device Id mapping array.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       A token for identifying the object
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetDeviceIdMappingArray (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  * PlatformRepo;
  UINTN                             Count;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  DEBUG ((DEBUG_INFO, "DeviceIdMapping - Token = %p\n"));

  if (Token == (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[0][0]) {
    Count = 2;
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Found DeviceIdMapping[0][0]\n"));
  } else if (Token ==
             (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[1][0]) {
    Count = 2;
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Found DeviceIdMapping[1][0]\n"));
  } else if (Token ==
             (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[2][0]) {
    Count  = 1;
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Found DeviceIdMapping[2][0]\n"));
  } else if (Token ==
             (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[2][1]) {
    Count  = 1;
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Found DeviceIdMapping[2][1]\n"));
  } else if (Token ==
             (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[Devicemapping_remote_smmu_pcie][0]) {
    Count = 2;
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Found DeviceIdMapping[Devicemapping_remote_smmu_pcie][0]\n"));
  } else if (Token ==
             (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[Devicemapping_remote_pcie][0]) {
    Count  = 1;
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Found DeviceIdMapping[Devicemapping_remote_pcie][0]\n"));
  } else {
    DEBUG ((DEBUG_INFO, "DeviceIdMapping - Not Found\n"));
    return EFI_NOT_FOUND;
  }

  CmObject->Data = (VOID*)Token;
  CmObject->ObjectId = CmObjectId;
  CmObject->Count = Count;
  CmObject->Size = Count * sizeof (CM_ARM_ID_MAPPING);

  return EFI_SUCCESS;
}

/** Return GIC CPU Interface Info.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Object ID of the CM object requested
  @param [in]        SearchToken A unique token for identifying the requested
                                 CM_ARM_GICC_INFO object.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
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

  if (PlatformRepo->PlatInfo->MultichipMode == 1) {
    TotalObjCount = PLAT_CPU_COUNT * 2;
  } else {
    TotalObjCount = PLAT_CPU_COUNT;
  }

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

/** Return a list of Configuration Manager object references pointed to by the
    given input token.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Object ID of the CM object requested
  @param [in]        SearchToken A unique token for identifying the requested
                                 CM_ARCH_COMMON_OBJ_REF list.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
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

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->ClusterResources) {
    CmObject->Size = sizeof (PlatformRepo->ClusterResources);
    CmObject->Data = (VOID*)&PlatformRepo->ClusterResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->ClusterResources);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->CoreResources) {
    CmObject->Size = sizeof (PlatformRepo->CoreResources);
    CmObject->Data = (VOID*)&PlatformRepo->CoreResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->CoreResources);
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->SocResources) {
    CmObject->Size = sizeof (PlatformRepo->SocResources);
    CmObject->Data = (VOID*)&PlatformRepo->SocResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->SocResources);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/** Return a standard namespace object.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
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
  UINT32                            AcpiTableCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;
  AcpiTableCount = ARRAY_SIZE (PlatformRepo->CmAcpiTableList);
  if (PlatformRepo->PlatInfo->MultichipMode == 0)
        AcpiTableCount -= 1;

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
  UINT32                            PciConfigInfoCount;
  UINT32                            ProcHierarchyInfoCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  if (PlatformRepo->PlatInfo->MultichipMode == 1) {
    PciConfigInfoCount = Root_pcie_max;
    ProcHierarchyInfoCount = PLAT_PROC_HIERARCHY_NODE_COUNT * 2;
  } else {
    PciConfigInfoCount = Root_pcie_master_chip_max;
    ProcHierarchyInfoCount = PLAT_PROC_HIERARCHY_NODE_COUNT;
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

    case  EArchCommonObjSerialDebugPortInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->DbgSerialPort,
                 sizeof (PlatformRepo->DbgSerialPort),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjFixedFeatureFlags:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FixedFeatureFlags,
                 sizeof (PlatformRepo->FixedFeatureFlags),
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
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->PciConfigInfo,
                 sizeof (PlatformRepo->PciConfigInfo),
                 PciConfigInfoCount,
                 CmObject
                 );
      break;

    case EArchCommonObjProcHierarchyInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->ProcHierarchyInfo,
                 sizeof (PlatformRepo->ProcHierarchyInfo),
                 ProcHierarchyInfoCount,
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

    case EArchCommonObjMemoryAffinityInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->MemAffInfo,
                 sizeof (PlatformRepo->MemAffInfo),
                 ARRAY_SIZE (PlatformRepo->MemAffInfo),
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

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
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
  UINT32                            GicRedistCount;
  UINT32                            GicCpuCount;
  UINT32                            GicItsInfoCount;
  UINT32                            ItsGroupInfoCount;
  UINT32                            ItsIdentifierArrayCount;
  UINT32                            SmmuV3InfoCount;
  UINT32                            DeviceIdMappingCount;
  UINT32                            RootComplexInfoCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  // Probe for multi chip information
  if (PlatformRepo->PlatInfo->MultichipMode == 1) {
    GicRedistCount = 2;
    GicCpuCount = PLAT_CPU_COUNT * 2;
    GicItsInfoCount = Its_max;
    ItsGroupInfoCount = Its_max;
    ItsIdentifierArrayCount = Its_max;
    SmmuV3InfoCount = Smmuv3info_max;
    DeviceIdMappingCount = Devicemapping_max;
    RootComplexInfoCount = Root_pcie_max;
  } else {
    GicRedistCount = 1;
    GicCpuCount = PLAT_CPU_COUNT;
    GicItsInfoCount = Its_master_chip_max;
    ItsGroupInfoCount = Its_master_chip_max;
    ItsIdentifierArrayCount = Its_master_chip_max;
    SmmuV3InfoCount = Smmuv3info_master_chip_max;
    DeviceIdMappingCount = Devicemapping_master_chip_max;
    RootComplexInfoCount = Root_pcie_master_chip_max;
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
                 GicCpuCount,
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
                 PlatformRepo->GicRedistInfo,
                 sizeof (PlatformRepo->GicRedistInfo),
                 GicRedistCount,
                 CmObject
                 );
      break;

    case EArmObjGicItsInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->GicItsInfo,
                 sizeof (PlatformRepo->GicItsInfo),
                 GicItsInfoCount,
                 CmObject
                 );
      break;

    case EArmObjSmmuV3:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->SmmuV3Info,
                 sizeof (PlatformRepo->SmmuV3Info),
                 SmmuV3InfoCount,
                 CmObject
                 );
      break;

    case EArmObjItsGroup:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->ItsGroupInfo,
                 sizeof (PlatformRepo->ItsGroupInfo),
                 ItsGroupInfoCount,
                 Token,
                 GetItsGroupInfo,
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
                 PlatformRepo->RootComplexInfo,
                 sizeof (PlatformRepo->RootComplexInfo),
                 RootComplexInfoCount,
                 CmObject
                 );
      break;

    case EArmObjIdMappingArray:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->DeviceIdMapping,
                 sizeof (PlatformRepo->DeviceIdMapping),
                 DeviceIdMappingCount,
                 Token,
                 GetDeviceIdMappingArray,
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

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
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

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
N1sdpPlatformGetObject (
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

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in]        CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the Object.

  @retval EFI_UNSUPPORTED        This operation is not supported.
**/
EFI_STATUS
EFIAPI
N1sdpPlatformSetObject (
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
EDKII_CONFIGURATION_MANAGER_PROTOCOL N1sdpPlatformConfigManagerProtocol = {
  CREATE_REVISION(1,0),
  N1sdpPlatformGetObject,
  N1sdpPlatformSetObject,
  &N1sdpRepositoryInfo
};

/**
  Entrypoint of Configuration Manager Dxe.

  @param [in]  ImageHandle
  @param [in]  SystemTable

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

  // Initialize the Platform Configuration Repository before installing the
  // Configuration Manager Protocol
  Status = InitializePlatformRepository (
             N1sdpPlatformConfigManagerProtocol.PlatRepoInfo
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to initialize the Platform Configuration Repository." \
      " Status = %r\n",
      Status
      ));
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiConfigurationManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID*)&N1sdpPlatformConfigManagerProtocol
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

error_handler:
  return Status;
}
