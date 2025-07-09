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
extern CHAR8  dsdtgicv5_aml_code[];                                                               // [CODE_FIRST] 11148

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
    EnergyEfficiency,         /* UINT8   ProcessorPowerEfficiencyClass                            // [CODE_FIRST] 11148 */ \
    0,                        /* UINT16  SpeOverflowInterrupt                                     // [CODE_FIRST] 11148 */ \
    0,                        /* UINT32  ProximityDomain                                          // [CODE_FIRST] 11148 */ \
    0,                        /* UINT32  ClockDomain                                              // [CODE_FIRST] 11148 */ \
    0,                        /* UINT32  AffinityFlags                                            // [CODE_FIRST] 11148 */ \
    CM_NULL_TOKEN,            /* CM_OBJECT_TOKEN CpcToken                                         // [CODE_FIRST] 11148 */ \
    0,                        /* UINT16  TrbeInterrupt                                            // [CODE_FIRST] 11148 */ \
    CM_NULL_TOKEN,            /* CM_OBJECT_TOKEN EtToken                                          // [CODE_FIRST] 11148 */ \
    CM_NULL_TOKEN,            /* CM_OBJECT_TOKEN PsdToken                                         // [CODE_FIRST] 11148 */ \
    CM_NULL_TOKEN,            /* CM_OBJECT_TOKEN ProximityDomainToken                             // [CODE_FIRST] 11148 */ \
    CM_NULL_TOKEN,            /* CM_OBJECT_TOKEN ClockDomainToken                                 // [CODE_FIRST] 11148 */ \
    }

/** A helper macro for populating the Processor Hierarchy Node flags                              // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define PROC_NODE_FLAGS(                                                                        /*// [CODE_FIRST] 11148 */ \
          PhysicalPackage,                                                                      /*// [CODE_FIRST] 11148 */ \
          AcpiProcessorIdValid,                                                                 /*// [CODE_FIRST] 11148 */ \
          ProcessorIsThread,                                                                    /*// [CODE_FIRST] 11148 */ \
          NodeIsLeaf,                                                                           /*// [CODE_FIRST] 11148 */ \
          IdenticalImplementation                                                               /*// [CODE_FIRST] 11148 */ \
          )                                                                                     /*// [CODE_FIRST] 11148 */ \
  (                                                                                             /*// [CODE_FIRST] 11148 */ \
    PhysicalPackage |                                                                           /*// [CODE_FIRST] 11148 */ \
    (AcpiProcessorIdValid << 1) |                                                               /*// [CODE_FIRST] 11148 */ \
    (ProcessorIsThread << 2) |                                                                  /*// [CODE_FIRST] 11148 */ \
    (NodeIsLeaf << 3) |                                                                         /*// [CODE_FIRST] 11148 */ \
    (IdenticalImplementation << 4)                                                              /*// [CODE_FIRST] 11148 */ \
  )                                                                                               // [CODE_FIRST] 11148

/** A helper macro for populating the Cache Type Structure's attributes                           // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define CACHE_ATTRIBUTES(                                                                       /*// [CODE_FIRST] 11148 */ \
          AllocationType,                                                                       /*// [CODE_FIRST] 11148 */ \
          CacheType,                                                                            /*// [CODE_FIRST] 11148 */ \
          WritePolicy                                                                           /*// [CODE_FIRST] 11148 */ \
          )                                                                                     /*// [CODE_FIRST] 11148 */ \
  (                                                                                             /*// [CODE_FIRST] 11148 */ \
    AllocationType |                                                                            /*// [CODE_FIRST] 11148 */ \
    (CacheType << 2) |                                                                          /*// [CODE_FIRST] 11148 */ \
    (WritePolicy << 4)                                                                          /*// [CODE_FIRST] 11148 */ \
  )                                                                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
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
#define PLAT_ACPI_TABLE_COUNT       12                                                           // [CODE_FIRST] 11148

#define PLAT_SMBIOS_TABLE_COUNT     2

/** The number of platform generic timer blocks
*/
#define PLAT_GTBLOCK_COUNT          1

/** The number of timer frames per generic timer block
*/
#define PLAT_GTFRAME_COUNT          2

/** Count of PCI address-range mapping struct.                                                    // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define PCI_ADDRESS_MAP_COUNT       3                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** Count of PCI device legacy interrupt mapping struct.                                          // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define PCI_INTERRUPT_MAP_COUNT     4                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** PCI space codes.                                                                              // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define PCI_SS_CONFIG   0                                                                         // [CODE_FIRST] 11148
#define PCI_SS_IO       1                                                                         // [CODE_FIRST] 11148
#define PCI_SS_M32      2                                                                         // [CODE_FIRST] 11148
#define PCI_SS_M64      3                                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** The number of Processor Hierarchy Nodes                                                       // [CODE_FIRST] 11148
    - one package node                                                                            // [CODE_FIRST] 11148
    - two cluster nodes                                                                           // [CODE_FIRST] 11148
    - eight cores                                                                                 // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define PLAT_PROC_HIERARCHY_NODE_COUNT  11                                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** The number of unique cache structures:                                                        // [CODE_FIRST] 11148
    - L1 instruction cache                                                                        // [CODE_FIRST] 11148
    - L1 data cache                                                                               // [CODE_FIRST] 11148
    - L2 cache                                                                                    // [CODE_FIRST] 11148
    - L3 cache
*/                                                                                                // [CODE_FIRST] 11148
#define PLAT_CACHE_COUNT                7

/** The number of resources private to the package
    - L3 cache
*/
#define PACKAGE_RESOURCE_COUNT  1
                                                                                                  // [CODE_FIRST] 11148
