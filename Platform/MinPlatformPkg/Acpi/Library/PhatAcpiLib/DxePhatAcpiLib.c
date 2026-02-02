/** @file
  Dxe Platform Health Assessment Table Library

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PhatAcpiLib.h>

#include <PiDxe.h>
#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/AcpiSystemDescriptionTable.h>

GLOBAL_REMOVE_IF_UNREFERENCED EFI_ACPI_TABLE_PROTOCOL  *mAcpiTableProtocol  = NULL;

/**
  Initialize the header of the Platform Health Assessment Table.

  @param[out]  Header     The header of the ACPI Table.
  @param[in]   OemId      The OEM ID.
  @param[in]   OemTableId The OEM table ID for the Phat.
**/
VOID
InitPhatTableHeader (
  OUT EFI_ACPI_DESCRIPTION_HEADER   *Header,
  IN  UINT8                         *OemId,
  IN  UINT64                        *OemTableId
  )
{
  ZeroMem (Header, sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  Header->Signature = EFI_ACPI_6_5_PLATFORM_HEALTH_ASSESSMENT_TABLE_SIGNATURE;
  //
  // total length (FVI, Driver Health).
  //
  Header->Length          = 0;
  Header->Revision        = EFI_ACPI_6_5_PLATFORM_HEALTH_ASSESSMENT_TABLE_REVISION;
  Header->Checksum        = 0;
  CopyMem (Header->OemId, OemId, sizeof (Header->OemId));
  CopyMem (&Header->OemTableId, OemTableId, sizeof (UINT64));
  Header->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  Header->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Header->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
}

/**
  This function scan ACPI table entry point.

  @retval ACPI table entry pointer
**/
VOID *
SearchAcpiTablePointer (
  VOID
  )
{
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Entry;
  EFI_STATUS                                    Status;

  Entry = NULL;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &mAcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[%a] Locate gEfiAcpiTableProtocolGuid failed with status: [%r].\n", __func__, Status));
    return NULL;
  }

  //
  // Find ACPI table RSD_PTR from the system table.
  //
  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **) &Rsdp);
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **) &Rsdp);
  }

  if (EFI_ERROR (Status) || (Rsdp == NULL)) {
    DEBUG ((DEBUG_INFO, "[%a] Can't find RSD_PTR from system table! \n", __func__));
    return NULL;
  } else if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION && Rsdp->XsdtAddress != 0) {
    Entry = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Rsdp->XsdtAddress;
  } else if (Rsdp->RsdtAddress != 0) {
    Entry = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Rsdp->RsdtAddress;
  }

  if (Entry == NULL) {
    DEBUG ((DEBUG_INFO, "[%a] XsdtAddress and RsdtAddress are NULL! \n", __func__));
    return NULL;
  }

  return Entry;
}

