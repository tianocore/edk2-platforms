/** @file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
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

  DEBUG ((DEBUG_INFO, "Entered - %a\n", __func__));
  Status = (*PeiServices)->AllocatePool (
                             PeiServices,
                             sizeof (FABRIC_RESOURCE_FOR_EACH_RB),
                             (VOID **)&FabricResourceForEachRb
                             );

  if (!EFI_ERROR (Status)) {
    SocPresent = (UINT8)FabricTopologyGetNumberOfProcessorsPresent ();
    DEBUG ((DEBUG_INFO, "SoC count - %d\n", SocPresent));

    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][0].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][1].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][2].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][3].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][0].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][1].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][2].Size = 0x0;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][3].Size = 0x0;

    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][0].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][1].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][2].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[0][3].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][0].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][1].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][2].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeAbove4G[1][3].Alignment = 1;

    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][0].Size = 0x8000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][1].Size = 0x8000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][2].Size = 0x8000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][3].Size = 0x10000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][0].Size = 0x8000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][1].Size = 0x8000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][2].Size = 0x8000000000;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][3].Size = 0x8000000000;

    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][0].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][1].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][2].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[0][3].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][0].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][1].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][2].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeAbove4G[1][3].Alignment = 0xffffff;

    if ( SocPresent == 2 ) {
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][0].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][1].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][2].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][3].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][0].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][1].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][2].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][3].Size = 0x2000000;
    } else {
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][0].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][1].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][2].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][3].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][4].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][5].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][6].Size = 0x2000000;
      FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][7].Size = 0x2000000;
    }

    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][0].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][1].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][2].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[0][3].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][0].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][1].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][2].Alignment = 1;
    FabricResourceForEachRb->NonPrefetchableMmioSizeBelow4G[1][3].Alignment = 1;

    if ( SocPresent == 2 ) {
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][0].Size = 0x10000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][1].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][2].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][3].Size = 0x8000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][0].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][1].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][2].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][3].Size = 0x4000000;
    } else {
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][0].Size = 0x10000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][1].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][2].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][3].Size = 0x8000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][4].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][5].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][6].Size = 0x4000000;
      FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][7].Size = 0x4000000;
    }

    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][0].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][1].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][2].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[0][3].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][0].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][1].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][2].Alignment = 0xffffff;
    FabricResourceForEachRb->PrefetchableMmioSizeBelow4G[1][3].Alignment = 0xffffff;

    FabricResourceForEachRb->PrimaryRbSecondNonPrefetchableMmioSizeBelow4G.Size      = 0;
    FabricResourceForEachRb->PrimaryRbSecondNonPrefetchableMmioSizeBelow4G.Alignment = 1;
    FabricResourceForEachRb->PrimaryRbSecondPrefetchableMmioSizeBelow4G.Size         = 1;
    FabricResourceForEachRb->PrimaryRbSecondPrefetchableMmioSizeBelow4G.Alignment    = 1;

    FabricResourceForEachRb->IO[0][0].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[0][1].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[0][2].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[0][3].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[1][0].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[1][1].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[1][2].Alignment = 0xFFFFF;
    FabricResourceForEachRb->IO[1][3].Alignment = 0xFFFFF;

    if ( SocPresent == 2 ) {
      FabricResourceForEachRb->IO[0][0].Size = 0x1000;
      FabricResourceForEachRb->IO[0][1].Size = 0x1000;
      FabricResourceForEachRb->IO[0][2].Size = 0x2000;
      FabricResourceForEachRb->IO[0][3].Size = 0x1000;
      FabricResourceForEachRb->IO[1][0].Size = 0x1000;
      FabricResourceForEachRb->IO[1][1].Size = 0x1000;
      FabricResourceForEachRb->IO[1][2].Size = 0x2000;
      FabricResourceForEachRb->IO[1][3].Size = 0x1000;
    } else {
      FabricResourceForEachRb->IO[0][0].Size = 0x1000;
      FabricResourceForEachRb->IO[0][1].Size = 0x1000;
      FabricResourceForEachRb->IO[0][2].Size = 0x2000;
      FabricResourceForEachRb->IO[0][3].Size = 0x1000;
      FabricResourceForEachRb->IO[0][4].Size = 0x2000;
      FabricResourceForEachRb->IO[0][5].Size = 0x1000;
      FabricResourceForEachRb->IO[0][6].Size = 0x1000;
      FabricResourceForEachRb->IO[0][7].Size = 0x1000;
    }

    if ( SocPresent == 2 ) {
      FabricResourceForEachRb->PciBusNumber[0][0] = 0x20;
      FabricResourceForEachRb->PciBusNumber[0][1] = 0x20;
      FabricResourceForEachRb->PciBusNumber[0][2] = 0x20;
      FabricResourceForEachRb->PciBusNumber[0][3] = 0x20;
      FabricResourceForEachRb->PciBusNumber[1][0] = 0x20;
      FabricResourceForEachRb->PciBusNumber[1][1] = 0x20;
      FabricResourceForEachRb->PciBusNumber[1][2] = 0x20;
      FabricResourceForEachRb->PciBusNumber[1][3] = 0x20;
    } else {
      FabricResourceForEachRb->PciBusNumber[0][0] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][1] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][2] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][3] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][4] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][5] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][6] = 0x10;
      FabricResourceForEachRb->PciBusNumber[0][7] = 0x10;
    }

    PcdSet64S (PcdAmdFabricResourceDefaultSizePtr, (UINT64)(UINTN)FabricResourceForEachRb);
  }

  return Status;
}
