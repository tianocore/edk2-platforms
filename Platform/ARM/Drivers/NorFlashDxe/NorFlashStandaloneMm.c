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
// Norflash Instance Type.
//
typedef enum {
  InstanceTypeVariable,
  InstanceTypeFwu,
  InstanceTypeUnknown,
} INSTANCE_TYPE;

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
    NorFlashBlockIoReset,            // Reset;
    NorFlashBlockIoReadBlocks,       // ReadBlocks
    NorFlashBlockIoWriteBlocks,      // WriteBlocks
    NorFlashBlockIoFlushBlocks,      // FlushBlocks
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

/**
  Check region is belonged to variable storage.

  @param [in]   NorFlashRegionBase        Start address of one single region
  @param [in]   NorFlashSize              Size of Device.

  @return TRUE                            Variable storage region.
  @return FALSE                           Other region
**/
STATIC
BOOLEAN
EFIAPI
IsVariableStorageRegion (
  IN UINTN                RegionBaseAddress,
  IN UINTN                Size
  )
{
  BOOLEAN ContainVariableStorage;

  if (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) != 0) {
    ContainVariableStorage =
      (RegionBaseAddress <= FixedPcdGet64 (PcdFlashNvStorageVariableBase64)) &&
      (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) + FixedPcdGet32 (PcdFlashNvStorageVariableSize) <=
       RegionBaseAddress + Size);
  } else {
    ContainVariableStorage =
      (RegionBaseAddress <= FixedPcdGet32 (PcdFlashNvStorageVariableBase)) &&
      (FixedPcdGet32 (PcdFlashNvStorageVariableBase) + FixedPcdGet32 (PcdFlashNvStorageVariableSize) <=
       RegionBaseAddress + Size);
  }

  return ContainVariableStorage;
}

/**
  Check region is belonged to firmware update storage.

  @param [in]   NorFlashRegionBase        Start address of one single region.
  @param [in]   NorFlashSize              Size of Device.

  @return TRUE                            Firmware update storage region.
  @return FALSE                           Other region.
**/
STATIC
BOOLEAN
EFIAPI
IsFirmwareUpdateStorageRegion (
  IN UINTN                RegionBaseAddress,
  IN UINTN                Size
  )
{
  BOOLEAN ContainFwuStorage;

  if (FixedPcdGet64 (PcdFlashNvStorageFwuBase64) != 0) {
    ContainFwuStorage =
      (RegionBaseAddress <= FixedPcdGet64 (PcdFlashNvStorageFwuBase64)) &&
      (FixedPcdGet64 (PcdFlashNvStorageFwuBase64) + FixedPcdGet32 (PcdFlashNvStorageFwuSize) <=
       RegionBaseAddress + Size);
  } else {
    ContainFwuStorage =
      (RegionBaseAddress <= FixedPcdGet32 (PcdFlashNvStorageFwuBase)) &&
      (FixedPcdGet32 (PcdFlashNvStorageFwuBase) + FixedPcdGet32 (PcdFlashNvStorageFwuSize) <=
       RegionBaseAddress + Size);
  }

  return ContainFwuStorage;
}

/**
  Free NorFlash device Instance.

  @param [in]   Instance                 NorFlash Instance.

**/
STATIC
VOID
EFIAPI
NorFlashFreeInstance (
  IN NOR_FLASH_INSTANCE *Instance
  )
{
  if (Instance != NULL) {
    if (Instance->ShadowBuffer != NULL) {
      FreePool (Instance->ShadowBuffer);
      Instance->ShadowBuffer = NULL;
    }

    FreePool (Instance);
  }
}


/**
  Allocate NorFlash device Instance.

  @param [in]   NorFlashDeviceBase        Start address of Device Base Address.
  @param [in]   NorFlashRegionBase        Start address of one single region
  @param [in]   NorFlashSize              Size of Device.
  @param [in]   Index                     Index among NorFlash Devices.
  @param [in]   BlockSize                 BlockSize.

  @return Other                           Allocated NorflashDevice Instance.
  @return NULL                            Failed to allocate.
**/
STATIC
NOR_FLASH_INSTANCE *
EFIAPI
NorFlashAllocInstance (
  IN UINTN                HostControllerBase,
  IN UINTN                NorFlashDeviceBase,
  IN UINTN                NorFlashRegionBase,
  IN UINTN                NorFlashSize,
  IN UINT32               Index,
  IN UINT32               BlockSize
  )
{
  NOR_FLASH_INSTANCE *Instance;

  Instance = AllocateRuntimeCopyPool (sizeof (NOR_FLASH_INSTANCE), &mNorFlashInstanceTemplate);
  if (Instance == NULL) {
    return NULL;
  }

  Instance->HostControllerBaseAddress = HostControllerBase;
  Instance->DeviceBaseAddress = NorFlashDeviceBase;
  Instance->RegionBaseAddress = NorFlashRegionBase;
  Instance->Size              = NorFlashSize;

  Instance->BlockIoProtocol.Media = &Instance->Media;

  Instance->Media.MediaId         = Index;
  Instance->Media.BlockSize       = BlockSize;
  Instance->Media.LastBlock       = (NorFlashSize / BlockSize) - 1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)Index;

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);
  if (Instance->ShadowBuffer == NULL) {
    FreePool (Instance);

    return NULL;
  }

  return Instance;
}

