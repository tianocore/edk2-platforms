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

  EPS can be generated, for example, as follows:

    EPS = SHA-512(TRNG_output || nonce || optional_mixing || DeviceUnique)

  Alternatively, EPS can be generated using DRBG_Generate(),
  as done in the TCG TPM 2.0 reference implementation.

  This function is not expected to fail; however,
  if it does have the potential to fail,
  the platform should handle the failure appropriately,
  for example by disabling TPM functionality or
  retrying the manufacturing process.

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
