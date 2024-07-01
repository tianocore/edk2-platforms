/** @file

  Generate ACPI SPMI table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/ServiceProcessorManagementInterfaceTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>

EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE  mAcpiSpmi = {
  {
    EFI_ACPI_6_5_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE),
    /// revision, as per specification
    5,
    0x00,
    { 0, 0, 0, 0, 0, 0 },
    0,
    0,
    0,
    0
  },
  /// Interface type
  0x00,
  /// Reserved
  0x01,
  /// IPMI specification revision
  0x0200,
  /// Interrupt type
  0x00,
  /// GPE number
  0x00,
  /// Reserved2
  0x00,
  /// PciDeviceFlag or _UID
  0x00,
  /// GlobalSystemInterrupt
  0x00,
  /// Base address
  {
    /// System IO address space
    EFI_ACPI_6_5_SYSTEM_IO,
    /// Bit width
    0x08,
    /// Bit offset
    0x00,
    /// Access size
    0x00,
    /// Base IO space address
    0x0CA2
  },
  /// Device identification information
  {
    { 0x00000000 }
  },
  /// Reserved3
  0x00
};

/**
  Implementation of AcpiSpmiLibConstructor for AMD platforms.
  This is library constructor for AcpiSpmiLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated and installed ACPI SPMI table.
  @retval EFI_UNSUPPORTED Unsupported IPMI interface type.
  @retval Others          Failed to locate ACPI Table Protocol or
                          failed to install SPMI table.

**/
EFI_STATUS
EFIAPI
AcpiSpmiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol;
  EFI_STATUS               Status;
  UINTN                    NewTableKey;

  /// SPMI Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI SPMI Table.\n"));

  /// Update interface type, currently supports only IPMI KCS
  mAcpiSpmi.InterfaceType = PcdGet8 (PcdIpmiInterfaceType);
  if (mAcpiSpmi.InterfaceType != 1) {
    DEBUG ((DEBUG_ERROR, "Unsupported IPMI Interface Type. Type(%d)\n", PcdGet8 (PcdIpmiInterfaceType)));
    DEBUG ((DEBUG_ERROR, "Cannot install SMPI table.\n"));
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    return EFI_UNSUPPORTED;
  }

  /// Update base address
  mAcpiSpmi.BaseAddress.Address = PcdGet16 (PcdIpmiKcsIoBaseAddress);

  /// Update SPMI Table Header
  mAcpiSpmi.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mAcpiSpmi.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  mAcpiSpmi.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mAcpiSpmi.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  CopyMem (&mAcpiSpmi.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mAcpiSpmi.Header.OemId));

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Install SPMI Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           &mAcpiSpmi,
                           sizeof (EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE),
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install SPMI Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Implementation of AcpiSpmiLibDestructor for AMD platforms.
  This is library destructor for AcpiSpmiLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiSpmiLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