/**
  Destory NorFlash device Instance.

  @param [in]   Instance                  NorFlash Instance.

**/
STATIC
VOID
EFIAPI
NorFlashDestoryInstance (
  IN NOR_FLASH_INSTANCE *Instance
  )
{
  if (Instance == NULL) {
    return ;
  }

  if (IsVariableStorageRegion (Instance->RegionBaseAddress, Instance->Size)) {
    gMmst->MmUninstallProtocolInterface (
             Instance->Handle,
             &gEfiSmmFirmwareVolumeBlockProtocolGuid,
             &Instance->FvbProtocol
             );
  } else {
    gMmst->MmUninstallProtocolInterface (
             Instance->Handle,
             &gEfiBlockIoProtocolGuid,
             &Instance->BlockIoProtocol
             );
  }

  NorFlashFreeInstance (Instance);
}

/**
  Create NorFlash device Instance.

  @param [in]   NorFlashDeviceBase        Start address of Device Base Address.
  @param [in]   NorFlashRegionBase        Start address of one single region
  @param [in]   NorFlashSize              Size of Device.
  @param [in]   Index                     Index among NorFlash Devices.
  @param [in]   BlockSize                 BlockSize.
  @param [in]   InstanceType              Instance type.
  @param [out]  NorFlashinstance          NorFlash Instance.

  @return EFI_SUCCESS                     Success.
  @return EFI_OUT_RESOURCES               Out of memory.
  @return Others                          Fail to install BlockIo Protocol.
**/
STATIC
EFI_STATUS
EFIAPI
NorFlashCreateInstanceType (
  IN UINTN                HostControllerBase,
  IN UINTN                NorFlashDeviceBase,
  IN UINTN                NorFlashRegionBase,
  IN UINTN                NorFlashSize,
  IN UINT32               Index,
  IN UINT32               BlockSize,
  IN INSTANCE_TYPE        InstanceType,
  OUT NOR_FLASH_INSTANCE  **NorFlashInstance
  )
{
  EFI_STATUS          Status;
  NOR_FLASH_INSTANCE  *Instance;
  NOR_FLASH_INFO      *FlashInfo;
  UINT8               JedecId[6];
  EFI_GUID            *ProtocolGuid;
  VOID                *Interface;

  ASSERT (NorFlashInstance != NULL);

  if (InstanceType >= InstanceTypeUnknown) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = NorFlashAllocInstance (
               HostControllerBase,
               NorFlashDeviceBase,
               NorFlashRegionBase,
               NorFlashSize,
               Index,
               BlockSize);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = NorFlashReadId (Instance, JedecId);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      goto ErrorHandler;
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

  if (InstanceType == InstanceTypeVariable) {
    /**
     * For variable storage.
     */
    ProtocolGuid = &gEfiSmmFirmwareVolumeBlockProtocolGuid;
    Interface = &Instance->FvbProtocol;
    NorFlashFvbInitialize (Instance);

  } else if (InstanceType == InstanceTypeFwu) {
    /**
     * For firmware update stoarge.
     */
    ProtocolGuid = &gEfiBlockIoProtocolGuid;
    Interface = &Instance->BlockIoProtocol;
  }

  Status = gMmst->MmInstallProtocolInterface (
                    &Instance->Handle,
                    ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    Interface
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to install Norflash protocol(%g)... Status:%r\n",
      ProtocolGuid,
      Status));
    goto ErrorHandler;
  }

  *NorFlashInstance = Instance;

  return EFI_SUCCESS;

ErrorHandler:
  NorFlashFreeInstance (Instance);

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
  UINT32                 Idx;
  INSTANCE_TYPE          InstanceType;

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

  mNorFlashInstances = AllocateZeroPool (sizeof (NOR_FLASH_INSTANCE *) * mNorFlashDeviceCount);
  if (mNorFlashInstances == NULL) {
    DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Failed to allocate nor flash instance\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    InstanceType = InstanceTypeUnknown;

    // Check if this NOR Flash device contain the variable storage region
    if (IsVariableStorageRegion (
          NorFlashDevices[Index].RegionBaseAddress,
          NorFlashDevices[Index].Size)) {
      InstanceType = InstanceTypeVariable;

    // Check if this NOR Flash device contain the firmware update storage region
    } else if (IsFirmwareUpdateStorageRegion (
                NorFlashDevices[Index].RegionBaseAddress,
                NorFlashDevices[Index].Size)) {
      InstanceType = InstanceTypeFwu;
    }

    if (InstanceType == InstanceTypeUnknown) {
      DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Not supported NorFlash[%d]... Skip\n", Index));
      continue;
    }

    Status = NorFlashCreateInstanceType (
               PcdGet32 (PcdNorFlashRegBaseAddress),
               NorFlashDevices[Index].DeviceBaseAddress,
               NorFlashDevices[Index].RegionBaseAddress,
               NorFlashDevices[Index].Size,
               Index,
               NorFlashDevices[Index].BlockSize,
               InstanceType,
               &mNorFlashInstances[Index]
               );
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }
  }

  return EFI_SUCCESS;

ErrorHandler:
  for (Idx = 0; Idx < Index; Idx++) {
    NorFlashDestoryInstance (mNorFlashInstances[Idx]);
    mNorFlashInstances[Idx] = NULL;
  }

  FreePool (mNorFlashInstances);

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
