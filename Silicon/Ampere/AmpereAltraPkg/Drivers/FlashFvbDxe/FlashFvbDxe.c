/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FlashLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/FirmwareVolumeBlock.h>

//
// These temporary buffers are used to calculate and convert linear virtual
// to physical address
//
STATIC UINT64  mNvFlashBase;
STATIC UINT32  mNvFlashSize;
STATIC UINT32  mFlashBlockSize;
STATIC UINT64  mNvStorageBase;
STATIC UINT64  mNvStorageSize;

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
FlashFvbAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mNvStorageBase);
}

/**
  The GetAttributes() function retrieves the attributes and
  current settings of the block.

  @param This       Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Attributes Pointer to EFI_FVB_ATTRIBUTES_2 in which the
                    attributes and current settings are
                    returned. Type EFI_FVB_ATTRIBUTES_2 is defined
                    in EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS The firmware volume attributes were
                      returned.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT      EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  ASSERT (Attributes != NULL);

  *Attributes = EFI_FVB2_READ_ENABLED_CAP   | // Reads may be enabled
                EFI_FVB2_READ_STATUS        | // Reads are currently enabled
                EFI_FVB2_WRITE_STATUS       | // Writes are currently enabled
                EFI_FVB2_WRITE_ENABLED_CAP  | // Writes may be enabled
                EFI_FVB2_STICKY_WRITE       | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
                EFI_FVB2_MEMORY_MAPPED      | // It is memory mapped
                EFI_FVB2_ALIGNMENT          |
                EFI_FVB2_ERASE_POLARITY;      // After erasure all bits take this value (i.e. '1')

  return EFI_SUCCESS;
}

/**
  The SetAttributes() function sets configurable firmware volume
  attributes and returns the new settings of the firmware volume.

  @param This         Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Attributes   On input, Attributes is a pointer to
                      EFI_FVB_ATTRIBUTES_2 that contains the
                      desired firmware volume settings. On
                      successful return, it contains the new
                      settings of the firmware volume. Type
                      EFI_FVB_ATTRIBUTES_2 is defined in
                      EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS           The firmware volume attributes were returned.

  @retval EFI_INVALID_PARAMETER The attributes requested are in
                                conflict with the capabilities
                                as declared in the firmware
                                volume header.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT   EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  return EFI_SUCCESS;  // ignore for now
}

/**
  The GetPhysicalAddress() function retrieves the base address of
  a memory-mapped firmware volume. This function should be called
  only for memory-mapped firmware volumes.

  @param This     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Address  Pointer to a caller-allocated
                  EFI_PHYSICAL_ADDRESS that, on successful
                  return from GetPhysicalAddress(), contains the
                  base address of the firmware volume.

  @retval EFI_SUCCESS       The firmware volume base address was returned.

  @retval EFI_UNSUPPORTED   The firmware volume is not memory mapped.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT      EFI_PHYSICAL_ADDRESS                 *Address
  )
{
  ASSERT (Address != NULL);

  *Address = (EFI_PHYSICAL_ADDRESS)mNvStorageBase;

  return EFI_SUCCESS;
}

/**
  The GetBlockSize() function retrieves the size of the requested
  block. It also returns the number of additional blocks with
  the identical size. The GetBlockSize() function is used to
  retrieve the block map (see EFI_FIRMWARE_VOLUME_HEADER).


  @param This           Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Lba            Indicates the block for which to return the size.

  @param BlockSize      Pointer to a caller-allocated UINTN in which
                        the size of the block is returned.

  @param NumberOfBlocks Pointer to a caller-allocated UINTN in
                        which the number of consecutive blocks,
                        starting with Lba, is returned. All
                        blocks in this range have a size of
                        BlockSize.


  @retval EFI_SUCCESS             The firmware volume base address was returned.

  @retval EFI_INVALID_PARAMETER   The requested LBA is out of range.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeGetBlockSize (
  IN  CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  UINTN  TotalNvStorageBlocks;

  ASSERT (BlockSize != NULL);
  ASSERT (NumberOfBlocks != NULL);

  TotalNvStorageBlocks = mNvStorageSize / mFlashBlockSize;

  if (TotalNvStorageBlocks <= (UINTN)Lba) {
    DEBUG ((DEBUG_ERROR, "The requested LBA is out of range\n"));
    return EFI_INVALID_PARAMETER;
  }

  *NumberOfBlocks = TotalNvStorageBlocks - (UINTN)Lba;
  *BlockSize      = mFlashBlockSize;

  return EFI_SUCCESS;
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

  @param This     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Lba      The starting logical block index
                  from which to read.

  @param Offset   Offset into the block at which to begin reading.

  @param NumBytes Pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes read.

  @param Buffer   Pointer to a caller-allocated buffer that will
                  be used to hold the data that is read.

  @retval EFI_SUCCESS         The firmware volume was read successfully,
                              and contents are in Buffer.

  @retval EFI_BAD_BUFFER_SIZE Read attempted across an LBA
                              boundary. On output, NumBytes
                              contains the total number of bytes
                              returned in Buffer.

  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              ReadDisabled state.

  @retval EFI_DEVICE_ERROR    The block device is not
                              functioning correctly and could
                              not be read.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN       EFI_LBA                              Lba,
  IN       UINTN                                Offset,
  IN OUT   UINTN                                *NumBytes,
  IN OUT   UINT8                                *Buffer
  )
{
  EFI_STATUS  Status;

  ASSERT (NumBytes != NULL);
  ASSERT (Buffer != NULL);

  if (Offset + *NumBytes > mFlashBlockSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = FlashReadCommand (
             mNvFlashBase + Lba * mFlashBlockSize + Offset,
             Buffer,
             *NumBytes
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to do flash read\n"));
    return EFI_DEVICE_ERROR;
  }

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

  @param This     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Lba      The starting logical block index to write to.

  @param Offset   Offset into the block at which to begin writing.

  @param NumBytes The pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes actually written.

  @param Buffer   The pointer to a caller-allocated buffer that
                  contains the source for the write.

  @retval EFI_SUCCESS         The firmware volume was written successfully.

  @retval EFI_BAD_BUFFER_SIZE The write was attempted across an
                              LBA boundary. On output, NumBytes
                              contains the total number of bytes
                              actually written.

  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.

  @retval EFI_DEVICE_ERROR    The block device is malfunctioning
                              and could not be written.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN       EFI_LBA                              Lba,
  IN       UINTN                                Offset,
  IN OUT   UINTN                                *NumBytes,
  IN       UINT8                                *Buffer
  )
{
  EFI_STATUS  Status;

  ASSERT (NumBytes != NULL);
  ASSERT (Buffer != NULL);

  if (Offset + *NumBytes > mFlashBlockSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = FlashWriteCommand (
             mNvFlashBase + Lba * mFlashBlockSize + Offset,
             Buffer,
             *NumBytes
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to do flash write\n"));
    return EFI_DEVICE_ERROR;
  }

  CopyMem ((UINT8 *)(UINTN)mNvStorageBase + Lba * mFlashBlockSize + Offset, Buffer, *NumBytes);

  return Status;
}

/**
  Erases and initializes a firmware volume block.

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

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL
                instance.

  @param ...    The variable argument list is a list of tuples.
                Each tuple describes a range of LBAs to erase
                and consists of the following:
                - An EFI_LBA that indicates the starting LBA
                - A UINTN that indicates the number of blocks to
                  erase.

                The list is terminated with an
                EFI_LBA_LIST_TERMINATOR. For example, the
                following indicates that two ranges of blocks
                (5-7 and 10-11) are to be erased: EraseBlocks
                (This, 5, 3, 10, 2, EFI_LBA_LIST_TERMINATOR);

  @retval EFI_SUCCESS The erase request successfully
                      completed.

  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.
  @retval EFI_DEVICE_ERROR  The block device is not functioning
                            correctly and could not be written.
                            The firmware device may have been
                            partially erased.
  @retval EFI_INVALID_PARAMETER One or more of the LBAs listed
                                in the variable argument list do
                                not exist in the firmware volume.

**/
EFI_STATUS
EFIAPI
FlashFvbDxeErase (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  )
{
  VA_LIST     Args;
  EFI_LBA     Start;
  UINTN       Length;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  VA_START (Args, This);

  for (Start = VA_ARG (Args, EFI_LBA);
       Start != EFI_LBA_LIST_TERMINATOR;
       Start = VA_ARG (Args, EFI_LBA))
  {
    Length = VA_ARG (Args, UINTN);
    Status = FlashEraseCommand (
               mNvFlashBase + Start * mFlashBlockSize,
               Length * mFlashBlockSize
               );
  }

  VA_END (Args);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to do flash erase\n"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mFlashFvbProtocol = {
  FlashFvbDxeGetAttributes,
  FlashFvbDxeSetAttributes,
  FlashFvbDxeGetPhysicalAddress,
  FlashFvbDxeGetBlockSize,
  FlashFvbDxeRead,
  FlashFvbDxeWrite,
  FlashFvbDxeErase
};

EFI_STATUS
EFIAPI
FlashFvbDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  FvbHandle = NULL;
  EFI_EVENT   VirtualAddressChangeEvent;

  // Get NV store FV info
  mFlashBlockSize = FixedPcdGet32 (PcdFvBlockSize);
  mNvStorageBase  = PcdGet64 (PcdFlashNvStorageVariableBase64);
  mNvStorageSize  = FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
                    FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
                    FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize);

  DEBUG ((
    DEBUG_INFO,
    "%a: Using NV store FV in-memory copy at 0x%lx with size 0x%x\n",
    __func__,
    mNvStorageBase,
    mNvStorageSize
    ));

  // Get NV Flash information
  Status = FlashGetNvRamInfo (&mNvFlashBase, &mNvFlashSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Flash info\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  if (mNvFlashSize >= (mNvStorageSize * 2)) {
    DEBUG ((DEBUG_INFO, "%a: NV store on Flash is valid\n", __func__));
  } else {
    DEBUG ((DEBUG_ERROR, "%a: NV store on Flash is invalid\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FlashFvbAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &VirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &FvbHandle,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  &mFlashFvbProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install Firmware Volume Block protocol\n"));
    return Status;
  }

  return EFI_SUCCESS;
}
