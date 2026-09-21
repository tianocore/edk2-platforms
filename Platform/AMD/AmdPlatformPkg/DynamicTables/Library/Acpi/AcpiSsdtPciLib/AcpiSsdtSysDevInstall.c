/** @file

  Install ACPI SSDT System Device table for AMD platforms.

  This module reads the pre-compiled AML bytecode from AcpiSsdtSysDev.asl
  and installs it as an SSDT ACPI table. The table contains FCH device
  resources including LPC bridge devices, UART configuration, DMA controller,
  RTC, speaker, timer, system resources, SPI ROM, and IPMI KCS.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

/** C array containing the compiled AML bytecode from AcpiSsdtSysDev.asl.
    This symbol is defined in the auto-generated C file produced by
    the ASL compiler (iasl).
*/
extern CHAR8  acpissdtsysdev_aml_code[];

/**
  Install the ACPI SSDT System Device table.

  This function takes the pre-compiled AML bytecode from AcpiSsdtSysDev.asl,
  updates the table header with platform-specific OEM information, and installs
  it as a Secondary System Description Table (SSDT) via the ACPI Table Protocol.

  @retval EFI_SUCCESS           The SSDT SysDev table was installed successfully.
  @retval EFI_NOT_FOUND         The ACPI Table Protocol was not found.
  @retval EFI_INVALID_PARAMETER The compiled AML table has an unexpected signature.
  @retval Others                Failed to install the ACPI table.
**/
EFI_STATUS
EFIAPI
InstallAcpiSsdtSysDevTable (
  VOID
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
  // The acpissdtsysdev_aml_code contains the compiled AML bytecode from
  // AcpiSsdtSysDev.asl. Cast it to the ACPI description header to access
  // table metadata.
  //
  Table = (EFI_ACPI_DESCRIPTION_HEADER *)acpissdtsysdev_aml_code;

  //
  // Validate table signature is SSDT
  //
  if (Table->Signature != EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid table signature 0x%08X. Expected SSDT (0x%08X).\n",
      __func__,
      Table->Signature,
      EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update table header with platform OEM information from PCDs.
  // InstallAcpiTable() makes an internal copy of the table, so modifying
  // the original buffer before installation is safe.
  //
  CopyMem (&Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId));
  if (PcdGet64 (PcdAmdAcpiSysDevSsdtOemTableId) != 0) {
    Table->OemTableId = PcdGet64 (PcdAmdAcpiSysDevSsdtOemTableId);
  } else {
    Table->OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  }

  //
  // Install the SSDT SysDev table
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
      "%a: Failed to install SSDT SysDev table. Status = %r\n",
      __func__,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: SSDT SysDev table installed successfully. TableHandle = 0x%lX, Length = 0x%X\n",
      __func__,
      TableHandle,
      Table->Length
      ));
  }

  return Status;
}
