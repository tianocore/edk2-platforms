/** @file
  MADT table builder for the RISC-V ACPI driver.

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatformDxe.h"

/**
  Compute the number of bits needed to represent values 0..N-1.
  Returns 0 for N <= 1.

  @param[in]  N  Upper bound (exclusive).

  @return  Bit width required.
**/
UINT8
EFIAPI
ImsicNumBits (
  IN UINT32  N
  )
{
  UINT8  Bits;

  if (N <= 1) {
    return 0;
  }

  Bits = 0;
  N--;
  while (N > 0) {
    Bits++;
    N >>= 1;
  }

  return Bits;
}

/**
  Generate the bytes for an RINTC structure.

  @param[in]  Topo             Platform topology.
  @param[in]  Rintc            RINTC structure to fill.
  @param[in]  GuestIndexBits   Number of bits in the IMSIC guest index
  @param[in]  HartIdx          Index of the hart for which to generate bytes.

**/
VOID
EFIAPI
GenerateRintcBytes (
  IN PLATFORM_TOPOLOGY             *Topo,
  IN EFI_ACPI_6_6_RINTC_STRUCTURE  *Rintc,
  IN UINT8                         GuestIndexBits,
  IN UINTN                         HartIdx
  )
{
  UINT32  ImsicSize;
  UINT64  ImsicAddr;

  Rintc->Type     = EFI_ACPI_6_6_RINTC;
  Rintc->Length   = sizeof (EFI_ACPI_6_6_RINTC_STRUCTURE);
  Rintc->Version  = EFI_ACPI_6_6_RINTC_STRUCTURE_VERSION;
  Rintc->Reserved = 0;
  Rintc->Flags    = EFI_ACPI_6_6_RINTC_FLAG_ENABLE;
  Rintc->HartId   = Topo->HartIds[HartIdx];
  Rintc->Uid      = (UINT32)HartIdx;

  //
  // ExtIntcId encodes socket and local hart index.
  // Format: (socket_id << 24) | local_cpu_id
  //
  Rintc->ExtIntcId = (Topo->SocketId[HartIdx] << 24) | Topo->LocalCpuId[HartIdx];

  //
  // Per-hart IMSIC MMIO address and size.
  //
  ImsicSize = (1u << GuestIndexBits) * 0x1000;
  ImsicAddr = Topo->ImsicBase
              + (Topo->SocketId[HartIdx] * Topo->ImsicGroupMaxSize)
              + (Topo->LocalCpuId[HartIdx] * ImsicSize);
  Rintc->ImsicAddr = ImsicAddr;
  Rintc->ImsicSize = ImsicSize;
}

