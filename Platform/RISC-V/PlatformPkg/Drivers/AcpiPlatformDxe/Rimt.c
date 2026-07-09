/** @file
  RIMT table builder for the RISC-V ACPI driver.

  Layout emitted:
    [0]  EFI_ACPI_6_6_RIMT_STRUCTURE              (table header)
    [1]  EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE   (IOMMU node, RimtNodeIommu)
    [2]  EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE
         + NUL-terminated ACPI path string
         + EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE (Platform Device node)

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

#include <IndustryStandard/RiscVIoMappingTable.h>

//
// Full ACPI namespace path of the IOMMU platform device.
// The DSDT declares Device ("IMU0") under Scope ("\_SB"), so the full path
// produced by acpi_get_name(ACPI_FULL_PATHNAME) is "\_SB_.IMU0" (ACPI pads
// 4-character names with trailing underscores).
//
#define RIMT_IOMMU_ACPI_PATH      "\\_SB_.IMU0"
#define RIMT_IOMMU_ACPI_PATH_LEN  (AsciiStrLen ("\\_SB_.IMU0") + 1)

/**
  Build and install the RISC-V IO Mapping Table (RIMT). Only one IOMMU supported for now.

  The RIMT is required whenever a RISC-V IOMMU platform device is present.

  This function emits a minimal RIMT containing:
    - One IOMMU node           (RimtNodeIommu)    BaseAddress = Topo->IommuBase
    - One Platform Device node (RimtNodePlatform) DeviceObjectName =
      RIMT_IOMMU_ACPI_PATH, one ID mapping pointing to the IOMMU node

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology.

  @retval EFI_SUCCESS           RIMT installed, or no IOMMU present (skipped).
  @retval EFI_OUT_OF_RESOURCES  Buffer allocation failed.
  @retval other                 InstallAcpiTable failed.
**/
EFI_STATUS
EFIAPI
InstallRimt (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS  Status;
  UINTN       TableKey;
  UINTN       TableSize;
  UINT8       *Buffer;
  UINT8       *Ptr;
  UINT32      IommuNodeOffset;
  UINTN       IommuNodeSize;
  UINTN       PlatNodeSize;

  //
  // Skip if no IOMMU was discovered in the FDT.
  //
  if (Topo->IommuBase == 0) {
    DEBUG ((DEBUG_INFO, "%a: No IOMMU present, skipping RIMT\n", __func__));
    return EFI_SUCCESS;
  }

  //
  // Compute the size of each node.
  //
  // IOMMU node: EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE already includes the
  //             node header.  No interrupt wires on a platform IOMMU.
  //
  // Platform Device node: EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE
  //             already includes the node header.  DeviceObjectName is a
  //             flexible array member, so we add RIMT_IOMMU_ACPI_PATH_LEN
  //             bytes for the NUL-terminated path string, then one
  //             EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE for the single
  //             mapping to the IOMMU node.
  //
  IommuNodeSize = sizeof (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE);
  PlatNodeSize  = sizeof (EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE)
                  + RIMT_IOMMU_ACPI_PATH_LEN
                  + sizeof (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE);

  //
  // EFI_ACPI_6_6_RIMT_STRUCTURE already includes the standard ACPI header.
  //
  TableSize = sizeof (EFI_ACPI_6_6_RIMT_STRUCTURE) + IommuNodeSize + PlatNodeSize;

  Buffer = AllocateZeroPool (TableSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = Buffer;

  //
  // RIMT table header (EFI_ACPI_6_6_RIMT_STRUCTURE).
  //
  {
    EFI_ACPI_6_6_RIMT_STRUCTURE  *Hdr = (EFI_ACPI_6_6_RIMT_STRUCTURE *)Ptr;

    Hdr->Header.Signature = SIGNATURE_32 ('R', 'I', 'M', 'T');
    Hdr->Header.Length    = (UINT32)TableSize;
    Hdr->Header.Revision  = EFI_ACPI_6_6_RIMT_STRUCTURE_VERSION;
    CopyMem ((VOID *)&Hdr->Header.OemId, ACPI_OEM_ID, sizeof (Hdr->Header.OemId));
    CopyMem ((VOID *)&Hdr->Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (Hdr->Header.OemTableId));
    Hdr->Header.OemRevision = 0x00000001;
    CopyMem ((VOID *)&Hdr->Header.CreatorId, ACPI_CREATOR_ID, sizeof (Hdr->Header.CreatorId));
    Hdr->Header.CreatorRevision = ACPI_CREATOR_REVISION;
    Hdr->NumberOfRimtNodes      = 2;  // IOMMU node + Platform Device node
    Hdr->OffsetToRimtNodeArray  = sizeof (EFI_ACPI_6_6_RIMT_STRUCTURE);
    Hdr->Reserved               = 0;
    Ptr                        += sizeof (EFI_ACPI_6_6_RIMT_STRUCTURE);
  }

  //
  // IOMMU node (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE, type RimtNodeIommu).
  //
  IommuNodeOffset = (UINT32)(Ptr - Buffer);
  {
    EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE  *Node =
      (EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE *)Ptr;

    Node->Header.Type              = (UINT8)RimtNodeIommu;
    Node->Header.Revision          = EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE_VERSION;
    Node->Header.Length            = (UINT16)IommuNodeSize;
    Node->Header.Reserved          = 0;
    Node->Header.Id                = 0;
    Node->HardwareId               = 0;
    Node->BaseAddress              = Topo->IommuBase;
    Node->Flags                    = 0;
    Node->ProximityDomain          = 0;
    Node->PcieSegmentNumber        = 0;
    Node->PcieBdf                  = 0;
    Node->NumberOfInterruptWires   = 0;
    Node->InterruptWireArrayOffset = 0;
    Ptr                           += IommuNodeSize;
  }

  //
  // Platform Device node (EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE,
  // type RimtNodePlatform) for the IOMMU ACPI device "\_SB_.IMU0".
  //
  // IdMappingArrayOffset is relative to the start of this node.
  // The ID mapping immediately follows the fixed node header + path string.
  //
  {
    EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE  *Node =
      (EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE *)Ptr;
    EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE  *Map;

    Node->Header.Type     = (UINT8)RimtNodePlatform;
    Node->Header.Revision = EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE_VERSION;
    Node->Header.Length   = (UINT16)PlatNodeSize;
    Node->Header.Reserved = 0;
    Node->Header.Id       = 1;
    //
    // IdMappingArrayOffset: offset from the start of this node to the first
    // ID mapping.  The path string occupies the bytes immediately after the
    // fixed structure (which contains a zero-length DeviceObjectName[]).
    //
    Node->IdMappingArrayOffset = (UINT16)(
                                          sizeof (EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE)
                                          + RIMT_IOMMU_ACPI_PATH_LEN
                                          );
    Node->NumberOfIdMappings = 1;
    AsciiStrCpyS (
      (CHAR8 *)Node->DeviceObjectName,
      RIMT_IOMMU_ACPI_PATH_LEN,
      RIMT_IOMMU_ACPI_PATH
      );
    Ptr += sizeof (EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE)
           + RIMT_IOMMU_ACPI_PATH_LEN;

    //
    // Single ID mapping: all IDs from this device map to the IOMMU node.
    // DestinationIommuOffset is the byte offset from the table start to
    // the destination IOMMU node.
    //
    Map                          = (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE *)Ptr;
    Map->SourceIdBase            = 0;
    Map->NumberOfIDs             = 0xFFFFFFFF;
    Map->DestinationDeviceIdBase = 0;
    Map->DestinationIommuOffset  = IommuNodeOffset;
    Map->Flags                   = 0;
    Ptr                         += sizeof (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE);
  }

  AcpiTableChecksum (Buffer, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)Buffer,
                        TableSize,
                        &TableKey
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install RIMT: %r\n", __func__, Status));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: RIMT installed (IommuBase=0x%lx)\n",
      __func__,
      Topo->IommuBase
      ));
  }

  FreePool (Buffer);
  return Status;
}
