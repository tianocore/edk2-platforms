/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2022, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/ArmMpCoreInfo.h>
#include <Library/ArmLib.h>
#include "AcpiPlatform.h"

EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER  SRATTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE,
    0, /* need fill in */
    EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_REVISION
    ),
  0x00000001,
  0x0000000000000000,
};

EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE  GicItsAffinityTemplate = {
  .Type            = EFI_ACPI_6_3_GIC_ITS_AFFINITY,
  sizeof (EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE),
  .ProximityDomain = 0, /* ProximityDomain */
  { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE },
  .ItsId           = 0,
};

STATIC
UINTN
SratCalculateNumMemoryRegion (
  VOID
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;
  UINTN              Count;
  UINT64             TmpVal;
  VOID               *Hob;
  UINTN              Result;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return 0;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  Result = 0;
  for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
    TmpVal = PlatformHob->DramInfo.Size[Count];
    if (TmpVal > 0) {
      Result++;
    }
  }

  return Result;
}

STATIC
EFI_STATUS
SratAddMemAffinity (
  EFI_ACPI_6_3_MEMORY_AFFINITY_STRUCTURE  *SratMemAffinity
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;
  UINTN              Count, NumRegion;
  UINT64             RegionSize, RegionBase;
  VOID               *Hob;
  UINTN              ProximityDomain;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  NumRegion = 0;

  for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
    RegionSize      = PlatformHob->DramInfo.Size[Count];
    RegionBase      = PlatformHob->DramInfo.Base[Count];
    ProximityDomain = PlatformHob->DramInfo.Node[Count];
    if (RegionSize > 0) {
      ZeroMem ((VOID *)&SratMemAffinity[NumRegion], sizeof (SratMemAffinity[NumRegion]));
      SratMemAffinity[NumRegion].Flags = EFI_ACPI_6_3_MEMORY_ENABLED;
      if (PlatformHob->DramInfo.NvdRegion[Count] != 0) {
        /* Mark NVDIMM-N region as HOT_PLUGGABLE and NON-VOLATILE */
        SratMemAffinity[NumRegion].Flags |= EFI_ACPI_6_3_MEMORY_HOT_PLUGGABLE |
                                            EFI_ACPI_6_3_MEMORY_NONVOLATILE;
      }

      SratMemAffinity[NumRegion].LengthLow =
        (UINT32)(RegionSize & 0xFFFFFFFF);
      SratMemAffinity[NumRegion].LengthHigh =
        (UINT32)((RegionSize & 0xFFFFFFFF00000000ULL) >> 32);
      SratMemAffinity[NumRegion].AddressBaseLow =
        (UINT32)(RegionBase & 0xFFFFFFFF);
      SratMemAffinity[NumRegion].AddressBaseHigh =
        (UINT32)((RegionBase & 0xFFFFFFFF00000000ULL) >> 32);
      SratMemAffinity[NumRegion].ProximityDomain = (UINT32)(ProximityDomain);
      SratMemAffinity[NumRegion].Type            = EFI_ACPI_6_3_MEMORY_AFFINITY;
      SratMemAffinity[NumRegion].Length          = sizeof (EFI_ACPI_6_3_MEMORY_AFFINITY_STRUCTURE);
      NumRegion++;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SratAddGiccAffinity (
  EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE  *SratGiccAffinity
  )
{
  VOID           *Hob;
  UINTN          NumberOfEntries;
  ARM_CORE_INFO  *ArmCoreInfoTable;
  UINTN          Count, NumNode, Idx;
  UINT32         AcpiProcessorUid;
  UINT8          Socket;
  UINT8          Cpm;

  Hob = GetFirstGuidHob (&gArmMpCoreInfoGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  ArmCoreInfoTable = (ARM_CORE_INFO *)GET_GUID_HOB_DATA (Hob);
  NumberOfEntries  = GET_GUID_HOB_DATA_SIZE (Hob) / sizeof (ARM_CORE_INFO);

  if (NumberOfEntries == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Count   = 0;
  NumNode = 0;
  while (Count != NumberOfEntries) {
    for (Idx = 0; Idx < NumberOfEntries; Idx++ ) {
      Socket = AC01_GET_SOCKET_ID (ArmCoreInfoTable[Idx].Mpidr);
      Cpm    = AC01_GET_CLUSTER_ID (ArmCoreInfoTable[Idx].Mpidr);
      if (CpuGetSubNumNode (Socket, Cpm) != NumNode) {
        /* We add nodes based on ProximityDomain order */
        continue;
      }

      AcpiProcessorUid =
        (AC01_GET_SOCKET_ID (ArmCoreInfoTable[Idx].Mpidr) << PLATFORM_SOCKET_UID_BIT_OFFSET)
        + (AC01_GET_CLUSTER_ID (ArmCoreInfoTable[Idx].Mpidr) << PLATFORM_CPM_UID_BIT_OFFSET)
        +  AC01_GET_CORE_ID (ArmCoreInfoTable[Idx].Mpidr);
      ZeroMem ((VOID *)&SratGiccAffinity[Count], sizeof (SratGiccAffinity[Count]));
      SratGiccAffinity[Count].AcpiProcessorUid = AcpiProcessorUid;
      SratGiccAffinity[Count].Flags            = 1;
      SratGiccAffinity[Count].Length           = sizeof (EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE);
      SratGiccAffinity[Count].Type             = EFI_ACPI_6_3_GICC_AFFINITY;
      SratGiccAffinity[Count].ProximityDomain  = CpuGetSubNumNode (Socket, Cpm);
      Count++;
    }

    NumNode++;
  }

  return EFI_SUCCESS;
}

STATIC UINT32
InstallGicItsAffinity (
  VOID    *EntryPointer,
  UINT32  Index
  )
{
  EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE  *ItsAffinityEntryPointer = EntryPointer;
  UINTN                                    Size;

  Size = sizeof (GicItsAffinityTemplate);
  CopyMem (ItsAffinityEntryPointer, &GicItsAffinityTemplate, Size);
  return Size;
}

STATIC
EFI_STATUS
SratAddGicItsAffinity (
  VOID  *TmpPtr
  )
{
  UINTN  Size = 0;
  UINTN  Index;

  /* Install Gic ITSAffinity */
  if (!IsSlaveSocketAvailable ()) {
    for (Index = 0; Index <= 1; Index++) {
      /* RCA0/1 */
      GicItsAffinityTemplate.ItsId           = Index;
      GicItsAffinityTemplate.ProximityDomain = 0;
      Size                                  += InstallGicItsAffinity ((VOID *)((UINT64)TmpPtr + Size), Index);
    }
  }

  for (Index = SOCKET0_FIRST_RC; Index <= SOCKET0_LAST_RC; Index++) {
    GicItsAffinityTemplate.ItsId           = Index;
    GicItsAffinityTemplate.ProximityDomain = 0;
    Size                                  += InstallGicItsAffinity ((VOID *)((UINT64)TmpPtr + Size), Index);
  }

  if (IsSlaveSocketActive ()) {
    for (Index = SOCKET1_FIRST_RC; Index <= SOCKET1_LAST_RC; Index++) {
      GicItsAffinityTemplate.ItsId           = Index;
      GicItsAffinityTemplate.ProximityDomain = 1;
      Size                                  += InstallGicItsAffinity ((VOID *)((UINT64)TmpPtr + Size), Index);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AcpiInstallSratTable (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL                             *AcpiTableProtocol;
  EFI_STATUS                                          Status;
  EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER  *SratTablePointer;
  UINT8                                               *TmpPtr;
  UINTN                                               SratTableKey;
  UINTN                                               Size;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Size = sizeof (SRATTableHeaderTemplate) +
         SratCalculateNumMemoryRegion () * sizeof (EFI_ACPI_6_3_MEMORY_AFFINITY_STRUCTURE) +
         GetNumberOfActiveCores () * sizeof (EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE) +
         ((SOCKET0_LAST_RC - SOCKET0_FIRST_RC +  1) * sizeof (GicItsAffinityTemplate));
  if (IsSlaveSocketActive ()) {
    Size += (SOCKET1_LAST_RC - SOCKET1_FIRST_RC +  1) * sizeof (GicItsAffinityTemplate);
  } else if (!IsSlaveSocketAvailable ()) {
    Size += 2 * sizeof (GicItsAffinityTemplate); /* RCA0/1 */
  }

  SratTablePointer = (EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *)AllocateZeroPool (Size);
  if (SratTablePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem ((VOID *)SratTablePointer, (VOID *)&SRATTableHeaderTemplate, sizeof (SRATTableHeaderTemplate));

  TmpPtr = (UINT8 *)SratTablePointer + sizeof (SRATTableHeaderTemplate);
  Status = SratAddMemAffinity ((EFI_ACPI_6_3_MEMORY_AFFINITY_STRUCTURE *)TmpPtr);
  ASSERT_EFI_ERROR (Status);

  TmpPtr += SratCalculateNumMemoryRegion () * sizeof (EFI_ACPI_6_3_MEMORY_AFFINITY_STRUCTURE);
  Status  = SratAddGiccAffinity ((EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE *)TmpPtr);
  ASSERT_EFI_ERROR (Status);

  TmpPtr += GetNumberOfActiveCores () * sizeof (EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE);
  SratAddGicItsAffinity ((VOID *)(UINT64)TmpPtr);
  SratTablePointer->Header.Length = Size;

  AcpiUpdateChecksum ((UINT8 *)SratTablePointer, SratTablePointer->Header.Length);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)SratTablePointer,
                                SratTablePointer->Header.Length,
                                &SratTableKey
                                );
  FreePool ((VOID *)SratTablePointer);

  return Status;
}
