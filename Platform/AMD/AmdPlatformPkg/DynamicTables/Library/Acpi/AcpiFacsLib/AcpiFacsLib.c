/** @file

  Generate ACPI FACS table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi65.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>
#include <Guid/Acpi.h>

STATIC
EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE  mAcpiFacs = {
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE,
  sizeof (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE),
  /// Hardware Signature
  0x00000000,
  /// Firmware Waking Vector
  0x00000000,
  /// Global Lock
  0x00000000,
  /// Flags
  0x00000000,
  /// XFirmware Waking Vector
  0x0000000000000000,
  /// Version
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION,
  /// Reserved0
  { 0,                                                   0,  0 },
  /// OspmFlags
  0x00000000,
  /// Reserved1
  { 0,                                                   0,  0, 0, 0, 0, 0, 0, \
    0,                                                   0,  0, 0, 0, 0, 0, 0, \
    0,                                                   0,  0, 0, 0, 0, 0, 0}
};

/**
  Update the hardware signature in the FACS table.
**/
VOID
AcpiFacsTableUpdate (
  VOID
  )
{
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Table;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  EFI_STATUS                                    Status;
  UINT32                                        *RsdtPtr32;
  UINT32                                        CollectedCrc[2];
  UINT32                                        ComputedCrc;
  UINT32                                        TableCount;
  UINT64                                        XsdtTablePtr;
  UINTN                                         Index;
  UINTN                                         XsdtPtr;

  DEBUG ((DEBUG_INFO, "Updating hardware signature in FACS Table.\n"));

  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get RSDP. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  CollectedCrc[0] = Rsdp->Checksum;
  CollectedCrc[1] = Rsdp->ExtendedChecksum;
  gBS->CalculateCrc32 ((UINT8 *)CollectedCrc, ARRAY_SIZE (CollectedCrc), &ComputedCrc);
  CollectedCrc[0] = ComputedCrc;

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->XsdtAddress;
  Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->RsdtAddress;
  if (Xsdt != NULL) {
    CollectedCrc[1] = Xsdt->Checksum;
    gBS->CalculateCrc32 ((UINT8 *)CollectedCrc, ARRAY_SIZE (CollectedCrc), &ComputedCrc);
    CollectedCrc[0] = ComputedCrc;

    TableCount = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
    XsdtPtr    = (UINTN)(Xsdt + 1);
    for (Index = 0; Index < TableCount; Index++) {
      CopyMem (&XsdtTablePtr, (VOID *)(XsdtPtr + Index * sizeof (UINT64)), sizeof (UINT64));
      Table           = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(XsdtTablePtr));
      CollectedCrc[1] = Table->Checksum;
      gBS->CalculateCrc32 ((UINT8 *)CollectedCrc, ARRAY_SIZE (CollectedCrc), &ComputedCrc);
      CollectedCrc[0] = ComputedCrc;
    }
  } else if (Rsdt != NULL) {
    /// compute the CRC
    CollectedCrc[1] = Rsdt->Checksum;
    gBS->CalculateCrc32 ((UINT8 *)Rsdt, Rsdt->Length, &ComputedCrc);
    CollectedCrc[0] = ComputedCrc;

    TableCount = (Rsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
    RsdtPtr32  = (UINT32 *)(Rsdt + 1);
    for (Index = 0; Index < TableCount; Index++, RsdtPtr32++) {
      Table           = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(*RsdtPtr32));
      CollectedCrc[1] = Table->Checksum;
      gBS->CalculateCrc32 ((UINT8 *)CollectedCrc, ARRAY_SIZE (CollectedCrc), &ComputedCrc);
      CollectedCrc[0] = ComputedCrc;
    }
  }

  Facs = (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE *)EfiLocateFirstAcpiTable (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE);
  if (Facs != NULL) {
    /// Update FACS signature
    Facs->HardwareSignature = ComputedCrc;
  }

  return;
}

/**
  Event notification function for AcpiFacsLib.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context, which is
                      implementation-dependent.
**/
STATIC
VOID
EFIAPI
AcpiFacsLibEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  /// Close the Event
  gBS->CloseEvent (Event);

  /// Update the FACS Table
  AcpiFacsTableUpdate ();
}

/**
  Implementation of AcpiFacsLibConstructor for AMD platforms.
  This is library constructor for AcpiFacsLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated ACPI FACS Table.
  @retval Others          Failed to generate ACPI FACS Table.
**/
EFI_STATUS
EFIAPI
AcpiFacsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol;
  EFI_EVENT                Event;
  EFI_STATUS               Status;
  UINTN                    NewTableKey;

  // FACS Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI FACS Table.\n"));

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Install FACS Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           &mAcpiFacs,
                           sizeof (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE),
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install FACS Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Register notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiFacsLibEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Implementation of AcpiFacsLibDestructor for AMD platforms.
  This is library destructor for AcpiFacsLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiFacsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
