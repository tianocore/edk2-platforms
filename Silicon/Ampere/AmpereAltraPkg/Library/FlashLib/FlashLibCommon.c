/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FlashLib.h>
#include <Library/MemoryAllocationLib.h>

#include "FlashLibCommon.h"

UINT8  *gFlashLibPhysicalBuffer;
UINT8  *gFlashLibVirtualBuffer;

/**
  Convert Virtual Address to Physical Address at Runtime.

  @param[in] VirtualPtr       Virtual Address Pointer.
  @param[in] Size             Total bytes of the buffer.

  @retval Pointer to the physical address of the converted buffer.
**/
STATIC
UINT8 *
ConvertToPhysicalBuffer (
  IN UINT8   *VirtualPtr,
  IN UINT32  Size
  )
{
  ASSERT (VirtualPtr != NULL);
  CopyMem (gFlashLibVirtualBuffer, VirtualPtr, Size);
  return gFlashLibPhysicalBuffer;
}

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
  )
{
  EFI_MM_COMMUNICATE_FAILSAFE_INFO_RESPONSE  FailSafeInfo;
  EFI_STATUS                                 Status;
  UINT64                                     MmData[5];

  if ((FailSafeBase == NULL) || (FailSafeSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MmData[0] = MM_SPINOR_FUNC_GET_FAILSAFE_INFO;

  Status = FlashMmCommunicate (
             MmData,
             sizeof (MmData),
             &FailSafeInfo,
             sizeof (FailSafeInfo)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FailSafeInfo.Status == MM_SPINOR_RES_SUCCESS) {
    *FailSafeBase = FailSafeInfo.FailSafeBase;
    *FailSafeSize = FailSafeInfo.FailSafeSize;

    DEBUG ((
      DEBUG_INFO,
      "%a: FailSafe Base 0x%llx, Size 0x%lx\n",
      __func__,
      *FailSafeBase,
      *FailSafeSize
      ));
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_MM_COMMUNICATE_NVRAM_INFO_RESPONSE  NvRamInfo;
  EFI_STATUS                              Status;
  UINT64                                  MmData[5];

  if ((NvRamBase == NULL) || (NvRamSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MmData[0] = MM_SPINOR_FUNC_GET_NVRAM_INFO;

  Status = FlashMmCommunicate (
             MmData,
             sizeof (MmData),
             &NvRamInfo,
             sizeof (NvRamInfo)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (NvRamInfo.Status == MM_SPINOR_RES_SUCCESS) {
    *NvRamBase = NvRamInfo.NvRamBase;
    *NvRamSize = NvRamInfo.NvRamSize;
    DEBUG ((
      DEBUG_INFO,
      "%a: NVRAM Base 0x%llx, Size 0x%lx\n",
      __func__,
      *NvRamBase,
      *NvRamSize
      ));
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_MM_COMMUNICATE_NVRAM_INFO_RESPONSE  NvRam2Info;
  EFI_STATUS                              Status;
  UINT64                                  MmData[5];

  if ((NvRam2Base == NULL) || (NvRam2Size == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MmData[0] = MM_SPINOR_FUNC_GET_NVRAM2_INFO;

  Status = FlashMmCommunicate (
             MmData,
             sizeof (MmData),
             &NvRam2Info,
             sizeof (NvRam2Info)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (NvRam2Info.Status == MM_SPINOR_RES_SUCCESS) {
    *NvRam2Base = NvRam2Info.NvRamBase;
    *NvRam2Size = NvRam2Info.NvRamSize;
    DEBUG ((
      DEBUG_INFO,
      "%a: NVRAM2 Base 0x%llx, Size 0x%lx\n",
      __func__,
      *NvRam2Base,
      *NvRam2Size
      ));
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_MM_COMMUNICATE_SPINOR_RESPONSE  MmSpiNorRes;
  EFI_STATUS                          Status;
  UINT64                              MmData[5];

  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  MmData[0] = MM_SPINOR_FUNC_ERASE;
  MmData[1] = ByteAddress;
  MmData[2] = Length;

  Status = FlashMmCommunicate (
             MmData,
             sizeof (MmData),
             &MmSpiNorRes,
             sizeof (MmSpiNorRes)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (MmSpiNorRes.Status != MM_SPINOR_RES_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: Device error %llx\n", __func__, MmSpiNorRes.Status));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_MM_COMMUNICATE_SPINOR_RESPONSE  MmSpiNorRes;
  EFI_STATUS                          Status;
  UINT64                              MmData[5];
  UINTN                               Remain, NumWrite;
  UINTN                               Count = 0;

  if ((Buffer == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Remain = Length;
  while (Remain > 0) {
    NumWrite = (Remain > EFI_MM_MAX_TMP_BUF_SIZE) ? EFI_MM_MAX_TMP_BUF_SIZE : Remain;

    MmData[0] = MM_SPINOR_FUNC_WRITE;
    MmData[1] = ByteAddress + Count;
    MmData[2] = NumWrite;
    MmData[3] = (UINT64)ConvertToPhysicalBuffer (Buffer + Count, NumWrite);

    Status = FlashMmCommunicate (
               MmData,
               sizeof (MmData),
               &MmSpiNorRes,
               sizeof (MmSpiNorRes)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (MmSpiNorRes.Status != MM_SPINOR_RES_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "%a: Device error 0x%llx\n", __func__, MmSpiNorRes.Status));
      return EFI_DEVICE_ERROR;
    }

    Remain -= NumWrite;
    Count  += NumWrite;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_MM_COMMUNICATE_SPINOR_RESPONSE  MmSpiNorRes;
  EFI_STATUS                          Status;
  UINT64                              MmData[5];
  UINTN                               Remain, NumRead;
  UINTN                               Count = 0;

  if ((Buffer == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Remain = Length;
  while (Remain > 0) {
    NumRead = (Remain > EFI_MM_MAX_TMP_BUF_SIZE) ? EFI_MM_MAX_TMP_BUF_SIZE : Remain;

    MmData[0] = MM_SPINOR_FUNC_READ;
    MmData[1] = ByteAddress + Count;
    MmData[2] = NumRead;
    MmData[3] = (UINT64)gFlashLibPhysicalBuffer;  // Read data into the temp buffer with specified virtual address

    Status = FlashMmCommunicate (
               MmData,
               sizeof (MmData),
               &MmSpiNorRes,
               sizeof (MmSpiNorRes)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (MmSpiNorRes.Status != MM_SPINOR_RES_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "%a: Device error %llx\n", __func__, MmSpiNorRes.Status));
      return EFI_DEVICE_ERROR;
    }

    //
    // Get data from the virtual address of the temp buffer.
    //
    CopyMem ((VOID *)(Buffer + Count), (VOID *)gFlashLibVirtualBuffer, NumRead);
    Remain -= NumRead;
    Count  += NumRead;
  }

  return EFI_SUCCESS;
}
