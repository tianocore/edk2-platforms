/** @file
  Endoresment Seed generation part of PlatformTpmLib to use TpmLib.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__GetEPS()

  This function used to generate Endorsement Seed when
  first initailization of TPM.

  @param [in]   Size              Size of endorsement seed
  @param [out]  EndorsementSeed   Endorsement Seed.

**/
VOID
EFIAPI
PlatformTpmLibGetEPS (
  IN  UINT16  Size,
  OUT UINT8   *EndorsementSeed
  )
{
  UINTN  Idx;

  PlatformTpmLibGetEntropy (EndorsementSeed, Size);

  for (Idx = 0; Idx < Size; Idx++) {
    DEBUG ((DEBUG_ERROR, "%2x ", EndorsementSeed[Idx]));
    if ((Idx != 0) && (Idx % 8 == 0)) {
      DEBUG ((DEBUG_ERROR, "\n"));
    }
  }

  DEBUG ((DEBUG_ERROR, "\n"));

  return;
}
