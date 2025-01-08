/** @file
  Configuration Manager Protocol implementation for AMD platform.
  This file implements the Configuration Manager Protocol for the AMD platform.
  The Configuration Manager Protocol is used to provide the Configuration
  Objects to the Configuration Manager.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include <IndustryStandard/ServiceProcessorManagementInterfaceTable.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include "ConfigurationManager.h"

/** The platform configuration repository information.
*/
STATIC
EDKII_PLATFORM_REPOSITORY_INFO  mAmdPlatformRepositoryInfo = {
  /// Configuration Manager Information
  {
    CONFIGURATION_MANAGER_REVISION,
    CFG_MGR_OEM_ID
  },
  /// ACPI Table List
  {
    /// FADT Table
    {
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
      NULL
    },
    /// HPET Table
    {
      EFI_ACPI_6_3_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE,
      EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdHpet),
      NULL
    },
    /// HPET Ssdt Table
    {
      EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtHpet),
      NULL
    },
    /// WSMT Table
    {
      EFI_ACPI_6_3_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE,
      EFI_WSMT_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdWsmt),
      NULL
    },
    /// SPMI Table
    {
      EFI_ACPI_6_5_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE,
      EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_5_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpmi),
      NULL
    },
    /// MCFG Table
    {
      EFI_ACPI_6_5_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
      NULL
    },
    /// MADT Table
    {
      EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
      NULL
    }
  },
  /// PmProfile
  {
    EFI_ACPI_6_5_PM_PROFILE_ENTERPRISE_SERVER
  },
  /// HypervisorVendorId
  {
    0x00000000
  },
  /// FixedFeatureFlags
  {
    EFI_ACPI_6_5_WBINVD                   | \
    EFI_ACPI_6_5_PROC_C1                  | \
    EFI_ACPI_6_5_P_LVL2_UP                | \
    EFI_ACPI_6_5_SLP_BUTTON               | \
    EFI_ACPI_6_5_TMR_VAL_EXT              | \
    EFI_ACPI_6_5_RESET_REG_SUP            | \
    EFI_ACPI_6_5_REMOTE_POWER_ON_CAPABLE
  },
  /// SciInterrupt
  {
    0x0009
  },
  /// SciCmdinfo
  {
    0x000000B2,
    0xA0,
    0xA1,
    0x00,
    0x00,
    0x00
  },
  /// PmBlockInfo
  {
    0x00000800,
    0x00000000,
    0x00000804,
    0x00000000,
    0x00000000,
    0x00000808,
    0x04,
    0x02,
    0x00,
    0x04
  },
  /// GPE block
  {
    0x00000820,
    0x00000000,
    0x08,
    0x00,
    0x00
  },
  /// X PM Event
  {
    { EFI_ACPI_6_5_SYSTEM_IO, 0x20, 0x0, EFI_ACPI_6_5_WORD,      0x0000000000000800 },
    { 0x0,                    0x0,  0x0, 0x0,                    0x0000000000000000 },
    { EFI_ACPI_6_5_SYSTEM_IO, 0x10, 0x0, EFI_ACPI_6_5_WORD,      0x0000000000000804 },
    { 0x0,                    0x0,  0x0, 0x0,                    0x0000000000000000 },
    { 0x0,                    0x0,  0x0, 0x0,                    0x0000000000000000 },
    { EFI_ACPI_6_5_SYSTEM_IO, 0x20, 0x0, EFI_ACPI_6_5_DWORD,     0x0000000000000808 }
  },
  /// X GPE Event
  {
    { EFI_ACPI_6_5_SYSTEM_IO, 0x40, 0x0, EFI_ACPI_6_5_BYTE,      0x0000000000000820 },
    { 0x0,                    0x0,  0x0, 0x0,                    0x0000000000000000 }
  },
  /// Sleep Event
  {
    { 0x0,                    0x0,  0x0, 0x0,                    0x0                },
    { 0x0,                    0x0,  0x0, 0x0,                    0x0                }
  },
  /// Reset Event
  {
    { EFI_ACPI_6_5_SYSTEM_IO, 0x8,  0x0, EFI_ACPI_6_5_UNDEFINED, 0x0000000000000CF9 },
    0x06
  },
  /// FadtMiscInfo
  {
    0x0064,
    0x03E9,
    0x0400,
    0x0010,
    0x01,
    0x03,
    0x0D,
    0x00,
    0x32
  },
  /// HpetInfo
  {
    FixedPcdGet64 (PcdHpetBaseAddress),
    0x37EE,
    0x0
  },
  /// Wsmt flags info
  {
    EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS                | \
    EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION | \
    EFI_WSMT_PROTECTION_FLAGS_SYSTEM_RESOURCE_PROTECTION
  },
  /// SpmiInterruptInfo
  {
    0x01,
    { EFI_ACPI_6_5_SYSTEM_IO, 0x8,  0,   0,                      0x0000000000000CA2 }
  }
};

/** A structure describing the configuration manager protocol interface.
*/
STATIC
EDKII_CONFIGURATION_MANAGER_PROTOCOL  mAmdPlatformConfigManagerProtocol = {
  CREATE_REVISION (1,   0),
  AmdPlatformGetObject,
  AmdPlatformSetObject,
  &mAmdPlatformRepositoryInfo
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  Status = UpdateMcfgTableInfo (&mAmdPlatformRepositoryInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Failed to update MCFG table info. Status = %r\n", Status));
    return Status;
  }

  /// set the OemTableId and OemRevision for the CmACpiTableList
  for (Index = 0; Index < ARRAY_SIZE (mAmdPlatformRepositoryInfo.CmAcpiTableList); Index++) {
    mAmdPlatformRepositoryInfo.CmAcpiTableList[Index].OemTableId  = PcdGet64 (PcdAcpiDefaultOemTableId);
    mAmdPlatformRepositoryInfo.CmAcpiTableList[Index].OemRevision = PcdGet32 (PcdAcpiDefaultOemRevision);
  }

  UpdateMadtTable (&mAmdPlatformRepositoryInfo);
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiConfigurationManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&mAmdPlatformConfigManagerProtocol
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
