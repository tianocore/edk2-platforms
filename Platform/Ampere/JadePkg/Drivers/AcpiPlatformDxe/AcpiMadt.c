/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

EFI_ACPI_6_3_GIC_ITS_STRUCTURE  GicItsTemplate = {
  EFI_ACPI_6_3_GIC_ITS,
  sizeof (EFI_ACPI_6_3_GIC_ITS_STRUCTURE),
  EFI_ACPI_RESERVED_WORD,
  0, /* GicItsId */
  0, /* PhysicalBaseAddress */
  0, /* Reserved2 */
};

EFI_ACPI_6_3_GICR_STRUCTURE  GicRTemplate = {
  EFI_ACPI_6_3_GICR,
  sizeof (EFI_ACPI_6_3_GICR_STRUCTURE),
  EFI_ACPI_RESERVED_WORD,
  AC01_GICR_MASTER_BASE_ADDRESS, /* DiscoveryRangeBaseAddress */
  0x1000000,                     /* DiscoveryRangeLength */
};

EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE  GicDTemplate = {
  EFI_ACPI_6_3_GICD,
  sizeof (EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE),
  EFI_ACPI_RESERVED_WORD,
  0,                                              /* GicDistHwId */
  AC01_GICD_MASTER_BASE_ADDRESS,                  /* GicDistBase */
  0,                                              /* GicDistVector */
  0x3,                                            /* GicVersion */
  { EFI_ACPI_RESERVED_BYTE,                       EFI_ACPI_RESERVED_BYTE,EFI_ACPI_RESERVED_BYTE }
};

EFI_ACPI_6_3_GIC_STRUCTURE  GiccTemplate = {
  EFI_ACPI_6_3_GIC,
  sizeof (EFI_ACPI_6_3_GIC_STRUCTURE),
  EFI_ACPI_RESERVED_WORD,
  0, /* GicId */
  0, /* AcpiCpuUid */
  0, /* Flags */
  0,
  23, /* PmuIrq */
  0,
  0,
  0,
  0,
  25, /* GsivId */
  0,  /* GicRBase */
  0,  /* Mpidr */
  0,  /* ProcessorPowerEfficiencyClass */
  0,  /* Reserved2 */
  21, /* SPE irq */
};

EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  MADTTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
    0, /* need fill in */
    EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION
    ),
};

EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *MadtTablePointer;

UINT32
AcpiInstallMadtProcessorNode (
  VOID    *EntryPointer,
  UINT32  CpuId
  )
{
  EFI_ACPI_6_3_GIC_STRUCTURE  *MadtProcessorEntryPointer = EntryPointer;
  UINT32                      SocketId;
  UINT32                      ClusterId;
  UINTN                       Size;

  Size = sizeof (GiccTemplate);
  CopyMem (MadtProcessorEntryPointer, &GiccTemplate, Size);

  SocketId  = SOCKET_ID (CpuId);
  ClusterId = CLUSTER_ID (CpuId);

  //
  // GICv2 compatibility mode is not supported.
  // Hence, set GIC's CPU Interface Number to 0.
  //
  MadtProcessorEntryPointer->CPUInterfaceNumber = 0;
  MadtProcessorEntryPointer->AcpiProcessorUid   =
    (SocketId << PLATFORM_SOCKET_UID_BIT_OFFSET) +
    (ClusterId << 8) + (CpuId  % PLATFORM_CPU_NUM_CORES_PER_CPM);
  MadtProcessorEntryPointer->Flags = 1;
  MadtProcessorEntryPointer->MPIDR =
    AC01_GET_MPIDR ((UINT64)SocketId, ClusterId, CpuId  % PLATFORM_CPU_NUM_CORES_PER_CPM);

  return Size;
}

UINT32
AcpiInstallMadtGicD (
  VOID  *EntryPointer
  )
{
  EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE  *GicDEntryPointer = EntryPointer;
  UINTN                                   Size;

  Size = sizeof (GicDTemplate);
  CopyMem (GicDEntryPointer, &GicDTemplate, Size);

  return Size;
}

UINT32
AcpiInstallMadtGicR (
  VOID    *EntryPointer,
  UINT32  SocketId
  )
{
  EFI_ACPI_6_3_GICR_STRUCTURE  *GicREntryPointer = EntryPointer;
  UINTN                        Size;

  /*
   * If the Slave socket is not present, discard the Slave socket
   * GIC redistributor region
   */
  if ((SocketId == 1) && !IsSlaveSocketActive ()) {
    return 0;
  }

  Size = sizeof (GicRTemplate);
  CopyMem (GicREntryPointer, &GicRTemplate, Size);

  if (SocketId == 1) {
    GicREntryPointer->DiscoveryRangeBaseAddress = AC01_GICR_SLAVE_BASE_ADDRESS;
  }

  return Size;
}

