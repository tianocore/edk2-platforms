/** @file

  Copyright (c) 2024, SOPHGO Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/NvVarStoreFormatted.h>

#include "FlashFvbDxe.h"

STATIC FVB_DEVICE    *mFvbDevice;
STATIC EFI_EVENT     mFvbVirtualAddrChangeEvent;

STATIC CONST FVB_DEVICE mFvbFlashInstanceTemplate = {
  NULL, // SPI_NOR ... NEED TO BE FILLED
  NULL, // NorFlashProtocol ... NEED TO BE FILLED
  NULL, // SpiMasterProtocol ... NEED TO BE FILLED
  NULL, // Handle ... NEED TO BE FILLED

  FVB_FLASH_SIGNATURE, // Signature

  0, // RegionBaseAddress ... NEED TO BE FILLED
  SIZE_256KB, // Size?
  0, // FvbOffset ... NEED TO BE FILLED
  0, // FvbSize ... NEED TO BE FILLED
  0, // StartLba

  {
    0,     // MediaId ... NEED TO BE FILLED
    FALSE, // RemovableMedia
    TRUE,  // MediaPresent
    FALSE, // LogicalPartition
    FALSE, // ReadOnly
    FALSE, // WriteCaching
    0,     // BlockSize ... NEED TO BE FILLED
    4,     // IoAlign
    0,     // LastBlock ... NEED TO BE FILLED
    0,     // LowestAlignedLba
    1,     // LogicalBlocksPerPhysicalBlock
  }, // Media;

  {
    FvbGetAttributes,       // GetAttributes
    FvbSetAttributes,       // SetAttributes
    FvbGetPhysicalAddress,  // GetPhysicalAddress
    FvbGetBlockSize,        // GetBlockSize
    FvbRead,                // Read
    FvbWrite,               // Write
    FvbEraseBlocks,         // EraseBlocks
    NULL,                   // ParentHandle
  }, //  FvbProtocol;

  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)sizeof (VENDOR_DEVICE_PATH),
          (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
        }
      },
      { 0x0C42D4FA, 0x3792, 0x497C, { 0xA3, 0x33, 0xF5, 0x0A, 0xF3, 0xE3, 0x8B, 0x7D } }
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
  } // DevicePath
};

///
/// The Firmware Volume Block Protocol is the low-level interface
/// to a firmware volume. File-level access to a firmware volume
/// should not be done using the Firmware Volume Block Protocol.
/// Normal access to a firmware volume must use the Firmware
/// Volume Protocol. Typically, only the file system driver that
/// produces the Firmware Volume Protocol will bind to the
/// Firmware Volume Block Protocol.
///

/**
  Initialises the FV Header and Variable Store Header
  to support variable operations.

  @param[in]  Ptr - Location to initialise the headers

**/
EFI_STATUS
InitializeFvAndVariableStoreHeaders (
  IN FVB_DEVICE  *Instance
  )
{
  EFI_STATUS                  Status;
  VOID                        *Headers;
  UINTN                       HeadersLength;
  UINTN                       BlockSize;
  EFI_FIRMWARE_VOLUME_HEADER  *FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINT32                      NvStorageFtwSpareSize;
  UINT32                      NvStorageFtwWorkingSize;
  UINT32                      NvStorageVariableSize;
  UINT64                      NvStorageFtwSpareBase;
  UINT64                      NvStorageFtwWorkingBase;
  UINT64                      NvStorageVariableBase;

  HeadersLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) +
                  sizeof (EFI_FV_BLOCK_MAP_ENTRY) +
                  sizeof (VARIABLE_STORE_HEADER);
  Headers = AllocateRuntimeZeroPool (HeadersLength);

  BlockSize = Instance->Media.BlockSize;

  NvStorageFtwSpareSize = PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  NvStorageFtwWorkingSize = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  NvStorageVariableSize = PcdGet32 (PcdFlashNvStorageVariableSize);

  NvStorageFtwSpareBase = PcdGet64 (PcdFlashNvStorageFtwSpareBase64);
  NvStorageFtwWorkingBase = PcdGet64 (PcdFlashNvStorageFtwWorkingBase64);
  NvStorageVariableBase = PcdGet64 (PcdFlashNvStorageVariableBase64);

  //
  // FirmwareVolumeHeader->FvLength is declared to have the Variable area
  // AND the FTW working area AND the FTW Spare contiguous.
  //
  if ((NvStorageVariableBase + NvStorageVariableSize) != NvStorageFtwWorkingBase) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NvStorageFtwWorkingBase is not contiguous with NvStorageVariableBase region\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((NvStorageFtwWorkingBase + NvStorageFtwWorkingSize) != NvStorageFtwSpareBase) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NvStorageFtwSpareBase is not contiguous with NvStorageFtwWorkingBase region\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the size of the area is at least one block size
  //
  if ((NvStorageVariableSize <= 0) || (NvStorageVariableSize / BlockSize <= 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NvStorageVariableSize is 0x%x, should be atleast one block size\n",
      __func__,
      NvStorageVariableSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((NvStorageFtwWorkingSize <= 0) || (NvStorageFtwWorkingSize / BlockSize <= 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NvStorageFtwWorkingSize is 0x%x, should be atleast one block size\n",
      __func__,
      NvStorageFtwWorkingSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((NvStorageFtwSpareSize <= 0) || (NvStorageFtwSpareSize / BlockSize <= 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NvStorageFtwSpareSize is 0x%x, should be atleast one block size\n",
      __func__,
      NvStorageFtwSpareSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Ensure the Variable area Base Addresses are aligned on a block size boundaries
  //
  if ((NvStorageVariableBase % BlockSize != 0) ||
      (NvStorageFtwWorkingBase % BlockSize != 0) ||
      (NvStorageFtwSpareBase % BlockSize != 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NvStorage Base addresses must be aligned to block size boundaries",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // EFI_FIRMWARE_VOLUME_HEADER
  //
  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Headers;
  CopyGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
  FirmwareVolumeHeader->FvLength = Instance->FvbSize;
  FirmwareVolumeHeader->Signature  = EFI_FVH_SIGNATURE;
  FirmwareVolumeHeader->Attributes = (EFI_FVB_ATTRIBUTES_2)(
                                      EFI_FVB2_READ_ENABLED_CAP   | // Reads may be enabled
                                      EFI_FVB2_READ_STATUS        | // Reads are currently enabled
                                      EFI_FVB2_STICKY_WRITE       | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
                                      EFI_FVB2_ERASE_POLARITY     | // After erasure all bits take this value (i.e. '1')
                                      EFI_FVB2_WRITE_STATUS       | // Writes are currently enabled
                                      EFI_FVB2_WRITE_ENABLED_CAP    // Writes may be enabled
                                                            );
  FirmwareVolumeHeader->HeaderLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) +
                                       sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  FirmwareVolumeHeader->Revision = EFI_FVH_REVISION;
  FirmwareVolumeHeader->BlockMap[0].NumBlocks = Instance->Media.LastBlock + 1;
  FirmwareVolumeHeader->BlockMap[0].Length = Instance->Media.BlockSize;
  FirmwareVolumeHeader->BlockMap[1].NumBlocks = 0;
  FirmwareVolumeHeader->BlockMap[1].Length = 0;
  FirmwareVolumeHeader->Checksum = CalculateCheckSum16 (
                                      (UINT16 *)FirmwareVolumeHeader,
                                      FirmwareVolumeHeader->HeaderLength
                                      );

  //
  // VARIABLE_STORE_HEADER
  //
  VariableStoreHeader = (VARIABLE_STORE_HEADER *)
                        ((UINTN)Headers + FirmwareVolumeHeader->HeaderLength);
  CopyGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
  VariableStoreHeader->Size = PcdGet32 (PcdFlashNvStorageVariableSize) -
                              FirmwareVolumeHeader->HeaderLength;
  VariableStoreHeader->Format = VARIABLE_STORE_FORMATTED;
  VariableStoreHeader->State = VARIABLE_STORE_HEALTHY;

  //
  // Install the combined super-header in the NorFlash
  //
  Status = FvbWrite (&Instance->FvbProtocol, 0, 0, &HeadersLength, Headers);

  FreePool (Headers);
  return Status;
}

/**
  Check the integrity of firmware volume header.

  @param[in] FwVolHeader - A pointer to a firmware volume header

  @retval  EFI_SUCCESS   - The firmware volume is consistent
  @retval  EFI_NOT_FOUND - The firmware volume has been corrupted.

**/
EFI_STATUS
ValidateFvHeader (
  IN  FVB_DEVICE  *Instance
  )
{
  UINT16                      Checksum;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINTN                       VariableStoreLength;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(Instance->RegionBaseAddress);

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if ((FwVolHeader->Revision  != EFI_FVH_REVISION) ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength  != Instance->FvbSize)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Firmware Volume header present\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Check the Firmware Volume Guid
  //
  if (!CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Firmware Volume Guid non-compatible\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Verify the header checksum
  //
  Checksum = CalculateSum16 ((UINT16 *)FwVolHeader, FwVolHeader->HeaderLength);
  if (Checksum != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: FV checksum is invalid (Checksum:0x%X)\n",
      __func__,
      Checksum
      ));
    return EFI_NOT_FOUND;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *)
                        ((UINTN)FwVolHeader + FwVolHeader->HeaderLength);

  //
  // Check the Variable Store Guid
  //
  if (!CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) &&
      !CompareGuid (&VariableStoreHeader->Signature,
                    &gEfiAuthenticatedVariableGuid)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Variable Store Guid non-compatible\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  VariableStoreLength = PcdGet32 (PcdFlashNvStorageVariableSize) -
                        FwVolHeader->HeaderLength;
  if (VariableStoreHeader->Size != VariableStoreLength) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Variable Store Length does not match\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
 The GetAttributes() function retrieves the attributes and
 current settings of the block.

 @param This         Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Attributes   Pointer to EFI_FVB_ATTRIBUTES_2 in which the attributes and
                     current settings are returned.
                     Type EFI_FVB_ATTRIBUTES_2 is defined in EFI_FIRMWARE_VOLUME_HEADER.

 @retval EFI_SUCCESS The firmware volume attributes were returned.

 **/
EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  EFI_FVB_ATTRIBUTES_2        *FlashFvbAttributes;
  FVB_DEVICE                  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Instance->RegionBaseAddress;

  FlashFvbAttributes = (EFI_FVB_ATTRIBUTES_2 *)&(FwVolHeader->Attributes);

  *Attributes = *FlashFvbAttributes;

  DEBUG ((DEBUG_VERBOSE, "FvbGetAttributes(0x%X)\n", *Attributes));

  return EFI_SUCCESS;
}

/**
 The SetAttributes() function sets configurable firmware volume attributes
 and returns the new settings of the firmware volume.


 @param This                     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Attributes               On input, Attributes is a pointer to EFI_FVB_ATTRIBUTES_2
                                 that contains the desired firmware volume settings.
                                 On successful return, it contains the new settings of
                                 the firmware volume.
                                 Type EFI_FVB_ATTRIBUTES_2 is defined in EFI_FIRMWARE_VOLUME_HEADER.

 @retval EFI_SUCCESS             The firmware volume attributes were returned.

 @retval EFI_INVALID_PARAMETER   The attributes requested are in conflict with the capabilities
                                 as declared in the firmware volume header.

 **/
EFI_STATUS
EFIAPI
FvbSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  DEBUG ((
    DEBUG_WARN,
    "FvbSetAttributes(0x%X) is not supported\n",
    *Attributes
    ));

  return EFI_UNSUPPORTED;
}

/**
 The GetPhysicalAddress() function retrieves the base address of
 a memory-mapped firmware volume. This function should be called
 only for memory-mapped firmware volumes.

 @param This               Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Address            Pointer to a caller-allocated
                           EFI_PHYSICAL_ADDRESS that, on successful
                           return from GetPhysicalAddress(), contains the
                           base address of the firmware volume.

 @retval EFI_SUCCESS       The firmware volume base address was returned.

 @retval EFI_NOT_SUPPORTED The firmware volume is not memory mapped.

 **/
EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  )
{
  FVB_DEVICE  *Instance;

  ASSERT (Address != NULL);

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_VERBOSE,
    "FvbGetPhysicalAddress(BaseAddress=0x%lX)\n",
    Instance->RegionBaseAddress
    ));

  *Address = Instance->RegionBaseAddress;

  return EFI_SUCCESS;
}

/**
 The GetBlockSize() function retrieves the size of the requested
 block. It also returns the number of additional blocks with
 the identical size. The GetBlockSize() function is used to
 retrieve the block map (see EFI_FIRMWARE_VOLUME_HEADER).


 @param This                     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Lba                      Indicates the block for which to return the size.

 @param BlockSize                Pointer to a caller-allocated UINTN in which
                                 the size of the block is returned.

 @param NumberOfBlocks           Pointer to a caller-allocated UINTN in
                                 which the number of consecutive blocks,
                                 starting with Lba, is returned. All
                                 blocks in this range have a size of
                                 BlockSize.


 @retval EFI_SUCCESS             The firmware volume base address was returned.

 @retval EFI_INVALID_PARAMETER   The requested LBA is out of range.

 **/
EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  FVB_DEVICE  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_VERBOSE,
    "FvbGetBlockSize(Lba=%ld, BlockSize=0x%x, LastBlock=%ld)\n",
    Lba,
    Instance->Media.BlockSize,
    Instance->Media.LastBlock
    ));

  if (Lba > Instance->Media.LastBlock) {
    DEBUG ((
      DEBUG_ERROR,
      "FvbGetBlockSize: ERROR - Parameter LBA %ld is beyond the last Lba (%ld).\n",
      Lba,
      Instance->Media.LastBlock
      ));
    return EFI_INVALID_PARAMETER;
  } else {
    //
    // Assume equal sized blocks in all flash devices.
    //
    *BlockSize = (UINTN)Instance->Media.BlockSize;
    *NumberOfBlocks = (UINTN)(Instance->Media.LastBlock - Lba + 1);

    DEBUG ((
      DEBUG_VERBOSE,
      "FvbGetBlockSize: *BlockSize=0x%x, *NumberOfBlocks=0x%x.\n",
      *BlockSize,
      *NumberOfBlocks
      ));

    return EFI_SUCCESS;
  }
}

