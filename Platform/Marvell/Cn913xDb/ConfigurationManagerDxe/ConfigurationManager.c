/** @file
  Configuration Manager Dxe

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>

//#include "ArmPlatform.h"
#include "ConfigurationManager.h"
#include "Platform.h"

#include "Pcie.h"

/** The platform configuration repository information.
*/
STATIC
EDKII_PLATFORM_REPOSITORY_INFO Cn913xDbPlatformRepositoryInfo = {
  /// Configuration Manager information
  { CONFIGURATION_MANAGER_REVISION, CFG_MGR_OEM_ID },

  // ACPI Table List
  {
    // FADT Table
    {
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
      0
    },
#if FixedPcdGet32(PcdMaxCpCount) == 2
    // SSDT Table
    {
      EFI_ACPI_6_2_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdt),
      (EFI_ACPI_DESCRIPTION_HEADER*)ssdt_aml_code
    },
#endif
    // DBG2 Table
    {
      EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE,
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
      0
    },
    // PCI MCFG Table
    {
      EFI_ACPI_6_2_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
      0
    },
    // GTDT Table
    {
      EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdGtdt),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
      0
    },
    // MADT Table
    {
      EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
    },
    // DSDT Table
    {
      EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDsdt),
      (EFI_ACPI_DESCRIPTION_HEADER*)dsdt_aml_code
    },
    // PPTT Table
    {
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
      EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdPptt),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
      0
    },
    // SPCR Table
    {
      EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpcr),
      NULL,
      CFG_MGR_OEM_TABLE_ID,
      0
    },
   },

  // Boot architecture information
  { EFI_ACPI_6_3_ARM_PSCI_COMPLIANT },           // BootArchFlags

  // Power management profile information
  { EFI_ACPI_6_3_PM_PROFILE_ENTERPRISE_SERVER }, // PowerManagement Profile

  // Fixed feature flag information
  { EFI_ACPI_6_3_HEADLESS },                     // Fixed feature flags

  // Generic Timer Info
  {
    // The physical base address for the counter control frame
    0xFFFFFFFFFFFFFFFF,
    // The physical base address for the counter read frame
    0xFFFFFFFFFFFFFFFF,
    // The secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerSecIntrNum),
    // The secure PL1 timer flags
    GTDT_GTIMER_FLAGS,
    // The non-secure PL1 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerIntrNum),
    // The non-secure PL1 timer flags
    GTDT_GTIMER_FLAGS,
    // The virtual timer interrupt
    FixedPcdGet32 (PcdArmArchTimerVirtIntrNum),
    // The virtual timer flags
    GTDT_GTIMER_FLAGS,
    // The non-secure PL2 timer interrupt
    FixedPcdGet32 (PcdArmArchTimerHypIntrNum),
    // The non-secure PL2 timer flags
    GTDT_GTIMER_FLAGS,

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
    GTDT_WDG_FLAGS
  },

  /* GIC CPU Interface information
     GIC_ENTRY (CPUInterfaceNumber, Mpidr, PmuIrq, VGicIrq, EnergyEfficiency)
  */
  {
    GICC_ENTRY (0, GET_MPID (0, 0), 130, 25, 0),
    GICC_ENTRY (1, GET_MPID (0, 1), 131, 25, 0),
    GICC_ENTRY (2, GET_MPID (1, 0), 132, 25, 0),
    GICC_ENTRY (3, GET_MPID (1, 1), 133, 25, 0),
  },

  // GIC Msi Frame Info
  {
    { 0, GIC_MSI_FRAME0, 0, 0, 0 },
    { 1, GIC_MSI_FRAME1, 0, 0, 0 },
    { 2, GIC_MSI_FRAME2, 0, 0, 0 },
    { 3, GIC_MSI_FRAME3, 0, 0, 0 }
  },

  // GIC Distributor Info
  {
    FixedPcdGet64 (PcdGicDistributorBase),  // UINT64  PhysicalBaseAddress
    0,                                      // UINT32  SystemVectorBase
    EFI_ACPI_6_2_GIC_V2                     // UINT8   GicVersion
  },

  // SPCR Serial Port
  {
    FixedPcdGet64(PcdSerialRegisterBase),                                // BaseAddress
    51,                                                                  // Interrupt
    0,                                                                   // BaudRate
    200000000,                                                                   // Clock
    EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERFACE_TYPE_16450, // Port subtype,
    0x100,                                                                   // BaseAddressLength
    1,                                                                    // AccessSize
    1,
    2
  },
  // Debug Serial Port
  {
    CN913X_DBG2_UART_REG_BASE,                                                   // BaseAddress
    38,                                                                          // Interrupt
    FixedPcdGet64 (PcdSerialDbgUartBaudRate),                                    // BaudRate
    200000000,                                                                           // Clock
    EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_SUBSET_COMPATIBLE_WITH_MS_DBGP_SPEC, // Port subtype
    0,                                                                           // BaseAddressLength
    1,                                                                            // AccessSize
    1,
    2
  },

  // PCI Configuration Space Info
  {
    // The physical base address for the PCI segment
    PCI_ECAM_BASE,
    // The PCI segment group number
    0,
    // The start bus number
    PCI_BUS_MIN,
    // The end bus number
    PCI_BUS_MAX
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
      // CM_OBJECT_TOKEN  GicCToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      1,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (PackageResources)
    },
    // cluster 0
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
      // CM_OBJECT_TOKEN  GicCToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      1,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (ClusterResources)
    },
    // cluster 1
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
      // CM_OBJECT_TOKEN  GicCToken
      CM_NULL_TOKEN,
      // UINT32  NoOfPrivateResources
      1,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (ClusterResources)
    },
    // cores
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> cluster 0
      // CM_OBJECT_TOKEN  GicCToken
      REFERENCE_TOKEN (GicCInfo[0]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
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
      REFERENCE_TOKEN (ProcHierarchyInfo[1]), // -> cluster 0
      // CM_OBJECT_TOKEN  GicCToken
      REFERENCE_TOKEN (GicCInfo[1]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
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
      REFERENCE_TOKEN (ProcHierarchyInfo[2]), // -> cluster 1
      // CM_OBJECT_TOKEN  GicCToken
      REFERENCE_TOKEN (GicCInfo[2]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
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
      // CM_OBJECT_TOKEN  GicCToken
      REFERENCE_TOKEN (GicCInfo[3]),
      // UINT32  NoOfPrivateResources
      CORE_RESOURCE_COUNT,
      // CM_OBJECT_TOKEN  PrivateResourcesArrayToken
      REFERENCE_TOKEN (CoreResources)
    },
  },

  // Cache information
  {
    // package L3 cache
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
    // cluster L2 cache
    {
      REFERENCE_TOKEN (CacheInfo[1]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_512KB,                      // UINT32  Size
      256,                             // UINT32  NumberOfSets
      16,                              // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // core's L1 data cache
    {
      REFERENCE_TOKEN (CacheInfo[2]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      SIZE_32KB,                       // UINT32  Size
      256,                             // UINT32  NumberOfSets
      2,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
    // core's L1 data instruction cache
    {
      REFERENCE_TOKEN (CacheInfo[3]),  // CM_OBJECT_TOKEN  Token
      CM_NULL_TOKEN,                   // CM_OBJECT_TOKEN  NextLevelOfCacheToken
      3 * SIZE_16KB,                   // UINT32  Size
      256,                             // UINT32  NumberOfSets
      3,                               // UINT32  Associativity
      CACHE_ATTRIBUTES (               // UINT8   Attributes
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
        EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
      ),
      64                               // UINT16  LineSize
    },
  },
  // Resources private to the package
  {
    REFERENCE_TOKEN (CacheInfo[0])      // -> package L3 cache
  },
  // Resources private to the cluster
  {
    REFERENCE_TOKEN (CacheInfo[1])      // -> cluster L2 cache
  },
  // Resources private to each individual core instance
  {
    { REFERENCE_TOKEN (CacheInfo[2]) }, // -> core's L1 I-cache
    { REFERENCE_TOKEN (CacheInfo[3]) }  // -> ' core's L1 D-cache
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

/** Return a list of Configuration Manager object references pointed to by the
    given input token.

  @param [in]      This           Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId     The Object ID of the CM object requested
  @param [in]      SearchToken    A unique token for identifying the requested
                                  CM_ARM_OBJ_REF list.
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

  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->PackageResources) {
    CmObject->Size = sizeof (PlatformRepo->PackageResources);
    CmObject->Data = (VOID*)&PlatformRepo->PackageResources;
    CmObject->Count = 1;
    return EFI_SUCCESS;
  }
   if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->ClusterResources) {
    CmObject->Size = sizeof (PlatformRepo->ClusterResources);
    CmObject->Data = (VOID*)&PlatformRepo->ClusterResources;
    CmObject->Count = 1;
    return EFI_SUCCESS;
  }
  if (SearchToken == (CM_OBJECT_TOKEN)&PlatformRepo->CoreResources) {
    CmObject->Size = sizeof (PlatformRepo->CoreResources);
    CmObject->Data = (VOID*)&PlatformRepo->CoreResources;
    CmObject->Count = ARRAY_SIZE (PlatformRepo->CoreResources);
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
    case EArmObjPowerManagementProfileInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PmProfileInfo,
                 sizeof (PlatformRepo->PmProfileInfo),
                 1,
                 CmObject
                 );
      break;
    case EArmObjFixedFeatureFlags:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FixedFeatureFlags,
                 sizeof (PlatformRepo->FixedFeatureFlags),
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
    case EArmObjSerialConsolePortInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SpcrSerialPort,
                 sizeof (PlatformRepo->SpcrSerialPort),
                 1,
                 CmObject
                 );
      break;

    case EArmObjSerialDebugPortInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->DbgSerialPort,
                 sizeof (PlatformRepo->DbgSerialPort),
                 1,
                 CmObject
                 );
      break;
    case EArmObjGicMsiFrameInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->GicMsiFrameInfo,
                 sizeof (PlatformRepo->GicMsiFrameInfo),
                 ARRAY_SIZE (PlatformRepo->GicMsiFrameInfo),
                 CmObject
                 );
      break;

    case EArmObjProcHierarchyInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->ProcHierarchyInfo,
                 sizeof (PlatformRepo->ProcHierarchyInfo),
                 ARRAY_SIZE (PlatformRepo->ProcHierarchyInfo),
                 CmObject
                 );
      break;

    case EArmObjCacheInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->CacheInfo,
                 sizeof (PlatformRepo->CacheInfo),
                 ARRAY_SIZE (PlatformRepo->CacheInfo),
                 CmObject
                 );
      break;

    case EArmObjCmRef:
      Status = HandleCmObjectSearchPlatformRepo (
                 This,
                 CmObjectId,
                 Token,
                 GetCmObjRefs,
                 CmObject
                 );
      break;

    case EArmObjPciConfigSpaceInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PciConfigInfo,
                 sizeof (PlatformRepo->PciConfigInfo),
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
Cn913xDbPlatformGetObject (
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
Cn913xDbPlatformSetObject (
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
EDKII_CONFIGURATION_MANAGER_PROTOCOL Cn913xDbPlatformConfigManagerProtocol = {
  CREATE_REVISION (1, 0),
  Cn913xDbPlatformGetObject,
  Cn913xDbPlatformSetObject,
  &Cn913xDbPlatformRepositoryInfo
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
                  (VOID*)&Cn913xDbPlatformConfigManagerProtocol
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
    &Cn913xDbPlatformConfigManagerProtocol
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
