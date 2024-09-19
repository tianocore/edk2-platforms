/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TrngLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Rng.h>

/**
  Returns information about the random number generation implementation.

  @param[in]     This                 A pointer to the EFI_RNG_PROTOCOL instance.
  @param[in,out] RNGAlgorithmListSize On input, the size in bytes of RNGAlgorithmList.
                                      On output with a return code of EFI_SUCCESS, the size
                                      in bytes of the data returned in RNGAlgorithmList. On output
                                      with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of RNGAlgorithmList required to obtain the list.
  @param[out] RNGAlgorithmList        A caller-allocated memory buffer filled by the driver
                                      with one EFI_RNG_ALGORITHM element for each supported
                                      RNG algorithm. The list must not change across multiple
                                      calls to the same driver. The first algorithm in the list
                                      is the default algorithm for the driver.

  @retval EFI_SUCCESS                 The RNG algorithm list was returned successfully.
  @retval EFI_INVALID_PARAMETER       One or more of the parameters are incorrect.
  @retval EFI_BUFFER_TOO_SMALL        The buffer RNGAlgorithmList is too small to hold the result.

**/
EFI_STATUS
EFIAPI
RngGetInfo (
  IN     EFI_RNG_PROTOCOL   *This,
  IN OUT UINTN              *RNGAlgorithmListSize,
  OUT    EFI_RNG_ALGORITHM  *RNGAlgorithmList
  )
{
  if ((This == NULL) || (RNGAlgorithmListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*RNGAlgorithmListSize < sizeof (EFI_RNG_ALGORITHM)) {
    *RNGAlgorithmListSize = sizeof (EFI_RNG_ALGORITHM);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (RNGAlgorithmList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *RNGAlgorithmListSize = sizeof (EFI_RNG_ALGORITHM);
  CopyGuid (RNGAlgorithmList, &gEfiRngAlgorithmRaw);

  return EFI_SUCCESS;
}

/**
  Produces and returns an RNG value using either the default or specified RNG algorithm.

  @param[in]  This                    A pointer to the EFI_RNG_PROTOCOL instance.
  @param[in]  RNGAlgorithm            A pointer to the EFI_RNG_ALGORITHM that identifies the RNG
                                      algorithm to use. May be NULL in which case the function will
                                      use its default RNG algorithm.
  @param[in]  RNGValueLength          The length in bytes of the memory buffer pointed to by
                                      RNGValue. The driver shall return exactly this numbers of bytes.
  @param[out] RNGValue                A caller-allocated memory buffer filled by the driver with the
                                      resulting RNG value.

  @retval EFI_SUCCESS                 The RNG value was returned successfully.
  @retval EFI_UNSUPPORTED             The algorithm specified by RNGAlgorithm is not supported by
                                      this driver.
  @retval EFI_DEVICE_ERROR            An RNG value could not be retrieved due to a hardware or
                                      firmware error.
  @retval EFI_INVALID_PARAMETER       RNGValue is NULL or RNGValueLength is zero.

**/
EFI_STATUS
EFIAPI
RngGetRNG (
  IN  EFI_RNG_PROTOCOL *This,
  IN  EFI_RNG_ALGORITHM *RNGAlgorithm, OPTIONAL
  IN  UINTN             RNGValueLength,
  OUT UINT8             *RNGValue
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (RNGValueLength == 0) || (RNGValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We only support the raw algorithm, so reject requests for anything else
  //
  if ((RNGAlgorithm != NULL) &&
      !CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmRaw))
  {
    return EFI_UNSUPPORTED;
  }

  Status = GenerateRandomNumbers (RNGValue, RNGValueLength);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to generate a random number. \n",
      __func__,
      __LINE__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/*
 * The Random Number Generator (RNG) protocol
 */
EFI_RNG_PROTOCOL  mRng = {
  RngGetInfo,
  RngGetRNG
};

/**
  The user Entry Point for the Random Number Generator (RNG) driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval EFI_NOT_SUPPORTED Platform does not support RNG.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
RngDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  //
  // Install UEFI RNG (Random Number Generator) Protocol
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRngProtocolGuid,
                  &mRng,
                  NULL
                  );

  return Status;
}
