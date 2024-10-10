/** @file
  In some implementations of the TPM, the hardware can provide a secret
  value to the TPM. This secret value is statistically unique to the
  instance of the TPM. Typical uses of this value are to provide
  personalization to the random number generation and as a shared secret
  between the TPM and the manufacturer.

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
  _plat__GetUnique()

  This function is used to access the platform-specific unique value.
  This function places the unique value in the provided buffer ('b')
  and returns the number of bytes transferred. The function will not
  copy more data than 'bSize'.

  NOTE: If a platform unique value has unequal distribution of uniqueness
  and 'bSize' is smaller than the size of the unique value, the 'bSize'
  portion with the most uniqueness should be returned.

  @param [in] Which    if == 0, Copy unique value from start
                       otherwise, copy from end
  @param [in] Size     Size of Buffer
  @param [out] Buffer

  @return Size of unique value.

**/
UINT32
EFIAPI
PlatformTpmLibGetUnique (
  IN  UINT32  Which,    // authorities (0) or details
  IN  UINT32  Size,     // size of the buffer
  OUT UINT8   *Buffer   // output buffer
  )
{
  INT32  Idx;
  UINT8  *TmpBuffer;

  SetMem (Buffer, 0x00, Size);

  Size = ((Size < sizeof (EFI_GUID)) ? Size : sizeof (EFI_GUID));

  if (Which == 0) {
    CopyMem (&Buffer, &gEfiCallerIdGuid, Size);
  } else {
    TmpBuffer = (UINT8 *)&gEfiCallerIdGuid;
    for (Idx = Size - 1; Idx >= 0; Idx--) {
      Buffer[Idx] = TmpBuffer[Idx];
    }
  }

  return Size;
}