/**
 Reads the specified number of bytes into a buffer from the specified block.

 The Read() function reads the requested number of bytes from the
 requested block and stores them in the provided buffer.
 Implementations should be mindful that the firmware volume
 might be in the ReadDisabled state. If it is in this state,
 the Read() function must return the status code
 EFI_ACCESS_DENIED without modifying the contents of the
 buffer. The Read() function must also prevent spanning block
 boundaries. If a read is requested that would span a block
 boundary, the read must read up to the boundary but not
 beyond. The output parameter NumBytes must be set to correctly
 indicate the number of bytes actually read. The caller must be
 aware that a read may be partially completed.

 @param This                 Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Lba                  The starting logical block index from which to read.

 @param Offset               Offset into the block at which to begin reading.

 @param NumBytes             Pointer to a UINTN.
                             At entry, *NumBytes contains the total size of the buffer.
                             At exit, *NumBytes contains the total number of bytes read.

 @param Buffer               Pointer to a caller-allocated buffer that will be used
                             to hold the data that is read.

 @retval EFI_SUCCESS         The firmware volume was read successfully,  and contents are
                             in Buffer.

 @retval EFI_BAD_BUFFER_SIZE Read attempted across an LBA boundary.
                             On output, NumBytes contains the total number of bytes
                             returned in Buffer.

 @retval EFI_ACCESS_DENIED   The firmware volume is in the ReadDisabled state.

 @retval EFI_DEVICE_ERROR    The block device is not functioning correctly and could not be read.

 **/
