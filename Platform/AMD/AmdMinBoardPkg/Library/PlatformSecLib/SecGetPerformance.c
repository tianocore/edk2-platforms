/** @file
  Sample to provide SecGetPerformance function.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/SecPerformance.h>
#include <Ppi/SecPlatformInformation.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>

UINT32 *
AsmSecPlatformGetTemporaryStackBase (
  VOID
  );

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
  UINT32  *TopOfStack;
  UINT32  Count;
  UINT32  TscHigh;
  UINT32  TscLow;
  UINT64  Ticker;

  DEBUG ((DEBUG_INFO, "SecGetPerformance Enter.\n"));

  //
  // |--------------| <- TopOfStack
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

  TopOfStack = AsmSecPlatformGetTemporaryStackBase ();

  Count   = *(TopOfStack - 1);
  TscHigh = *(TopOfStack - 2 - Count);
  TscLow  = *(TopOfStack - 3 - Count);

  DEBUG ((DEBUG_INFO, "SEC TSC Low = 0x%X\n", TscLow));
  DEBUG ((DEBUG_INFO, "SEC TSC High = 0x%X\n", TscHigh));

  Ticker                = LShiftU64 ((UINT64)TscHigh, 32) | (UINT64)TscLow;
  Performance->ResetEnd = GetTimeInNanoSecond (Ticker);

  DEBUG ((DEBUG_INFO, "SEC SecGetPerformance Exit.\n"));
  return EFI_SUCCESS;
}
