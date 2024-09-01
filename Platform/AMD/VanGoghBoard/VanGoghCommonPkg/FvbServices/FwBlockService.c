/** @file
  Implements FvbServicesSmm

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013-2016 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifdef _MSC_VER
  #pragma optimize( "", off )
#endif

#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
  #else
    #pragma clang optimize off
  #endif
#endif

#include "FwBlockService.h"

#define EFI_FVB2_STATUS  (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)

ESAL_FWB_GLOBAL  *mFvbModuleGlobal;

FV_MEMMAP_DEVICE_PATH  mFvMemmapDevicePathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      {
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8)
      }
    },
    EfiMemoryMappedIO,
    (EFI_PHYSICAL_ADDRESS)0,
    (EFI_PHYSICAL_ADDRESS)0,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

FV_PIWG_DEVICE_PATH  mFvPIWGDevicePathTemplate = {
  {
    {
      MEDIA_DEVICE_PATH,
      MEDIA_PIWG_FW_VOL_DP,
      {
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH)),
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH) >> 8)
      }
    },
    { 0 }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

EFI_FW_VOL_BLOCK_DEVICE  mFvbDeviceTemplate = {
  FVB_DEVICE_SIGNATURE,
  NULL,
  0,
  {
    FvbProtocolGetAttributes,
    FvbProtocolSetAttributes,
    FvbProtocolGetPhysicalAddress,
    FvbProtocolGetBlockSize,
    FvbProtocolRead,
    FvbProtocolWrite,
    FvbProtocolEraseBlocks,
    NULL
  }
};

/**
  Retrieves the physical address of a memory mapped FV

  @param[in]  Instance    The FV instance whose base address is going to be returned.

  @param[in]  Global      Pointer to ESAL_FWB_GLOBAL that contains all instance data.

  @param[out] FwhInstance The EFI_FW_VOL_INSTANCE fimrware instance structure
  @param[in]  Virtual     Whether CPU is in virtual or physical mode

  @retval  EFI_SUCCESS            Successfully returns
  @retval  EFI_INVALID_PARAMETER  Instance not found           -

**/
EFI_STATUS
GetFvbInstance (
  IN  UINTN                Instance,
  IN  ESAL_FWB_GLOBAL      *Global,
  OUT EFI_FW_VOL_INSTANCE  **FwhInstance,
  IN BOOLEAN               Virtual
  )
{
  EFI_FW_VOL_INSTANCE  *FwhRecord;

  if (Instance >= Global->NumFv) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the right instance of the FVB private data
  //
  FwhRecord = Global->FvInstance[Virtual];
  while (Instance > 0) {
    FwhRecord = (EFI_FW_VOL_INSTANCE *)
                (
                 (UINTN)((UINT8 *)FwhRecord) + FwhRecord->VolumeHeader.HeaderLength +
                 (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
                );
    Instance--;
  }

  *FwhInstance = FwhRecord;

  return EFI_SUCCESS;
}

/**
  Retrieves the physical address of a memory mapped FV

  @param[in]  Instance  The FV instance whose base address is going to be returned.

  @param[out]  Address  Pointer to a caller allocated EFI_PHYSICAL_ADDRESS
                        that on successful return, contains the base address
                        of the firmware volume.
  @param[in]  Global    Pointer to ESAL_FWB_GLOBAL that contains all
                        instance data
  @param[in]  Virtual   Whether CPU is in virtual or physical mode

  @retval  EFI_SUCCESS            Successfully returns
  @retval  EFI_INVALID_PARAMETER  Instance not found            -
   -

**/
EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                  Instance,
  OUT EFI_PHYSICAL_ADDRESS  *Address,
  IN ESAL_FWB_GLOBAL        *Global,
  IN BOOLEAN                Virtual
  )
{
  EFI_FW_VOL_INSTANCE  *FwhInstance;
  EFI_STATUS           Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);
  *Address = FwhInstance->FvBase[Virtual];

  return EFI_SUCCESS;
}

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

  @param[in]  Instance   The FV instance whose attributes is going to be
                         returned
  @param[out] Attributes Output buffer which contains attributes
  @param[in]  Global     Pointer to ESAL_FWB_GLOBAL that contains all
                         instance data
  @param[in]  Virtual    Whether CPU is in virtual or physical mode

  @retval    EFI_SUCCESS            Successfully returns
  @retval    EFI_INVALID_PARAMETER  Instance not found

