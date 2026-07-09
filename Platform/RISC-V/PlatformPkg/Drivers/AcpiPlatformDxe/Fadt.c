/** @file
  FADT table builder for the RISC-V ACPI driver.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

/**
  Build and install the Fixed ACPI Description Table (FADT).

  RISC-V uses the Hardware-Reduced ACPI profile (HW_REDUCED_ACPI flag set).
  There are no PM1 event/control blocks, no SMI command port, and no legacy
  power management hardware. The DSDT pointer is left at zero here; the DXE
  core's ACPI table manager links FADT to DSDT automatically when both are
  installed via EFI_ACPI_TABLE_PROTOCOL.

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology (unused for FADT but kept for
                         consistency with other builders).

  @retval EFI_SUCCESS  FADT installed.
  @retval other        Installation failed.
**/
EFI_STATUS
EFIAPI
InstallFadt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS                                 Status;
  UINTN                                      TableKey;
  EFI_ACPI_6_6_FIXED_ACPI_DESCRIPTION_TABLE  Fadt;

  ZeroMem (&Fadt, sizeof (Fadt));

  //
  // Common header.
  //
  Fadt.Header.Signature = EFI_ACPI_6_6_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE;
  Fadt.Header.Length    = sizeof (Fadt);
  Fadt.Header.Revision  = EFI_ACPI_6_6_FIXED_ACPI_DESCRIPTION_TABLE_REVISION;
  Fadt.MinorVersion     = EFI_ACPI_6_6_FIXED_ACPI_DESCRIPTION_TABLE_MINOR_REVISION;
  CopyMem ((VOID *)&Fadt.Header.OemId, ACPI_OEM_ID, sizeof (Fadt.Header.OemId));
  CopyMem ((VOID *)&Fadt.Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (Fadt.Header.OemTableId));
  Fadt.Header.OemRevision = 0x00000001;
  CopyMem ((VOID *)&Fadt.Header.CreatorId, ACPI_CREATOR_ID, sizeof (Fadt.Header.CreatorId));
  Fadt.Header.CreatorRevision = ACPI_CREATOR_REVISION;

  //
  // HW-reduced ACPI: no legacy PM hardware.
  //
  Fadt.Flags = EFI_ACPI_6_6_HW_REDUCED_ACPI;

  //
  // Preferred PM profile: Enterprise Server.
  //
  Fadt.PreferredPmProfile = EFI_ACPI_6_6_PM_PROFILE_ENTERPRISE_SERVER;

  //
  // No IA-PC boot architecture flags (not x86).
  //
  Fadt.IaPcBootArch = 0;

  Status = AllocateAndInstallAcpiTable (AcpiTable, &Fadt, sizeof (Fadt), &TableKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install FADT: %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a: FADT installed\n", __func__));
  }

  return Status;
}
