/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <IndustryStandard/ServiceProcessorManagementInterfaceTable.h>
#include <IndustryStandard/Acpi65.h>
#include "AcpiCommon.h"

EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE  gSpmi = {
  {
    EFI_ACPI_6_5_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE),
    5,
    //
    // Checksum will be updated at runtime
    //
    0x00,
    //
    // It is expected that these values will be programmed at runtime
    //
    { 'A', 'M', 'D', 'I', 'N', 'C' },
    SIGNATURE_64 ('S', 'P', 'M', 'I', 'T', 'a', 'b', 'l'), // updated during installation
    0x00,                                                  // Spmi revision,
    SIGNATURE_32 ('A', 'M', 'D', ' '),
    0x00 // OEM Revision
  },
  0x00,   // Interface type
  0x01,   // Reserved
  0x0200, // IPMI specification revision
  0x00,   // InterruptType
  0x00,   // Gpe
  0x00,   // Reserved2
  0x00,   // PciDeviceFlag or _UID
  0x00,   // GobalSystemInterrupt
  {       // BaseAddress
    EFI_ACPI_6_5_SYSTEM_IO,
    0x08,               // BASE_ADDRESS_BIT_WIDTH,
    0x00,               // BASE_ADDRESS_BIT_OFFSET,
    0x00,               // RESERVED_BYTE,
    0x0CA2              // BASE_ADDRESS_ADDRESS,
  },
  {
    { 0x00000000 }
  },
  0x00
};

/**
  Installs the ACPI SPMI Table to the System Table.
**/
VOID
EFIAPI
InstallAcpiSpmiTable (
  VOID
  )
{
  UINT64                   AcpiTableOemId;
  UINTN                    TurnKey;
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTablProtocol;

  if (!PcdGet8 (PcdIpmiInterfaceType)) {
    return;
  }

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTablProtocol
                  );
  if (EFI_ERROR (Status)) {
    // return if ACPI protocol not found
    return;
  }

  DEBUG ((DEBUG_ERROR, "Installing ACPI SPMI Table.\n"));
  // OEM info
  CopyMem (
    (VOID *)&gSpmi.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (gSpmi.Header.OemId)
    );

  AcpiTableOemId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (
    (VOID *)&gSpmi.Header.OemTableId,
    (VOID *)&AcpiTableOemId,
    sizeof (gSpmi.Header.OemTableId)
    );

  gSpmi.Header.OemRevision     = 0;
  gSpmi.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  gSpmi.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  gSpmi.InterfaceType       = PcdGet8 (PcdIpmiInterfaceType);
  gSpmi.BaseAddress.Address = PcdGet16 (PcdIpmiKcsIoBaseAddress);
  //
  // Add table
  //
  Status = AcpiTablProtocol->InstallAcpiTable (
                               AcpiTablProtocol,
                               &gSpmi,
                               sizeof (EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE),
                               &TurnKey
                               );
  ASSERT_EFI_ERROR (Status);
}
