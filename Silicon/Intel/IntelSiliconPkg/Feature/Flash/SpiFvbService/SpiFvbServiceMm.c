/** @file
  MM driver source for several Serial Flash devices
  which are compliant with the Intel(R) Serial Flash Interface Compatibility Specification.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SpiFvbServiceCommon.h"
#include <Library/MmServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Guid/VariableFormat.h>

/**
  The function installs the given EFI_FIRMWARE_VOLUME_BLOCK protocol instance.

  @param[in]  FvbInstance   The pointer to a FW volume instance structure,
                            which contains the information about one FV.

**/
VOID
InstallFvbProtocol (
  IN  EFI_FVB_INSTANCE               *FvbInstance
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  EFI_STATUS                            Status;
  EFI_HANDLE                            FvbHandle;

  if (FvbInstance == NULL) {
    ASSERT (FvbInstance != NULL);
    return;
  }

  CopyMem (&FvbInstance->FvbProtocol, &mFvbProtocolTemplate, sizeof (EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL));

  FvHeader = &FvbInstance->FvHeader;
  if (FvHeader == NULL) {
    return;
  }

  //
  // Set up the devicepath
  //
  DEBUG ((DEBUG_INFO, "SpiFvbServiceSmm.c: Setting up DevicePath for 0x%lx:\n", FvbInstance->FvBase));
  if (FvHeader->ExtHeaderOffset == 0) {
    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH
    //
    FvbInstance->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
    if (FvbInstance->DevicePath == NULL) {
      DEBUG ((DEBUG_ERROR, "SpiFvbServiceSmm.c: Memory allocation for MEMMAP_DEVICE_PATH failed\n"));
      return;
    }
    ((FV_MEMMAP_DEVICE_PATH *) FvbInstance->DevicePath)->MemMapDevPath.StartingAddress = FvbInstance->FvBase;
    ((FV_MEMMAP_DEVICE_PATH *) FvbInstance->DevicePath)->MemMapDevPath.EndingAddress   = FvbInstance->FvBase + FvHeader->FvLength - 1;
  } else {
    FvbInstance->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
    if (FvbInstance->DevicePath == NULL) {
      DEBUG ((DEBUG_ERROR, "SpiFvbServiceSmm.c: Memory allocation for FV_PIWG_DEVICE_PATH failed\n"));
      return;
    }
    CopyGuid (
      &((FV_PIWG_DEVICE_PATH *)FvbInstance->DevicePath)->FvDevPath.FvName,
      (GUID *)(UINTN)(FvbInstance->FvBase + FvHeader->ExtHeaderOffset)
      );
  }

  //
  // LocateDevicePath fails so install a new interface and device path
  //
  FvbHandle = NULL;

  Status = gMmst->MmInstallProtocolInterface (
                    &FvbHandle,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &(FvbInstance->FvbProtocol)
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gMmst->MmInstallProtocolInterface (
                    &FvbHandle,
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &(FvbInstance->DevicePath)
                    );
  ASSERT_EFI_ERROR (Status);
}

/**
  The function does the necessary initialization work for
  the Firmware Volume Block Driver.

**/
VOID
FvbInitialize (
  VOID
  )
{
  EFI_FVB_INSTANCE                      *FvbInstance;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  EFI_FV_BLOCK_MAP_ENTRY                *PtrBlockMapEntry;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  UINTN                                 Idx;
  UINT32                                MaxLbaSize;
  UINT32                                BytesWritten;
  UINTN                                 BytesErased;
  EFI_PHYSICAL_ADDRESS                  NvStorageBaseAddress;
  UINT64                                NvStorageFvSize;
  UINT32                                ExpectedBytesWritten;
  VARIABLE_STORE_HEADER                 *VariableStoreHeader;
  UINT8                                 VariableStoreType;
  UINT8                                 *NvStoreBuffer;

  Status = GetVariableFlashNvStorageInfo (&BaseAddress, &NvStorageFvSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_ERROR, "[%a] - An error ocurred getting variable info - %r.\n", __func__, Status));
    return;
  }

  // Stay within the current UINT32 size assumptions in the variable stack.
  Status = SafeUint64ToUint32 (BaseAddress, &mPlatformFvBaseAddress[0].FvBase);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_ERROR, "[%a] - 64-bit variable storage base address not supported.\n", __func__));
    return;
  }
  NvStorageBaseAddress = mPlatformFvBaseAddress[0].FvBase;

  Status = SafeUint64ToUint32 (NvStorageFvSize, &mPlatformFvBaseAddress[0].FvSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_ERROR, "[%a] - 64-bit variable storage size not supported.\n", __func__));
    return;
  }
  NvStorageFvSize = mPlatformFvBaseAddress[0].FvSize;

  mPlatformFvBaseAddress[1].FvBase = PcdGet32(PcdFlashMicrocodeFvBase);
  mPlatformFvBaseAddress[1].FvSize = PcdGet32(PcdFlashMicrocodeFvSize);

  {
    //
    // Make sure all FVB are valid and/or fix if possible
    //
    for (Idx = 0;; Idx++) {
      if (mPlatformFvBaseAddress[Idx].FvSize == 0 && mPlatformFvBaseAddress[Idx].FvBase == 0) {
        break;
      }

      BaseAddress = mPlatformFvBaseAddress[Idx].FvBase;
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;

      if (!IsFvHeaderValid (BaseAddress, FvHeader)) {
        BytesWritten = 0;
        BytesErased = 0;
        DEBUG ((DEBUG_ERROR, "ERROR - The FV at 0x%x is invalid!\n", FvHeader));

        //
        // Attempt to recover the FV
        //
        FvHeader = NULL;
        Status   = GetGeneratedFvByAddress (BaseAddress, &FvHeader);
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_WARN | DEBUG_ERROR,
            "ERROR - Can't recover FV header at 0x%x.  GetGeneratedFvByAddress Status %r\n",
            BaseAddress,
            Status
            ));
          continue;
        }
        DEBUG ((DEBUG_INFO, "Rewriting FV header at 0x%X with static data\n", BaseAddress));
        //
        // Spi erase
        //
        BytesErased = (UINTN) FvHeader->BlockMap->Length;
        Status = SpiFlashBlockErase( (UINTN) BaseAddress, &BytesErased);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - SpiFlashBlockErase Error  %r\n", Status));
          if (FvHeader != NULL) {
            FreePool (FvHeader);
          }
          continue;
        }
        if (BytesErased != FvHeader->BlockMap->Length) {
          DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - BytesErased != FvHeader->BlockMap->Length\n"));
          DEBUG ((DEBUG_ERROR, " BytesErased = 0x%X\n Length = 0x%X\n", BytesErased, FvHeader->BlockMap->Length));
          if (FvHeader != NULL) {
            FreePool (FvHeader);
          }
          continue;
        }

        BytesWritten         = FvHeader->HeaderLength;
        ExpectedBytesWritten = BytesWritten;
        if (BaseAddress != NvStorageBaseAddress) {
          Status = SpiFlashWrite ((UINTN)BaseAddress, &BytesWritten, (UINT8 *)FvHeader);
        } else {
          //
          // This is Variable Store, rewrite both EFI_FIRMWARE_VOLUME_HEADER and VARIABLE_STORE_HEADER.
          // The corrupted Variable content should be taken care by FaultTolerantWrite driver later.
          //
          NvStoreBuffer = NULL;
          NvStoreBuffer = AllocateZeroPool (sizeof (VARIABLE_STORE_HEADER) + FvHeader->HeaderLength);
          if (NvStoreBuffer != NULL) {
            //
            // Combine FV header and VariableStore header into the buffer.
            //
            CopyMem (NvStoreBuffer, FvHeader, FvHeader->HeaderLength);
            VariableStoreHeader = (VARIABLE_STORE_HEADER *)(NvStoreBuffer + FvHeader->HeaderLength);
            VariableStoreType   = PcdGet8 (PcdFlashVariableStoreType);
            switch (VariableStoreType) {
              case 0:
                DEBUG ((DEBUG_ERROR, "Type: gEfiVariableGuid\n"));
                CopyGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid);
                break;
              case 1:
                DEBUG ((DEBUG_ERROR, "Type: gEfiAuthenticatedVariableGuid\n"));
                CopyGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
                break;
              default:
                break;
            }

            //
            // Initialize common VariableStore header fields
            //
            VariableStoreHeader->Size      = (UINT32) (NvStorageFvSize - FvHeader->HeaderLength);
            VariableStoreHeader->Format    = VARIABLE_STORE_FORMATTED;
            VariableStoreHeader->State     = VARIABLE_STORE_HEALTHY;
            VariableStoreHeader->Reserved  = 0;
            VariableStoreHeader->Reserved1 = 0;

            //
            // Write buffer to flash
            //
            BytesWritten         = FvHeader->HeaderLength + sizeof (VARIABLE_STORE_HEADER);
            ExpectedBytesWritten = BytesWritten;
            Status               = SpiFlashWrite ((UINTN)BaseAddress, &BytesWritten, NvStoreBuffer);
            FreePool (NvStoreBuffer);
          } else {
            Status = EFI_OUT_OF_RESOURCES;
          }
        }

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - SpiFlashWrite Error  %r\n", Status));
          if (FvHeader != NULL) {
            FreePool (FvHeader);
          }
          continue;
        }
        if (BytesWritten != ExpectedBytesWritten) {
          DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - BytesWritten != ExpectedBytesWritten\n"));
          DEBUG ((DEBUG_ERROR, " BytesWritten = 0x%X\n ExpectedBytesWritten = 0x%X\n", BytesWritten, ExpectedBytesWritten));
          if (FvHeader != NULL) {
            FreePool (FvHeader);
          }
          continue;
        }
        Status = SpiFlashLock ();
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - SpiFlashLock Error  %r\n", Status));
          if (FvHeader != NULL) {
            FreePool (FvHeader);
          }
          continue;
        }
        DEBUG ((DEBUG_ERROR, "FV Header @ 0x%X restored with static data\n", BaseAddress));
        //
        // Clear cache for this range.
        //
        WriteBackInvalidateDataCacheRange ( (VOID *) (UINTN) BaseAddress, FvHeader->BlockMap->Length);
        if (FvHeader != NULL) {
          FreePool (FvHeader);
        }
      }
    }

    //
    // Calculate the total size for all firmware volume block instances
    //
    BufferSize = 0;
    for (Idx = 0; ; Idx++) {
      if (mPlatformFvBaseAddress[Idx].FvSize == 0 && mPlatformFvBaseAddress[Idx].FvBase == 0) {
        break;
      }
      BaseAddress = mPlatformFvBaseAddress[Idx].FvBase;
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;

      if (!IsFvHeaderValid (BaseAddress, FvHeader)) {
        DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - The FV in 0x%x is invalid!\n", FvHeader));
        continue;
      }

      BufferSize += (FvHeader->HeaderLength +
                    sizeof (EFI_FVB_INSTANCE) -
                    sizeof (EFI_FIRMWARE_VOLUME_HEADER)
                    );
    }

    mFvbModuleGlobal.FvbInstance =  (EFI_FVB_INSTANCE *) AllocateRuntimeZeroPool (BufferSize);
    if (mFvbModuleGlobal.FvbInstance == NULL) {
      ASSERT (FALSE);
      return;
    }

    MaxLbaSize      = 0;
    FvbInstance     = mFvbModuleGlobal.FvbInstance;
    mFvbModuleGlobal.NumFv   = 0;

    for (Idx = 0; ; Idx++) {
      if (mPlatformFvBaseAddress[Idx].FvSize == 0 && mPlatformFvBaseAddress[Idx].FvBase == 0) {
        break;
      }
      BaseAddress = mPlatformFvBaseAddress[Idx].FvBase;
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;

      if (!IsFvHeaderValid (BaseAddress, FvHeader)) {
        DEBUG ((DEBUG_WARN | DEBUG_ERROR, "ERROR - The FV in 0x%x is invalid!\n", FvHeader));
        continue;
      }

      FvbInstance->Signature = FVB_INSTANCE_SIGNATURE;
      CopyMem (&(FvbInstance->FvHeader), FvHeader, FvHeader->HeaderLength);

      FvHeader = &(FvbInstance->FvHeader);
      FvbInstance->FvBase = (UINTN)BaseAddress;

      //
      // Process the block map for each FV
      //
      FvbInstance->NumOfBlocks   = 0;
      for (PtrBlockMapEntry = FvHeader->BlockMap;
           PtrBlockMapEntry->NumBlocks != 0;
           PtrBlockMapEntry++) {
        //
        // Get the maximum size of a block.
        //
        if (MaxLbaSize < PtrBlockMapEntry->Length) {
          MaxLbaSize  = PtrBlockMapEntry->Length;
        }
        FvbInstance->NumOfBlocks += PtrBlockMapEntry->NumBlocks;
      }

      //
      // Add a FVB Protocol Instance
      //
      InstallFvbProtocol (FvbInstance);
      mFvbModuleGlobal.NumFv++;

      //
      // Move on to the next FvbInstance
      //
      FvbInstance = (EFI_FVB_INSTANCE *) ((UINTN)((UINT8 *)FvbInstance) +
                                            FvHeader->HeaderLength +
                                            (sizeof (EFI_FVB_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER)));

    }
  }
}
