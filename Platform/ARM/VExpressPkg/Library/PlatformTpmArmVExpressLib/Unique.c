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
  This function places the unique value in the provided buffer ('Buffer')
  and returns the number of bytes transferred. The function will not
  copy more data than 'Size'.

  NOTE: If a platform unique value has unequal distribution of uniqueness
  and 'Size' is smaller than the size of the unique value, the 'Size'
  portion with the most uniqueness should be returned.

  @param [in]   Which    0: reserved, 1: permanent vendor unique value.
  @param [in]   Size     Size of Buffer
  @param [out]  Buffer   Buffer

  @return Size of unique value.

**/
UINT32
EFIAPI
PlatformTpmLibGetUnique (
  IN  UINT32  Which,
  IN  UINT32  Size,
  OUT UINT8   *Buffer
  )
{
  EFI_STATUS  Status;
  UINT64      Value;

  if (Which == 0) {
    /*
     * @Which == 0 menas "reserved" However,
     * TCG TPM Library prohibits __plat_GetUnique() with @Which == 0.
     */
    ASSERT (0);
    return 0;
  }

  ZeroMem (Buffer, Size);

  Size  = ((Size < sizeof (Value)) ? Size : sizeof (Value));
  Value = PcdGet64 (PcdTpmUniqueValue);

  if (Value == 0) {
    DEBUG ((
      DEBUG_WARN,
      "%a: PcdTpmUniqueValue doesn't specified at build. Use Random value\n",
      __func__
      ));

    PlatformTpmLibGetEntropy ((UINT8 *)&Value, sizeof (Value));

    Status = (EFI_STATUS)PcdSet64S (PcdTpmUniqueValue, Value);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return 0;
    }
  }

  CopyMem (Buffer, &Value, Size);

  return Size;
}
