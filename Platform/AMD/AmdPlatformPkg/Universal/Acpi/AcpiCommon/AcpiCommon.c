/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiCommon.h"

EFI_ACPI_TABLE_PROTOCOL  *mAcpiTableProtocol;
EFI_ACPI_SDT_PROTOCOL    *mAcpiSdtProtocol;

/**
  Locates an existing ACPI Table

  @param[in]      Signature     - The Acpi table signature
  @param[in]      OemTableId    - The Acpi table OEM Table ID. Ignored if 0
  @param[out]     Table         - Table if Found or NULL

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
GetExistingAcpiTable (
  IN      UINT32                  Signature,
  IN      UINT64                  OemTableId,
     OUT  EFI_ACPI_SDT_HEADER     **Table
)
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  EFI_ACPI_SDT_HEADER         *LocalTable;
  EFI_ACPI_TABLE_VERSION      LocalVersion;
  UINTN                       LocalTableKey;

  Status = EFI_NOT_FOUND;
  *Table = NULL;

  for (Index = 0; ; Index++) {
    Status = mAcpiSdtProtocol->GetAcpiTable (Index, &LocalTable, &LocalVersion, &LocalTableKey);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!(LocalTable->Signature == Signature)) {
      continue;
    }
    // Accept table if OemTableId is zero.
    if (OemTableId == 0 ||
        CompareMem (&LocalTable->OemTableId, &OemTableId, 8) == 0) {
      *Table = LocalTable;
      return EFI_SUCCESS;
    }
  }
}

/**
  Appends generated AML to an existing ACPI Table

  1. Locate the existing ACPI table
  2. Allocate pool for original table plus new data size
  3. Copy original table to new buffer
  4. Append new data to buffer
  5. Update Table header length (Checksum will be calculated on install)
  6. Uninstall original ACPI table
  7. Install appended table
  8. Free new table buffer since ACPI made a copy.

  @param[in]      Signature     - The Acpi table signature
  @param[in]      OemId         - The Acpi table OEM ID
  @param[in]      AmlData       - The AML data to append

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
AppendExistingAcpiTable (
  IN      UINT32                       Signature,
  IN      UINT64                       OemId,
  IN      EFI_ACPI_DESCRIPTION_HEADER  *AmlData
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  EFI_ACPI_SDT_HEADER     *Table;
  EFI_ACPI_TABLE_VERSION  Version;
  UINTN                   TableKey;
  EFI_ACPI_SDT_HEADER     *ReplacementAcpiTable;
  UINT32                  ReplacementAcpiTableLength;
  UINTN                   TableHandle;

  for (Index = 0; ; Index++) {
    Status = mAcpiSdtProtocol->GetAcpiTable (Index, &Table, &Version, &TableKey);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: ACPI table not found with signature=0x%X\n", Signature));
      return Status;
    }

    if ((Table->Signature == Signature) &&
        (CompareMem (&Table->OemTableId, &OemId, 8) == 0))
    {
      break;
    }
  }

  // Calculate new DSDT Length and allocate space
  ReplacementAcpiTableLength = Table->Length + (UINT32)(AmlData->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER));
  ReplacementAcpiTable       =  AllocatePool (ReplacementAcpiTableLength);
  if (ReplacementAcpiTable == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Unable to allocate Replacement Table space.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Copy the old DSDT to the new buffer
  CopyMem (ReplacementAcpiTable, Table, Table->Length);
  // Append new data to DSDT
  CopyMem (
    (UINT8 *)ReplacementAcpiTable + Table->Length,
    (UINT8 *)AmlData + sizeof (EFI_ACPI_DESCRIPTION_HEADER),
    AmlData->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)
    );
  ReplacementAcpiTable->Length = ReplacementAcpiTableLength;

  // Uninstall the original DSDT
  Status = mAcpiTableProtocol->UninstallAcpiTable (
                                 mAcpiTableProtocol,
                                 TableKey
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Unable to uninstall original ACPI Table signature=0x%X\n", Signature));
  } else {
    // Install ACPI table
    Status = mAcpiTableProtocol->InstallAcpiTable (
                                   mAcpiTableProtocol,
                                   ReplacementAcpiTable,
                                   ReplacementAcpiTableLength,
                                   &TableHandle
                                   );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Unable to re-install ACPI Table signature=0x%X\n", Signature));
    }
  }

  // Release this copy of table
  FreePool (ReplacementAcpiTable);
  return Status;
}

/**
  Install common platform SSDTs and DSDT additions

  Place to install all generically identifiable SSDT tables.  These tables will
  be programmattically created from UEFI or AGESA resources and should cover
  many different Processor Family IPs.

  Might need to split this driver into LibraryClasses for each
  functionality/SSDT while keeping a single driver to reduce the AmlLib overhead.

  @param[in]      ImageHandle   - Standard UEFI entry point Image Handle
  @param[in]      SystemTable   - Standard UEFI entry point System Table

  @retval         EFI_SUCCESS, various EFI FAILUREs.
**/
EFI_STATUS
EFIAPI
InstallAllAcpiTables (
  IN      EFI_HANDLE        ImageHandle,
  IN      EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  // Get Acpi Table Protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get Acpi SDT Protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InstallCpuAcpi (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: CPU SSDT install error: Status=%r\n",
      __FUNCTION__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = InstallPciAcpi (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: PCI SSDT install error: Status=%r\n",
      __FUNCTION__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  InstallAcpiSpmiTable ();

  return Status;
}
