/** @file
  Common FVP and SoC Morello GOP platform implementation details

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/LcdPlatformLib.h>

STATIC_ASSERT (
  FixedPcdGet64 (PcdPlatformGopBufferBase) >> 40 == 0,
  "The ARM Mali Dxx frame-buffer address base cannot be wider than 40 bits"
  );

/** Return info about reserved frame-buffer memory.

  @param[out] VramBaseAddress     A pointer to the frame-buffer address.
  @param[out] VramSize            A pointer to the size of the frame
                                  buffer in bytes

  @retval EFI_SUCCESS             Frame-buffer memory allocation success.
  @retval EFI_UNSUPPORTED         No frame-buffer memory reserved.
**/
EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS  *VramBaseAddress,
  OUT UINTN                 *VramSize
  )
{
  if (FixedPcdGet32 (PcdPlatformGopBufferSize) == 0) {
    return EFI_UNSUPPORTED;
  }

  *VramBaseAddress = FixedPcdGet64 (PcdPlatformGopBufferBase);
  *VramSize        = FixedPcdGet32 (PcdPlatformGopBufferSize);
  return EFI_SUCCESS;
}