**/
EFI_STATUS
FvbGetVolumeAttributes (
  IN UINTN                  Instance,
  OUT EFI_FVB_ATTRIBUTES_2  *Attributes,
  IN ESAL_FWB_GLOBAL        *Global,
  IN BOOLEAN                Virtual
  )
{
  EFI_FW_VOL_INSTANCE  *FwhInstance;
  EFI_STATUS           Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);
  *Attributes = FwhInstance->VolumeHeader.Attributes;

  return EFI_SUCCESS;
}

/**
  Retrieves the starting address of an LBA in an FV

  @param[in]   Instance               The FV instance which the Lba belongs to
  @param[in]   Lba                    The logical block address
  @param[out]  LbaAddress             On output, contains the physical starting address
                          of the Lba
  @param[out]  LbaLength              On output, contains the length of the block
  @param[out]  NumOfBlocks            A pointer to a caller allocated UINTN in which the
                                      number of consecutive blocks starting with Lba is
                                      returned. All blocks in this range have a size of
                                      BlockSize
  @param[in]  Global                  Pointer to ESAL_FWB_GLOBAL that contains all
                                      instance data
  @param[in]  Virtual                 Whether CPU is in virtual or physical mode
  @retval  EFI_SUCCESS             Successfully returns
  @retval  EFI_INVALID_PARAMETER   Instance not found

**/
EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN            Instance,
  IN  EFI_LBA          Lba,
  OUT UINTN            *LbaAddress,
  OUT UINTN            *LbaLength,
  OUT UINTN            *NumOfBlocks,
  IN  ESAL_FWB_GLOBAL  *Global,
  IN  BOOLEAN          Virtual
  )
{
  UINT32                  NumBlocks;
  UINT32                  BlockLength;
  UINTN                   Offset;
  EFI_LBA                 StartLba;
  EFI_LBA                 NextLba;
  EFI_FW_VOL_INSTANCE     *FwhInstance;
  EFI_FV_BLOCK_MAP_ENTRY  *BlockMap;
  EFI_STATUS              Status;

  FwhInstance = NULL;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  StartLba = 0;
  Offset   = 0;
  BlockMap = &(FwhInstance->VolumeHeader.BlockMap[0]);

  //
  // Parse the blockmap of the FV to find which map entry the Lba belongs to
  //
  while (TRUE) {
    NumBlocks   = BlockMap->NumBlocks;
    BlockLength = BlockMap->Length;

    if ((NumBlocks == 0) || (BlockLength == 0)) {
      return EFI_INVALID_PARAMETER;
    }

    NextLba = StartLba + NumBlocks;

    //
    // The map entry found
    //
    if ((Lba >= StartLba) && (Lba < NextLba)) {
      Offset = Offset + (UINTN)MultU64x32 ((Lba - StartLba), BlockLength);
      if (LbaAddress) {
        *LbaAddress = FwhInstance->FvBase[Virtual] + Offset;
      }

      if (LbaLength) {
        *LbaLength = BlockLength;
      }

      if (NumOfBlocks) {
        *NumOfBlocks = (UINTN)(NextLba - Lba);
      }

      return EFI_SUCCESS;
    }

    StartLba = NextLba;
    Offset   = Offset + NumBlocks * BlockLength;
    BlockMap++;
  }
}

/**
  Reads specified number of bytes into a buffer from the specified block

  @param[in]       Instance      The FV instance to be read from
  @param[in]       Lba           The logical block address to be read from
  @param[in]       BlockOffset   Offset into the block at which to begin reading
  @param[in, out]  NumBytes      Pointer that on input contains the total size of
                                 the buffer. On output, it contains the total number
                                 of bytes read
  @param[in]       Buffer        Pointer to a caller allocated buffer that will be
                                 used to hold the data read
  @param[in]       Global        Pointer to ESAL_FWB_GLOBAL that contains all
                                 instance data
  @param[in]       Virtual       Whether CPU is in virtual or physical mode

  @retval   EFI_SUCCESS            The firmware volume was read successfully and
                                   contents are in Buffer
  @retval   EFI_BAD_BUFFER_SIZE    Read attempted across a LBA boundary. On output,
                                   NumBytes contains the total number of bytes returned
                                   in Buffer
  @retval   EFI_ACCESS_DENIED      The firmware volume is in the ReadDisabled state
  @retval   EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                   could not be read
  @retval   EFI_INVALID_PARAMETER  Instance not found, or NumBytes, Buffer are NULL

**/
EFI_STATUS
FvbReadBlock (
  IN UINTN            Instance,
  IN EFI_LBA          Lba,
  IN UINTN            BlockOffset,
  IN OUT UINTN        *NumBytes,
  IN UINT8            *Buffer,
  IN ESAL_FWB_GLOBAL  *Global,
  IN BOOLEAN          Virtual
  )

