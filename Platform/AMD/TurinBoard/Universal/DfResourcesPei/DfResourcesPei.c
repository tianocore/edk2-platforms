/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <FabricResourceManagerCmn.h>
#include <Library/BaseFabricTopologyLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Uefi/UefiBaseType.h>

/**
  Entry point for Data Fabric Resouces PEIM.

  @param   FileHandle Pointer to the FFS file header.
  @param   PeiServices Pointer to the PEI services table.

  @retval  EFI_STATUS EFI_SUCCESS
           EFI_STATUS respective failure status.
**/
EFI_STATUS
EFIAPI
PeiDfResourcesInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                   Status;
  FABRIC_RESOURCE_FOR_EACH_RB  *FabricResourceForEachRb;
  UINT8                        SocPresent;
  UINT8                        RbPerSocketPresent;
  UINT8                        i;
  UINT8                        j;

  DEBUG ((DEBUG_INFO, "Entered - %a\n", __func__));
  Status = (*PeiServices)->AllocatePool (
                             PeiServices,
                             sizeof (FABRIC_RESOURCE_FOR_EACH_RB),
                             (VOID **)&FabricResourceForEachRb
                             );

  if (!EFI_ERROR (Status)) {
    SocPresent         = (UINT8)FabricTopologyGetNumberOfProcessorsPresent ();
    RbPerSocketPresent = (UINT8)FabricTopologyGetNumberOfRootBridgesOnSocket (0);
    DEBUG ((DEBUG_INFO, "SoC count - %d\n", SocPresent));
    DEBUG ((DEBUG_INFO, "RB count - %d\n", RbPerSocketPresent * SocPresent));

    //
    // Mapping of resources for 1P system
    //
    // Logical Socket 0, Rb 0 is Physical Socket0, Rb 4 ---> PCI(0), FabricResourceForEachRb [0][4]
    // Logical Socket 0, Rb 1 is Physical Socket0, Rb 7 ---> PCI(1), FabricResourceForEachRb [0][7]
    // Logical Socket 0, Rb 2 is Physical Socket0, Rb 6 ---> PCI(2), FabricResourceForEachRb [0][6]
    // Logical Socket 0, Rb 3 is Physical Socket0, Rb 5 ---> PCI(3), FabricResourceForEachRb [0][5]
    // Logical Socket 0, Rb 4 is Physical Socket0, Rb 3 ---> PCI(4), FabricResourceForEachRb [0][3]
    // Logical Socket 0, Rb 5 is Physical Socket0, Rb 2 ---> PCI(5), FabricResourceForEachRb [0][2]
    // Logical Socket 0, Rb 6 is Physical Socket0, Rb 1 ---> PCI(6), FabricResourceForEachRb [0][1]
    // Logical Socket 0, Rb 7 is Physical Socket0, Rb 0 ---> PCI(7), FabricResourceForEachRb [0][0]
    //

    //
    // Mapping of resources for 2P system, with 8 root bridges
    //
    // Logical Socket 0, Rb 0 is Physical Socket0, Rb 2 ---> PCI(0), FabricResourceForEachRb [0][2]
    // Logical Socket 0, Rb 1 is Physical Socket0, Rb 3 ---> PCI(1), FabricResourceForEachRb [0][3]
    // Logical Socket 0, Rb 2 is Physical Socket0, Rb 1 ---> PCI(2), FabricResourceForEachRb [0][1]
    // Logical Socket 0, Rb 3 is Physical Socket0, Rb 0 ---> PCI(3), FabricResourceForEachRb [0][0]
    // Logical Socket 1, Rb 0 is Physical Socket1, Rb 2 ---> PCI(4), FabricResourceForEachRb [1][2]
    // Logical Socket 1, Rb 1 is Physical Socket1, Rb 3 ---> PCI(5), FabricResourceForEachRb [1][3]
    // Logical Socket 1, Rb 2 is Physical Socket1, Rb 1 ---> PCI(6), FabricResourceForEachRb [1][1]
    // Logical Socket 1, Rb 3 is Physical Socket1, Rb 0 ---> PCI(7), FabricResourceForEachRb [1][0]
    //

    for (i = 0; i < 2; i++) {
      for (j = 0; j < 8; j++) {
        FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[i][j].Size      = 0x0;
        FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[i][j].Alignment = 1;

        FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[i][j].Size      = SIZE_64GB;
        FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[i][j].Alignment = 1;

        FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[i][j].Alignment = 1;

        FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[i][j].Size      = SIZE_16MB;
        FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[i][j].Alignment = 0xffffff;
      }
    }

    // Adjust IO SPACE, MAX available size is 64KB
    // FabricResourceForEachRb->IO[0][0].Size = SIZE_8KB + SIZE_4KB;

    if (SocPresent == 1) {
      FabricResourceForEachRb->IO[0][4].Size                             = SIZE_8KB;
      FabricResourceForEachRb->IO[0][2].Size                             = SIZE_4KB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][4].Size = SIZE_2MB;
	  FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][5].Size = SIZE_1MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][6].Size = SIZE_1MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][0].Size = SIZE_8MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][2].Size = SIZE_16MB + SIZE_8MB;
    } else {
      FabricResourceForEachRb->IO[0][4].Size                             = SIZE_8KB;
      FabricResourceForEachRb->IO[0][2].Size                             = SIZE_8KB;
      FabricResourceForEachRb->IO[0][1].Size                             = SIZE_4KB;
      FabricResourceForEachRb->IO[0][0].Size                             = SIZE_4KB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][2].Size = SIZE_2MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][3].Size = SIZE_2MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][1].Size = SIZE_32MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][0].Size = SIZE_32MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][2].Size = SIZE_1MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][3].Size = SIZE_1MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][1].Size = SIZE_2MB + SIZE_1MB;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][0].Size = SIZE_1MB;
    }

    // Above 4G, PMem
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][0].Size = SIZE_512GB;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][1].Size = SIZE_512GB;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][2].Size = SIZE_512GB;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][3].Size = SIZE_1TB;

    // Below 4G, PMem
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][0].Size = SIZE_256MB + SIZE_32MB;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][1].Size = SIZE_128MB;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][2].Size = SIZE_128MB;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][3].Size = SIZE_128MB;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][4].Size = SIZE_64MB;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][0].Size = SIZE_64MB;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][4].Size = SIZE_64MB;

    // Primary RootBridge 2nd MMIO
    // if NonP is 0 and PMem is non-zero, all available size would be assigned to PMem
    FabricResourceForEachRb->PrimaryRbSecondNonPrefetchableMmioSizeBelow4G.Size      = 0;
    FabricResourceForEachRb->PrimaryRbSecondNonPrefetchableMmioSizeBelow4G.Alignment = 1;
    FabricResourceForEachRb->PrimaryRbSecondPrefetchableMmioSizeBelow4G.Size         = 1;
    FabricResourceForEachRb->PrimaryRbSecondPrefetchableMmioSizeBelow4G.Alignment    = 1;

    // Program bus numbers based on topology info
    for (i = 0; i < SocPresent; i++) {
      for (j = 0; j < RbPerSocketPresent; j++) {
        FabricResourceForEachRb->PciBusNumber[i][j] = (UINT16) (FabricTopologyGetHostBridgeBusLimit(i, 0, j) - FabricTopologyGetHostBridgeBusBase (i, 0, j) + 1);
      }
    }

    PcdSet64S (PcdAmdFabricResourceDefaultSizePtr, (UINT64)(UINTN)FabricResourceForEachRb);
  }

  return Status;
}
