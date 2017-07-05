/** @file

  Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_H__
#define CONFIGURATION_MANAGER_H__

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

/** A helper macro for returning configuration manager objects
*/
#define HANDLE_CM_OBJECT(CmObjectId, Object)   \
  case CmObjectId: {                           \
    CmObject->Size = sizeof (Object);          \
    CmObject->Data = (VOID*)&Object;           \
    DEBUG ((                                   \
      DEBUG_INFO,                              \
      #CmObjectId ": Ptr = 0x%p, Size = %d\n", \
      CmObject->Data,                          \
      CmObject->Size                           \
      ));                                      \
    break;                                     \
  }

/** A helper macro for returning configuration manager objects
    referenced by token
*/
#define HANDLE_CM_OBJECT_REF_BY_TOKEN(                          \
          CmObjectId,                                           \
          Object,                                               \
          Token,                                                \
          HandlerProc                                           \
          )                                                     \
  case CmObjectId: {                                            \
    if (Token == CM_NULL_TOKEN) {                               \
      CmObject->Size = sizeof (Object);                         \
      CmObject->Data = (VOID*)&Object;                          \
      DEBUG ((                                                  \
        DEBUG_INFO,                                             \
        #CmObjectId ": Ptr = 0x%p, Size = %d\n",                \
        CmObject->Data,                                         \
        CmObject->Size                                          \
        ));                                                     \
    } else {                                                    \
      Status = HandlerProc (This, CmObjectId, Token, CmObject); \
      DEBUG ((                                                  \
        DEBUG_INFO,                                             \
        #CmObjectId ": Token = 0x%p, Ptr = 0x%p, Size = %d\n",  \
        (VOID*)Token,                                           \
        CmObject->Data,                                         \
        CmObject->Size                                          \
        ));                                                     \
    }                                                           \
    break;                                                      \
  }

/** The number of CPUs
*/
#define PLAT_CPU_COUNT              8

/** The number of ACPI tables to install
*/
#define PLAT_ACPI_TABLE_COUNT       6

/** The number of platform generic timer blocks
*/
#define PLAT_GTBLOCK_COUNT          1

/** The number of timer frames per generic timer block
*/
#define PLAT_GTFRAME_COUNT          2

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
  CM_ARM_FIXED_FEATURE_FLAGS            FixedFeatureFlags;
#endif

  /// Power management profile information
  CM_ARM_POWER_MANAGEMENT_PROFILE_INFO  PmProfileInfo;

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
  CM_ARM_SERIAL_PORT_INFO               SpcrSerialPort;

  /// Serial port information for the DBG2 UART port
  CM_ARM_SERIAL_PORT_INFO               DbgSerialPort;

  /// GIC ITS information
  CM_ARM_GIC_ITS_INFO                   GicItsInfo;
} EFI_PLATFORM_REPOSITORY_INFO;

#endif // CONFIGURATION_MANAGER_H__
