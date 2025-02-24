/** @file
  FMP Authentication Lib in PEI phase.

  Copyright (C) 2022 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/FmpAuthenticationLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Authentication for FMP capsule in PEI carries very big CryptoLIB.
  Skip it here and do it in BDS when actual capsule update is happening.
  Borrow the interface to install CV Hob to enable capsule update in BDS automatically in recovery boot.

  @param[in]  Image                   Points to an FMP authentication image, started from EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               Size of the authentication image in bytes.
  @param[in]  PublicKeyData           The public key data used to validate the signature.
  @param[in]  PublicKeyDataLength     The length of the public key data.

  @retval EFI_SUCCESS                 Authentication pass.

**/
RETURN_STATUS
EFIAPI
AuthenticateFmpImage (
  IN EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN UINTN                              ImageSize,
  IN CONST UINT8                        *PublicKeyData,
  IN UINTN                              PublicKeyDataLength
  )
{
  UINT8  *CapsuleBuffer;
  UINTN  CapsuleSize;

  CapsuleBuffer = (UINT8 *)Image;
  //
  // Earlier processing removes Capsule image headers, get it back
  //
  for (CapsuleSize = 0; !CompareGuid (&gEfiFmpCapsuleGuid, (EFI_GUID *)(--CapsuleBuffer)); CapsuleSize++) {
    if (CapsuleSize > SIZE_4KB) {
      // Search 4KB and stop if not found.
      DEBUG ((DEBUG_ERROR, "Invalid Capsule Image"));
      CpuDeadLoop ();
    }
  }

  CapsuleSize += ImageSize + 1;
  DEBUG ((DEBUG_INFO, "Capsule saved in address = 0x%x CapsuleSize = 0x%x\n", CapsuleBuffer, CapsuleSize));
  BuildCvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)CapsuleBuffer, CapsuleSize);

  return EFI_SUCCESS;
}
