/** @file

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef AMD_CONFIGURATION_MANAGER_H_
#define AMD_CONFIGURATION_MANAGER_H_

#include <StandardNameSpaceObjects.h>
#include <X64NameSpaceObjects.h>
#include <ArchCommonNameSpaceObjects.h>
#include <ConfigurationManagerObject.h>

/** The number of ACPI tables to install
*/
#define PLAT_ACPI_TABLE_COUNT  7

/** The configuration manager version.
*/
#define CONFIGURATION_MANAGER_REVISION  CREATE_REVISION (1, 0)

/** The OEM ID
*/
#define CFG_MGR_OEM_ID  { 'A', 'M', 'D', 'I', 'N', 'C' }

#pragma pack(1)

/** A structure describing the platform configuration
    manager repository information
*/
typedef struct PlatformRepositoryInfo {
  /// Configuration Manager Information
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO           CmInfo;

  /// List of ACPI tables
  CM_STD_OBJ_ACPI_TABLE_INFO                      CmAcpiTableList[PLAT_ACPI_TABLE_COUNT];
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
} EDKII_PLATFORM_REPOSITORY_INFO;
#pragma pack()

#endif // AMD_CONFIGURATION_MANAGER_H_