UINT32
AcpiInstallMadtGicIts (
  VOID    *EntryPointer,
  UINT32  Index
  )
{
  EFI_ACPI_6_3_GIC_ITS_STRUCTURE  *GicItsEntryPointer = EntryPointer;
  UINTN                           Size, Offset;
  UINT64                          GicBase = AC01_GICD_MASTER_BASE_ADDRESS;
  UINT32                          ItsId   = Index;

  if (Index > SOCKET0_LAST_RC) {
    /* Socket 1, Index: 8-15 */
    GicBase = AC01_GICD_SLAVE_BASE_ADDRESS;
    Index  -= (SOCKET0_LAST_RC + 1); /* Socket 1, Index:8 -> RCA0 */
  }

  Size = sizeof (GicItsTemplate);
  CopyMem (GicItsEntryPointer, &GicItsTemplate, Size);
  Offset                                  = 0x40000 + Index * 0x20000;
  GicItsEntryPointer->GicItsId            = ItsId;
  GicItsEntryPointer->PhysicalBaseAddress = Offset + GicBase;

  return Size;
}

/*
 *  Install MADT table.
 */
EFI_STATUS
AcpiInstallMadtTable (
  VOID
  )
{
  EFI_ACPI_6_3_GIC_STRUCTURE  *GiccEntryPointer = NULL;
  EFI_ACPI_TABLE_PROTOCOL     *AcpiTableProtocol;
  UINTN                       MadtTableKey = 0;
  INTN                        Index;
  EFI_STATUS                  Status;
  UINTN                       Size;
  UINTN                       Length;
  UINT32                      *CoreOrder;
  UINT32                      SktMaxCoreNum;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Size = sizeof (MADTTableHeaderTemplate) +
         sizeof (GiccTemplate) * GetNumberOfActiveCores () +
         sizeof (GicDTemplate) +
         sizeof (GicRTemplate) * GetNumberOfActiveSockets () +
         sizeof (GicItsTemplate) * (SOCKET0_LAST_RC - SOCKET0_FIRST_RC +  1);
  if (IsSlaveSocketActive ()) {
    Size += (sizeof (GicItsTemplate) * (SOCKET1_LAST_RC - SOCKET1_FIRST_RC +  1));
  } else if (!IsSlaveSocketAvailable ()) {
    Size += sizeof (GicItsTemplate) * 2; /* RCA0/1 */
  }

  MadtTablePointer =
    (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)AllocateZeroPool (Size);
  if (MadtTablePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  GiccEntryPointer =
    (EFI_ACPI_6_3_GIC_STRUCTURE *)((UINT64)MadtTablePointer +
                                   sizeof (MADTTableHeaderTemplate));

  /* Install Gic interface for each processor */
  Length    = 0;
  CoreOrder = CpuGetCoreOrder ();
  ASSERT (CoreOrder != NULL);
  SktMaxCoreNum = PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM;
  for (Index = 0; Index < SktMaxCoreNum; Index++) {
    if (IsCpuEnabled (CoreOrder[Index])) {
      Length += AcpiInstallMadtProcessorNode ((VOID *)((UINT64)GiccEntryPointer + Length), CoreOrder[Index]);
    }
  }

  for (Index = 0; Index < SktMaxCoreNum; Index++) {
    if (IsCpuEnabled (CoreOrder[Index] + SktMaxCoreNum)) {
      Length += AcpiInstallMadtProcessorNode ((VOID *)((UINT64)GiccEntryPointer + Length), CoreOrder[Index] + SktMaxCoreNum);
    }
  }

  /* Install Gic Distributor */
  Length += AcpiInstallMadtGicD ((VOID *)((UINT64)GiccEntryPointer + Length));

  /* Install Gic Redistributor */
  for (Index = 0; Index < PLATFORM_CPU_MAX_SOCKET; Index++) {
    Length += AcpiInstallMadtGicR ((VOID *)((UINT64)GiccEntryPointer + Length), Index);
  }

  /* Install Gic ITS */
  if (!IsSlaveSocketAvailable ()) {
    for (Index = 0; Index <= 1; Index++) {
      /* RCA0/1 */
      Length += AcpiInstallMadtGicIts ((VOID *)((UINT64)GiccEntryPointer + Length), Index);
    }
  }

  for (Index = SOCKET0_FIRST_RC; Index <= SOCKET0_LAST_RC; Index++) {
    Length += AcpiInstallMadtGicIts ((VOID *)((UINT64)GiccEntryPointer + Length), Index);
  }

  if (IsSlaveSocketActive ()) {
    for (Index = SOCKET1_FIRST_RC; Index <= SOCKET1_LAST_RC; Index++) {
      Length += AcpiInstallMadtGicIts ((VOID *)((UINT64)GiccEntryPointer + Length), Index);
    }
  }

  CopyMem (
    MadtTablePointer,
    &MADTTableHeaderTemplate,
    sizeof (MADTTableHeaderTemplate)
    );

  Length                         += sizeof (MADTTableHeaderTemplate);
  MadtTablePointer->Header.Length = Length;

  ASSERT (Size == Length);

  CopyMem (
    MadtTablePointer->Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (MadtTablePointer->Header.OemId)
    );

  AcpiUpdateChecksum ((UINT8 *)MadtTablePointer, MadtTablePointer->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)MadtTablePointer,
                                MadtTablePointer->Header.Length,
                                &MadtTableKey
                                );
  FreePool ((VOID *)MadtTablePointer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
