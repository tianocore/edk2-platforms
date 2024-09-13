/** @file

  Generate ACPI FADT table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>
#include <IndustryStandard/Acpi.h>

/// FADT Table Definition
STATIC
EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE  mAcpiFadt = {
  {
    EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE),
    EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
    0x00,
    { 0, 0, 0, 0, 0, 0 },
    0,
    0,
    0,
    0
  },
  /// Firmware Control Structure
  0,
  /// DSDT
  0,
  /// Reserved
  EFI_ACPI_RESERVED_BYTE,
  /// Preferred Power Management Profile
  EFI_ACPI_6_5_PM_PROFILE_ENTERPRISE_SERVER,
  /// SCI Interrupt
  0x0009,
  /// SMI Command Port
  0x000000B2,
  /// ACPI Enable
  0xA0,
  /// ACPI Disable
  0xA1,
  /// S4BIOS Request
  0x00,
  /// PSTATE Control
  0x00,
  /// PM1a Event Block Address
  0x00000800,
  /// PM1b Event Block Address
  0x00000000,
  /// PM1a Control Block Address
  0x00000804,
  /// PM1b Control Block Address
  0x00000000,
  /// PM2 Control Block Address
  0x00000000,
  /// PM Timer Block Address
  0x00000808,
  /// GPE0 Block Address
  0x00000820,
  /// GPE1 Block Address
  0x00000000,
  /// PM1 Event Length
  0x04,
  /// PM1 Control Length
  0x02,
  /// PM2 Control Length
  0x00,
  /// PM Timer Length
  0x04,
  /// GPE0 Length
  0x08,
  /// GPE1 Length
  0x00,
  /// GPE1 Base
  0x00,
  /// C State Control
  0x00,
  /// Worst C2 Latency
  0x0064,
  /// Worst C3 Latency
  0x03E9,
  /// Flush Size
  0x0400,
  /// Flush Stride
  0x0010,
  /// Duty Offset
  0x01,
  /// Duty Width
  0x00,
  /// Day Alarm
  0x0D,
  /// Month Alarm
  0x00,
  /// Century
  0x32,
  /// IAPC Boot Arch
  0x00,
  /// Reserved
  0x00,
  /// Flags
  0x0002052D,
  /// Reset Register
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x08,
    0x00,
    EFI_ACPI_6_5_BYTE,
    0x0000000000000CF9
  },
  /// Reset Value
  0x06,
  /// Arm Boot Arch
  0x0000,
  /// Minor Version
  0x05,
  /// XFirmware Control
  0x00,
  /// XDsdt
  0x00000000,
  /// XPm1a Event Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x20,
    0x00,
    EFI_ACPI_6_5_WORD,
    0x0000000000000800
  },
  /// XPm1b Event Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x00,
    0x00,
    EFI_ACPI_6_5_UNDEFINED,
    0x0000000000000000
  },
  /// XPm1a Control Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x10,
    0x00,
    EFI_ACPI_6_5_WORD,
    0x0000000000000804
  },
  /// XPm1b Control Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x00,
    0x00,
    EFI_ACPI_6_5_UNDEFINED,
    0x0000000000000000
  },
  /// XPm2 Control Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x00,
    0x00,
    EFI_ACPI_6_5_UNDEFINED,
    0x0000000000000000
  },
  /// XPm Timer Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x20,
    0x00,
    EFI_ACPI_6_5_DWORD,
    0x0000000000000808
  },
  /// XGpe0 Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x40,
    0x00,
    EFI_ACPI_6_5_BYTE,
    0x0000000000000820
  },
  /// XGpe1 Block
  {
    EFI_ACPI_6_5_SYSTEM_IO,
    0x00,
    0x00,
    EFI_ACPI_6_5_BYTE,
    0x0000000000000000
  },
  /// Sleep Control Register
  {
    EFI_ACPI_6_5_SYSTEM_MEMORY,
    0x00,
    0x00,
    EFI_ACPI_6_5_UNDEFINED,
    0x0000000000000000
  },
  /// Sleep Status Register
  {
    EFI_ACPI_6_5_SYSTEM_MEMORY,
    0x00,
    0x00,
    EFI_ACPI_6_5_UNDEFINED,
    0x0000000000000000
  },
  /// Hypervisor Vendor Identity
  0x0000000000000000
};

/**
  Implementation of AcpiFadtLibConstructor for AMD platforms.
  This is library constructor for AcpiFadtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated and installed FADT Table.
  @retval Others          Failed to generate and install FADT Table.
**/
EFI_STATUS
EFIAPI
AcpiFadtLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol;
  EFI_STATUS               Status;
  UINTN                    NewTableKey;

  /// FADT Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI FADT Table.\n"));

  /// Update FADT Table Header
  mAcpiFadt.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mAcpiFadt.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  mAcpiFadt.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mAcpiFadt.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  CopyMem (&mAcpiFadt.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mAcpiFadt.Header.OemId));

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Install FADT Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           &mAcpiFadt,
                           sizeof (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE),
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install FADT Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Implementation of AcpiFadtLibDestructor for AMD platforms.
  This is library destructor for AcpiFadtLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiFadtLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
