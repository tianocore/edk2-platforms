/** @file

  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#pragma once

#include <StandardNameSpaceObjects.h>
#include <X64NameSpaceObjects.h>
#include <ArchCommonNameSpaceObjects.h>
#include <ConfigurationManagerObject.h>

/** The number of ACPI tables to install
*/
#define PLAT_ACPI_TABLE_COUNT  8

/** The maximum number of ACPI tables to install
*/
#define MAX_PLAT_ACPI_TABLE_COUNT  16

/** The configuration manager version.
*/
#define CONFIGURATION_MANAGER_REVISION  CREATE_REVISION (0, 0)

/** The OEM ID
*/
#define CFG_MGR_OEM_ID  { 'A', 'M', 'D', 'I', 'N', 'C' }

/** MAX PSTATE supported by the platform
*/
#define CM_MAX_PSTATE  3

/** MAX STA state supported by the platform
*/
#define CM_MAX_STA_STATE  2

/** The Enable index for STA object in StaInfo array.
*/
#define CM_STA_ENABLE_INDEX  0x0

/** The Disable index for STA object in StaInfo array.
*/
#define CM_STA_DISABLE_INDEX  0x1

/** The number of resources private to 'core instance
    - C-STATE
*/
#define CORE_RESOURCE_COUNT  1

/** The maximum number of C-States supported.
*/
#define CM_MAX_CSTATE  5

/** The maximum number of C-States supported by the platform.
*/
#define CM_MAX_CSTATE_PLATFORM  2

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
typedef EFI_STATUS (EFIAPI *CM_OBJECT_HANDLER_PROC)(
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );

#pragma pack(1)

/** A structure describing the platform configuration
    manager repository information
*/
typedef struct PlatformRepositoryInfo {
  /// Configuration Manager Information
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO           CmInfo;

  /// List of ACPI tables
  CM_STD_OBJ_ACPI_TABLE_INFO                      CmAcpiTableList[MAX_PLAT_ACPI_TABLE_COUNT];
  UINTN                                           CurrentAcpiTableCount;
  CM_X64_FACS_INFO                                FacsInfo;
  CM_ARCH_COMMON_POWER_MANAGEMENT_PROFILE_INFO    PowerManagementProfile;
  CM_ARCH_COMMON_HYPERVISOR_VENDOR_ID             HypervisorVendorId;
  CM_ARCH_COMMON_FIXED_FEATURE_FLAGS              FixedFeatureFlags;
  CM_X64_FADT_SCI_INTERRUPT                       SciInterrupt;
  CM_X64_FADT_SCI_CMD_INFO                        SciCmdinfo;
  CM_X64_FADT_PM_BLOCK_INFO                       PmBlockInfo;
  CM_X64_FADT_GPE_BLOCK_INFO                      GpeBlockInfo;
  CM_X64_FADT_X_PM_BLOCK_INFO                     XpmBlockInfo;
  CM_X64_FADT_X_GPE_BLOCK_INFO                    XgpeBlockInfo;
  CM_X64_FADT_SLEEP_BLOCK_INFO                    SleepBlockInfo;
  CM_X64_FADT_RESET_BLOCK_INFO                    ResetBlockInfo;
  CM_X64_FADT_MISC_INFO                           FadtMiscInfo;
  CM_X64_HPET_INFO                                HpetInfo;
  CM_X64_WSMT_FLAGS_INFO                          WsmtFlagsInfo;
  CM_ARCH_COMMON_SPMI_INTERFACE_INFO              SpmiInterfaceInfo;
  CM_ARCH_COMMON_STA_INFO                         StaInfo[2];
  CM_ARCH_COMMON_OBJ_REF                          CstateResources[CM_MAX_CSTATE_PLATFORM];
  // Core private resources
  CM_ARCH_COMMON_OBJ_REF                          CoreResources[CORE_RESOURCE_COUNT];
  CM_ARCH_COMMON_CST_INFO                         CstInfo[CM_MAX_CSTATE];
  CM_ARCH_COMMON_CPC_INFO                         CpcInfo;
  CM_ARCH_COMMON_PCT_INFO                         PctInfo;
  CM_ARCH_COMMON_PSS_INFO                         PssInfo[CM_MAX_PSTATE];
  CM_ARCH_COMMON_PPC_INFO                         PpcInfo;
  CM_ARCH_COMMON_PSD_INFO                         *PsdInfo;
  UINTN                                           PsdInfoCount;
  CM_ARCH_COMMON_CSD_INFO                         CsdInfo[CM_MAX_CSTATE_PLATFORM];
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO            *PciConfigSpaceInfo;
  UINTN                                           PciConfigSpaceInfoCount;
  CM_X64_MADT_INFO                                MadtInfo;
  CM_X64_LOCAL_APIC_X2APIC_INFO                   *LocalApicX2ApicInfo;
  UINTN                                           LocalApicX2ApicInfoCount;
  CM_X64_IO_APIC_INFO                             *IoApicInfo;
  UINTN                                           IoApicInfoCount;
  CM_X64_INTR_SOURCE_OVERRIDE_INFO                *IntrSourceOverrideInfo;
  UINTN                                           IntrSourceOverrideInfoCount;
  CM_X64_LOCAL_APIC_X2APIC_NMI_INFO               *LocalApicX2ApicNmiInfo;
  UINTN                                           LocalApicX2ApicNmiInfoCount;
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO          *LocalApicX2ApicAffinityInfo;
  UINTN                                           LocalApicX2ApicAffinityInfoCount;
  CM_ARCH_COMMON_MEMORY_AFFINITY_INFO             *MemoryAffinityInfo;
  UINTN                                           MemoryAffinityInfoCount;
  ///
  /// CERT-C scan flags all the macros starting with 'E' as DCL37-C issues, even
  /// though these macros are not defined in errno.h and other C Standard library files.
  /// These issues are false positives.
  ///
  // coverity[cert_dcl37_c_violation]
} EDKII_PLATFORM_REPOSITORY_INFO;
#pragma pack()
