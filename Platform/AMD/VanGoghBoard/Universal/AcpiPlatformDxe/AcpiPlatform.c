/** @file
  Sample ACPI Platform Driver

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Acpi.h>

#include "AcpiPlatformHooks.h"
#include <Protocol/GlobalNvsArea.h>

EFI_GLOBAL_NVS_AREA_PROTOCOL  mGlobalNvsArea;

/**
  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.

  @param  Instance      Return pointer to the first instance of the protocol

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateFvInstanceWithTables (
  OUT EFI_FIRMWARE_VOLUME2_PROTOCOL  **Instance
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          NumberOfHandles;
  EFI_FV_FILETYPE                FileType;
  UINT32                         FvStatus;
  EFI_FV_FILE_ATTRIBUTES         Attributes;
  UINTN                          Size;
  UINTN                          Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FvInstance;

  FvStatus = 0;

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    return Status;
  }

  //
  // Looking for FV with ACPI storage file
  //

  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&FvInstance
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // See if it has the ACPI storage file
    //
    Status = FvInstance->ReadFile (
                           FvInstance,
                           (EFI_GUID *)PcdGetPtr (PcdAcpiTableStorageFile),
                           NULL,
                           &Size,
                           &FileType,
                           &Attributes,
                           &FvStatus
                           );

    //
    // If we found it, then we are done
    //
    if (Status == EFI_SUCCESS) {
      *Instance = FvInstance;
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations
  // If the protocol was found, Instance already points to it.
  //

  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return Status;
}

/**
  This function calculates and updates an UINT8 checksum.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum

**/
VOID
AcpiPlatformChecksum (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )
{
  UINTN  ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

/**
  This function will update any runtime platform specific information.
  This currently includes:
    Setting OEM table values, ID, table ID, creator ID and creator revision.
    Enabling the proper processor entries in the APIC tables.

  @param[in]  Table       The table to update.

  @retval  EFI_SUCCESS    The function completed successfully.

**/
EFI_STATUS
PlatformUpdateTables (
  IN OUT EFI_ACPI_COMMON_HEADER  *Table
  )
{
  switch (Table->Signature) {
    case EFI_ACPI_5_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      //
      // Patch the memory resource.
      //
      PatchDsdtTable ((EFI_ACPI_DESCRIPTION_HEADER *)Table);
      break;

    case EFI_ACPI_5_0_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
      PatchMadtTable ((EFI_ACPI_DESCRIPTION_HEADER *)Table);
      break;

    default:
      break;
  }

  return EFI_SUCCESS;
}

/**
  Entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     AcpiStatus;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FwVol;
  INTN                           Instance;
  EFI_ACPI_COMMON_HEADER         *CurrentTable;
  UINTN                          TableHandle;
  UINT32                         FvStatus;
  UINTN                          TableSize;
  UINTN                          Size;
  EFI_HANDLE                     Handle;

  Instance     = 0;
  CurrentTable = NULL;
  TableHandle  = 0;

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Allocate and initialize the NVS area for SMM and ASL communication.
  //
  Status = gBS->AllocatePool (
                  EfiACPIMemoryNVS,
                  sizeof (EFI_GLOBAL_NVS_AREA),
                  (void **)&mGlobalNvsArea.Area
                  );
  ASSERT_EFI_ERROR (Status);
  gBS->SetMem (
         mGlobalNvsArea.Area,
         sizeof (EFI_GLOBAL_NVS_AREA),
         0
         );
  DEBUG ((DEBUG_INFO, "mGlobalNvsArea.Area is at 0x%X\n", mGlobalNvsArea.Area));

  //
  // Update global NVS area for ASL and SMM init code to use.
  //
  mGlobalNvsArea.Area->PcieBaseAddress = (UINT32)PcdGet64 (PcdPciExpressBaseAddress);
  mGlobalNvsArea.Area->PcieBaseLimit   = (UINT32)(PcdGet64 (PcdPciExpressBaseAddress) + (PcdGet32 (PcdPciExpressSize) - 1));

  if (FeaturePcdGet (PcdNbIoApicSupport)) {
    mGlobalNvsArea.Area->NbIoApic = TRUE;
  } else {
    mGlobalNvsArea.Area->NbIoApic = FALSE;
  }

  mGlobalNvsArea.Area->TopOfMem = (UINT32)AsmReadMsr64 (0xC001001A);

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiGlobalNvsAreaProtocolGuid,
                  &mGlobalNvsArea,
                  NULL
                  );

  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables (&FwVol);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Read tables from the storage file.
  //
  while (Status == EFI_SUCCESS) {
    Status = FwVol->ReadSection (
                      FwVol,
                      (EFI_GUID *)PcdGetPtr (PcdAcpiTableStorageFile),
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID **)&CurrentTable,
                      &Size,
                      &FvStatus
                      );
    if (!EFI_ERROR (Status)) {
      AcpiStatus = PlatformUpdateTables (CurrentTable);
      if (!EFI_ERROR (AcpiStatus)) {
        //
        // Add the table
        //
        TableHandle = 0;

        TableSize = ((EFI_ACPI_DESCRIPTION_HEADER *)CurrentTable)->Length;
        ASSERT (Size >= TableSize);

        //
        // Checksum ACPI table
        //
        AcpiPlatformChecksum ((UINT8 *)CurrentTable, TableSize);

        //
        // Install ACPI table
        //
        Status = AcpiTable->InstallAcpiTable (
                              AcpiTable,
                              CurrentTable,
                              TableSize,
                              &TableHandle
                              );
      }

      //
      // Free memory allocated by ReadSection
      //
      gBS->FreePool (CurrentTable);

      if (EFI_ERROR (Status)) {
        return EFI_ABORTED;
      }

      //
      // Increment the instance
      //
      Instance++;
      CurrentTable = NULL;
    }
  }

  //
  // The driver does not require to be kept loaded.
  //
  return EFI_SUCCESS;
}