/** The number of resources private to Cluster 0                                                  // [CODE_FIRST] 11148
    - L2 cache                                                                                    // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define CLUSTER0_RESOURCE_COUNT  1                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** The number of resources private to each Cluster 0 core instance                               // [CODE_FIRST] 11148
    - L1 data cache                                                                               // [CODE_FIRST] 11148
    - L1 instruction cache                                                                        // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define CLUSTER0_CORE_RESOURCE_COUNT  2                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** The number of resources private to Cluster 1                                                  // [CODE_FIRST] 11148
    - L2 cache                                                                                    // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define CLUSTER1_RESOURCE_COUNT  1                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** The number of resources private to each Cluster 1 core instance                               // [CODE_FIRST] 11148
    - L1 data cache                                                                               // [CODE_FIRST] 11148
    - L1 instruction cache                                                                        // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define CLUSTER1_CORE_RESOURCE_COUNT  2                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/** The number of Lpi states for the platform:                                                    // [CODE_FIRST] 11148
    - two for the cores                                                                           // [CODE_FIRST] 11148
    - one for the clusters                                                                        // [CODE_FIRST] 11148
*/                                                                                                // [CODE_FIRST] 11148
#define CORES_LPI_STATE_COUNT           2                                                         // [CODE_FIRST] 11148
#define CLUSTERS_LPI_STATE_COUNT        1                                                         // [CODE_FIRST] 11148
#define LPI_STATE_COUNT                 (CORES_LPI_STATE_COUNT + CLUSTERS_LPI_STATE_COUNT)        // [CODE_FIRST] 11148

/** A structure describing the platform configuration
    manager repository information
*/
typedef struct PlatformRepositoryInfo {
  /// Configuration Manager Information
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CmInfo;

  /// List of ACPI tables
  CM_STD_OBJ_ACPI_TABLE_INFO            CmAcpiTableList[PLAT_ACPI_TABLE_COUNT];

  CM_STD_OBJ_SMBIOS_TABLE_INFO          SmbiosTableList[PLAT_SMBIOS_TABLE_COUNT];

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

  /// GIC ITSv5 information                                                                       // [CODE_FIRST] 11148
  CM_ARM_GIC_ITSV5_INFO                 GicItsV5Info;                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  /// GIC ITSv5 Translate Frame information                                                       // [CODE_FIRST] 11148
  CM_ARM_GIC_ITSV5_TRANSLATE_FRAME_INFO GicItsV5TransFrameInfo[1];                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  /// GIC IRS information                                                                         // [CODE_FIRST] 11148
  CM_ARM_GIC_IRS_INFO                   GicIrsInfo;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  /// GIC IWB information                                                                         // [CODE_FIRST] 11148
  CM_ARM_GIC_IWB_INFO                   GicIwbInfo;                                               // [CODE_FIRST] 11148

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
  CM_ARM_ID_MAPPING                     DeviceIdMapping[4];                                       // [CODE_FIRST] 11148

  /// PCI configuration space information
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  PciConfigInfo;

  // PCI address-range mapping references                                                         // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                PciAddressMapRef[PCI_ADDRESS_MAP_COUNT];                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // PCI address-range mapping information                                                        // [CODE_FIRST] 11148
  CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO   PciAddressMapInfo[PCI_ADDRESS_MAP_COUNT];                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // PCI device legacy interrupts mapping references                                              // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                PciInterruptMapRef[PCI_INTERRUPT_MAP_COUNT];              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // PCI device legacy interrupts mapping information                                             // [CODE_FIRST] 11148
  CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO PciInterruptMapInfo[PCI_INTERRUPT_MAP_COUNT];             // [CODE_FIRST] 11148

  CM_ARM_ET_INFO                        EtInfo;

  // Processor topology information                                                               // [CODE_FIRST] 11148
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO    ProcHierarchyInfo[PLAT_PROC_HIERARCHY_NODE_COUNT];        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // Cache information                                                                            // [CODE_FIRST] 11148
  CM_ARCH_COMMON_CACHE_INFO             CacheInfo[PLAT_CACHE_COUNT];                              // [CODE_FIRST] 11148

  // package private resources
  CM_ARCH_COMMON_OBJ_REF                PackageResources[PACKAGE_RESOURCE_COUNT];

  // cluster 0 private resources                                                                  // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                Cluster0Resources[CLUSTER0_RESOURCE_COUNT];               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // cluster 0 core private resources                                                             // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                Cluster0CoreResources[CLUSTER0_CORE_RESOURCE_COUNT];      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // cluster 1 private resources                                                                  // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                Cluster1Resources[CLUSTER1_RESOURCE_COUNT];               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // cluster 1 core private resources                                                             // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                Cluster1CoreResources[CLUSTER1_CORE_RESOURCE_COUNT];      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // Low Power Idle state information (LPI) for all cores/clusters                                // [CODE_FIRST] 11148
  CM_ARCH_COMMON_LPI_INFO               LpiInfo[LPI_STATE_COUNT];                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // Clusters Low Power Idle state references (LPI)                                               // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                ClustersLpiRef[CLUSTERS_LPI_STATE_COUNT];                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  // Cores Low Power Idle state references (LPI)                                                  // [CODE_FIRST] 11148
  CM_ARCH_COMMON_OBJ_REF                CoresLpiRef[CORES_LPI_STATE_COUNT];                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  /// System ID
  UINT32                                SysId;
                                                                                                  // [CODE_FIRST] 11148
  BOOLEAN                               HasGicV5;                                                 // [CODE_FIRST] 11148
} EDKII_PLATFORM_REPOSITORY_INFO;

#endif // CONFIGURATION_MANAGER_H__
