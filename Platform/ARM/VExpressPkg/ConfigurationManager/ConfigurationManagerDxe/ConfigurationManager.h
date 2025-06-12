/** @file

  Copyright (c) 2017 - 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_H__
#define CONFIGURATION_MANAGER_H__

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  dsdt_aml_code[];

/** The configuration manager version.
*/
#define CONFIGURATION_MANAGER_REVISION CREATE_REVISION (1, 0)

/** The OEM ID
*/
#define CFG_MGR_OEM_ID    { 'A', 'R', 'M', 'L', 'T', 'D' }

/** A helper macro for populating the GIC CPU information
*/
#define GICC_ENTRY(                                                      \
          CPUInterfaceNumber,                                            \
          Mpidr,                                                         \
          PmuIrq,                                                        \
          VGicIrq,                                                       \
          EnergyEfficiency                                               \
          ) {                                                            \
    CPUInterfaceNumber,       /* UINT32  CPUInterfaceNumber           */ \
    CPUInterfaceNumber,       /* UINT32  AcpiProcessorUid             */ \
    EFI_ACPI_6_2_GIC_ENABLED, /* UINT32  Flags                        */ \
    0,                        /* UINT32  ParkingProtocolVersion       */ \
    PmuIrq,                   /* UINT32  PerformanceInterruptGsiv     */ \
    0,                        /* UINT64  ParkedAddress                */ \
    FixedPcdGet64 (                                                      \
      PcdGicInterruptInterfaceBase                                       \
      ),                      /* UINT64  PhysicalBaseAddress          */ \
    0,                        /* UINT64  GICV                         */ \
    0,                        /* UINT64  GICH                         */ \
    VGicIrq,                  /* UINT32  VGICMaintenanceInterrupt     */ \
    0,                        /* UINT64  GICRBaseAddress              */ \
    Mpidr,                    /* UINT64  MPIDR                        */ \
    EnergyEfficiency          /* UINT8   ProcessorPowerEfficiencyClass*/ \
    }

/** A helper macro for populating the Processor Hierarchy Node flags
*/
#define PROC_NODE_FLAGS(                                                \
          PhysicalPackage,                                              \
          AcpiProcessorIdValid,                                         \
          ProcessorIsThread,                                            \
          NodeIsLeaf,                                                   \
          IdenticalImplementation                                       \
          )                                                             \
  (                                                                     \
    PhysicalPackage |                                                   \
    (AcpiProcessorIdValid << 1) |                                       \
    (ProcessorIsThread << 2) |                                          \
    (NodeIsLeaf << 3) |                                                 \
    (IdenticalImplementation << 4)                                      \
  )

/** A helper macro for populating the Cache Type Structure's attributes
*/
#define CACHE_ATTRIBUTES(                                               \
          AllocationType,                                               \
          CacheType,                                                    \
          WritePolicy                                                   \
          )                                                             \
  (                                                                     \
    AllocationType |                                                    \
    (CacheType << 2) |                                                  \
    (WritePolicy << 4)                                                  \
  )

/** A function that prepares Configuration Manager Objects for returning.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       A token for identifying the object.
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
typedef EFI_STATUS (*CM_OBJECT_HANDLER_PROC) (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     * CONST CmObject
  );

/** A helper macro for mapping a reference token.
*/
#define REFERENCE_TOKEN(Field)                                    \
          (CM_OBJECT_TOKEN)((UINT8*)&VExpressPlatRepositoryInfo + \
           OFFSET_OF (EDKII_PLATFORM_REPOSITORY_INFO, Field))

/** Macro to return MPIDR for Multi Threaded Cores
*/
#define GET_MPID_MT(Cluster, Core, Thread)                        \
          (((Cluster) << 16) | ((Core) << 8) | (Thread))

/** The number of CPUs
*/
#define PLAT_CPU_COUNT              8

/** The number of ACPI tables to install
*/
#define PLAT_ACPI_TABLE_COUNT       11

/** The number of platform generic timer blocks
*/
#define PLAT_GTBLOCK_COUNT          1

/** The number of timer frames per generic timer block
*/
#define PLAT_GTFRAME_COUNT          2

/** Count of PCI address-range mapping struct.
*/
#define PCI_ADDRESS_MAP_COUNT       3

/** Count of PCI device legacy interrupt mapping struct.
*/
#define PCI_INTERRUPT_MAP_COUNT     4

/** PCI space codes.
*/
#define PCI_SS_CONFIG   0
#define PCI_SS_IO       1
#define PCI_SS_M32      2
#define PCI_SS_M64      3

/** The number of Processor Hierarchy Nodes
    - one package node
    - two cluster nodes
    - eight cores
*/
#define PLAT_PROC_HIERARCHY_NODE_COUNT  11

/** The number of unique cache structures:
    - L1 instruction cache
    - L1 data cache
    - L2 cache
    - L3 cache
*/
#define PLAT_CACHE_COUNT                7

/** The number of resources private to the package
    - L3 cache
*/
#define PACKAGE_RESOURCE_COUNT  1

/** The number of resources private to Cluster 0
    - L2 cache
*/
#define CLUSTER0_RESOURCE_COUNT  1

/** The number of resources private to each Cluster 0 core instance
    - L1 data cache
    - L1 instruction cache
*/
#define CLUSTER0_CORE_RESOURCE_COUNT  2

/** The number of resources private to Cluster 1
    - L2 cache
*/
#define CLUSTER1_RESOURCE_COUNT  1

/** The number of resources private to each Cluster 1 core instance
    - L1 data cache
    - L1 instruction cache
*/
#define CLUSTER1_CORE_RESOURCE_COUNT  2