EFI_STATUS
EFIAPI
FvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  )
{
  UINTN               BlockSize;
  UINTN               DataOffset;
  FVB_DEVICE          *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_VERBOSE,
    "FvbRead(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%lX)\n",
    Instance->StartLba + Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  //
  // Only write to the first 64k. We don't bother saving the FTW Spare
  // block into the flash memory.
  //
  //if (Lba > 0) {
  //  return EFI_INVALID_PARAMETER;
  //}

  //
  // Cache the block size to avoid de-referencing pointers all the time
  //
  BlockSize = Instance->Media.BlockSize;

  DEBUG ((
    DEBUG_VERBOSE,
    "FvbRead: Check if (Offset=0x%x + NumBytes=0x%x) <= BlockSize=0x%x\n",
    Offset,
    *NumBytes,
    BlockSize
    ));

  //
  // The read must not span block boundaries.
  // We need to check each variable individually because adding two large
  // values together overflows.
  //
  if ((Offset               >= BlockSize) ||
      (*NumBytes            >  BlockSize) ||
      ((Offset + *NumBytes) >  BlockSize)) {
    DEBUG ((
      DEBUG_ERROR,
      "FvbRead: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n",
      Offset,
      *NumBytes,
      BlockSize
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // We must have some bytes to read
  //
  if (*NumBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  DataOffset = GET_DATA_OFFSET (Instance->RegionBaseAddress + Offset,
                  Instance->StartLba + Lba,
                  Instance->Media.BlockSize);

  //
  // Read the memory-mapped data
  //
  CopyMem (Buffer, (UINTN *)DataOffset, *NumBytes);

  return EFI_SUCCESS;
}

/**
 Writes the specified number of bytes from the input buffer to the block.

 The Write() function writes the specified number of bytes from
 the provided buffer to the specified block and offset. If the
 firmware volume is sticky write, the caller must ensure that
 all the bits of the specified range to write are in the
 EFI_FVB_ERASE_POLARITY state before calling the Write()
 function, or else the result will be unpredictable. This
 unpredictability arises because, for a sticky-write firmware
 volume, a write may negate a bit in the EFI_FVB_ERASE_POLARITY
 state but cannot flip it back again.  Before calling the
 Write() function,  it is recommended for the caller to first call
 the EraseBlocks() function to erase the specified block to
 write. A block erase cycle will transition bits from the
 (NOT)EFI_FVB_ERASE_POLARITY state back to the
 EFI_FVB_ERASE_POLARITY state. Implementations should be
 mindful that the firmware volume might be in the WriteDisabled
 state. If it is in this state, the Write() function must
 return the status code EFI_ACCESS_DENIED without modifying the
 contents of the firmware volume. The Write() function must
 also prevent spanning block boundaries. If a write is
 requested that spans a block boundary, the write must store up
 to the boundary but not beyond. The output parameter NumBytes
 must be set to correctly indicate the number of bytes actually
 written. The caller must be aware that a write may be
 partially completed. All writes, partial or otherwise, must be
 fully flushed to the hardware before the Write() service
 returns.

 @param This                 Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param Lba                  The starting logical block index to write to.

 @param Offset               Offset into the block at which to begin writing.

 @param NumBytes             The pointer to a UINTN.
                             At entry, *NumBytes contains the total size of the buffer.
                             At exit, *NumBytes contains the total number of bytes actually written.

 @param Buffer               The pointer to a caller-allocated buffer that contains the source for the write.

 @retval EFI_SUCCESS         The firmware volume was written successfully.

 @retval EFI_BAD_BUFFER_SIZE The write was attempted across an LBA boundary.
                             On output, NumBytes contains the total number of bytes
                             actually written.

 @retval EFI_ACCESS_DENIED   The firmware volume is in the WriteDisabled state.

 @retval EFI_DEVICE_ERROR    The block device is malfunctioning and could not be written.


 **/
EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  )
{
  FVB_DEVICE  *Instance;
  EFI_STATUS  Status;
  UINTN       DataOffset;

  ASSERT (NumBytes != NULL);
  ASSERT (Buffer != NULL);

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_VERBOSE,
    "FvbWrite(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%lX)\n",
    Instance->StartLba + Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  DataOffset = GET_DATA_OFFSET (Instance->FvbOffset + Offset,
                  Instance->StartLba + Lba,
                  Instance->Media.BlockSize);

  Status = Instance->NorFlashProtocol->WriteData (
              Instance->Nor,
              DataOffset,
              *NumBytes,
              (UINT8 *)Buffer
              );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to do flash write\n",
      __func__
      ));
    return Status;
  }

  //
  // Update data in RAM space
  //
  DataOffset = GET_DATA_OFFSET (Instance->RegionBaseAddress + Offset,
		  Instance->StartLba + Lba,
		  Instance->Media.BlockSize);
  CopyMem ((UINTN *)DataOffset, Buffer, *NumBytes);

  return Status;
}

