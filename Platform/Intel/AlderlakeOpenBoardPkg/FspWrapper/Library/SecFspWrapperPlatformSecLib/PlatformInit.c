/** @file
  Provide platform init function.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/SecBoardInitLib.h>
#include <Library/TestPointCheckLib.h>
#include <Register/TcoRegs.h>
#include <Library/IoLib.h>

/**
  Platform initialization.

  @param[in] FspHobList   HobList produced by FSP.
  @param[in] StartOfRange Start of temporary RAM.
  @param[in] EndOfRange   End of temporary RAM.
**/
VOID
EFIAPI
PlatformInit (
  IN VOID                 *FspHobList,
  IN VOID                 *StartOfRange,
  IN VOID                 *EndOfRange
  )
{

  ///
  /// Halt the TCO timer as early as possible
  ///
  IoWrite16 (PcdGet16 (PcdTcoBaseAddress) + R_TCO_IO_TCO1_CNT, B_TCO_IO_TCO1_CNT_TMR_HLT);

  //
  // Platform initialization
  // Enable Serial port here
  //
  if (PcdGetBool(PcdSecSerialPortDebugEnable)) {
    SerialPortInitialize ();
  }

  DEBUG ((DEBUG_INFO, "PrintPeiCoreEntryPointParam in PlatformInit\n"));
  DEBUG ((DEBUG_INFO, "FspHobList - 0x%x\n", FspHobList));
  DEBUG ((DEBUG_INFO, "StartOfRange - 0x%x\n", StartOfRange));
  DEBUG ((DEBUG_INFO, "EndOfRange - 0x%x\n", EndOfRange));

  BoardAfterTempRamInit ();

  TestPointTempMemoryFunction (StartOfRange, EndOfRange);
}
