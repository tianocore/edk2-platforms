/** @file
  Endoresment Seed generation part of PlatformTpmLib to use TpmLib.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
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
  EFI_STATUS  Status;
  UINT64      DeviceUnique;
  VOID        *HashCtx;
  UINT8       HashVal[SHA512_DIGEST_SIZE];

  HashCtx = AllocatePool (Sha512GetContextSize ());
  if (HashCtx == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to Allocate HashCtx... \n", __func__));
    return;
  }

  PlatformTpmLibGetEntropy (EndorsementSeed, Size);
  PlatformTpmLibGetUnique (1, sizeof (DeviceUnique), (UINT8 *)&DeviceUnique);

  Status = Sha512Init (HashCtx);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to initialise HashCtx... \n", __func__));
    goto ErrorHandler;
  }

  Status = Sha512Update (HashCtx, EndorsementSeed, Size);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to update HashCtx... \n", __func__));
    goto ErrorHandler;
  }

  Status = Sha512Update (HashCtx, &DeviceUnique, sizeof (DeviceUnique));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to update HashCtx... \n", __func__));
    goto ErrorHandler;
  }

  Status = Sha512Final (HashCtx, HashVal);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to finalise HashCtx... \n", __func__));
    goto ErrorHandler;
  }

  ZeroMem (EndorsementSeed, Size);
  CopyMem (EndorsementSeed, HashVal, Size);

ErrorHandler:
  FreePool (HashCtx);

  return;
}
