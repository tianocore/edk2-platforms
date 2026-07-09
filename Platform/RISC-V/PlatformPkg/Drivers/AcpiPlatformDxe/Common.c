/** @file
  Common ACPI table helpers for the RISC-V ACPI driver.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

// ---------------------------------------------------------------------------
// ACPI Internal helpers
// ---------------------------------------------------------------------------

/**
  Compute and store the ACPI table checksum in the Checksum field of the
  EFI_ACPI_DESCRIPTION_HEADER at the start of Buffer.

  @param[in,out]  Buffer  Pointer to the ACPI table.
  @param[in]      Size    Total size of the table in bytes.
**/
VOID
EFIAPI
AcpiTableChecksum (
  IN OUT UINT8  *Buffer,
  IN     UINTN  Size
  )
{
  UINTN  ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  Buffer[ChecksumOffset] = 0;
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

/**
  Allocate EfiACPIReclaimMemory pages, copy the table data into them,
  fix up the Length field, compute the checksum, and install via
  EFI_ACPI_TABLE_PROTOCOL.

  @param[in]   AcpiTable   The ACPI table protocol.
  @param[in]   TableData   Pointer to the table data to install.
  @param[in]   TableSize   Size of the table in bytes.
  @param[out]  TableKey    Receives the installed table key.

  @retval EFI_SUCCESS           Table installed.
  @retval EFI_OUT_OF_RESOURCES  Page allocation failed.
  @retval other                 InstallAcpiTable failed.
**/
EFI_STATUS
EFIAPI
AllocateAndInstallAcpiTable (
  IN  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN  VOID                     *TableData,
  IN  UINTN                    TableSize,
  OUT UINTN                    *TableKey
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *Buffer;

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AllocatePages failed: %r\n", __func__, Status));
    return EFI_OUT_OF_RESOURCES;
  }

  Buffer = (UINT8 *)(UINTN)PageAddress;
  CopyMem (Buffer, TableData, TableSize);

  //
  // Fix up Length field in the common ACPI header.
  //
  ((EFI_ACPI_DESCRIPTION_HEADER *)Buffer)->Length = (UINT32)TableSize;

  AcpiTableChecksum (Buffer, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)Buffer,
                        TableSize,
                        TableKey
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InstallAcpiTable failed: %r\n", __func__, Status));
    gBS->FreePages (PageAddress, EFI_SIZE_TO_PAGES (TableSize));
  }

  return Status;
}
