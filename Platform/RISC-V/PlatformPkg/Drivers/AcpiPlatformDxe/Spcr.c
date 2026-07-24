/** @file
  SPCR table builder for the RISC-V ACPI driver.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>

//
// Concrete Rev 4 SPCR layout: the standard
// EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4 ends with a zero-length
// flexible array NameSpaceString[0].  We append the two bytes needed for the
// root-scope path ".\0" in a wrapper struct so the compiler knows the exact
// allocation size.
//
#pragma pack(1)
typedef struct {
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4    Spcr;
  CHAR8                                               NameSpaceString[2];  // ".\0"
} SPCR_TABLE_4;
#pragma pack()

/**
  Build and install the Serial Port Console Redirection Table (SPCR).

  Describes the 16550-compatible UART.

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology (unused but kept for consistency).

  @retval EFI_SUCCESS  SPCR installed.
  @retval other        Installation failed.
**/
EFI_STATUS
EFIAPI
InstallSpcr (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS    Status;
  UINTN         TableKey;
  SPCR_TABLE_4  Table;

  ZeroMem (&Table, sizeof (Table));

  //
  // Standard ACPI table header.
  //
  Table.Spcr.Header.Signature = SIGNATURE_32 ('S', 'P', 'C', 'R');
  Table.Spcr.Header.Length    = sizeof (Table);
  Table.Spcr.Header.Revision  = EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION_4;
  CopyMem ((VOID *)&Table.Spcr.Header.OemId, ACPI_OEM_ID, sizeof (Table.Spcr.Header.OemId));
  CopyMem ((VOID *)&Table.Spcr.Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (Table.Spcr.Header.OemTableId));
  Table.Spcr.Header.OemRevision = 0x00000001;
  CopyMem ((VOID *)&Table.Spcr.Header.CreatorId, ACPI_CREATOR_ID, sizeof (Table.Spcr.Header.CreatorId));
  Table.Spcr.Header.CreatorRevision = ACPI_CREATOR_REVISION;

  //
  // 16550-compatible UART with parameters in Generic Address Structure.
  //
  Table.Spcr.InterfaceType = EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERFACE_TYPE_16550_WITH_GAS;

  //
  // MMIO base address, 8-bit register width, byte access.
  //
  Table.Spcr.BaseAddress.AddressSpaceId    = EFI_ACPI_6_6_SYSTEM_MEMORY;
  Table.Spcr.BaseAddress.RegisterBitWidth  = 8;
  Table.Spcr.BaseAddress.RegisterBitOffset = 0;
  Table.Spcr.BaseAddress.AccessSize        = EFI_ACPI_6_6_BYTE;
  Table.Spcr.BaseAddress.Address           = Topo->UartBase;

  //
  // Interrupt type: BIT4 = RISC-V PLIC/APLIC (SPCR Rev 4).
  //
  Table.Spcr.InterruptType         = BIT4;
  Table.Spcr.GlobalSystemInterrupt = Topo->UartIrq;

  Table.Spcr.BaudRate     = EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_115200;
  Table.Spcr.Parity       = EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_PARITY_NO_PARITY;
  Table.Spcr.StopBits     = EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_STOP_BITS_1;
  Table.Spcr.FlowControl  = 0;
  Table.Spcr.TerminalType = EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_ANSI;

  //
  // Not a PCI device.
  //
  Table.Spcr.PciDeviceId = 0xFFFF;
  Table.Spcr.PciVendorId = 0xFFFF;

  //
  // Namespace string "." (root scope), immediately following the fixed header.
  //
  Table.Spcr.NameSpaceStrLength = 2;
  Table.Spcr.NameSpaceStrOffset = (UINT16)OFFSET_OF (SPCR_TABLE_4, NameSpaceString);
  Table.NameSpaceString[0]      = '.';
  Table.NameSpaceString[1]      = '\0';

  Status = AllocateAndInstallAcpiTable (AcpiTable, &Table, sizeof (Table), &TableKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install SPCR: %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a: SPCR installed\n", __func__));
  }

  return Status;
}
