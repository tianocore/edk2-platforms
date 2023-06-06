/** @file
 Source code for the board SA configuration Pcd init functions in Pre-Memory init phase.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BoardSaConfigPreMem.h"
#include <Library/CpuPlatformLib.h>
#include <Pins/GpioPinsVer2Lp.h>
#include <PlatformBoardId.h>
#include <PlatformBoardConfig.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/PeiServicesLib.h>
/**
  MRC configuration init function for PEI pre-memory phase.

  @param[in]  VOID

  @retval VOID
**/
VOID
AdlPSaMiscConfigInit (
  VOID
  )
{
  // Type4 RVP
  PcdSet8S (PcdSaMiscUserBd, 6);        // btUlxUltType4
  PcdSet16S (PcdSaDdrFreqLimit, 0);

  return;
}

/**
  Board Memory Init related configuration init function for PEI pre-memory phase.

  @param[in]  VOID

  @retval VOID
**/
VOID
AdlPMrcConfigInit (
  VOID
  )
{
  UINT16    BoardId;
  BOOLEAN   ExternalSpdPresent;
  MRC_DQS   *MrcDqs;
  MRC_DQ    *MrcDq;
  SPD_DATA  *SpdData;

  BoardId = PcdGet16(PcdBoardId);

  // SPD is the same size for all boards
  PcdSet16S (PcdMrcSpdDataSize, 512);

  ExternalSpdPresent = PcdGetBool (PcdSpdPresent);

  // Assume internal SPD is used
  PcdSet8S (PcdMrcSpdAddressTable0,  0);
  PcdSet8S (PcdMrcSpdAddressTable1,  0);
  PcdSet8S (PcdMrcSpdAddressTable2,  0);
  PcdSet8S (PcdMrcSpdAddressTable3,  0);
  PcdSet8S (PcdMrcSpdAddressTable4,  0);
  PcdSet8S (PcdMrcSpdAddressTable5,  0);
  PcdSet8S (PcdMrcSpdAddressTable6,  0);
  PcdSet8S (PcdMrcSpdAddressTable7,  0);
  PcdSet8S (PcdMrcSpdAddressTable8,  0);
  PcdSet8S (PcdMrcSpdAddressTable9,  0);
  PcdSet8S (PcdMrcSpdAddressTable10, 0);
  PcdSet8S (PcdMrcSpdAddressTable11, 0);
  PcdSet8S (PcdMrcSpdAddressTable12, 0);
  PcdSet8S (PcdMrcSpdAddressTable13, 0);
  PcdSet8S (PcdMrcSpdAddressTable14, 0);
  PcdSet8S (PcdMrcSpdAddressTable15, 0);

  // Check for external SPD presence
  if (ExternalSpdPresent) {
    switch (BoardId) {
      case BoardIdAdlPDdr5Rvp:
        PcdSet8S (PcdMrcSpdAddressTable0,  0xA0);
        PcdSet8S (PcdMrcSpdAddressTable1,  0xA2);
        PcdSet8S (PcdMrcSpdAddressTable8,  0xA4);
        PcdSet8S (PcdMrcSpdAddressTable9,  0xA6);
        break;
      default:
        break;
    }
  }

  // Setting the default DQ Byte Map. It may be overriden to board specific settings below.
  PcdSet32S (PcdMrcDqByteMap, (UINTN) DqByteMapAdlP);
  PcdSet16S (PcdMrcDqByteMapSize, sizeof (DqByteMapAdlP));

  // ADL uses the same RCOMP resistors for all DDR types
  PcdSet32S (PcdMrcRcompResistor, (UINTN) AdlPRcompResistorZero);

  // Use default RCOMP target values for all boards
  PcdSet32S (PcdMrcRcompTarget, (UINTN) RcompTargetAdlP);

  // Default is NIL
  PcdSetBoolS (PcdMrcDqPinsInterleavedControl, TRUE);
  PcdSetBoolS (PcdMrcDqPinsInterleaved, FALSE);

  // DqsMapCpu2Dram is the same size for all boards
  PcdSet16S (PcdMrcDqsMapCpu2DramSize, sizeof (MRC_DQS));
  // DqMapCpu2Dram is the same size for all boards
  PcdSet16S (PcdMrcDqMapCpu2DramSize, sizeof (MRC_DQ));
      PcdSet8S (PcdMrcLp5CccConfig, 0x0);

  // CPU-DRAM DQ mapping
  MrcDq = PcdGetPtr (VpdPcdMrcDqMapCpu2Dram);
  if (MrcDq != NULL) {
    PcdSet32S (PcdMrcDqMapCpu2Dram, (UINTN)MrcDq->DqMapCpu2Dram);
  }

  // CPU-DRAM DQS mapping
  MrcDqs = PcdGetPtr (VpdPcdMrcDqsMapCpu2Dram);
  if (MrcDqs != NULL) {
    PcdSet32S (PcdMrcDqsMapCpu2Dram, (UINTN)MrcDqs->DqsMapCpu2Dram);
  }

  // DRAM SPD Data
  SpdData = PcdGetPtr (VpdPcdMrcSpdData);
  if (SpdData != NULL) {
    if (SpdData->OverrideSpd == TRUE) {
      PcdSet32S (PcdMrcSpdData, (UINTN)SpdData->SpdData);
    }
  }

  //
  // CA Vref routing: board-dependent
  // 0 - VREF_CA goes to both CH_A and CH_B (LPDDR3/DDR3L)
  // 1 - VREF_CA to CH_A, VREF_DQ_A to CH_B (should not be used)
  // 2 - VREF_CA to CH_A, VREF_DQ_B to CH_B (DDR4)
  //
  // Set it to 2 for all our DDR4 boards; it is ignored for LPDDR4
  //
  PcdSet8S (PcdMrcCaVrefConfig, 2);

  return;
}

