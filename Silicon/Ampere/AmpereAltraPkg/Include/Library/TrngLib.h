/** @file
  RNG (Random Number Generator) Library that uses Hardware RNG in SMpro.

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRNG_LIB_H_
#define TRNG_LIB_H_

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
  );

#endif /* TRNG_LIB_H_ */
