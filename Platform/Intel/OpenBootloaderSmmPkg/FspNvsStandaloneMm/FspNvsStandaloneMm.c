/** @file
  Copyright (c) 2026 Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Base.h>
#include <Uefi/UefiMultiPhase.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/Spi.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FspWrapperNvsProtocol.h>
#include <Library/TimerLib.h>
#include <Library/HobLib.h>

#include <Guid/FspNvsLocationHob.h>

EFI_GUID gFspNvsLocationHobGuid = FSP_NVS_LOCATION_GUID;

//
// Per the FSP NVS Manager spec
// the first 4 bytes are the FSP NVS data size,
// and remaining bytes are the FSP NVS data
//
typedef struct {
UINT32  NvsDataSize;
UINT8   NvsData[0];
} FSP_NVS_STORAGE_HEADER;

#define ALIGN_4K_LOW(Addr)    ((UINTN)(Addr) & ~(UINTN)(SIZE_4KB - 1))

/**
  Write DataLength bytes pointed by Data to the Offset-th byte in NVS buffer.

  @param[in] Offset               Byte offset from the beginning of NVS buffer.
  @param[in] Data                 Pointer to the data to write.
  @param[in] DataLength           Number of bytes to write.

  @retval EFI_SUCCESS             Write succeeds.
  @retval EFI_DEVICE_ERROR        Write fails due to NVS device error.
  @retval EFI_INVALID_PARAMETER   Offset + DataLength exceeds the total length of NVS buffer.

**/
EFI_STATUS
EFIAPI
StandaloneMmWriteFspNvs (
  IN UINTN  Offset,
  IN VOID   *Data,
  IN UINTN  DataLength
  );

FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL  mFspWrapperNvsProtocol = {
  0x1,  // Revision
  StandaloneMmWriteFspNvs
};


/**
  Read the FSP NVS region from flash, if the NVS region is not programed or failed to get the data,
  the NVS data size will be set to 0 and returned.

  @param[in, out]  NvsSize    - The size of the FSP NVS data
  @param[in]       Offset     - The offset from the beginning of the FSP NVS Data region to start reading.
  @param[in]       DataLength - The length of data to read from the FSP NVS region.

  @return The pointer to the FSP NVS data, NULL if failed to read the FSP NVS region from flash.
          Return the Data buffer and size

**/
VOID*
EFIAPI
ReadFspNvs (
  IN OUT UINT32   *NvsSize,
  IN UINTN        Offset,
  IN UINTN        DataLength
  )
{
  VOID                    *NvsDataPtr;
  UINT32                  FspNvsRegionSize;
  FSP_NVS_STORAGE_HEADER  *NvsStorageHeader;
  EFI_HOB_GUID_TYPE       *GuidHob;
  FSP_NVS_LOCATION        *FspNvsLocationPtr;
  UINTN                   NvsReadSize;

  GuidHob                 = NULL;
  FspNvsLocationPtr        = NULL;

  NvsDataPtr = NULL;

  DEBUG ((DEBUG_ERROR, "%a: Enter.\n", __FUNCTION__));

  if (NvsSize == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Bad input parameter.\n", __FUNCTION__));
    return NULL;
  }

  GuidHob = GetFirstGuidHob (&gFspNvsLocationHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FSP NVS location HOB.\n", __FUNCTION__));
    goto Error;
  }
  FspNvsLocationPtr = (FSP_NVS_LOCATION *)(GET_GUID_HOB_DATA (GuidHob));
  if (FspNvsLocationPtr->CpuLinearAddress != 0 && FspNvsLocationPtr->Size != 0) {
    NvsStorageHeader = (FSP_NVS_STORAGE_HEADER *)(UINTN) FspNvsLocationPtr->CpuLinearAddress;
    FspNvsRegionSize   = (UINT32)FspNvsLocationPtr->Size;
    *NvsSize         = NvsStorageHeader->NvsDataSize;
  }
  DEBUG ((DEBUG_INFO, "%a: Get from gFspNvsLocationHobGuid FspNvsRegionSize:  0x%x\n", __FUNCTION__,
  FspNvsRegionSize));

  //
  // On the first boot, all the region data are all 0xFF's,
  // If the Nvs Data Size is 0xFFFFFFFF, it means the region is not programmed yet.
  //
  if (*NvsSize == MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "%a: The FSP NVS region is not programmed yet.\n", __FUNCTION__));
    goto Error;
  }

  if (*NvsSize > FspNvsRegionSize) {
    DEBUG ((DEBUG_ERROR, "%a: Illegal Nvs Data Size programed.\n", __FUNCTION__));
    goto Error;
  }

  if (DataLength != 0) {
    if ((Offset + DataLength) > *NvsSize) {
      DEBUG ((DEBUG_ERROR, "%a: Offset exceeds the total length of NVS buffer.\n", __FUNCTION__));
      goto Error;
    }
    NvsReadSize = DataLength;
  } else {
    NvsReadSize = *NvsSize;
  }

  NvsDataPtr = (VOID *)AllocatePages (EFI_SIZE_TO_PAGES (NvsReadSize));
  if (NvsDataPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate pages for NvsData.\n", __FUNCTION__));
    goto Error;
  }
  CopyMem (NvsDataPtr, NvsStorageHeader->NvsData + Offset, NvsReadSize);

  DEBUG ((DEBUG_ERROR, "%a: Exit.\n", __FUNCTION__));
  return NvsDataPtr;