/**
  This function calculates and updates an UINT8 checksum.

  @param[in]  Buffer          Pointer to buffer to checksum
  @param[in]  Size            Number of bytes to checksum
**/
VOID
AcpiPlatformChecksum (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  UINTN ChecksumOffset;

  if (Buffer == NULL) {
    return;
  }

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  // Set checksum to 0 first
  Buffer[ChecksumOffset] = 0;

  // Update checksum value
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

/**
  Convert AIP data block to PHAT ACPI style, and publish it onto
  an existing ACPI  PHAT structure or initialize and install a new
  instance.

  @param[in]   InfoBlock          Point to AIP data block.
  @param[in]   InfoBlockSize      The size of AIP data.

  @retval EFI_SUCCESS             Success
  @retval EFI_OUT_OF_RESOURCES    Out of memory space.
  @retval EFI_INVALID_PARAMETER   Either InfoBlock is NULL,
                                  TableKey is NULL, or
                                  AcpiTableBufferSize and the size
                                  field embedded in the ACPI table
                                  pointed to by AcpiTableBuffer
                                  are not in sync.
  @retval EFI_ACCESS_DENIED       The table signature matches a table already
                                  present in the system and platform policy
                                  does not allow duplicate tables of this type.
  @retval EFI_NOT_FOUND           AcpiEntry is NULL.
**/
EFI_STATUS
EFIAPI
InstallPhatTable (
  IN  VOID        *InfoBlock,
  IN  UINTN       InfoBlockSize
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  *AcpiEntry;
  EFI_ACPI_SDT_PROTOCOL        *AcpiSdtProtocol;
  EFI_ACPI_DESCRIPTION_HEADER  *PhatHeader;
  UINT8                        *PhatTable;
  UINT32                       PhatLen;
  UINTN                        TableIndex;
  UINT8                        *TableHeader;
  EFI_ACPI_TABLE_VERSION       TableVersion;
  UINTN                        TableKey;

  if ((InfoBlock == NULL) || (InfoBlockSize == 0)) {
    DEBUG ((DEBUG_ERROR, "[%a] Table Data Invalid!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_SUCCESS;
  TableIndex  = 0;
  TableKey    = 0;
  TableHeader = NULL;

  AcpiEntry = SearchAcpiTablePointer ();
  if (AcpiEntry == NULL) {
    DEBUG((DEBUG_ERROR, "[%a] ACPI table pointer not found\n", __func__));
    return EFI_NOT_FOUND;
  }

  //
  // Locate the EFI_ACPI_SDT_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&AcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to locate AcpiSdt with status: %r\n", __func__, Status));
    return Status;
  }

  // Search ACPI table for PHAT
  while (!EFI_ERROR (Status)) {
    Status = AcpiSdtProtocol->GetAcpiTable (
                                 TableIndex,
                                 (EFI_ACPI_SDT_HEADER **)&TableHeader,
                                 &TableVersion,
                                 &TableKey
                                 );
    if (!EFI_ERROR (Status)) {
      TableIndex++;

      if (((EFI_ACPI_SDT_HEADER *) TableHeader)->Signature ==
          EFI_ACPI_6_5_PLATFORM_HEALTH_ASSESSMENT_TABLE_SIGNATURE)
      {
        DEBUG ((DEBUG_INFO, "[%a] Existing Phat AcpiTable is found.\n", __func__));
        break;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // A PHAT is already in the ACPI table, update existing table and re-install
    //
    PhatHeader = (EFI_ACPI_DESCRIPTION_HEADER *) TableHeader;
    PhatLen    = PhatHeader->Length + (UINT32) InfoBlockSize;
    PhatTable  = (UINT8 *) AllocateZeroPool (PhatLen);
    if (PhatTable == NULL) {
      DEBUG ((DEBUG_ERROR, "[%a] Failed to allocated new PHAT pool with.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    // Copy original table content to the new PHAT table pool
    CopyMem (PhatTable, TableHeader, PhatHeader->Length);

    // Append InfoBlock in the end of the origin PHAT
    CopyMem (PhatTable + PhatHeader->Length, InfoBlock, InfoBlockSize);

    // Update the PHAT head pointer.
    PhatHeader = (EFI_ACPI_DESCRIPTION_HEADER *) PhatTable;

    // Update the length field to found table plus appended new data
    PhatHeader->Length = PhatLen;

    // Uninstall the origin PHAT from the ACPI table.
    Status = mAcpiTableProtocol->UninstallAcpiTable (
                                    mAcpiTableProtocol,
                                    TableKey
                                    );
    ASSERT_EFI_ERROR (Status);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] Failed to uninstall existing PHAT ACPI table with status: %r\n", __func__, Status));
      FreePool (PhatTable);
      return Status;
    }
  } else {
    //
    // PHAT ACPI table does not exist, install new one
    //
    PhatTable = AllocateZeroPool (InfoBlockSize + sizeof (EFI_ACPI_DESCRIPTION_HEADER));
    if (PhatTable == NULL) {
      DEBUG ((DEBUG_ERROR, "[%a] Failed to allocate new PHAT pool.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }
    PhatHeader = (EFI_ACPI_DESCRIPTION_HEADER *) PhatTable;

    // Initialize the header of the Platform Health Assessment Table.
    InitPhatTableHeader (PhatHeader, AcpiEntry->OemId, &AcpiEntry->OemTableId);

    PhatHeader->Length = sizeof (EFI_ACPI_DESCRIPTION_HEADER) + (UINT32)InfoBlockSize;

    // Connect a telemetry data to ACPI table header.
    CopyMem (PhatTable + sizeof (EFI_ACPI_DESCRIPTION_HEADER), InfoBlock, InfoBlockSize);
  }

  // Update table checksum
  AcpiPlatformChecksum ((UINT8 *) PhatTable, ((EFI_ACPI_DESCRIPTION_HEADER *) PhatHeader)->Length);

  // Install or update the Phat table.
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                 mAcpiTableProtocol,
                                 PhatTable,
                                 PhatHeader->Length,
                                 &TableKey
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[%a] Install Phat AcpiTable failed, Status = [%r]. \n", __func__, Status));
  }

  if (PhatTable != NULL) {
    FreePool (PhatTable);
  }

  DEBUG ((DEBUG_INFO, "[%a] Install PHAT table, status: %r \n", __func__, Status));
  return Status;
}