/**
 Erases and initialises a firmware volume block.

 The EraseBlocks() function erases one or more blocks as denoted
 by the variable argument list. The entire parameter list of
 blocks must be verified before erasing any blocks. If a block is
 requested that does not exist within the associated firmware
 volume (it has a larger index than the last block of the
 firmware volume), the EraseBlocks() function must return the
 status code EFI_INVALID_PARAMETER without modifying the contents
 of the firmware volume. Implementations should be mindful that
 the firmware volume might be in the WriteDisabled state. If it
 is in this state, the EraseBlocks() function must return the
 status code EFI_ACCESS_DENIED without modifying the contents of
 the firmware volume. All calls to EraseBlocks() must be fully
 flushed to the hardware before the EraseBlocks() service
 returns.

 @param This                     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL
 instance.

 @param ...                      The variable argument list is a list of tuples.
                                 Each tuple describes a range of LBAs to erase
                                 and consists of the following:
                                 - An EFI_LBA that indicates the starting LBA
                                 - A UINTN that indicates the number of blocks to erase.

                                 The list is terminated with an EFI_LBA_LIST_TERMINATOR.
                                 For example, the following indicates that two ranges of blocks
                                 (5-7 and 10-11) are to be erased:
                                 EraseBlocks (This, 5, 3, 10, 2, EFI_LBA_LIST_TERMINATOR);

 @retval EFI_SUCCESS             The erase request successfully completed.

 @retval EFI_ACCESS_DENIED       The firmware volume is in the WriteDisabled state.

 @retval EFI_DEVICE_ERROR        The block device is not functioning correctly and could not be written.
                                 The firmware device may have been partially erased.

 @retval EFI_INVALID_PARAMETER   One or more of the LBAs listed in the variable argument list do
                                 not exist in the firmware volume.

 **/
EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  )
{
  EFI_STATUS          Status;
  VA_LIST             Args;
  UINTN               BlockAddress; // Physical address of Lba to erase
  EFI_LBA             StartingLba;  // Lba from which we start erasing
  UINTN               NumOfLba;     // Number of Lba blocks to erase
  FVB_DEVICE          *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_VERBOSE,
    "FvbEraseBlocks()\n"
    ));

  Status = EFI_SUCCESS;

  //
  // Before erasing, check the entire list of parameters to ensure all specified blocks are valid
  //
  VA_START (Args, This);
  do {
    //
    // Get the Lba from which we start erasing
    //
    StartingLba = VA_ARG (Args, EFI_LBA);

    //
    // Have we reached the end of the list?
    //
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      //
      // Exit the while loop
      //
      break;
    }

    //
    // How many Lba blocks are we requested to erase?
    //
    NumOfLba = VA_ARG (Args, UINTN);

    //
    // All blocks must be within range
    //
    DEBUG ((
      DEBUG_VERBOSE,
      "FvbEraseBlocks: Check if: ( StartingLba=%ld + NumOfLba=%Lu - 1 ) > LastBlock=%ld.\n",
      Instance->StartLba + StartingLba,
      (UINT64)NumOfLba,
      Instance->Media.LastBlock
      ));
    if ((NumOfLba == 0) ||
       ((Instance->StartLba + StartingLba + NumOfLba - 1) >
         Instance->Media.LastBlock)) {
      VA_END (Args);
      DEBUG ((
        DEBUG_ERROR,
        "%a: Error: Requested LBA are beyond the last available LBA (%ld).\n",
        __func__,
        Instance->Media.LastBlock
        ));

      VA_END (Args);

      return EFI_INVALID_PARAMETER;
    }
  } while (TRUE);

  VA_END (Args);

  //
  // To get here, all must be ok, so start erasing
  //
  VA_START (Args, This);
  do {
    //
    // Get the Lba from which we start erasing
    //
    StartingLba = VA_ARG (Args, EFI_LBA);

    //
    // Have we reached the end of the list?
    //
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      //
      // Exit the while loop
      //
      break;
    }

    //
    // How many Lba blocks are we requested to erase?
    //
    NumOfLba = VA_ARG (Args, UINTN);

    //
    // Go through each one and erase it
    //
    while (NumOfLba > 0) {
      //
      // Get the physical address of Lba to erase
      //
      BlockAddress = GET_DATA_OFFSET (Instance->FvbOffset,
                       Instance->StartLba + StartingLba,
                       Instance->Media.BlockSize);
      //
      // Erase single block
      //
      DEBUG ((
        DEBUG_VERBOSE,
        "FvbEraseBlocks: Erasing Lba=%ld @ 0x%lX.\n",
        Instance->StartLba + StartingLba,
        BlockAddress
        ));
      Status = Instance->NorFlashProtocol->Erase (
                    Instance->Nor,
                    BlockAddress,
                    Instance->Media.BlockSize
                    );
      if (EFI_ERROR (Status)) {
        VA_END (Args);
        return EFI_DEVICE_ERROR;
      }

      //
      // Move to the next Lba
      //
      StartingLba++;
      NumOfLba--;
    }
  } while (TRUE);

  VA_END (Args);

  return EFI_SUCCESS;
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
FvbVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->NorFlashProtocol);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->SpiMasterProtocol);

  //
  // Convert SPI memory mapped region
  //
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->RegionBaseAddress);

  //
  // Convert SPI device description
  //
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->Nor->Info->Name);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->Nor->Info);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->Nor->BounceBuf);

  mFvbDevice->Nor->SpiBase = mFvbDevice->Nor->SpiBase & 0x7fffffffff;
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->Nor->SpiBase);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->Nor);

  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->NorFlashProtocol);

  //
  // Convert Fvb
  //
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.GetAttributes);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.SetAttributes);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.GetPhysicalAddress);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.GetBlockSize);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.Read);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.Write);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol.EraseBlocks);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice->FvbProtocol);
  EfiConvertPointer (0x0, (VOID**)&mFvbDevice);

  return;
}

