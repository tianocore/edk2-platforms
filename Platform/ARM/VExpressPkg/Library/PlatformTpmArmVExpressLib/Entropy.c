/** @file
  Get entropy part of PlatformTpmLib to use TpmLib.

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
#include <Library/RngLib.h>

/**
  _plat__GetEntropy()

  This function is used to get available hardware entropy. In a hardware
  implementation of this function, there would be no call to the system
  to get entropy.

  @param [out] Entropy
  @param [in]  Amount    amount reuqested.

  @return < 0   Failed to generate entropy
  @return >= 0  The returned amount of entropy (bytes)

**/
INT32
EFIAPI
PlatformTpmLibGetEntropy (
  OUT UINT8   *Entropy,
  IN  UINT32  Amount
  )
{
  UINT16  Rand16;
  UINT32  Rand32;
  UINT64  Rand64;
  UINT64  Rand128[2];
  UINT32  Size;

  // Pseudo Entropy...
  Size = Amount;

  while (Size != 0) {
    if (Size >= sizeof (Rand128)) {
      GetRandomNumber128 (Rand128);
      CopyMem (Entropy, Rand128, sizeof (Rand128));
      Entropy += sizeof (Rand128);
      Size    -= sizeof (Rand128);
    } else if (Size >= sizeof (Rand64)) {
      GetRandomNumber64 (&Rand64);
      CopyMem (Entropy, &Rand64, sizeof (Rand64));
      Entropy += sizeof (Rand64);
      Size    -= sizeof (Rand64);
    } else if (Size >= sizeof (Rand32)) {
      GetRandomNumber32 (&Rand32);
      CopyMem (Entropy, &Rand32, sizeof (Rand32));
      Entropy += sizeof (Rand32);
      Size    -= sizeof (Rand32);
    } else if (Size >= sizeof (Rand16)) {
      GetRandomNumber16 (&Rand16);
      CopyMem (Entropy, &Rand16, sizeof (Rand16));
      Entropy += sizeof (Rand16);
      Size    -= sizeof (Rand16);
    } else {
      GetRandomNumber16 (&Rand16);
      CopyMem (Entropy, &Rand16, sizeof (UINT8));
      Entropy += sizeof (UINT8);
      Size    -= sizeof (UINT8);
    }
  }

  return Amount;
}
