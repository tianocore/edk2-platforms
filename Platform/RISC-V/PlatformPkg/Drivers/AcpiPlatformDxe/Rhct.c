/** @file
  RHCT table builder for the RISC-V ACPI driver.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

/**
  Build and install the RISC-V Hart Capabilities Table (RHCT).

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology.

  @retval EFI_SUCCESS           RHCT installed.
  @retval EFI_OUT_OF_RESOURCES  Buffer allocation failed.
  @retval other                 InstallAcpiTable failed.
**/
EFI_STATUS
EFIAPI
InstallRhct (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN PLATFORM_TOPOLOGY        *Topo
  )
{
  EFI_STATUS            Status;
  UINTN                 TableKey;
  UINTN                 TableSize;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  UINT8                 *Buffer;
  UINT8                 *Ptr;
  UINTN                 HartIdx;
  UINT32                IsaOffset;
  UINT32                CmoOffset;
  UINT32                MmuOffset;
  UINTN                 IsaLen;
  UINTN                 IsaAlignedLen;
  UINT16                NumOffsets;
  UINT32                NumRhctNodes;

  //
  // ISA string length including NUL terminator, padded to 2-byte alignment
  // as required by the RHCT ISA String node format.
  //
  IsaLen        = AsciiStrLen (Topo->IsaString) + 1;
  IsaAlignedLen = (IsaLen % 2) ? (IsaLen + 1) : IsaLen;

  //
  // Count RHCT nodes: 1 ISA + optional CMO +  MMU + N Hart Info.
  //
  NumRhctNodes = 1 + Topo->NumHarts;
  if (Topo->HasZicbom || Topo->HasZicboz) {
    NumRhctNodes++;
  }

  // Count MMU node
  NumRhctNodes++;

  //
  // Count the number of node offsets each Hart Info node will carry.
  // Hart Info always references the ISA String, MMU nodes; CMO is
  // optional.
  //
  NumOffsets = 2; // ISA String + MMU
  if (Topo->HasZicbom || Topo->HasZicboz) {
    NumOffsets++;
  }

  //
  // Calculate total table size.
  //
  TableSize = sizeof (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE);

  // Add ISA string node size
  TableSize += sizeof (EFI_ACPI_6_6_RHCT_ISA_STRING_NODE) + IsaAlignedLen;

  // Add CMO node size if needed
  if (Topo->HasZicbom || Topo->HasZicboz) {
    TableSize += sizeof (EFI_ACPI_6_6_RHCT_CMO_NODE);
  }

  // Add MMU node size
  TableSize += sizeof (EFI_ACPI_6_6_RHCT_MMU_NODE);

  // Add hart nodes size
  TableSize += Topo->NumHarts * (sizeof (EFI_ACPI_6_6_RHCT_HART_INFO_NODE) + NumOffsets * sizeof (UINT32));

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  EFI_SIZE_TO_PAGES (TableSize),
                  &PageAddress
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Buffer = (UINT8 *)(UINTN)PageAddress;
  ZeroMem (Buffer, TableSize);
  Ptr = Buffer;

  //
  // RHCT header
  //
  {
    EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE  *RhctHdrPtr = (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE *)Ptr;

    RhctHdrPtr->Header.Signature = SIGNATURE_32 ('R', 'H', 'C', 'T');
    RhctHdrPtr->Header.Length    = (UINT32)TableSize;
    RhctHdrPtr->Header.Revision  = EFI_ACPI_6_6_RHCT_TABLE_REVISION;
    CopyMem ((VOID *)&RhctHdrPtr->Header.OemId, ACPI_OEM_ID, sizeof (RhctHdrPtr->Header.OemId));
    CopyMem ((VOID *)&RhctHdrPtr->Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (RhctHdrPtr->Header.OemTableId));
    RhctHdrPtr->Header.OemRevision = 0x00000001;
    CopyMem ((VOID *)&RhctHdrPtr->Header.CreatorId, ACPI_CREATOR_ID, sizeof (RhctHdrPtr->Header.CreatorId));
    RhctHdrPtr->Header.CreatorRevision = ACPI_CREATOR_REVISION;
    RhctHdrPtr->Flags                  = 0;
    RhctHdrPtr->TimeBaseFreq           = Topo->TimebaseFrequency;
    RhctHdrPtr->NodeCount              = NumRhctNodes;
    RhctHdrPtr->NodeOffset             = sizeof (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE);
    Ptr                               += sizeof (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE);
  }

  //
  // ISA String node.
  //
  {
    EFI_ACPI_6_6_RHCT_ISA_STRING_NODE  *RhctIsaStrPtr =  (EFI_ACPI_6_6_RHCT_ISA_STRING_NODE *)Ptr;

    IsaOffset                    = (UINT32)(Ptr - Buffer);
    RhctIsaStrPtr->Node.Type     = EFI_ACPI_6_6_RHCT_NODE_TYPE_ISA_STRING;
    RhctIsaStrPtr->Node.Length   = (UINT16)(sizeof (EFI_ACPI_6_6_RHCT_ISA_STRING_NODE) + IsaAlignedLen);
    RhctIsaStrPtr->Node.Revision = EFI_ACPI_6_6_RHCT_ISA_NODE_STRUCTURE_VERSION;
    RhctIsaStrPtr->IsaLength     = (UINT16)IsaLen;         // ISA string length incl. NUL
    CopyMem ((VOID *)RhctIsaStrPtr->Isa, (VOID *)Topo->IsaString, IsaLen);
    Ptr += RhctIsaStrPtr->Node.Length;
  }
  //
  // CMO node - optional.
  //
  CmoOffset = 0;
  if (Topo->HasZicbom || Topo->HasZicboz) {
    EFI_ACPI_6_6_RHCT_CMO_NODE  *RhctCmoPtr = (EFI_ACPI_6_6_RHCT_CMO_NODE *)Ptr;

    CmoOffset                 = (UINT32)(Ptr - Buffer);
    RhctCmoPtr->Node.Type     = EFI_ACPI_6_6_RHCT_NODE_TYPE_CMO;
    RhctCmoPtr->Node.Length   = (UINT16)sizeof (EFI_ACPI_6_6_RHCT_CMO_NODE);
    RhctCmoPtr->Node.Revision = EFI_ACPI_6_6_RHCT_CMO_NODE_STRUCTURE_VERSION;
    RhctCmoPtr->Reserved      = 0;
    RhctCmoPtr->CbomBlockSize = Topo->HasZicbom ? (UINT8)Topo->CbomBlockSize : 0;
    RhctCmoPtr->CbopBlockSize = 0; // CBOP not supported yet
    RhctCmoPtr->CbozBlockSize = Topo->HasZicboz ? (UINT8)Topo->CbozBlockSize : 0;
    Ptr                      += RhctCmoPtr->Node.Length;
  }

  //
  // MMU node.
  //
  {
    EFI_ACPI_6_6_RHCT_MMU_NODE  *RhctMmuPtr = (EFI_ACPI_6_6_RHCT_MMU_NODE *)Ptr;

    MmuOffset                 = (UINT32)(Ptr - Buffer);
    RhctMmuPtr->Node.Type     = EFI_ACPI_6_6_RHCT_NODE_TYPE_MMU;
    RhctMmuPtr->Node.Length   = (UINT16)sizeof (EFI_ACPI_6_6_RHCT_MMU_NODE);
    RhctMmuPtr->Node.Revision = EFI_ACPI_6_6_RHCT_MMU_NODE_STRUCTURE_VERSION;
    RhctMmuPtr->MmuType       = Topo->MmuType; // Sv39=0, Sv48=1, Sv57=2
    Ptr                      += RhctMmuPtr->Node.Length;
  }

  //
  // Hart Info nodes.
  //
  {
    EFI_ACPI_6_6_RHCT_HART_INFO_NODE  *RhctHartInfoPtr;
    UINT32                            *OffsetPtr;

    for (HartIdx = 0; HartIdx < Topo->NumHarts; HartIdx++) {
      RhctHartInfoPtr                = (EFI_ACPI_6_6_RHCT_HART_INFO_NODE *)Ptr;
      RhctHartInfoPtr->Node.Type     = EFI_ACPI_6_6_RHCT_NODE_TYPE_HART_INFO;
      RhctHartInfoPtr->Node.Length   = (UINT16)(sizeof (EFI_ACPI_6_6_RHCT_HART_INFO_NODE) + NumOffsets * sizeof (UINT32));
      RhctHartInfoPtr->Node.Revision = EFI_ACPI_6_6_RHCT_HART_INFO_NODE_STRUCTURE_VERSION;
      RhctHartInfoPtr->NumOffsets    = NumOffsets;
      RhctHartInfoPtr->Uid           = (UINT32)HartIdx;
      OffsetPtr                      = RhctHartInfoPtr->Offsets;
      *OffsetPtr++                   = IsaOffset;
      if (CmoOffset != 0) {
        *OffsetPtr++ = CmoOffset;
      }

      *OffsetPtr = MmuOffset;
      Ptr       += RhctHartInfoPtr->Node.Length;
    }
  }

  AcpiTableChecksum (Buffer, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)Buffer,
                        TableSize,
                        &TableKey
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install RHCT: %r\n", __func__, Status));
    gBS->FreePages (PageAddress, EFI_SIZE_TO_PAGES (TableSize));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: RHCT installed (%u nodes, mmu=%u)\n",
      __func__,
      NumRhctNodes,
      Topo->MmuType
      ));
  }

  return Status;
}