STATIC
EFI_STATUS
FlashFvbProbe (
  IN FVB_DEVICE *FlashInstance
  )
{
  SOPHGO_NOR_FLASH_PROTOCOL *NorFlashProtocol;
  EFI_STATUS                Status;

  NorFlashProtocol = FlashInstance->NorFlashProtocol;

  //
  // Read Nor Flash ID
  //
  Status = NorFlashProtocol->GetFlashid (FlashInstance->Nor, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Read Nor flash ID failed!\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Initialize Nor Flash
  //
  Status = NorFlashProtocol->Init (NorFlashProtocol, FlashInstance->Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Cannot initialize flash device\n",
      __func__
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FlashFvbPrepareFvHeader (
  IN FVB_DEVICE *FlashInstance
  )
{
  EFI_BOOT_MODE BootMode;
  EFI_STATUS    Status;

  //
  // Check if it is required to use default environment
  //
  BootMode = GetBootModeHob ();
  if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    //
    // Validate header at the beginning of FV region
    //
    Status = ValidateFvHeader (FlashInstance);
  }

  //
  // Install the default FVB header if required
  //
  if (EFI_ERROR (Status)) {
    //
    // There is no valid header, so time to install one.
    //
    DEBUG ((
      DEBUG_ERROR,
      "%a: The FVB Header is not valid.\n",
      __func__
      ));
    DEBUG ((
      DEBUG_ERROR,
      "%a: Installing a correct one for this volume.\n",
      __func__
      ));

    //
    // Erase entire region that is reserved for variable storage
    //
    Status = FlashInstance->NorFlashProtocol->Erase (FlashInstance->Nor,
                                                FlashInstance->FvbOffset,
                                                FlashInstance->FvbSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Install all appropriate headers
    //
    Status = InitializeFvAndVariableStoreHeaders (FlashInstance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FlashFvbConfigureFlashInstance (
  IN OUT FVB_DEVICE *FlashInstance
  )
{
  UINTN      VariableSize;
  UINTN      FtwWorkingSize;
  UINTN      FtwSpareSize;
  UINTN      MemorySize;
  UINTN      DataOffset;
  EFI_STATUS Status;

  //
  // Locate SPI Master protocol
  //
  Status = gBS->LocateProtocol (
                  &gSophgoSpiMasterProtocolGuid,
                  NULL,
                  (VOID **)&FlashInstance->SpiMasterProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Cannot locate SPI Master protocol\n",
      __func__
      ));
    return Status;
  }

  //
  // Locate Nor Flash protocol
  //
  Status = gBS->LocateProtocol (
                  &gSophgoNorFlashProtocolGuid,
                  NULL,
                  (VOID **)&FlashInstance->NorFlashProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Cannot locate Nor Flash protocol\n",
      __func__
      ));
    return Status;
  }

  //
  // Setup and probe Nor flash
  //
  FlashInstance->Nor = FlashInstance->SpiMasterProtocol->SetupDevice (
                  FlashInstance->SpiMasterProtocol,
                  0
                );

  if (FlashInstance->Nor == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Nor Flash not found!\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  Status = FlashFvbProbe (FlashInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error while performing Nor flash probe [Status=%r]\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Get flash variable offset
  //
  Status = FlashInstance->NorFlashProtocol->GetFlashVariableOffset (FlashInstance->Nor);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Get flash variable offset from partition table failed!\n",
      __func__
      ));
    return Status;
  }

  //
  // Fill remaining flash description
  //
  VariableSize = PcdGet32 (PcdFlashNvStorageVariableSize);
  FtwWorkingSize = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  FtwSpareSize = PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  FlashInstance->FvbSize = VariableSize + FtwWorkingSize + FtwSpareSize;
  FlashInstance->FvbOffset = PcdGet64 (PcdFlashVariableOffset);

  FlashInstance->Media.MediaId = 0;
  FlashInstance->Media.BlockSize = PcdGet32 (PcdVariableFdBlockSize);

  FlashInstance->Media.LastBlock = FlashInstance->Size /
                                   FlashInstance->Media.BlockSize - 1;

  //
  // Our platform does not support XIP (eXecute In Place) from Flash.
  // Regardless of whether booting from a microSD card or NOR Flash, we first
  // read the variables from NOR Flash into a reallocated RAM space based on
  // PcdFlashVariableOffset.
  //
  MemorySize = EFI_SIZE_TO_PAGES (FlashInstance->FvbSize);

  //
  // FaultTolerantWriteDxe requires memory to be aligned to FtwWorkingSize
  //
  FlashInstance->RegionBaseAddress = (UINTN) AllocateAlignedRuntimePages (MemorySize, SIZE_64KB);
  if (FlashInstance->RegionBaseAddress == (UINTN) NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PcdSet64S (PcdFlashNvStorageVariableBase64,
		  (UINT64) FlashInstance->RegionBaseAddress);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageFtwWorkingBase64,
		  (UINT64) FlashInstance->RegionBaseAddress + VariableSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageFtwSpareBase64,
		  (UINT64) FlashInstance->RegionBaseAddress + VariableSize + FtwWorkingSize);
  ASSERT_EFI_ERROR (Status);

  //
  // Fill the buffer with data from flash
  //
  DataOffset = GET_DATA_OFFSET (FlashInstance->FvbOffset,
                   FlashInstance->StartLba,
                   FlashInstance->Media.BlockSize);
  Status = FlashInstance->NorFlashProtocol->ReadData (FlashInstance->Nor,
                                                DataOffset,
                                                FlashInstance->FvbSize,
                                                (UINT8 *)FlashInstance->RegionBaseAddress);

  if (EFI_ERROR (Status)) {
    goto ErrorFreeAllocatedPages;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                      &FlashInstance->Handle,
                      &gEfiDevicePathProtocolGuid,
                      &FlashInstance->DevicePath,
                      &gEfiFirmwareVolumeBlockProtocolGuid,
                      &FlashInstance->FvbProtocol,
                      NULL);
  if (EFI_ERROR (Status)) {
    return Status;;
  }

  Status = FlashFvbPrepareFvHeader (FlashInstance);
  if (EFI_ERROR (Status)) {
    goto ErrorPrepareFvbHeader;
  }

  return EFI_SUCCESS;

ErrorPrepareFvbHeader:
  gBS->UninstallMultipleProtocolInterfaces (
                &FlashInstance->Handle,
                &gEfiDevicePathProtocolGuid,
                &gEfiFirmwareVolumeBlockProtocolGuid,
                NULL);

  return Status;

ErrorFreeAllocatedPages:
  FreeAlignedPages ((VOID *)FlashInstance->RegionBaseAddress, MemorySize);

  return Status;
}

EFI_STATUS
EFIAPI
FlashFvbEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Create FVB flash device
  //
  mFvbDevice = AllocateRuntimeCopyPool (sizeof (FVB_DEVICE),
                 &mFvbFlashInstanceTemplate);
  if (mFvbDevice == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Cannot allocate memory\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Detect and configure flash device
  //
  Status = FlashFvbConfigureFlashInstance (mFvbDevice);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to configure Fvb SPI device\n",
      __func__
      ));
    goto ErrorConfigureFlash;
  }

  //
  // The driver implementing the variable read service can now be dispatched;
  // the varstore headers are in place.
  //
  Status = gBS->InstallProtocolInterface (&gImageHandle,
                  &gEdkiiNvVarStoreFormattedGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install gEdkiiNvVarStoreFormattedGuid\n",
      __func__
      ));
    goto ErrorInstallNvVarStoreFormatted;
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mFvbVirtualAddrChangeEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to register VA change event\n",
      __func__
      ));
    goto ErrorAddSpace;
  }

  return EFI_SUCCESS;

ErrorAddSpace:
  gBS->UninstallProtocolInterface (gImageHandle,
         &gEdkiiNvVarStoreFormattedGuid,
         NULL);

ErrorInstallNvVarStoreFormatted:
  gBS->UninstallMultipleProtocolInterfaces (
            &mFvbDevice->Handle,
            &gEfiDevicePathProtocolGuid,
            &gEfiFirmwareVolumeBlockProtocolGuid,
            NULL);

ErrorConfigureFlash:
  FreePool (mFvbDevice);

  return Status;
}
