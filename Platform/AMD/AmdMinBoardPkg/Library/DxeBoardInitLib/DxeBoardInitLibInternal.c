/** @file
  BoardInitLib library internal implementation for DXE phase.

Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "DxeBoardInitLibInternal.h"

/**
  A helper function to uninstall or update the ACPI table.
  It searches for ACPI table for provided table signature,
  if found then creates a copy of the table and calls the callbackfunction.

  @param[in] Signature           ACPI table signature
  @param[in] CallbackFunction    The function to call to patch the searching ACPI table.
                                 If NULL then uninstalls the table.

  @return EFI_SUCCESS            Successfully Re-install the ACPI Table
  @return EFI_NOT_FOUND          Table not found
  @return EFI_STATUS             returns non-EFI_SUCCESS value in case of failure

**/
EFI_STATUS
EFIAPI
UpdateReinstallAcpiTable (
  IN UINT32           Signature,
  IN PATCH_ACPITABLE  CallbackFunction
  )
{
  EFI_ACPI_SDT_PROTOCOL    *AcpiSdtProtocol;
  EFI_STATUS               Status;
  UINTN                    Index;
  EFI_ACPI_SDT_HEADER      *Table;
  EFI_ACPI_TABLE_VERSION   Version;
  UINTN                    OriginalTableKey;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  EFI_ACPI_SDT_HEADER      *NewTable;
  UINTN                    NewTableKey;
  BOOLEAN                  Found;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error(%r): Unable to locate ACPI Table protocol.\n", Status));
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdtProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error(%r): Unable to locate ACPI SDT protocol.\n", Status));
    return Status;
  }

  Found = FALSE;
  Index = 0;
  do {
    Status = AcpiSdtProtocol->GetAcpiTable (Index, &Table, &Version, &OriginalTableKey);
    if (EFI_ERROR (Status)) {
      goto END_OF_SEARCH;
    }

    // Look for given table
    if (Table->Signature == Signature) {
      if (CallbackFunction == NULL) {
        Status = AcpiTableProtocol->UninstallAcpiTable (AcpiTableProtocol, OriginalTableKey);
        return Status;
      }

      NewTable = AllocateCopyPool (Table->Length, Table);
      if (NULL == NewTable) {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG ((DEBUG_ERROR, "Error(%r): Not enough resource to allocate table.\n", Status));
        return Status;
      }

      Status = CallbackFunction (NewTable);
      if (!EFI_ERROR (Status)) {
        // Uninstall the old table
        Status = AcpiTableProtocol->UninstallAcpiTable (AcpiTableProtocol, OriginalTableKey);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Error(%r): Uninstall old table error.\n", Status));
          FreePool (NewTable);
          return Status;
        }

        // Install the new table
        Status = AcpiTableProtocol->InstallAcpiTable (AcpiTableProtocol, NewTable, NewTable->Length, &NewTableKey);
        FreePool (NewTable);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Error(%r): Failed to install new table.\n", Status));
          return Status;
        }

        // If non SSDT table, then return status
        if (Table->Signature != EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
          return Status;
        }

        // Atleast one SSDT table update is success
        Found = TRUE;
      }

      // continue to search next SSDT table.
      Status = EFI_SUCCESS;
    }

    Index++;
  } while (!EFI_ERROR (Status));

