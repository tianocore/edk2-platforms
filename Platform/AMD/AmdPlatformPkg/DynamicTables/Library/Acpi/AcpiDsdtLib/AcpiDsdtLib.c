/** @file

  Install ACPI DSDT table for AMD platforms.

  This module reads the pre-compiled AML bytecode from Dsdt.asl and installs
  it as the Differentiated System Description Table (DSDT) via the ACPI Table
  Protocol. The DSDT contains platform-specific ASL code including sleep state
  definitions, PCIe _OSC/_DSM/_OST methods, MPRAS communication helpers, CXL
  support, and FCH device resources.

  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

/** C array containing the compiled AML bytecode from Dsdt.asl.
    This symbol is defined in the auto-generated C file produced by
    the ASL compiler (iasl).
*/
extern CHAR8  dsdt_aml_code[];

/**
  Constructor for AcpiDsdtLib.

  Installs the pre-compiled DSDT from Dsdt.asl as the ACPI DSDT table.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS           The DSDT table was installed successfully.
  @retval EFI_NOT_FOUND         The ACPI Table Protocol was not found.
  @retval EFI_INVALID_PARAMETER The compiled AML table has an unexpected signature.
  @retval Others                Failed to install the ACPI table.
**/
EFI_STATUS
EFIAPI
AcpiDsdtLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_TABLE_PROTOCOL      *AcpiTableProtocol;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  UINTN                        TableHandle;

  //
  // Locate ACPI Table Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to locate ACPI Table Protocol. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // The dsdt_aml_code contains the compiled AML bytecode from Dsdt.asl.
  // Cast it to the ACPI description header to access table metadata.
  //
  Table = (EFI_ACPI_DESCRIPTION_HEADER *)dsdt_aml_code;

  //
  // Validate table signature is DSDT
  //
  if (Table->Signature != EFI_ACPI_6_5_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid table signature 0x%08X. Expected DSDT (0x%08X).\n",
      __func__,
      Table->Signature,
      EFI_ACPI_6_5_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update table header with platform OEM information from PCDs.
  // InstallAcpiTable() makes an internal copy of the table, so modifying
  // the original buffer before installation is safe.
  //
  CopyMem (&Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId));
  if (PcdGet64 (PcdAmdAcpiDsdtOemTableId) != 0) {
    Table->OemTableId = PcdGet64 (PcdAmdAcpiDsdtOemTableId);
  } else {
    Table->OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  }

  //
  // Install the DSDT table
  //
  TableHandle = 0;
  Status      = AcpiTableProtocol->InstallAcpiTable (
                                     AcpiTableProtocol,
                                     Table,
                                     Table->Length,
                                     &TableHandle
                                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install DSDT table. Status = %r\n",
      __func__,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: DSDT table installed successfully. TableHandle = 0x%lX, Length = 0x%X\n",
      __func__,
      TableHandle,
      Table->Length
      ));
  }

  return Status;
}

/**
  Destructor for AcpiDsdtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Always succeeds.
**/
EFI_STATUS
EFIAPI
AcpiDsdtLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
