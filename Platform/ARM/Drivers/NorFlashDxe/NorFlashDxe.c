/** @file  NorFlashDxe.c

  Copyright (c) 2011 - 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NorFlashInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>

#include "NorFlashCommon.h"

STATIC EFI_EVENT  mNorFlashVirtualAddrChangeEvent;

//
// Global variable declarations
//
NOR_FLASH_INSTANCE  **mNorFlashInstances;
UINT32              mNorFlashDeviceCount;
UINTN               mFlashNvStorageVariableBase;
EFI_EVENT           mFvbVirtualAddrChangeEvent;

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
    NorFlashBlockIoReset,            // Reset;
    NorFlashBlockIoReadBlocks,       // ReadBlocks
    NorFlashBlockIoWriteBlocks,      // WriteBlocks
    NorFlashBlockIoFlushBlocks       // FlushBlocks
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
    NorFlashDiskIoReadDisk,        // ReadDisk
    NorFlashDiskIoWriteDisk        // WriteDisk
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

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Instance->Handle,
                    &gEfiDevicePathProtocolGuid,
                    &Instance->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Instance->BlockIoProtocol,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    &Instance->FvbProtocol,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto error_handler2;
    }
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Instance->Handle,
                    &gEfiDevicePathProtocolGuid,
                    &Instance->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Instance->BlockIoProtocol,
                    &gEfiDiskIoProtocolGuid,
                    &Instance->DiskIoProtocol,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto error_handler2;
    }
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  UINT32                 Index;
  NOR_FLASH_DESCRIPTION  *NorFlashDevices;
  BOOLEAN                ContainVariableStorage;
  EFI_PHYSICAL_ADDRESS   HostControllerBaseAddress;

  // Host controller base address region if available
  HostControllerBaseAddress = PcdGet32 (PcdNorFlashRegBaseAddress);

  if (HostControllerBaseAddress != 0) {
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    HostControllerBaseAddress,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gDS->SetMemorySpaceAttributes (
                    HostControllerBaseAddress,
                    SIZE_4KB,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR (Status);
  }

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

  mNorFlashInstances = AllocateRuntimePool (sizeof (NOR_FLASH_INSTANCE *) * mNorFlashDeviceCount);

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    // Check if this NOR Flash device contain the variable storage region

    if (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0) {
      ContainVariableStorage =
        (NorFlashDevices[Index].RegionBaseAddress <= PcdGet64 (PcdFlashNvStorageVariableBase64)) &&
        (PcdGet64 (PcdFlashNvStorageVariableBase64) + PcdGet32 (PcdFlashNvStorageVariableSize) <=
         NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
    } else {
      ContainVariableStorage =
        (NorFlashDevices[Index].RegionBaseAddress <= PcdGet32 (PcdFlashNvStorageVariableBase)) &&
        (PcdGet32 (PcdFlashNvStorageVariableBase) + PcdGet32 (PcdFlashNvStorageVariableSize) <=
         NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
    }

    Status = NorFlashCreateInstance (
               HostControllerBaseAddress,
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

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NorFlashVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mNorFlashVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS     Status;
  UINT32         FvbNumLba;
  EFI_BOOT_MODE  BootMode;
  UINTN          RuntimeMmioRegionSize;

  DEBUG ((DEBUG_BLKIO, "NorFlashFvbInitialize\n"));
  ASSERT ((Instance != NULL));

  //
  // Declare the Non-Volatile storage as EFI_MEMORY_RUNTIME
  //

  // Note: all the NOR Flash region needs to be reserved into the UEFI Runtime memory;
  //       even if we only use the small block region at the top of the NOR Flash.
  //       The reason is when the NOR Flash memory is set into program mode, the command
  //       is written as the base of the flash region (ie: Instance->DeviceBaseAddress)
  RuntimeMmioRegionSize = (Instance->RegionBaseAddress - Instance->DeviceBaseAddress) + Instance->Size;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Instance->DeviceBaseAddress,
                  RuntimeMmioRegionSize,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gDS->SetMemorySpaceAttributes (
                  Instance->DeviceBaseAddress,
                  RuntimeMmioRegionSize,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  mFlashNvStorageVariableBase = (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0) ?
                                PcdGet64 (PcdFlashNvStorageVariableBase64) : PcdGet32 (PcdFlashNvStorageVariableBase);

  // Set the index of the first LBA for the FVB
  Instance->StartLba = (mFlashNvStorageVariableBase - Instance->RegionBaseAddress) / Instance->Media.BlockSize;

  BootMode = GetBootModeHob ();
  if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    // Determine if there is a valid header at the beginning of the NorFlash
    Status = ValidateFvHeader (Instance);
  }

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

  //
  // The driver implementing the variable read service can now be dispatched;
  // the varstore headers are in place.
  //
  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEdkiiNvVarStoreFormattedGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mFvbVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
NorFlashVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->HostControllerBaseAddress);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->DeviceBaseAddress);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->RegionBaseAddress);

    // Convert BlockIo protocol
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.FlushBlocks);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.ReadBlocks);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.Reset);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->BlockIoProtocol.WriteBlocks);

    // Convert Fvb
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.EraseBlocks);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.GetAttributes);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.GetBlockSize);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.GetPhysicalAddress);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.Read);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.SetAttributes);
    EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->FvbProtocol.Write);

    if (mNorFlashInstances[Index]->ShadowBuffer != NULL) {
      EfiConvertPointer (0x0, (VOID **)&mNorFlashInstances[Index]->ShadowBuffer);
    }
  }

  return;
}

/**
  Lock all pending read/write to Nor flash device

  @param[in]     *OriginalTPL     Pointer to Nor flash device Original TPL.
**/
VOID
EFIAPI
NorFlashLock (
  IN EFI_TPL  *OriginalTPL
  )
{
  if (!EfiAtRuntime ()) {
    // Raise TPL to TPL_HIGH to stop anyone from interrupting us.
    *OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  } else {
    // This initialization is only to prevent the compiler to complain about the
    // use of uninitialized variables
    *OriginalTPL = TPL_HIGH_LEVEL;
  }
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
  if (!EfiAtRuntime ()) {
    // Interruptions can resume.
    gBS->RestoreTPL (OriginalTPL);
  }
}