{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  DEBUG ((DEBUG_INFO, "Smm %a() enter\n", __func__));

  //
  // Check for invalid conditions
  //
  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if the FV is read enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32)(LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  // DEBUG ((DEBUG_INFO, "ReadAddress: 0x%x, NumBytes: 0x%x\n", (UINT8 *) (LbaAddress + BlockOffset), *NumBytes));
  CopyMem (Buffer, (UINT8 *)(LbaAddress + BlockOffset), (UINTN)(*NumBytes));

  return Status;
}

/**
  Writes specified number of bytes from the input buffer to the block

  @param[in]       Instance      The FV instance to be written to
  @param[in]       Lba           The starting logical block index to write to
  @param[in]       BlockOffset   Offset into the block at which to begin writing
  @param[in, out]  NumBytes      Pointer that on input contains the total size of
                                 the buffer. On output, it contains the total number
                                 of bytes actually written
  @param[in]       Buffer        Pointer to a caller allocated buffer that contains
                                 the source for the write
  @param[in]       Global        Pointer to ESAL_FWB_GLOBAL that contains all
                                 instance data
  @param[in]       Virtual       Whether CPU is in virtual or physical mode

  @retval  EFI_SUCCESS             The firmware volume was written successfully
  @retval  EFI_BAD_BUFFER_SIZE     Write attempted across a LBA boundary. On output,
                                   NumBytes contains the total number of bytes
                                   actually written
  @retval  EFI_ACCESS_DENIED       The firmware volume is in the WriteDisabled state
  @retval  EFI_DEVICE_ERROR        The block device is not functioning correctly and
                                   could not be written
  @retval  EFI_INVALID_PARAMETER   Instance not found, or NumBytes, Buffer are NULL

**/
EFI_STATUS
FvbWriteBlock (
  IN UINTN            Instance,
  IN EFI_LBA          Lba,
  IN UINTN            BlockOffset,
  IN OUT UINTN        *NumBytes,
  IN UINT8            *Buffer,
  IN ESAL_FWB_GLOBAL  *Global,
  IN BOOLEAN          Virtual
  )
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  //
  // Check for invalid conditions
  //
  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32)(LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  //
  // Write data
  //
  Status = FlashFdWrite (
             LbaAddress + BlockOffset,
             LbaAddress,
             NumBytes,
             Buffer,
             LbaLength
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**
  Erases and initializes a firmware volume block

  @param[in]  Instance    The FV instance to be erased
  @param[in]  Lba         The logical block index to be erased
  @param[in]  Global      Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  @param[in]  Virtual     Whether CPU is in virtual or physical mode

  @retval  EFI_SUCCESS             The erase request was successfully completed
  @retval  EFI_ACCESS_DENIED       The firmware volume is in the WriteDisabled state
  @retval  EFI_DEVICE_ERROR        The block device is not functioning correctly and
                                   could not be written. Firmware device may have been
                                   partially erased
  @retval  EFI_INVALID_PARAMETER   Instance not found

**/
EFI_STATUS
FvbEraseBlock (
  IN UINTN            Instance,
  IN EFI_LBA          Lba,
  IN ESAL_FWB_GLOBAL  *Global,
  IN BOOLEAN          Virtual
  )
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Get the starting address of the block for erase.
  //
  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FlashFdErase (LbaAddress, LbaLength);

  return Status;
}

/**
  Modifies the current settings of the firmware volume according to the
  input parameter, and returns the new setting of the volume

  @param[in]       Instance          The FV instance whose attributes is going to be
                                     modified
  @param[in, out]  Attributes        On input, it is a pointer to EFI_FVB_ATTRIBUTES_2
                                     containing the desired firmware volume settings.
                                     On successful return, it contains the new settings
                                     of the firmware volume
  @param[in]       Global            Pointer to ESAL_FWB_GLOBAL that contains all
                                     instance data
  @param[in]       Virtual           Whether CPU is in virtual or physical mode

  @retval  EFI_SUCCESS             Successfully returns
  @retval  EFI_ACCESS_DENIED       The volume setting is locked and cannot be modified
  @retval  EFI_INVALID_PARAMETER   Instance not found, or The attributes requested are
                                   in conflict with the capabilities as declared in the
                                   firmware volume header

**/
EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                     Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2  *Attributes,
  IN ESAL_FWB_GLOBAL           *Global,
  IN BOOLEAN                   Virtual
  )
{
  EFI_FW_VOL_INSTANCE   *FwhInstance;
  EFI_FVB_ATTRIBUTES_2  OldAttributes;
  EFI_FVB_ATTRIBUTES_2  *AttribPtr;
  UINT32                Capabilities;
  UINT32                OldStatus;
  UINT32                NewStatus;
  EFI_STATUS            Status;
  EFI_FVB_ATTRIBUTES_2  UnchangedAttributes;

  FwhInstance = NULL;
  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  AttribPtr     = (EFI_FVB_ATTRIBUTES_2 *)&(FwhInstance->VolumeHeader.Attributes);
  OldAttributes = *AttribPtr;
  Capabilities  = OldAttributes & (EFI_FVB2_READ_DISABLED_CAP | \
                                   EFI_FVB2_READ_ENABLED_CAP | \
                                   EFI_FVB2_WRITE_DISABLED_CAP | \
                                   EFI_FVB2_WRITE_ENABLED_CAP | \
                                   EFI_FVB2_LOCK_CAP \
                                   );
  OldStatus = OldAttributes & EFI_FVB2_STATUS;
  NewStatus = *Attributes & EFI_FVB2_STATUS;

  UnchangedAttributes = EFI_FVB2_READ_DISABLED_CAP  | \
                        EFI_FVB2_READ_ENABLED_CAP   | \
                        EFI_FVB2_WRITE_DISABLED_CAP | \
                        EFI_FVB2_WRITE_ENABLED_CAP  | \
                        EFI_FVB2_LOCK_CAP           | \
                        EFI_FVB2_STICKY_WRITE       | \
                        EFI_FVB2_MEMORY_MAPPED      | \
                        EFI_FVB2_ERASE_POLARITY     | \
                        EFI_FVB2_READ_LOCK_CAP      | \
                        EFI_FVB2_WRITE_LOCK_CAP     | \
                        EFI_FVB2_ALIGNMENT;

  //
  // Some attributes of FV is read only can *not* be set
  //
  if ((OldAttributes & UnchangedAttributes) ^ (*Attributes & UnchangedAttributes)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If firmware volume is locked, no status bit can be updated
  //
  if (OldAttributes & EFI_FVB2_LOCK_STATUS) {
    if (OldStatus ^ NewStatus) {
      return EFI_ACCESS_DENIED;
    }
  }

  //
  // Test read disable
  //
  if ((Capabilities & EFI_FVB2_READ_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test read enable
  //
  if ((Capabilities & EFI_FVB2_READ_ENABLED_CAP) == 0) {
    if (NewStatus & EFI_FVB2_READ_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test write disable
  //
  if ((Capabilities & EFI_FVB2_WRITE_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test write enable
  //
  if ((Capabilities & EFI_FVB2_WRITE_ENABLED_CAP) == 0) {
    if (NewStatus & EFI_FVB2_WRITE_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Test lock
  //
  if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
    if (NewStatus & EFI_FVB2_LOCK_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  *AttribPtr  = (*AttribPtr) & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
  *AttribPtr  = (*AttribPtr) | NewStatus;
  *Attributes = *AttribPtr;

  return EFI_SUCCESS;
}

/**
  Retrieves the physical address of the device.

  @param[in]   This        Calling context
  @param[out]  Address     Output buffer containing the address.

  @retval EFI_SUCCESS  Successfully returns

**/
EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_PHYSICAL_ADDRESS                     *Address
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  return FvbGetPhysicalAddress (FvbDevice->Instance, Address, mFvbModuleGlobal, FALSE); // Hard coded to FALSE for SMM driver.
}

/**
  Retrieve the size of a logical block

  @param[in]   This           alling context
  @param[in]   Lba            Indicates which block to return the size for.
  @param[out]  BlockSize      A pointer to a caller allocated UINTN in which
                              the size of the block is returned
  @param[out]  NumOfBlocks    a pointer to a caller allocated UINTN in which the
                              number of consecutive blocks starting with Lba is
                              returned. All blocks in this range have a size of
                              BlockSize

  @retval  EFI_SUCCESS        The firmware volume was read successfully and
                              contents are in Buffer

**/
EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN CONST EFI_LBA                             Lba,
  OUT UINTN                                    *BlockSize,
  OUT UINTN                                    *NumOfBlocks
  )

{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetLbaAddress (
           FvbDevice->Instance,
           Lba,
           NULL,
           BlockSize,
           NumOfBlocks,
           mFvbModuleGlobal,
                 // EfiGoneVirtual ()
           FALSE // Hard coded to FALSE for SMM driver.
           );
}

/**
    Retrieves Volume attributes.  No polarity translations are done.

  @param[in]     This                 Calling context
  @param[out]    Attributes           output buffer which contains attributes

  @retval  EFI_SUCCESS            Successfully returns

**/
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_FVB_ATTRIBUTES_2                     *Attributes
  )

{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  return FvbGetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, FALSE); // Hard coded to FALSE for SMM driver.
}

/**
  Sets Volume attributes. No polarity translations are done.

  @param[in]   This                   Calling context
  @param[out]  Attributes             output buffer which contains attributes

  @retval  EFI_SUCCESS                Successfully returns

**/
EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                  *Attributes
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  // return FvbSetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, EfiGoneVirtual ());
  return FvbSetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, FALSE); // Hard coded to FALSE for SMM driver.
}

/**
  The EraseBlock() function erases one or more blocks as denoted by the
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist
  within the associated firmware volume (it has a larger index than the last
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param[in]  This                    Calling context
  @param[in]  ...                     Starting LBA followed by Number of Lba to erase.
                                      a -1 to terminate the list.

  @retval  EFI_SUCCESS             The erase request was successfully completed
  @retval  EFI_ACCESS_DENIED       The firmware volume is in the WriteDisabled state
  @retval  EFI_DEVICE_ERROR        The block device is not functioning correctly and
                                   could not be written. Firmware device may have been
                                   partially erased.

**/
EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  ...
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  EFI_FW_VOL_INSTANCE      *FwhInstance;
  UINTN                    NumOfBlocks;
  VA_LIST                  args;
  EFI_LBA                  StartingLba;
  UINTN                    NumOfLba;
  EFI_STATUS               Status;

  FwhInstance = NULL;
  FvbDevice   = FVB_DEVICE_FROM_THIS (This);

  // Status    = GetFvbInstance (FvbDevice->Instance, mFvbModuleGlobal, &FwhInstance, EfiGoneVirtual ());
  Status = GetFvbInstance (FvbDevice->Instance, mFvbModuleGlobal, &FwhInstance, FALSE);    // Hard coded to FALSE for SMM driver.
  ASSERT_EFI_ERROR (Status);

  NumOfBlocks = FwhInstance->NumOfBlocks;

  VA_START (args, This);

  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINTN);

    //
    // Check input parameters
    //
    if ((NumOfLba == 0) || ((StartingLba + NumOfLba) > NumOfBlocks)) {
      VA_END (args);
      return EFI_INVALID_PARAMETER;
    }
  } while (TRUE);

  VA_END (args);

  VA_START (args, This);
  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINTN);

    while (NumOfLba > 0) {
      // Status = FvbEraseBlock (FvbDevice->Instance, StartingLba, mFvbModuleGlobal, EfiGoneVirtual ());
      Status = FvbEraseBlock (FvbDevice->Instance, StartingLba, mFvbModuleGlobal, FALSE); // Hard coded to FALSE for SMM driver.
      if (EFI_ERROR (Status)) {
        VA_END (args);
        return Status;
      }

      StartingLba++;
      NumOfLba--;
    }
  } while (TRUE);

  VA_END (args);

  return EFI_SUCCESS;
}

