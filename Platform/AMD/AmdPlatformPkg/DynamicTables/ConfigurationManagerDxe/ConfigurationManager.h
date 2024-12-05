/** @file

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_H_
#define CONFIGURATION_MANAGER_H_

#include <ConfigurationManagerObject.h>
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
AmdPlatformSetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );

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
AmdPlatformGetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );

/** Updates ACPI MCFG table information in the platform repository.

  @param [in]  PlatformRepo  Pointer to the platform repository.

  @retval EFI_SUCCESS        The ACPI MCFG table information is updated.
  @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
  @retval EFI_UNSUPPORTED    The operation is not supported.
**/
EFI_STATUS
EFIAPI
UpdateMcfgTableInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  );

/** The UpdateMadtTable function updates the MADT table.

    @param [in, out]  PlatformRepo  Pointer to the platform repository information.

    @retval EFI_SUCCESS            The MADT table is updated successfully.
    @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
**/
EFI_STATUS
EFIAPI
UpdateMadtTable (
  IN OUT EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  );

#endif // CONFIGURATION_MANAGER_H_
