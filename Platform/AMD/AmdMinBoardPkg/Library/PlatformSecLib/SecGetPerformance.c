/** @file
  Sample to provide SecGetPerformance function.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/SecPerformance.h>
#include <Ppi/SecPlatformInformation.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>

/**
  This interface conveys performance information out of the Security (SEC) phase into PEI.

  This service is published by the SEC phase. The SEC phase handoff has an optional
  EFI_PEI_PPI_DESCRIPTOR list as its final argument when control is passed from SEC into the
  PEI Foundation. As such, if the platform supports collecting performance data in SEC,
  this information is encapsulated into the data structure abstracted by this service.
  This information is collected for the boot-strap processor (BSP) on IA-32.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of the PEI_SEC_PERFORMANCE_PPI.
  @param[out] Performance  The pointer to performance data collected in SEC phase.

  @retval EFI_SUCCESS  The data was successfully returned.

**/
EFI_STATUS
EFIAPI
SecGetPerformance (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN       PEI_SEC_PERFORMANCE_PPI   *This,
  OUT      FIRMWARE_SEC_PERFORMANCE  *Performance
  )
{
  UINT32      Size;
  UINT32      Count;
  UINTN       TopOfTemporaryRam;
  UINT64      Ticker;
  VOID        *TopOfTemporaryRamPpi;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "SecGetPerformance\n"));

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gAmdTopOfTemporaryRamPpiGuid,
                             0,
                             NULL,
                             (VOID **)&TopOfTemporaryRamPpi
                             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // |--------------| <- TopOfTemporaryRam
  // | SEC PEI HAND |
  // |--------------|
  // |Number of BSPs|
  // |--------------|
  // |     BIST     |
  // |--------------|
  // |     ....     |
  // |--------------|
  // |  TSC[63:32]  |
  // |--------------|
  // |  TSC[31:00]  |
  // |--------------|
  //

  //
  // AMD push the EFI_SEC_PEI_HAND_OFF first to the stack.
  //
  TopOfTemporaryRam     = (UINT32)(UINTN)TopOfTemporaryRamPpi - sizeof (EFI_SEC_PEI_HAND_OFF);
  TopOfTemporaryRam    -= sizeof (UINT32) * 2;
  Count                 = *(UINT32 *)(UINTN)(TopOfTemporaryRam);
  Size                  = Count * sizeof (IA32_HANDOFF_STATUS);
  Ticker                = *(UINT64 *)(TopOfTemporaryRam - sizeof (Count) - Size - sizeof (UINT32));
  Performance->ResetEnd = GetTimeInNanoSecond (Ticker);
  return EFI_SUCCESS;
}