/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]       This                   Calling context
  @param[in]       Lba                    Block in which to begin write
  @param[in]       Offset                 Offset in the block at which to begin write
  @param[in, out]  NumBytes               On input, indicates the requested write size. On
                                          output, indicates the actual number of bytes written
  @param[in]       Buffer                 Buffer containing source data for the write.

  @retval  EFI_SUCCESS            The firmware volume was written successfully
  @retval  EFI_BAD_BUFFER_SIZE    Write attempted across a LBA boundary. On output,
                                  NumBytes contains the total number of bytes
                                  actually written
  @retval  EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled state
  @retval  EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                  could not be written
  @retval  EFI_INVALID_PARAMETER  NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN       EFI_LBA                             Lba,
  IN       UINTN                               Offset,
  IN OUT   UINTN                               *NumBytes,
  IN       UINT8                               *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbWriteBlock (FvbDevice->Instance, (EFI_LBA)Lba, (UINTN)Offset, NumBytes, (UINT8 *)Buffer, mFvbModuleGlobal, FALSE); // Hard coded to FALSE for SMM driver.
}

/**
  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]   This       Calling context
  @param[in]   Lba        Block in which to begin Read
  @param[in]   Offset     Offset in the block at which to begin Read
  @param[out]  NumBytes   On input, indicates the requested write size. On
                          output, indicates the actual number of bytes Read
  @param[in]  Buffer      Buffer containing source data for the Read.

  @retval  EFI_SUCCESS             The firmware volume was read successfully and
                                   contents are in Buffer
  @retval  EFI_BAD_BUFFER_SIZE     Read attempted across a LBA boundary. On output,
                                   NumBytes contains the total number of bytes returned
                                   in Buffer
  @retval  EFI_ACCESS_DENIED       The firmware volume is in the ReadDisabled state
  @retval  EFI_DEVICE_ERROR        The block device is not functioning correctly and
                                   could not be read
  @retval  EFI_INVALID_PARAMETER   NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN CONST EFI_LBA                             Lba,
  IN CONST UINTN                               Offset,
  IN OUT UINTN                                 *NumBytes,
  IN UINT8                                     *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  // return FvbReadBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer, mFvbModuleGlobal, EfiGoneVirtual ());
  return FvbReadBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer, mFvbModuleGlobal, FALSE); // Hard coded to FALSE for SMM driver.
}

/**
  Check the integrity of firmware volume header

  @param[in]  FwVolHeader            A pointer to a firmware volume header

  @retval  EFI_SUCCESS            The firmware volume is consistent
  @retval  EFI_NOT_FOUND          The firmware volume has corrupted. So it is not an FV

**/
EFI_STATUS
ValidateFvHeader (
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  )
{
  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if ((FwVolHeader->Revision != EFI_FVH_REVISION) ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINTN)-1)) ||
      ((FwVolHeader->HeaderLength & 0x01) != 0)
      )
  {
    return EFI_NOT_FOUND;
  }

  //
  // Verify the header checksum
  //
  if (CalculateCheckSum16 ((UINT16 *)FwVolHeader, FwVolHeader->HeaderLength) != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**

  This function does common initialization for FVB services

  @param[in]  ImageHandle            A pointer to a image volume header
  @param[in]  SystemTable            Pointer to the System Table

  @retval  EFI_SUCCESS               initialization for FVB services successfully

**/
EFI_STATUS
EFIAPI
FvbInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_FW_VOL_INSTANCE              *FwhInstance;
  EFI_FIRMWARE_VOLUME_HEADER       *FwVolHeader;
  EFI_DXE_SERVICES                 *DxeServices;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;
  UINT32                           BufferSize;
  EFI_FV_BLOCK_MAP_ENTRY           *PtrBlockMapEntry;
  EFI_HANDLE                       FwbHandle;
  EFI_FW_VOL_BLOCK_DEVICE          *FvbDevice;
  UINT32                           MaxLbaSize;
  EFI_PHYSICAL_ADDRESS             BaseAddress;
  UINTN                            NumOfBlocks;
  UINT32                           PlatformFvBaseAddress;

  //
  // Get the DXE services table
  //
  DxeServices = gDS;

  //
  // Allocate runtime services data for global variable, which contains
  // the private data of all firmware volume block instances
  //
  mFvbModuleGlobal = AllocateRuntimePool (sizeof (ESAL_FWB_GLOBAL));
  ASSERT (mFvbModuleGlobal);

  Status = gSmst->SmmLocateProtocol (&gEfiSmmSpiProtocolGuid, NULL, (VOID **)&mFvbModuleGlobal->SpiProtocol);
  ASSERT_EFI_ERROR (Status);

  //
  // Calculate the total size for all firmware volume block instances
  //
  BufferSize            = 0;
  PlatformFvBaseAddress = PcdGet32 (PcdFlashNvStorageVariableBase);
  FwVolHeader           = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PlatformFvBaseAddress;
  BufferSize           += (FwVolHeader->HeaderLength +
                           sizeof (EFI_FW_VOL_INSTANCE) -
                           sizeof (EFI_FIRMWARE_VOLUME_HEADER)
                           );

  //
  // Only need to allocate once. There is only one copy of physical memory for
  // the private data of each FV instance. But in virtual mode or in physical
  // mode, the address of the the physical memory may be different.
  //
  mFvbModuleGlobal->FvInstance[FVB_PHYSICAL] = AllocateRuntimePool (BufferSize);
  ASSERT (mFvbModuleGlobal->FvInstance[FVB_PHYSICAL] != NULL);

  //
  // Make a virtual copy of the FvInstance pointer.
  //
  FwhInstance                               = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  mFvbModuleGlobal->FvInstance[FVB_VIRTUAL] = FwhInstance;

  mFvbModuleGlobal->NumFv = 0;
  MaxLbaSize              = 0;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PlatformFvBaseAddress;
  BaseAddress = (UINTN)FwVolHeader;

  //
  // Check if it is a "real" flash
  //
  Status = DxeServices->GetMemorySpaceDescriptor (
                          BaseAddress,
                          &Descriptor
                          );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)BaseAddress;
  Status      = ValidateFvHeader (FwVolHeader);
  if (EFI_ERROR (Status)) {
    //
    // Get FvbInfo to provide in FwhInstance.
    //
    Status = GetFvbInfo (BaseAddress, &FwVolHeader);
    //
    //  Write healthy FV header back.
    //
    CopyMem (
      (VOID *)(UINTN)BaseAddress,
      (VOID *)FwVolHeader,
      FwVolHeader->HeaderLength
      );
  }

  FwhInstance->FvBase[FVB_PHYSICAL] = (UINTN)BaseAddress;
  FwhInstance->FvBase[FVB_VIRTUAL]  = (UINTN)BaseAddress;

  CopyMem ((UINTN *)&(FwhInstance->VolumeHeader), (UINTN *)FwVolHeader, FwVolHeader->HeaderLength);
  FwVolHeader = &(FwhInstance->VolumeHeader);
  EfiInitializeLock (&(FwhInstance->FvbDevLock), TPL_HIGH_LEVEL);

  NumOfBlocks = 0;

  for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    //
    // Get the maximum size of a block.
    //
    if (MaxLbaSize < PtrBlockMapEntry->Length) {
      MaxLbaSize = PtrBlockMapEntry->Length;
    }

    NumOfBlocks = NumOfBlocks + PtrBlockMapEntry->NumBlocks;
  }

  //
  // The total number of blocks in the FV.
  //
  FwhInstance->NumOfBlocks = NumOfBlocks;

  //
  // Add a FVB Protocol Instance
  //
  FvbDevice = AllocateRuntimePool (sizeof (EFI_FW_VOL_BLOCK_DEVICE));
  ASSERT (FvbDevice != NULL);

  CopyMem (FvbDevice, &mFvbDeviceTemplate, sizeof (EFI_FW_VOL_BLOCK_DEVICE));

  FvbDevice->Instance = mFvbModuleGlobal->NumFv;
  mFvbModuleGlobal->NumFv++;

  //
  // Set up the devicepath
  //
  if (FwVolHeader->ExtHeaderOffset == 0) {
    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH
    //
    FvbDevice->DevicePath                                                           = (EFI_DEVICE_PATH_PROTOCOL *)AllocateCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
    ((FV_MEMMAP_DEVICE_PATH *)FvbDevice->DevicePath)->MemMapDevPath.StartingAddress = BaseAddress;
    ((FV_MEMMAP_DEVICE_PATH *)FvbDevice->DevicePath)->MemMapDevPath.EndingAddress   = BaseAddress + FwVolHeader->FvLength - 1;
  } else {
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)AllocateCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
    CopyGuid (
      &((FV_PIWG_DEVICE_PATH *)FvbDevice->DevicePath)->FvDevPath.FvName,
      (GUID *)(UINTN)(BaseAddress + FwVolHeader->ExtHeaderOffset)
      );
  }

  FwbHandle = NULL;
  Status    = gSmst->SmmInstallProtocolInterface (
                       &FwbHandle,
                       &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                       EFI_NATIVE_INTERFACE,
                       &FvbDevice->FwVolBlockInstance
                       );
  ASSERT_EFI_ERROR (Status);
  FwhInstance = (EFI_FW_VOL_INSTANCE *)
                (
                 (UINTN)((UINT8 *)FwhInstance) + FwVolHeader->HeaderLength +
                 (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
                );

  return EFI_SUCCESS;
}