Error:
  *NvsSize = 0;
  DEBUG ((DEBUG_ERROR, "%a: Exit with Error.\n", __FUNCTION__));
  return NULL;
}

/**
  Write DataLength bytes pointed by Data to the Offset-th byte in NVS buffer.

  @param[in] Offset               Byte offset from the beginning of NVS buffer.
  @param[in] Data                 Pointer to the data to write.
  @param[in] DataLength           Number of bytes to write.

  @retval EFI_SUCCESS             Write succeeds.
  @retval EFI_DEVICE_ERROR        Write fails due to NVS device error.
  @retval EFI_INVALID_PARAMETER   Offset + DataLength exceeds the total length of NVS buffer.

**/
EFI_STATUS
EFIAPI
StandaloneMmWriteFspNvs (
  IN UINTN  Offset,
  IN VOID   *Data,
  IN UINTN  DataLength
  )
{
  UINT32             NvsSize;
  UINT8              *NvsData;
  UINT8              *DataToFlash;
  UINT32             FspNvsRegionOffset;
  UINT32             FspNvsRegionSize;
  UINT32             FlashWriteStart;
  UINT32             FlashWriteEnd;
  UINT32             FlashWriteSize;
  PCH_SPI_PROTOCOL   *SmmSpiProtocol;
  EFI_STATUS         Status;
  UINT64             TscStart;
  UINT64             TscEnd;
  BOOLEAN            OnlyBitClearOperations;

  NvsData                 = NULL;
  DataToFlash             = NULL;
  NvsSize                 = 0;
  OnlyBitClearOperations  = TRUE;


  FSP_NVS_STORAGE_HEADER  *NvsStorageHeader;
  EFI_HOB_GUID_TYPE       *GuidHob;
  FSP_NVS_LOCATION        *FspNvsLocationPtr;

  DEBUG ((DEBUG_ERROR, "%a: Enter.\n", __FUNCTION__));

  GuidHob = GetFirstGuidHob (&gFspNvsLocationHobGuid);

  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get FSP NVS location HOB.\n", __FUNCTION__));
    goto Exit;
  }
  FspNvsLocationPtr = (FSP_NVS_LOCATION *)(GET_GUID_HOB_DATA (GuidHob));
  if (FspNvsLocationPtr->CpuLinearAddress != 0 && FspNvsLocationPtr->Size != 0) {
    FspNvsRegionOffset = FspNvsLocationPtr->FlashLinearAddress;
    FspNvsRegionSize   = (UINT32) FspNvsLocationPtr->Size;
  }
  DEBUG ((DEBUG_INFO, "%a: FspNvsRegionOffset: 0x%x FspNvsRegionSize:  0x%x\n", __FUNCTION__,
    FspNvsRegionOffset,
    FspNvsRegionSize));
  //
  // Locate the SMM SPI Protocol Interface
  //
  Status = gMmst->MmLocateProtocol (&gPchSmmSpiProtocolGuid, NULL, (VOID **) &SmmSpiProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate SPI PPI.\n", __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  TscStart = GetPerformanceCounter ();

  NvsData = ReadFspNvs (&NvsSize, Offset, DataLength);
  if (NvsData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read FSP NVS original data.\n", __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (CompareMem (NvsData, Data, DataLength) == 0) {
    DEBUG ((DEBUG_INFO, "%a: Same FSP NVS data, no need to update.\n", __FUNCTION__));
    Status = EFI_SUCCESS;
    goto Exit;
  }

  //
  // Check if the write operation only clears bits (1->0).
  // Flash memory can clear bits without requiring an erase operation.
  //
  for (UINTN Index = 0; Index < DataLength; Index++) {
    if ((NvsData[Index] ^ ((UINT8 *)Data)[Index]) & ((UINT8 *)Data)[Index]) {
      OnlyBitClearOperations = FALSE;
      break;
    }
  }

  if (OnlyBitClearOperations == TRUE) {
    //
    // For bit-clear operations (1->0), we can write directly without erase
    // Calculate the exact offset and size for the targeted write
    //
    FlashWriteStart = FspNvsRegionOffset + (UINT32)sizeof(FSP_NVS_STORAGE_HEADER) + (UINT32)Offset;
    FlashWriteSize  = (UINT32)DataLength;
    DataToFlash = (UINT8 *)Data;
  } else {
    if (NvsData != NULL) {
      FreePages (NvsData, EFI_SIZE_TO_PAGES (DataLength));
    }
    NvsData = ReadFspNvs (&NvsSize, 0, 0);

    if (NvsData == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to read FSP NVS original data.\n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    FlashWriteStart    = FspNvsRegionOffset + ALIGN_4K_LOW (sizeof(FSP_NVS_STORAGE_HEADER) + Offset);
    FlashWriteEnd      = FspNvsRegionOffset + (UINT32)ALIGN_VALUE (sizeof(FSP_NVS_STORAGE_HEADER) + Offset + DataLength, SIZE_4KB) - 1;
    FlashWriteSize     = FlashWriteEnd - FlashWriteStart + 1;

    if (FlashWriteSize > FspNvsRegionSize) {
      DEBUG ((DEBUG_ERROR, "%a: The FSP NVS flash region size is too small, the request size is 0x%x\n", __FUNCTION__, FlashWriteSize));
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    CopyMem (NvsData + Offset, Data, DataLength);
    DataToFlash = AllocatePages (EFI_SIZE_TO_PAGES (FlashWriteSize));
    if (DataToFlash == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to allocate pages for Nvs Data.\n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    if (FlashWriteStart == FspNvsRegionOffset) {
      //
      // If the write starts at the beginning of the FSP NVS region, we need to write the header as well.
      //
      NvsStorageHeader = (FSP_NVS_STORAGE_HEADER *) DataToFlash;
      NvsStorageHeader->NvsDataSize = NvsSize;
      CopyMem (DataToFlash + sizeof(FSP_NVS_STORAGE_HEADER), NvsData, FlashWriteSize - sizeof(FSP_NVS_STORAGE_HEADER));
    } else {
      //
      // If the write starts at an offset, we just copy the data.
      //
      CopyMem (DataToFlash, NvsData + ALIGN_4K_LOW (sizeof(FSP_NVS_STORAGE_HEADER) + Offset) - sizeof(FSP_NVS_STORAGE_HEADER), FlashWriteSize);
    }

    DEBUG ((DEBUG_INFO, "%a: Performing Fsp Nvs Erase (Bits changing from 0->1 detected)\n", __FUNCTION__));

    if ((FlashWriteSize % SIZE_4KB) != 0) {
      DEBUG ((DEBUG_ERROR, "%a: SPI flash erase require an integral multiple of 4KB !!)\n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      ASSERT (FALSE);
      goto Exit;
    }

    Status = SmmSpiProtocol->FlashErase (
                        SmmSpiProtocol,
                        FlashRegionBios,
                        FlashWriteStart,
                        FlashWriteSize
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to erase the FSP NVS region.\n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }

  DEBUG ((DEBUG_INFO, "%a: Fsp Nvs Write - START\n", __FUNCTION__));
  DEBUG ((DEBUG_INFO, "%a: Data size to flash 0x%x\n", __FUNCTION__, FlashWriteSize));

  Status = SmmSpiProtocol->FlashWrite (
                      SmmSpiProtocol,
                      FlashRegionBios,
                      FlashWriteStart,
                      FlashWriteSize,
                      DataToFlash
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to write the FSP NVS region.\n", __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "%a: Fsp Nvs Write - END\n", __FUNCTION__));

  TscEnd = GetPerformanceCounter ();
  DEBUG ((DEBUG_INFO, "Total time spent to flash FSP NVS region: %d ns.\n", GetTimeInNanoSecond (TscEnd - TscStart)));

Exit:

  if (OnlyBitClearOperations == TRUE) {
    //
    // For bit-clear operations, DataToFlash points to NvsData,
    // so we only need to free NvsData to avoid double-free
    //
    if (NvsData != NULL) {
      FreePages (NvsData, EFI_SIZE_TO_PAGES (DataLength));
    }
  } else {
    if (DataToFlash != NULL) {
      FreePages (DataToFlash, EFI_SIZE_TO_PAGES (FlashWriteSize));
    }
    if (NvsData != NULL) {
      FreePages (NvsData, EFI_SIZE_TO_PAGES (NvsSize));
    }
  }
  DEBUG ((DEBUG_ERROR, "%a: Exit.\n", __FUNCTION__));
  return Status;
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
FspNvsStandaloneMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS     Status;
  EFI_HANDLE     Handle;

  Handle = NULL;

  DEBUG ((DEBUG_ERROR, "%a: Enter.\n", __FUNCTION__));

  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gFspWrapperNonVolatileStorageProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mFspWrapperNvsProtocol
                    );

  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: gFspWrapperNonVolatileStorageProtocolGuid Fail.\n", __FUNCTION__));
    return Status;
  }

  DEBUG ((DEBUG_ERROR, "%a: Exit.\n", __FUNCTION__));
  return EFI_SUCCESS;
}
