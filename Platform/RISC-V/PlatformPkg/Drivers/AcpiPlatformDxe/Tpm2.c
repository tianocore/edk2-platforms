/** @file
  TPM2 table builder for the RISC-V ACPI driver.

  This installs the standalone "TPM2" ACPI table (TCG ACPI Specification),
  which is distinct from the \_SB.TPM0 device node created in the DSDT.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

#include <IndustryStandard/Tpm2Acpi.h>

/**
  Build and install the Trusted Computing Platform 2 (TPM2) ACPI table.

  The platform only supports a TIS/MMIO TPM interface (as discovered from
  the FDT "tcg,tpm-tis-mmio" node), so StartMethod is always set to
  EFI_TPM2_ACPI_TABLE_START_METHOD_TIS and no Control Area / start method
  specific parameters are required.

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology.

  @retval EFI_SUCCESS           TPM2 installed (or skipped — no TPM present).
  @retval EFI_OUT_OF_RESOURCES  Buffer allocation failed.
  @retval other                 InstallAcpiTable failed.
**/
EFI_STATUS
EFIAPI
InstallTpm2 (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS           Status;
  UINTN                TableKey;
  EFI_TPM2_ACPI_TABLE  Tpm2;

  //
  // Skip if no TPM 2.0 (TIS/MMIO) device was discovered in the FDT.
  //
  if (Topo->TpmSize == 0) {
    DEBUG ((DEBUG_INFO, "%a: No TPM present, skipping TPM2\n", __func__));
    return EFI_SUCCESS;
  }

  ZeroMem (&Tpm2, sizeof (Tpm2));

  //
  // TPM2 table header. Use revision 3 (matches this platform's default
  // PcdTpm2AcpiTableRev) since we do not populate PlatformClass or the
  // optional Laml/Lasa event-log fields introduced in revision 4+.
  //
  Tpm2.Header.Signature = EFI_ACPI_6_6_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE;
  Tpm2.Header.Length    = sizeof (Tpm2);
  Tpm2.Header.Revision  = EFI_TPM2_ACPI_TABLE_REVISION;
  CopyMem ((VOID *)&Tpm2.Header.OemId, ACPI_OEM_ID, sizeof (Tpm2.Header.OemId));
  CopyMem ((VOID *)&Tpm2.Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (Tpm2.Header.OemTableId));
  Tpm2.Header.OemRevision = 0x00000001;
  CopyMem ((VOID *)&Tpm2.Header.CreatorId, ACPI_CREATOR_ID, sizeof (Tpm2.Header.CreatorId));
  Tpm2.Header.CreatorRevision = ACPI_CREATOR_REVISION;

  //
  // No event log (Laml/Lasa) is advertised; the TIS interface exposes
  // its own log via the TPM itself.
  //
  Tpm2.Flags       = 0;
  Tpm2.StartMethod = EFI_TPM2_ACPI_TABLE_START_METHOD_TIS;

  Status = AllocateAndInstallAcpiTable (AcpiTable, &Tpm2, sizeof (Tpm2), &TableKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install TPM2: %r\n", __func__, Status));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: TPM2 installed (Base=0x%lx, Size=0x%lx, StartMethod=TIS)\n",
      __func__,
      Topo->TpmBase,
      Topo->TpmSize
      ));
  }

  return Status;
}
