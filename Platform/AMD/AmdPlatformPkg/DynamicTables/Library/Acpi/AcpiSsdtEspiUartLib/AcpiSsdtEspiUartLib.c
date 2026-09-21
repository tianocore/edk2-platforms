/** @file

  Install ACPI SSDT eSPI UART table for AMD platforms.

  This module detects whether an eSPI UART is present by probing the FCH eSPI
  SLAVE0_DECODE_EN registers. If a UART I/O range (0x3F8 or 0x2F8) is enabled
  on either eSPI controller, the pre-compiled SSDT from AcpiSsdtEspiUart.asl
  is installed via the ACPI Table Protocol.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

#define FCH_ESPI0_BASE_ADDRESS  0xFEC20000
#define FCH_ESPI1_BASE_ADDRESS  0xFEC30000

#define FCH_ESPI_SLAVE0_DECODE_EN     0x40
#define FCH_ESPI_SLAVE0_IO_BASE_REG0  0x44
#define FCH_ESPI_SLAVE0_IO_BASE_REG1  0x48

#define ESPI_UART_COM1_ADDRESS  0x3F8
#define ESPI_UART_COM2_ADDRESS  0x2F8

#define ESPI_DECODE_EN_RANGE1  BIT9
#define ESPI_DECODE_EN_RANGE2  BIT10

extern CHAR8  acpissdtespiuart_aml_code[];

/**
  Check eSPI I/O ranges for UART addresses (0x3F8 or 0x2F8).

  Reads the eSPI SLAVE0_DECODE_EN register and checks:
    - Range 2 (0x3F8): REG1 bits[15:0], enable bit BIT10
    - Range 1 (0x2F8): REG0 bits[31:16], enable bit BIT9

  @param[in]  EspiBase  The eSPI base address.

  @retval  0x3F8 or 0x2F8 if eSPI UART is found and enabled.
  @retval  0 if eSPI UART is not found in enabled I/O ranges.
**/
STATIC
UINT16
GetEspiLegacyUartPort (
  IN  UINT32  EspiBase
  )
{
  UINT32  DecodeEnReg;
  UINT32  IoBaseReg;
  UINT16  IoAddress;

  DecodeEnReg = MmioRead32 (EspiBase + FCH_ESPI_SLAVE0_DECODE_EN);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: eSPI Base=0x%08X, SLAVE0_DECODE_EN=0x%08X\n",
    __func__,
    EspiBase,
    DecodeEnReg
    ));

  if ((DecodeEnReg & ESPI_DECODE_EN_RANGE2) != 0) {
    IoBaseReg = MmioRead32 (EspiBase + FCH_ESPI_SLAVE0_IO_BASE_REG1);
    IoAddress = (UINT16)(IoBaseReg & 0xFFFF);
    if (IoAddress == ESPI_UART_COM1_ADDRESS) {
      return IoAddress;
    }
  }

  if ((DecodeEnReg & ESPI_DECODE_EN_RANGE1) != 0) {
    IoBaseReg = MmioRead32 (EspiBase + FCH_ESPI_SLAVE0_IO_BASE_REG0);
    IoAddress = (UINT16)(IoBaseReg >> 16);
    if (IoAddress == ESPI_UART_COM2_ADDRESS) {
      return IoAddress;
    }
  }

  return 0;
}

/**
  Install ACPI SSDT eSPI UART table.
**/
VOID
AcpiSsdtEspiUartTableInstall (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_TABLE_PROTOCOL      *AcpiTableProtocol;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  UINTN                        TableHandle;

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
    return;
  }

  Table = (EFI_ACPI_DESCRIPTION_HEADER *)acpissdtespiuart_aml_code;

  if (Table->Signature != EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid table signature 0x%08X. Expected SSDT (0x%08X).\n",
      __func__,
      Table->Signature,
      EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
      ));
    return;
  }

  //
  // Update table header with platform OEM information from PCDs.
  // InstallAcpiTable() makes an internal copy of the table, so modifying
  // the original buffer before installation is safe.
  //
  CopyMem (&Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId));
  if (PcdGet64 (PcdAmdAcpiEspiUartSsdtOemTableId) != 0) {
    Table->OemTableId = PcdGet64 (PcdAmdAcpiEspiUartSsdtOemTableId);
  } else {
    Table->OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  }

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
      "%a: Failed to install SSDT eSPI UART table. Status = %r\n",
      __func__,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: SSDT eSPI UART table installed successfully. TableHandle = 0x%lX, Length = 0x%X\n",
      __func__,
      TableHandle,
      Table->Length
      ));
  }
}

/**
  Event notification function for AcpiSsdtEspiUartLib.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context, which is
                      implementation-dependent.
**/
STATIC
VOID
EFIAPI
AcpiSsdtEspiUartLibEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gBS->CloseEvent (Event);
  AcpiSsdtEspiUartTableInstall ();
}

/**
  Constructor for AcpiSsdtEspiUartLib.

  If eSPI UART decode is detected, registers Ready To Boot event to install the SSDT eSPI UART table.
  Otherwise, returns without registering event.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS           Ready To Boot event created or no eSPI UART detected (skip).
  @retval Others                Failed to create the Ready to Boot event.
**/
EFI_STATUS
EFIAPI
AcpiSsdtEspiUartLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT16      EspiPort;
  EFI_EVENT   Event;

  EspiPort = GetEspiLegacyUartPort (FCH_ESPI0_BASE_ADDRESS);
  if (EspiPort == 0) {
    EspiPort = GetEspiLegacyUartPort (FCH_ESPI1_BASE_ADDRESS);
  }

  if (EspiPort == 0) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: No eSPI UART detected, skipping SSDT installation\n",
      __func__
      ));
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: eSPI UART detected at IO port 0x%04X\n",
    __func__,
    EspiPort
    ));

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiSsdtEspiUartLibEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create ready-to-boot event. Status = %r\n",
      __func__,
      Status
      ));
  }

  return Status;
}

/**
  Destructor for AcpiSsdtEspiUartLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Always succeeds.
**/
EFI_STATUS
EFIAPI
AcpiSsdtEspiUartLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
