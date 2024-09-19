/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/RngLib.h>
#include <Library/TrngLib.h>

/**
  Generates a 16-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber16 (
  OUT UINT16  *Rand
  )
{
  EFI_STATUS  Status;

  ASSERT (Rand != NULL);
  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumbers ((UINT8 *)Rand, sizeof (UINT16));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates a 32-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber32 (
  OUT UINT32  *Rand
  )
{
  EFI_STATUS  Status;

  ASSERT (Rand != NULL);
  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumbers ((UINT8 *)Rand, sizeof (UINT32));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates a 64-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT UINT64  *Rand
  )
{
  EFI_STATUS  Status;

  ASSERT (Rand != NULL);
  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumbers ((UINT8 *)Rand, sizeof (UINT64));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates a 128-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 128-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber128 (
  OUT UINT64  *Rand
  )
{
  EFI_STATUS  Status;

  ASSERT (Rand != NULL);
  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumbers ((UINT8 *)Rand, 2 * sizeof (UINT64));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}