/**
  Writes specified number of bytes from the input buffer to the address

  @param[in]       WriteAddress      The FV address to be written to
  @param[in]       Address           The FV address to be written to
  @param[in, out]  NumBytes          Pointer that on input contains the total size of
                                     the buffer. On output, it contains the total number
                                     of bytes actually written
  @param[in]       Buffer            Pointer to a caller allocated buffer that contains
                                     the source for the write
  @param[in]       LbaLength         contains the length of the Buffer.

  @return The status returned from SpiProtocol().

**/
EFI_STATUS
FlashFdWrite (
  IN  UINTN     WriteAddress,
  IN  UINTN     Address,
  IN OUT UINTN  *NumBytes,
  IN  UINT8     *Buffer,
  IN  UINTN     LbaLength
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // TODO:  Suggested that this code be "critical section"
  //
  WriteAddress -= (PcdGet32 (PcdFlashAreaBaseAddress));
  Status        = mFvbModuleGlobal->SpiProtocol->Execute (
                                                   mFvbModuleGlobal->SpiProtocol,
                                                   SPI_OPCODE_WRITE_INDEX, // OpcodeIndex
                                                   0,                      // PrefixOpcodeIndex
                                                   TRUE,                   // DataCycle
                                                   TRUE,                   // Atomic
                                                   TRUE,                   // ShiftOut
                                                   WriteAddress,           // Address
                                                   (UINT32)(*NumBytes),    // Data Number
                                                   Buffer,
                                                   EnumSpiRegionBios
                                                   );

  AsmWbinvd ();

  return Status;
}

