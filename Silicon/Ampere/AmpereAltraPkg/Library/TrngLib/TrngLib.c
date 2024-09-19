/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>
#include <Library/TrngLib.h>

/**
  Generates a random number by using Hardware RNG in SMpro.

  @param[out] Buffer      Buffer to receive the random number.
  @param[in]  BufferSize  Number of bytes in Buffer.

  @retval EFI_SUCCESS           The random value was returned successfully.
  @retval EFI_DEVICE_ERROR      A random value could not be retrieved
                                due to a hardware or firmware error.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or BufferSize is zero.
**/
EFI_STATUS
EFIAPI
GenerateRandomNumbers (
  OUT UINT8  *Buffer,
  IN  UINTN  BufferSize
  )
{
  UINTN       Count;
  UINTN       RandSize;
  UINT64      Value;
  EFI_STATUS  Status;

  if ((BufferSize == 0) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // SMpro only supports generating a 64-bits random number once.
  //
  RandSize = sizeof (UINT64);
  for (Count = 0; Count < (BufferSize / sizeof (UINT64)) + 1; Count++) {
    if (Count == (BufferSize / sizeof (UINT64))) {
      RandSize = BufferSize % sizeof (UINT64);
    }

    if (RandSize != 0) {
      Status = MailboxMsgGetRandomNumber64 ((UINT8 *)&Value);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to get random number!\n", __func__));
        return EFI_DEVICE_ERROR;
      }

      CopyMem (Buffer + Count * sizeof (UINT64), &Value, RandSize);
    }
  }

  return EFI_SUCCESS;
}
