/** @file  NorFlashStandaloneMm.c

  Copyright (c) 2011 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2020, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/NorFlashInfoLib.h>

#include "NorFlashCommon.h"

//
// Global variable declarations
//
NOR_FLASH_INSTANCE  **mNorFlashInstances;
UINT32              mNorFlashDeviceCount;
UINTN               mFlashNvStorageVariableBase;

NOR_FLASH_INSTANCE  mNorFlashInstanceTemplate = {
  NOR_FLASH_SIGNATURE, // Signature
  NULL,                // Handle ... NEED TO BE FILLED

  0, // Optional HostControllerBaseAddress  ... NEED TO BE FILLED
  0, // DeviceBaseAddress ... NEED TO BE FILLED
  0, // RegionBaseAddress ... NEED TO BE FILLED
  0, // Size ... NEED TO BE FILLED
  0, // StartLba

  {
    EFI_BLOCK_IO_PROTOCOL_REVISION2, // Revision
    NULL,                            // Media ... NEED TO BE FILLED
    NULL,                            // Reset;
    NULL,                            // ReadBlocks
    NULL,                            // WriteBlocks
    NULL                             // FlushBlocks
  }, // BlockIoProtocol

  {
    0,     // MediaId ... NEED TO BE FILLED
    FALSE, // RemovableMedia
    TRUE,  // MediaPresent
    FALSE, // LogicalPartition
    FALSE, // ReadOnly
    FALSE, // WriteCaching;
    0,     // BlockSize ... NEED TO BE FILLED
    4,     //  IoAlign
    0,     // LastBlock ... NEED TO BE FILLED
    0,     // LowestAlignedLba
    1,     // LogicalBlocksPerPhysicalBlock
  }, // Media;

  {
    EFI_DISK_IO_PROTOCOL_REVISION, // Revision
    NULL,                          // ReadDisk
    NULL                           // WriteDisk
  },

  {
    FvbGetAttributes,      // GetAttributes
    FvbSetAttributes,      // SetAttributes
    FvbGetPhysicalAddress, // GetPhysicalAddress
    FvbGetBlockSize,       // GetBlockSize
    FvbRead,               // Read
    FvbWrite,              // Write
    FvbEraseBlocks,        // EraseBlocks
    NULL,                  // ParentHandle
  },    //  FvbProtoccol;
  NULL, // ShadowBuffer
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(OFFSET_OF (NOR_FLASH_DEVICE_PATH, End)),
          (UINT8)(OFFSET_OF (NOR_FLASH_DEVICE_PATH, End) >> 8)
        }
      },
      { 0x0,                               0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
      },                                                             // GUID ... NEED TO BE FILLED
    },
    0, // Index
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
  }   // DevicePath
};

EFI_STATUS
NorFlashCreateInstance (
  IN UINTN                HostControllerBase,
  IN UINTN                NorFlashDeviceBase,
  IN UINTN                NorFlashRegionBase,
  IN UINTN                NorFlashSize,
  IN UINT32               Index,
  IN UINT32               BlockSize,
  IN BOOLEAN              SupportFvb,
  OUT NOR_FLASH_INSTANCE  **NorFlashInstance
  )
{
  EFI_STATUS          Status;
  NOR_FLASH_INSTANCE  *Instance;
  NOR_FLASH_INFO      *FlashInfo;
  UINT8               JedecId[6];

  ASSERT (NorFlashInstance != NULL);

  Instance = AllocateRuntimeCopyPool (sizeof (NOR_FLASH_INSTANCE), &mNorFlashInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->HostControllerBaseAddress = HostControllerBase;
  Instance->DeviceBaseAddress         = NorFlashDeviceBase;
  Instance->RegionBaseAddress         = NorFlashRegionBase;
  Instance->Size                      = NorFlashSize;

  Instance->BlockIoProtocol.Media = &Instance->Media;
  Instance->Media.MediaId         = Index;
  Instance->Media.BlockSize       = BlockSize;
  Instance->Media.LastBlock       = (NorFlashSize / BlockSize)-1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)Index;

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);
  if (Instance->ShadowBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto error_handler1;
  }

  Status = NorFlashReadId (Instance, JedecId);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      goto error_handler2;
    }
  } else {
    Status = NorFlashGetInfo (JedecId, &FlashInfo, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "NorFlashCreateInstance: JedecID not supported\n"));
    } else {
      NorFlashPrintInfo (FlashInfo);
      FreePool (FlashInfo);
    }
  }

  if (SupportFvb) {
    NorFlashFvbInitialize (Instance);

    Status = gMmst->MmInstallProtocolInterface (
                      &Instance->Handle,
                      &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &Instance->FvbProtocol
                      );
    if (EFI_ERROR (Status)) {
      goto error_handler2;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "standalone MM NOR Flash driver only support FVB.\n"));
    Status = EFI_UNSUPPORTED;
    goto error_handler2;
  }

  *NorFlashInstance = Instance;
  return Status;

error_handler2:
  FreePool (Instance->ShadowBuffer);

error_handler1:
  FreePool (Instance);
  return Status;
}

EFI_STATUS
EFIAPI
NorFlashInitialise (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS             Status;
  UINT32                 Index;
  NOR_FLASH_DESCRIPTION  *NorFlashDevices;
  BOOLEAN                ContainVariableStorage;

  Status = NorFlashPlatformInitialization ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Fail to initialize Nor Flash devices\n"));
    return Status;
  }

  Status = NorFlashPlatformGetDevices (&NorFlashDevices, &mNorFlashDeviceCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Fail to get Nor Flash devices\n"));
    return Status;
  }

  mNorFlashInstances = AllocatePool (sizeof (NOR_FLASH_INSTANCE *) * mNorFlashDeviceCount);

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    // Check if this NOR Flash device contain the variable storage region

    if (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) != 0) {
      ContainVariableStorage =
        (NorFlashDevices[Index].RegionBaseAddress <= FixedPcdGet64 (PcdFlashNvStorageVariableBase64)) &&
        (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) + FixedPcdGet32 (PcdFlashNvStorageVariableSize) <=
         NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
    } else {
      ContainVariableStorage =
        (NorFlashDevices[Index].RegionBaseAddress <= FixedPcdGet32 (PcdFlashNvStorageVariableBase)) &&
        (FixedPcdGet32 (PcdFlashNvStorageVariableBase) + FixedPcdGet32 (PcdFlashNvStorageVariableSize) <=
         NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
    }

    Status = NorFlashCreateInstance (
               PcdGet32 (PcdNorFlashRegBaseAddress),
               NorFlashDevices[Index].DeviceBaseAddress,
               NorFlashDevices[Index].RegionBaseAddress,
               NorFlashDevices[Index].Size,
               Index,
               NorFlashDevices[Index].BlockSize,
               ContainVariableStorage,
               &mNorFlashInstances[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Fail to create instance for NorFlash[%d]\n", Index));
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS  Status;
  UINT32      FvbNumLba;

  ASSERT ((Instance != NULL));

  mFlashNvStorageVariableBase = (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) != 0) ?
                                FixedPcdGet64 (PcdFlashNvStorageVariableBase64) : FixedPcdGet32 (PcdFlashNvStorageVariableBase);
  // Set the index of the first LBA for the FVB
  Instance->StartLba = (mFlashNvStorageVariableBase - Instance->RegionBaseAddress) / Instance->Media.BlockSize;

  // Determine if there is a valid header at the beginning of the NorFlash
  Status = ValidateFvHeader (Instance);

  // Install the Default FVB header if required
  if (EFI_ERROR (Status)) {
    // There is no valid header, so time to install one.
    DEBUG ((DEBUG_INFO, "%a: The FVB Header is not valid.\n", __func__));
    DEBUG ((
      DEBUG_INFO,
      "%a: Installing a correct one for this volume.\n",
      __func__
      ));

    // Erase all the NorFlash that is reserved for variable storage
    FvbNumLba = (PcdGet32 (PcdFlashNvStorageVariableSize) + PcdGet32 (PcdFlashNvStorageFtwWorkingSize) + PcdGet32 (PcdFlashNvStorageFtwSpareSize)) / Instance->Media.BlockSize;

    Status = FvbEraseBlocks (&Instance->FvbProtocol, (EFI_LBA)0, FvbNumLba, EFI_LBA_LIST_TERMINATOR);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Install all appropriate headers
    Status = InitializeFvAndVariableStoreHeaders (Instance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

/**
  Lock all pending read/write to Nor flash device

  @param[in]     OriginalTPL     Nor flash device Original TPL.
**/
VOID
EFIAPI
NorFlashLock (
  IN EFI_TPL  *OriginalTPL
  )
{
}

/**
  Unlock all pending read/write to Nor flash device

  @param[in]     OriginalTPL     Nor flash device Original TPL.
**/
VOID
EFIAPI
NorFlashUnlock (
  IN EFI_TPL  OriginalTPL
  )
{
}