/**
  Erase specified FV address

  @param[in]       WriteAddress      The FV address to be written to
  @param[in]       LbaLength         contains the length of the Buffer.

  @return The status returned from SpiProtocol().

**/
EFI_STATUS
FlashFdErase (
  IN UINTN  WriteAddress,
  IN UINTN  LbaLength
  )
{
  EFI_STATUS    Status;
  SPI_INSTANCE  *SpiInstance;

  Status        = EFI_SUCCESS;
  SpiInstance   = SPI_INSTANCE_FROM_SPIPROTOCOL (mFvbModuleGlobal->SpiProtocol);
  WriteAddress -= (PcdGet32 (PcdFlashAreaBaseAddress));
  while ((INTN)LbaLength > 0) {
    if ((WriteAddress >= 0) && (WriteAddress + LbaLength <= SpiInstance->SpiInitTable.BiosSize)) {
      Status = mFvbModuleGlobal->SpiProtocol->Execute (
                                                mFvbModuleGlobal->SpiProtocol,
                                                SPI_OPCODE_ERASE_INDEX, // OpcodeIndex
                                                0,                      // PrefixOpcodeIndex
                                                FALSE,                  // DataCycle
                                                TRUE,                   // Atomic
                                                TRUE,                   // ShiftOut
                                                WriteAddress,           // Address
                                                0,                      // Data Number
                                                NULL,
                                                EnumSpiRegionBios     // SPI_REGION_TYPE
                                                );

      if (Status != EFI_SUCCESS) {
        return Status;
      }
    }

    WriteAddress += SpiInstance->SpiInitTable.OpcodeMenu[SPI_OPCODE_ERASE_INDEX].Operation;
    LbaLength    -= SpiInstance->SpiInitTable.OpcodeMenu[SPI_OPCODE_ERASE_INDEX].Operation;
  }

  AsmWbinvd ();

  return Status;
}

#ifdef _MSC_VER
  #pragma optimize( "", on )
#endif
#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC pop_options
  #endif
#endif