END_OF_SEARCH:
  if (!Found) {
    DEBUG ((DEBUG_ERROR, "Error(%r): Unable to locate ACPI Table.\n", Status));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  A Callback function to patch the ACPI FADT table.
  Updates FADT table with AMD specific values, which
  are different than MinPlatformPkg.

  @param[in, out] NewTable       Pointer to ACPI FADT table

  @return         EFI_SUCCESS    Always return EFI_SUCCESSe

**/
EFI_STATUS
EFIAPI
FadtAcpiTablePatch (
  IN OUT  EFI_ACPI_SDT_HEADER  *NewTable
  )
{
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE  *NewFadt;

  NewFadt = (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE *)NewTable;
  // Patch the Table
  NewFadt->PLvl2Lat                  = 0x64;
  NewFadt->Pm2CntLen                 = 0;
  NewFadt->XGpe0Blk.RegisterBitWidth = 0x40;
  NewFadt->FlushSize                 = 0x400;
  NewFadt->FlushStride               = 0x10;
  NewFadt->XGpe1Blk.AccessSize       = 0x01;

  return EFI_SUCCESS;
}

/**
  A Callback function to patch the ACPI DSDT/SSDT table.
  Which has ASL code that needs to be updated.

  @param[in, out] NewTable       Pointer to ACPI FADT table

  @return         EFI_SUCCESS    If table is modified.
                  EFI_NOT_FOUND  If table is not modified.

**/
EFI_STATUS
EFIAPI
AcpiTableAmlUpdate (
  IN OUT  EFI_ACPI_SDT_HEADER  *NewTable
  )
{
  UINT64  OemTableId;

  if ((AsciiStrnCmp (NewTable->OemTableId, "AmdTable", 8) == 0)) {
    DEBUG ((DEBUG_INFO, "Found (D/S)SDT table for patching OemTableId.\n"));
    OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (NewTable->OemTableId, &OemTableId, 8);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Reserve Legay VGA IO space.

  @retval  EFI_SUCCESS  MMIO at Legacy VGA region has been allocated.
  @retval  !EFI_SUCCESS Error allocating the legacy VGA region.

**/
EFI_STATUS
EFIAPI
ReserveLegacyVgaIoSpace (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  VgaMemAddress;

  VgaMemAddress = (EFI_PHYSICAL_ADDRESS)VGA_MEM_BASE;
  Status        = gBS->AllocatePages (
                         AllocateAddress,
                         EfiMemoryMappedIO,
                         EFI_SIZE_TO_PAGES (VGA_MEM_SIZE),
                         &VgaMemAddress
                         );
  return Status;
}

/**
  Helper function to get size of MMIO region required for the Bus Range
  configured.

  @param[in]    BusRange      Chipset representation of Bus Range

  @retval                     Size of MMIO required for bus range
**/
UINT64
DecodeMmioBusRange (
  UINT64  BusRange
  )
{
  // Minimum MMIO region required is 1MB (1 Segment - 1 Bus).
  // Set Mmio Size to 1MB.
  UINT64  MmioSize;

  MmioSize = 0x100000;

  if (BusRange > 0x0E) {
    MmioSize = SIZE_32GB;
  } else {
    MmioSize = (MmioSize << BusRange);
  }

  return MmioSize;
}

/**
  Reserve PCIe Extended Config Space MMIO in the GCD and mark it runtime

  @param[in]  ImageHandle  ImageHandle of the loaded driver.
  @param[in]  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS  One or more of the drivers returned a success code.
  @retval  !EFI_SUCCESS  Error initializing the Legacy PIC.

**/
EFI_STATUS
EFIAPI
ReservePcieExtendedConfigSpace (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  AMD_MMIO_CFG_MSR  MmioCfgMsr;
  UINT64            MmioCfgBase;
  UINT64            MmioCfgSize;

  Status = EFI_SUCCESS;
  //
  // Reserve MMIO for PCI-Config space
  //
  MmioCfgMsr.AsUint64 = AsmReadMsr64 (AMD_MMIO_CFG_MSR_ADDR);
  MmioCfgBase         = MmioCfgMsr.AsUint64 & AMD_MMIO_CFG_ADDR_MASK;
  MmioCfgSize         = DecodeMmioBusRange (MmioCfgMsr.AsBits.BusRange);
  DEBUG ((DEBUG_INFO, "\nMMIO_CFG MSR = 0x%08lX\n", MmioCfgMsr.AsUint64));
  DEBUG ((DEBUG_INFO, "  Enable = %d\n", MmioCfgMsr.AsBits.Enable));
  DEBUG ((DEBUG_INFO, "  BusRange = %d\n", MmioCfgMsr.AsBits.BusRange));
  DEBUG ((DEBUG_INFO, "  MmioCfgBase = 0x%08lX\n", MmioCfgBase));
  DEBUG ((DEBUG_INFO, "  MmioCfgSize = 0x%08lX\n", MmioCfgSize));

  if (MmioCfgMsr.AsBits.Enable) {
    // Free Memory if it is allocated (call will likely return Not Found)
    Status = gDS->FreeMemorySpace (
                    MmioCfgBase,
                    MmioCfgSize
                    );
    // Remove Memory Space from GCD map (could return Not Found)
    Status = gDS->RemoveMemorySpace (
                    MmioCfgBase,
                    MmioCfgSize
                    );
    // Make sure Adding memory space succeeds or assert
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeReserved,
                    MmioCfgBase,
                    MmioCfgSize,
                    EFI_MEMORY_RUNTIME | EFI_MEMORY_UC
                    );
    ASSERT_EFI_ERROR (Status);
    // Make sure Allocating memory space succeed or assert
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    EfiGcdMemoryTypeReserved,
                    0,
                    MmioCfgSize,
                    &MmioCfgBase,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    DEBUG ((
      DEBUG_INFO,
      "\nReserved PciCfg MMIO: Base = 0x%lX, Size = 0x%lX\n",
      MmioCfgBase,
      MmioCfgSize
      ));
  }

  return Status;
}
