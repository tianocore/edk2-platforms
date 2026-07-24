/** @file
  MCFG table builder for the RISC-V ACPI driver.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

//
// Concrete MCFG table: standard header + one allocation entry.
//
#pragma pack(1)
typedef struct {
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                           Header;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE    Segment0;
} MCFG_TABLE;
#pragma pack()

/**
  Build and install the PCI Express Memory Mapped Configuration Space
  Base Address Description Table (MCFG).

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology.

  @retval EFI_SUCCESS           MCFG installed (or skipped — no PCIe present).
  @retval EFI_OUT_OF_RESOURCES  Buffer allocation failed.
  @retval other                 InstallAcpiTable failed.
**/
EFI_STATUS
EFIAPI
InstallMcfg (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS  Status;
  UINTN       TableKey;
  MCFG_TABLE  Mcfg;

  //
  // Skip if no PCIe ECAM window was discovered in the FDT.
  //
  if (Topo->PcieEcamBase == 0) {
    DEBUG ((DEBUG_INFO, "%a: No PCIe ECAM present, skipping MCFG\n", __func__));
    return EFI_SUCCESS;
  }

  ZeroMem (&Mcfg, sizeof (Mcfg));

  //
  // MCFG table header.
  //
  Mcfg.Header.Header.Signature = EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE;
  Mcfg.Header.Header.Length    = sizeof (Mcfg);
  Mcfg.Header.Header.Revision  = EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION;
  CopyMem ((VOID *)&Mcfg.Header.Header.OemId, ACPI_OEM_ID, sizeof (Mcfg.Header.Header.OemId));
  CopyMem ((VOID *)&Mcfg.Header.Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (Mcfg.Header.Header.OemTableId));
  Mcfg.Header.Header.OemRevision = 0x00000001;
  CopyMem ((VOID *)&Mcfg.Header.Header.CreatorId, ACPI_CREATOR_ID, sizeof (Mcfg.Header.Header.CreatorId));
  Mcfg.Header.Header.CreatorRevision = ACPI_CREATOR_REVISION;
  Mcfg.Header.Reserved               = 0;

  //
  // Single allocation entry: segment group 0, buses 0..EndBusNumber.
  //
  Mcfg.Segment0.BaseAddress           = Topo->PcieEcamBase;
  Mcfg.Segment0.PciSegmentGroupNumber = 0;
  Mcfg.Segment0.StartBusNumber        = Topo->PcieBusMin;
  Mcfg.Segment0.EndBusNumber          = Topo->PcieBusMax;
  Mcfg.Segment0.Reserved              = 0;

  Status = AllocateAndInstallAcpiTable (AcpiTable, &Mcfg, sizeof (Mcfg), &TableKey);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install MCFG: %r\n", __func__, Status));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: MCFG installed (EcamBase=0x%lx, buses 0x%02x..0x%02x)\n",
      __func__,
      Topo->PcieEcamBase,
      Topo->PcieBusMin,
      Topo->PcieBusMax
      ));
  }

  return Status;
}
