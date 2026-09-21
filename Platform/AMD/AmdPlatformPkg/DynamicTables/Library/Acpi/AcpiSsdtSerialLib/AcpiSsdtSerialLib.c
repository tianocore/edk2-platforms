/** @file

  Generate ACPI SSDT Serial table for AMD platforms.

  Copyright (c) 2019 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Protocol/AcpiTable.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "AcpiSsdtSerialLib.h"

EFI_ACPI_DESCRIPTION_HEADER  *mTable;
EFI_EVENT                    mEvent;

/**
  Install ACPI SSDT Serial table.
**/
VOID
AcpiSsdtSerialTableInstall (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  EFI_STATUS               Status;
  UINTN                    TableKey;

  // Add null pointer check for mTable
  if (mTable == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: mTable is NULL, cannot install table.\n"
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: SSDT-SERIAL: Installing ACPI SSDT Serial Table.\n"
    ));

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to locate ACPI Table Protocol. Status = %r\n",
      Status
      ));
    return;
  }

  // Initialize TableKey properly
  TableKey = 0;

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                mTable,
                                mTable->Length,
                                &TableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to install ACPI SSDT Table. Status = %r\n",
      Status
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: SSDT-SERIAL: ACPI SSDT Table installed successfully. TableKey = 0x%x\n",
    TableKey
    ));
}

/**
  Event notification function for AcpiSsdtSerialLib.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context, which is
                      implementation-dependent.
**/
STATIC
VOID
EFIAPI
AcpiSsdtSerialLibEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  /// Close the Event and clear the global handle
  gBS->CloseEvent (Event);
  mEvent = NULL;

  /// Update the SSDT Serial Table
  AcpiSsdtSerialTableInstall ();
}

/** Constructor for the AcpiSsdtSerialLib.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS successful initialization.
  @retval EFI_ERROR   Failed to create the Ready to Boot event.
**/
EFI_STATUS
EFIAPI
AcpiSsdtSerialLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  SERIAL_PORT_CONFIG  *SerialPortInfo;
  UINT8               SerialPortCount;

  mTable          = NULL;
  mEvent          = NULL;
  SerialPortInfo  = NULL;
  SerialPortCount = 0;

  if (!FixedPcdGetBool (PcdUsePlatformAcpi)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: PcdUsePlatformAcpi is FALSE, skipping SSDT Serial table\n",
      __func__
      ));
    return EFI_SUCCESS;
  }

  if (FixedPcdGetBool (PcdAcpiSsdtSerialSkipOnSimNow)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: PcdAcpiSsdtSerialSkipOnSimNow is set, skipping SSDT Serial table\n",
      __func__
      ));
    return EFI_SUCCESS;
  }

  Status = GetSocUartInfo (&SerialPortInfo, &SerialPortCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get SoC UART info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if ((SerialPortInfo == NULL) || (SerialPortCount == 0)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: No FCH UARTs enabled, skipping SSDT Serial table\n",
      __func__
      ));
    if (SerialPortInfo != NULL) {
      FreePool (SerialPortInfo);
    }

    return EFI_SUCCESS;
  }

  Status = AmdBuildSsdtSerialPortTable (
             SerialPortInfo,
             SerialPortCount,
             &mTable
             );

  FreePool (SerialPortInfo);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to build SSDT Serial table. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Update table header with platform OEM information from PCDs.
  // InstallAcpiTable() makes an internal copy of the table, so modifying
  // the original buffer before installation is safe.
  //
  CopyMem (&mTable->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTable->OemId));
  if (PcdGet64 (PcdAmdAcpiSerialSsdtOemTableId) != 0) {
    mTable->OemTableId = PcdGet64 (PcdAmdAcpiSerialSsdtOemTableId);
  } else {
    mTable->OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  }

  //
  // Table built successfully - register ready-to-boot event to install it
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiSsdtSerialLibEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mEvent
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

/** Destructor for the AcpiSsdtSerialLib.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS Always return success.
**/
EFI_STATUS
EFIAPI
AcpiSsdtSerialLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  // Cleanup event if not triggered yet
  if (mEvent != NULL) {
    gBS->CloseEvent (mEvent);
    mEvent = NULL;
  }

  // Cleanup allocated memory for mTable
  if (mTable != NULL) {
    FreePool (mTable);
    mTable = NULL;
  }

  return EFI_SUCCESS;
}