/** The number of Lpi states for the platform:
    - two for the cores
    - one for the clusters
*/
#define CORES_LPI_STATE_COUNT           2
#define CLUSTERS_LPI_STATE_COUNT        1
#define LPI_STATE_COUNT                 (CORES_LPI_STATE_COUNT +              \
                                         CLUSTERS_LPI_STATE_COUNT)

/** A structure describing the platform configuration
    manager repository information
*/
typedef struct PlatformRepositoryInfo {
  /// Configuration Manager Information
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CmInfo;

  /// List of ACPI tables
  CM_STD_OBJ_ACPI_TABLE_INFO            CmAcpiTableList[PLAT_ACPI_TABLE_COUNT];

  /// Boot architecture information
  CM_ARM_BOOT_ARCH_INFO                 BootArchInfo;

#ifdef HEADLESS_PLATFORM
  /// Fixed feature flag information
  CM_ARCH_COMMON_FIXED_FEATURE_FLAGS    FixedFeatureFlags;
#endif

  /// Power management profile information
  CM_ARCH_COMMON_POWER_MANAGEMENT_PROFILE_INFO  PmProfileInfo;

  /// GIC CPU interface information
  CM_ARM_GICC_INFO                      GicCInfo[PLAT_CPU_COUNT];

  /// GIC distributor information
  CM_ARM_GICD_INFO                      GicDInfo;

  /// GIC Redistributor information
  CM_ARM_GIC_REDIST_INFO                GicRedistInfo;

  /// Generic timer information
  CM_ARM_GENERIC_TIMER_INFO             GenericTimerInfo;

  /// Generic timer block information
  CM_ARM_GTBLOCK_INFO                   GTBlockInfo[PLAT_GTBLOCK_COUNT];

  /// Generic timer frame information
  CM_ARM_GTBLOCK_TIMER_FRAME_INFO       GTBlock0TimerInfo[PLAT_GTFRAME_COUNT];

  /// Watchdog information
  CM_ARM_GENERIC_WATCHDOG_INFO          Watchdog;

  /** Serial port information for the
      serial port console redirection port
  */
  CM_ARCH_COMMON_SERIAL_PORT_INFO       SpcrSerialPort;

  /// Serial port information for the DBG2 UART port
  CM_ARCH_COMMON_SERIAL_PORT_INFO       DbgSerialPort;

  /// GIC ITS information
  CM_ARM_GIC_ITS_INFO                   GicItsInfo;

  // FVP RevC components
  /// SMMUv3 node
  CM_ARM_SMMUV3_NODE                    SmmuV3Info;

  /// ITS Group node
  CM_ARM_ITS_GROUP_NODE                 ItsGroupInfo;

  /// ITS Identifier array
  CM_ARM_ITS_IDENTIFIER                 ItsIdentifierArray[1];

  /// PCI Root complex node
  CM_ARM_ROOT_COMPLEX_NODE              RootComplexInfo;

  /// Array of DeviceID mapping
  CM_ARM_ID_MAPPING                     DeviceIdMapping[2];

  /// PCI configuration space information
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  PciConfigInfo;

  // PCI address-range mapping references
  CM_ARCH_COMMON_OBJ_REF                PciAddressMapRef[PCI_ADDRESS_MAP_COUNT];

  // PCI address-range mapping information
  CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO   PciAddressMapInfo[PCI_ADDRESS_MAP_COUNT];

  // PCI device legacy interrupts mapping references
  CM_ARCH_COMMON_OBJ_REF                PciInterruptMapRef[PCI_INTERRUPT_MAP_COUNT];

  // PCI device legacy interrupts mapping information
  CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO PciInterruptMapInfo[PCI_INTERRUPT_MAP_COUNT];

  CM_ARM_ET_INFO                        EtInfo;

  // Processor topology information
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO    ProcHierarchyInfo[PLAT_PROC_HIERARCHY_NODE_COUNT];

  // Cache information
  CM_ARCH_COMMON_CACHE_INFO             CacheInfo[PLAT_CACHE_COUNT];

  // package private resources
  CM_ARCH_COMMON_OBJ_REF                PackageResources[PACKAGE_RESOURCE_COUNT];

  // cluster 0 private resources
  CM_ARCH_COMMON_OBJ_REF                Cluster0Resources[CLUSTER0_RESOURCE_COUNT];

  // cluster 0 core private resources
  CM_ARCH_COMMON_OBJ_REF                Cluster0CoreResources[CLUSTER0_CORE_RESOURCE_COUNT];

  // cluster 1 private resources
  CM_ARCH_COMMON_OBJ_REF                Cluster1Resources[CLUSTER1_RESOURCE_COUNT];

  // cluster 1 core private resources
  CM_ARCH_COMMON_OBJ_REF                Cluster1CoreResources[CLUSTER1_CORE_RESOURCE_COUNT];

  // Low Power Idle state information (LPI) for all cores/clusters
  CM_ARCH_COMMON_LPI_INFO               LpiInfo[LPI_STATE_COUNT];

  // Clusters Low Power Idle state references (LPI)
  CM_ARCH_COMMON_OBJ_REF                ClustersLpiRef[CLUSTERS_LPI_STATE_COUNT];

  // Cores Low Power Idle state references (LPI)
  CM_ARCH_COMMON_OBJ_REF                CoresLpiRef[CORES_LPI_STATE_COUNT];

  /// System ID
  UINT32                                SysId;
} EDKII_PLATFORM_REPOSITORY_INFO;

#endif // CONFIGURATION_MANAGER_H__
