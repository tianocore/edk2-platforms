/** @file

  Generate ACPI WSMT table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>

/// WSMT Table Definition
STATIC
EFI_ACPI_WSMT_TABLE  mAcpiWsmt = {
  {
    EFI_ACPI_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_WSMT_TABLE),
    EFI_WSMT_TABLE_REVISION,
    0x00,
    { 0, 0, 0, 0, 0, 0 },
    0,
    0,
    0,
    0
  },
  EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS |                \
  EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION | \
  EFI_WSMT_PROTECTION_FLAGS_SYSTEM_RESOURCE_PROTECTION
};

/**
  Implementation of AcpiWsmtLibConstructor for AMD platforms.
  This is library constructor for AcpiWsmtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated and installed WSMT Table.
  @retval Others          Failed to generate and install WSMT Table.
**/
EFI_STATUS
EFIAPI
AcpiWsmtLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol;
  EFI_STATUS               Status;
  UINTN                    NewTableKey;

  /// WSMT Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI WSMT Table.\n"));

  /// Update WSMT Table Header
  mAcpiWsmt.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mAcpiWsmt.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  mAcpiWsmt.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mAcpiWsmt.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  CopyMem (&mAcpiWsmt.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mAcpiWsmt.Header.OemId));

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Install WSMT Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           &mAcpiWsmt,
                           sizeof (EFI_ACPI_WSMT_TABLE),
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install WSMT Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Implementation of AcpiWsmtLibDestructor for AMD platforms.
  This is library destructor for AcpiWsmtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiWsmtLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
