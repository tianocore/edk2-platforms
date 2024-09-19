/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLASH_LIB_H_
#define FLASH_LIB_H_

/**
  Get the information about the Flash region to store the FailSafe status.

  @param[out] FailSafeBase       Base address of the FailSafe space.
  @param[out] FailSafeSize       Total size of the FailSafe space.

  @retval EFI_SUCCESS            Operation succeeded.
  @retval EFI_INVALID_PARAMETER  FailSafeBase or FailSafeSize is NULL.
  @retval Others                 An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashGetFailSafeInfo (
  OUT UINTN   *FailSafeBase,
  OUT UINT32  *FailSafeSize
  );

/**
  Get the information about the Flash region to store the NVRAM variables.

  @param[out] NvRamBase          Base address of the NVRAM space.
  @param[out] NvRamSize          Total size of the NVRAM space.

  @retval EFI_SUCCESS            Operation succeeded.
  @retval EFI_INVALID_PARAMETER  NvRamBase or NvRamSize is NULL.
  @retval Others                 An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashGetNvRamInfo (
  OUT UINTN   *NvRamBase,
  OUT UINT32  *NvRamSize
  );

/**
  Get the information about the second Flash region to store the NVRAM variables.

  @param[out] NvRam2Base         Base address of the NVRAM space.
  @param[out] NvRam2Size         Total size of the NVRAM space.

  @retval EFI_SUCCESS            Operation succeeded.
  @retval EFI_INVALID_PARAMETER  NvRam2Base or NvRam2Size is NULL.
  @retval Others                 An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashGetNvRam2Info (
  OUT UINTN   *NvRam2Base,
  OUT UINT32  *NvRam2Size
  );

/**
  Erase a region of the Flash.

  @param[in] ByteAddress         Start address of the region.
  @param[in] Length              Number of bytes to erase.

  @retval EFI_SUCCESS            Operation succeeded.
  @retval EFI_INVALID_PARAMETER  Length is Zero.
  @retval Others                 An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashEraseCommand (
  IN  UINTN   ByteAddress,
  IN  UINT32  Length
  );

/**
  Write data buffer to the Flash.

  @param[in] ByteAddress         Start address of the region.
  @param[in] Buffer              Pointer to the data buffer.
  @param[in] Length              Number of bytes to write.

  @retval EFI_SUCCESS            Operation succeeded.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or Length is Zero.
  @retval Others                 An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashWriteCommand (
  IN  UINTN   ByteAddress,
  IN  VOID    *Buffer,
  IN  UINT32  Length
  );

/**
  Read data from the Flash into Buffer.

  @param[in]  ByteAddress        Start address of the region.
  @param[out] Buffer             Pointer to the data buffer.
  @param[in]  Length             Number of bytes to read.

  @retval EFI_SUCCESS            Operation succeeded.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or Length is Zero.
  @retval Others                 An error has occurred.
**/
EFI_STATUS
EFIAPI
FlashReadCommand (
  IN  UINTN   ByteAddress,
  OUT VOID    *Buffer,
  IN  UINT32  Length
  );

#endif /* FLASH_LIB_H_ */
