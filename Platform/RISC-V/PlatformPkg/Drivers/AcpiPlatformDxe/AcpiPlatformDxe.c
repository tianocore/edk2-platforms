/** @file
  Entry point for the RISC-V ACPI driver.

  Installs all required ACPI tables for the platform:
    FADT, MADT, RHCT, SPCR, MCFG, RIMT, DSDT

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

/**
  Driver entry point.

  Locates EFI_ACPI_TABLE_PROTOCOL, discovers the platform topology from the
  FDT, then builds and installs all required ACPI tables.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS   All tables installed successfully.
  @retval other         A table installation failed; partial tables may have
                        been installed. The system is unlikely to boot an OS.
**/
EFI_STATUS
EFIAPI
AcpiPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  PLATFORM_TOPOLOGY        Topo;

  //
  // Locate the ACPI table protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to locate EFI_ACPI_TABLE_PROTOCOL: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Discover platform topology from the FDT.
  //
  Status = DiscoverPlatformTopology (&Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to discover platform topology: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Install FADT first so the ACPI table manager can link it to the DSDT.
  //
  Status = InstallFadt (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallFadt failed: %r\n", __func__, Status));
    return Status;
  }

  //
  // Install MADT (interrupt topology).
  //
  Status = InstallMadt (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallMadt failed: %r\n", __func__, Status));
    return Status;
  }

  //
  // Install RHCT (RISC-V hart capabilities).
  //
  Status = InstallRhct (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallRhct failed: %r\n", __func__, Status));
    return Status;
  }

  //
  // Install SPCR (serial console redirection).
  //
  Status = InstallSpcr (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallSpcr failed: %r\n", __func__, Status));
    return Status;
  }

  //
  // Install MCFG (PCIe ECAM window).
  //
  Status = InstallMcfg (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallMcfg failed: %r\n", __func__, Status));
    return Status;
  }

  //
  // Install RIMT (RISC-V IO Mapping Table) — required for IOMMU support.
  //
  Status = InstallRimt (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallRimt failed: %r\n", __func__, Status));
    return Status;
  }

  //
  // Install DSDT (device descriptions).
  //
  Status = InstallDsdt (AcpiTable, &Topo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallDsdt failed: %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: All ACPI tables installed successfully\n", __func__));
  return EFI_SUCCESS;
}
