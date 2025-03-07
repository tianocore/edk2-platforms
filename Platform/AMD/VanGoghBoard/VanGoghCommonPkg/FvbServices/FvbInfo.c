/** @file
Defines data structure that is the volume header found.These data is intent
to decouple FVB driver with FV header.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013 Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// The package level header files this module uses
//
#include <PiDxe.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/EventGroup.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/DevicePath.h>
//
// The Library classes this module consumes
//
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#define FVB_MEDIA_BLOCK_SIZE  (0x00010000)

#define SYSTEM_NV_BLOCK_NUM  ((FixedPcdGet32(PcdFlashNvStorageVariableSize)+ FixedPcdGet32(PcdFlashNvStorageFtwWorkingSize) + FixedPcdGet32(PcdFlashNvStorageFtwSpareSize))/ FVB_MEDIA_BLOCK_SIZE)

typedef struct {
  EFI_PHYSICAL_ADDRESS          BaseAddress;
  EFI_FIRMWARE_VOLUME_HEADER    FvbInfo;
  //
  // EFI_FV_BLOCK_MAP_ENTRY    ExtraBlockMap[n];//n=0
  //
  EFI_FV_BLOCK_MAP_ENTRY        End[1];
} EFI_FVB2_MEDIA_INFO;

EFI_FVB2_MEDIA_INFO  mPlatformFvbMediaInfo =
  //
  // Systen NvStorage FVB
  //
{
  0,
  {
    {
      0,
    },    // ZeroVector[16]
    EFI_SYSTEM_NV_DATA_FV_GUID,
    FVB_MEDIA_BLOCK_SIZE *SYSTEM_NV_BLOCK_NUM,
    EFI_FVH_SIGNATURE,
    EFI_FVB2_MEMORY_MAPPED |
    EFI_FVB2_READ_ENABLED_CAP |
    EFI_FVB2_READ_STATUS |
    EFI_FVB2_WRITE_ENABLED_CAP |
    EFI_FVB2_WRITE_STATUS |
    EFI_FVB2_ERASE_POLARITY |
    EFI_FVB2_ALIGNMENT_16,
    sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
    0xFBFF,    // CheckSum
    0,         // ExtHeaderOffset
    {
      0,
    },    // Reserved[1]
    2,    // Revision
    {
      {
        SYSTEM_NV_BLOCK_NUM,
        FVB_MEDIA_BLOCK_SIZE,
      }
    }
  },
  {
    {
      0,
      0
    }
  }
};

/**
  Get Fvb information.

  @param[in] BaseAddress    The base address compare with NvStorageVariable base address.
  @param[out] FvbInfo        Fvb information.

  @retval EFI_SUCCESS       Get Fvb information successfully.
  @retval EFI_NOT_FOUND     Not find Fvb information.

**/
EFI_STATUS
EFIAPI
GetFvbInfo (
  IN  UINT64                      BaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FvbInfo
  )
{
  mPlatformFvbMediaInfo.BaseAddress = PcdGet32 (PcdFlashNvStorageVariableBase);

  if (mPlatformFvbMediaInfo.BaseAddress == BaseAddress) {
    *FvbInfo = &mPlatformFvbMediaInfo.FvbInfo;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}
