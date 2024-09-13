/** @file

  Generate ACPI MCFG table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciSegmentInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>

/**
  Implementation of AcpiMcfgLibConstructor for AMD platforms.
  This is library constructor for AcpiMcfgLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS     Successfully generated and installed MCFG Table.
  @retval Others          Failed to generate and install MCFG Table.
**/
EFI_STATUS
EFIAPI
AcpiMcfgLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                         *AcpiMcfgTable;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *McfgStructure;
  EFI_ACPI_TABLE_PROTOCOL                                                                *AcpiProtocol;
  EFI_STATUS                                                                             Status;
  PCI_SEGMENT_INFO                                                                       *PciSegmentInfo;
  UINTN                                                                                  Index;
  UINTN                                                                                  NewTableKey;
  UINTN                                                                                  PciSegmentCount;
  UINTN                                                                                  TableSize;

  /// MCFG Table Generation code goes here
  DEBUG ((DEBUG_INFO, "Generating ACPI MCFG Table.\n"));

  /// Get the PCI Segment Info
  PciSegmentInfo = GetPciSegmentInfo (&PciSegmentCount);
  if (PciSegmentInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to get PCI Segment Info.\n"));
    ASSERT (PciSegmentInfo != NULL);
    return EFI_NOT_FOUND;
  }

  /// allocate zero based memory for McfgStructure
  McfgStructure = AllocateZeroPool (
                    PciSegmentCount * \
                    sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE)
                    );
  if (McfgStructure == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for McfgStructure.\n"));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  /// Fill the McfgStructure
  for (Index = 0; Index < PciSegmentCount; Index++) {
    McfgStructure[Index].BaseAddress           = PciSegmentInfo[Index].BaseAddress;
    McfgStructure[Index].PciSegmentGroupNumber = PciSegmentInfo[Index].SegmentNumber;
    McfgStructure[Index].StartBusNumber        = PciSegmentInfo[Index].StartBusNumber;
    McfgStructure[Index].EndBusNumber          = PciSegmentInfo[Index].EndBusNumber;
  }

  /// Calculate the size of the MCFG Table
  TableSize = sizeof (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER) + \
              (PciSegmentCount * sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE));

  /// Allocate memory for MCFG Table
  AcpiMcfgTable = AllocateZeroPool (TableSize);
  if (AcpiMcfgTable == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for MCFG Table.\n"));
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    FreePool (McfgStructure);
    return EFI_OUT_OF_RESOURCES;
  }

  /// Locate ACPI Table Protocol
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate ACPI Table Protocol. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Update MCFG Table Header
  AcpiMcfgTable->Header.Signature       = EFI_ACPI_6_5_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE;
  AcpiMcfgTable->Header.Length          = (UINT32)TableSize;
  AcpiMcfgTable->Header.Revision        = EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION;
  AcpiMcfgTable->Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  AcpiMcfgTable->Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  AcpiMcfgTable->Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  AcpiMcfgTable->Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  CopyMem (
    &AcpiMcfgTable->Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (AcpiMcfgTable->Header.OemId)
    );

  /// Append the McfgStructure to MCFG Table
  CopyMem (
    (VOID *)((UINTN)AcpiMcfgTable + sizeof (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER)),
    McfgStructure,
    PciSegmentCount * sizeof (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE)
    );

  FreePool (McfgStructure);

  /// Install MCFG Table
  Status = AcpiProtocol->InstallAcpiTable (
                           AcpiProtocol,
                           AcpiMcfgTable,
                           AcpiMcfgTable->Header.Length,
                           &NewTableKey
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install MCFG Table. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  FreePool (AcpiMcfgTable);
  return Status;
}

/**
  Implementation of AcpiMcfgLibDestructor for AMD platforms.
  This is library destructor for AcpiMcfgLib.

  @param[in] ImageHandle  Image handle of the loaded driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
AcpiMcfgLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
