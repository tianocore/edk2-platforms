/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Ipmi.h>
#include <IndustryStandard/ServiceProcessorManagementInterfaceTable.h>
#include <Library/IpmiCommandLib.h>
#include "AcpiPlatform.h"

//
// SPMI Revision (as defined in IPMI v2.0 spec.)
//
#define EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE_REVISION  0x05

STATIC EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE  mSpmiTable = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE,
    EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE,
    EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE_REVISION
    ),
  0x04,                     // SMBUS System Interface (SSIF)
  0x01,                     // Reserved - Must be 0x01 for backward compatiblity
  0,                        // Specification Revision
  0,                        // Interrupt Type
  0,                        // GPE
  EFI_ACPI_RESERVED_BYTE,   // Reserved
  0,                        // PCI Device Flag
  0,                        // Global System Interrupt
  {
    0x04,                                     // Address Space ID: 4 (SMBUS)
    0,                                        // Register Bit Width
    0,                                        // Register Bit Offset
    0x01,                                     // Address Size: 1 (Byte Access)
    FixedPcdGet8 (PcdIpmiSsifSmbusSlaveAddr), // Address (7-bit SMBUS Address of BMC SSIF)
  },
  {
    {
      0,  // UID Byte 1
      0,  // UID Byte 2
      0,  // UID Byte 3
      0   // UID Byte 4
    }
  },
  EFI_ACPI_RESERVED_BYTE    // Reserved for backward compatiblity
};

static
EFI_STATUS
UpdateIpmiSpecRevision (
  EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE  *SpmiTable
  )
{
  EFI_STATUS                   Status;
  IPMI_GET_DEVICE_ID_RESPONSE  DeviceId;

  Status = IpmiGetDeviceId (&DeviceId);
  if (!EFI_ERROR (Status) && (DeviceId.CompletionCode == IPMI_COMP_CODE_NORMAL)) {
    // BCD Format
    SpmiTable->SpecificationRevision  = DeviceId.SpecificationVersion & 0xF0;
    SpmiTable->SpecificationRevision |= (DeviceId.SpecificationVersion & 0xF) << 8;
  }

  return Status;
}

/**
  Install SPMI (Service Processor Management Interface table) table.

  @retval EFI_SUCCESS   The table was installed successfully.
  @retval Others        Failed to install the table.

**/
EFI_STATUS
AcpiInstallSpmiTable (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  UINTN                    SpmiTableKey;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateIpmiSpecRevision (&mSpmiTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to update IPMI Specification Revision - %r\n", __func__, Status));
    return Status;
  }

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)&mSpmiTable,
                                mSpmiTable.Header.Length,
                                &SpmiTableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install SPMI table - %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}