/**
  Build and install the Multiple APIC Description Table (MADT).

  The MADT contains one EFI_ACPI_6_6_RINTC_STRUCTURE per hart, one
  EFI_ACPI_6_6_IMSIC_STRUCTURE for the platform, and one
  EFI_ACPI_6_6_APLIC_STRUCTURE per socket.

  @param[in]  AcpiTable  The ACPI table protocol.
  @param[in]  Topo       Platform topology.

  @retval EFI_SUCCESS           MADT installed.
  @retval EFI_OUT_OF_RESOURCES  Buffer allocation failed.
  @retval other                 InstallAcpiTable failed.
**/
EFI_STATUS
EFIAPI
InstallMadt (
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
  UINT32                SocketIdx;
  UINT8                 GuestIndexBits;
  UINT8                 HartIndexBits;
  UINT8                 GroupIndexBits;
  UINT16                MaxHartsPerSocket;

  //
  // Compute IMSIC geometry bits up front; needed for both size calculation
  // and for filling RINTC ImsicAddr/ImsicSize fields.
  //
  GuestIndexBits = ImsicNumBits (Topo->AiaGuests + 1);
  DEBUG ((DEBUG_INFO, "%a: GuestIndexBits=%u, AiaGuests=%u\n", __func__, GuestIndexBits, Topo->AiaGuests));
  MaxHartsPerSocket = 0;
  for (SocketIdx = 0; SocketIdx < Topo->NumSockets; SocketIdx++) {
    if (Topo->HartsPerSocket[SocketIdx] > MaxHartsPerSocket) {
      MaxHartsPerSocket = (UINT16)Topo->HartsPerSocket[SocketIdx];
    }
  }

  HartIndexBits  = ImsicNumBits (MaxHartsPerSocket);
  GroupIndexBits = ImsicNumBits (Topo->NumSockets);

  //
  // Calculate total table size.
  //
  TableSize = sizeof (EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER)
              + (Topo->NumHarts * sizeof (EFI_ACPI_6_6_RINTC_STRUCTURE))
              + sizeof (EFI_ACPI_6_6_IMSIC_STRUCTURE)
              + (Topo->NumSockets * sizeof (EFI_ACPI_6_6_APLIC_STRUCTURE));

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
  // MADT header (EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER).
  //
  {
    EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *Hdr =
      (EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)Ptr;

    Hdr->Header.Signature = EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE;
    Hdr->Header.Length    = (UINT32)TableSize;
    Hdr->Header.Revision  = EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION;
    CopyMem ((VOID *)&Hdr->Header.OemId, ACPI_OEM_ID, sizeof (Hdr->Header.OemId));
    CopyMem ((VOID *)&Hdr->Header.OemTableId, ACPI_OEM_TABLE_ID, sizeof (Hdr->Header.OemTableId));
    Hdr->Header.OemRevision = 0x00000001;
    CopyMem ((VOID *)&Hdr->Header.CreatorId, ACPI_CREATOR_ID, sizeof (Hdr->Header.CreatorId));
    Hdr->Header.CreatorRevision = ACPI_CREATOR_REVISION;
    Hdr->LocalApicAddress       = 0;
    Hdr->Flags                  = 0;
    Ptr                        += sizeof (EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  }

  //
  // RINTC structures (EFI_ACPI_6_6_RINTC_STRUCTURE) - one per hart.
  //
  for (HartIdx = 0; HartIdx < Topo->NumHarts; HartIdx++) {
    EFI_ACPI_6_6_RINTC_STRUCTURE  *Rintc = (EFI_ACPI_6_6_RINTC_STRUCTURE *)Ptr;
    GenerateRintcBytes (
      Topo,
      Rintc,
      GuestIndexBits,
      HartIdx
      );
    Ptr += sizeof (EFI_ACPI_6_6_RINTC_STRUCTURE);
  }

  //
  // IMSIC structure (EFI_ACPI_6_6_IMSIC_STRUCTURE) - one per platform.
  //
  {
    EFI_ACPI_6_6_IMSIC_STRUCTURE  *Imsic = (EFI_ACPI_6_6_IMSIC_STRUCTURE *)Ptr;

    Imsic->Type            = EFI_ACPI_6_6_IMSIC;
    Imsic->Length          = sizeof (EFI_ACPI_6_6_IMSIC_STRUCTURE);
    Imsic->Version         = EFI_ACPI_6_6_IMSIC_STRUCTURE_VERSION;
    Imsic->Reserved        = 0;
    Imsic->Flags           = 0;
    Imsic->NumIds          = (UINT16)Topo->ImsicNumIds;
    Imsic->NumGuestIds     = (UINT16)Topo->ImsicNumIds;
    Imsic->GuestIndexBits  = GuestIndexBits;
    Imsic->HartIndexBits   = HartIndexBits;
    Imsic->GroupIndexBits  = GroupIndexBits;
    Imsic->GroupIndexShift = (UINT8)Topo->ImsicGroupShift;
    Ptr                   += sizeof (EFI_ACPI_6_6_IMSIC_STRUCTURE);
  }

  //
  // APLIC structures (EFI_ACPI_6_6_APLIC_STRUCTURE) - one per socket.
  //
  for (SocketIdx = 0; SocketIdx < Topo->NumSockets; SocketIdx++) {
    EFI_ACPI_6_6_APLIC_STRUCTURE  *Aplic = (EFI_ACPI_6_6_APLIC_STRUCTURE *)Ptr;

    Aplic->Type    = EFI_ACPI_6_6_APLIC;
    Aplic->Length  = sizeof (EFI_ACPI_6_6_APLIC_STRUCTURE);
    Aplic->Version = EFI_ACPI_6_6_APLIC_STRUCTURE_VERSION;
    Aplic->Id      = (UINT8)SocketIdx;
    Aplic->Flags   = 0;
    ZeroMem (Aplic->HwId, sizeof (Aplic->HwId));
    Aplic->NumIdcs    = (UINT16)Topo->HartsPerSocket[SocketIdx];
    Aplic->NumSources = (UINT16)Topo->Intc[SocketIdx].NumSources;
    Aplic->GsiBase    = Topo->Intc[SocketIdx].GsiBase;
    Aplic->BaseAddr   = Topo->Intc[SocketIdx].Base;
    Aplic->Size       = (UINT32)Topo->Intc[SocketIdx].Size;
    Ptr              += sizeof (EFI_ACPI_6_6_APLIC_STRUCTURE);
  }

  AcpiTableChecksum (Buffer, TableSize);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)Buffer,
                        TableSize,
                        &TableKey
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install MADT: %r\n", __func__, Status));
    gBS->FreePages (PageAddress, EFI_SIZE_TO_PAGES (TableSize));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "%a: MADT installed (%u harts, %u sockets)\n",
      __func__,
      (UINT32)Topo->NumHarts,
      Topo->NumSockets
      ));
  }

  return Status;
}
