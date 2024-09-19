/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiApei.h"
#include "AcpiPlatform.h"

STATIC EFI_EVENT  mAcpiRegistration = NULL;

/*
 * This GUID must match the FILE_GUID in AcpiTables.inf of each boards
 */
STATIC CONST EFI_GUID  mAcpiCommonTableFile = {
  0xCEFA2AEB, 0x357E, 0x4F48, { 0x80, 0x66, 0xEA, 0x95, 0x08, 0x53, 0x05, 0x6E }
};
STATIC CONST EFI_GUID  mJadeAcpiTableFile = {
  0x5addbc13, 0x8634, 0x480c, { 0x9b, 0x94, 0x67, 0x1b, 0x78, 0x55, 0xcd, 0xb8 }
};
STATIC CONST EFI_GUID  mJadeAcpiTableAc02File = {
  0x5CA064B6, 0x5AA4, 0x4E29, { 0xAB, 0xDC, 0x8B, 0xF3, 0xB3, 0x4D, 0xBF, 0x9E }
};

/**
 * Callback called when ACPI Protocol is installed
 */
STATIC VOID
AcpiNotificationEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                                    Status;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;

  Status = LocateAndInstallAcpiFromFv (&mAcpiCommonTableFile);
  ASSERT_EFI_ERROR (Status);

  if (IsAc01Processor ()) {
    Status = LocateAndInstallAcpiFromFv (&mJadeAcpiTableFile);
  } else {
    Status = LocateAndInstallAcpiFromFv (&mJadeAcpiTableAc02File);
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Find ACPI table RSD_PTR from the system table.
  //
  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&Rsdp);
  }

  if (!EFI_ERROR (Status) &&
      (Rsdp != NULL) &&
      (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) &&
      (Rsdp->RsdtAddress != 0))
  {
    // ARM Platforms must set the RSDT address to NULL
    Rsdp->RsdtAddress = 0;
  }

  DEBUG ((DEBUG_INFO, "[%a:%d]-\n", __func__, __LINE__));
}

VOID
EFIAPI
InstallAcpiOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = AcpiInstallMadtTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed MADT table\n"));
  }

  Status = AcpiInstallPpttTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed PPTT table\n"));
  }

  Status = AcpiInstallSlitTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed SLIT table\n"));
  }

  Status = AcpiInstallSratTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed SRAT table\n"));
  }

  Status = AcpiInstallPcctTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed PCCT table\n"));
  }

  Status = AcpiInstallNfitTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed NFIT table\n"));
  }

  Status = AcpiInstallIort ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed IORT table\n"));
  }

  Status = AcpiInstallMcfg ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed MCFG table\n"));
  }

  Status = AcpiPopulateBert ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Populate BERT record\n"));
  }

  Status = AcpiInstallSpmiTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed SPMI table\n"));
  }

  Status = AcpiInstallBdatTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Installed BDAT table\n"));
  }

  //
  // Close the event, so it will not be signalled again.
  //
  gBS->CloseEvent (Event);
}

VOID
EFIAPI
UpdateAcpiOnExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = AcpiPatchDsdtTable ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "DSDT Table updated!\n"));
  }

  // Configure ACPI Platform Error Interfaces
  Status = AcpiApeiUpdate ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "APEI Table updated!\n"));
  }

  // Advertise shared memory regions to SMpro/PMpro and unmask interrupt
  Status = AcpiPcctInitializeSharedMemory ();
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "PCCT Table updated!\n"));
  }

  //
  // Close the event, so it will not be signalled again.
  //
  gBS->CloseEvent (Event);
}

EFI_STATUS
EFIAPI
AcpiPlatformDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   ReadyToBootEvent;
  EFI_EVENT   ExitBootServicesEvent;
  EFI_STATUS  Status;

  EfiCreateProtocolNotifyEvent (
    &gEfiAcpiTableProtocolGuid,
    TPL_CALLBACK,
    AcpiNotificationEvent,
    NULL,
    &mAcpiRegistration
    );

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  UpdateAcpiOnExitBootServices,
                  NULL,
                  &ExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InstallAcpiOnReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