/**
  Board SA related GPIO configuration init function for PEI pre-memory phase.

  @param[in]  VOID

  @retval VOID
**/
VOID
AdlPSaGpioConfigInit (
  VOID
  )
{
  UINT16    BoardId;
  BoardId = PcdGet16(PcdBoardId);
  //
  // Assigning default values to PCIE RTD3 GPIOs
  //
  switch (BoardId) {
    case BoardIdAdlPDdr5Rvp:
      PcdSet8S (PcdRootPortIndex, 4);
      PcdSet8S (PcdPcieSlot1GpioSupport, 1);
      PcdSet8S (PcdPcieSlot1PwrEnableExpanderNo, 0);
      //
      // Configure CPU M.2 SSD 1 GPIO PCDs
      //
      PcdSetBoolS (PcdPcieSsd2PwrEnableGpioPolarity, PIN_GPIO_ACTIVE_HIGH);
      //
      // Configure CPU M.2 SSD 2 GPIO PCDs
      //
      PcdSet32S (PcdPcieSsd3PwrEnableGpioNo, GPIO_VER2_LP_GPP_C2);
      PcdSetBoolS (PcdPcieSsd3PwrEnableGpioPolarity, PIN_GPIO_ACTIVE_HIGH);
      break;
    default:
      break;
  }
  return;
}

/**
  SA Display DDI configuration init function for PEI pre-memory phase.

  @param[in]  VOID

  @retval     VOID
**/
VOID
AdlPSaDisplayConfigInit (
  VOID
  )
{
  UINT16    BoardId;
  UINT16    DisplayId;

  BoardId   = PcdGet16 (PcdBoardId);
  DisplayId = PcdGet16 (PcdDisplayId);

  DEBUG ((DEBUG_INFO, "BoardId = 0x%x DisplayId = 0x%x\n",BoardId,DisplayId));

  switch (BoardId) {
    case BoardIdAdlPDdr5Rvp:
        DEBUG ((DEBUG_INFO, "DDI Configuration ADLP Edp DP\n"));
        PcdSet32S (PcdSaDisplayConfigTable, (UINTN) mAdlPDdr5RvpDisplayDdiConfig);
        PcdSet16S (PcdSaDisplayConfigTableSize, sizeof (mAdlPDdr5RvpDisplayDdiConfig));
      break;
    default:
      break;
  }

  return;
}

/**
  Board USB related configuration init function for PEI pre-memory phase.

  @param[in]  VOID

  @retval VOID
**/
VOID
AdlPSaUsbConfigInit (
  VOID
  )
{
  UINT16    BoardId;
  BoardId = PcdGet16(PcdBoardId);

  switch (BoardId) {
    //
    // Override ALL port setting if required.
    //
    case BoardIdAdlPDdr5Rvp:
      PcdSet8S (PcdCpuXhciPortSupportMap, 0x0F);
      break;
  }
  //
  // Update Cpu Xhci Port Enable Map PCD
  //
  TcssUpdateCpuXhciPortEnableMapPcd ();
}
