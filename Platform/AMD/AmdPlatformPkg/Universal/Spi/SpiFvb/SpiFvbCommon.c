/** @file

  FV block common implementation.

  Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Pi/PiFirmwareVolume.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <FchRegistersCommon.h>

#define BLOCK_SIZE  (FixedPcdGet32 (PcdFlashNvStorageBlockSize))
// Set to 1 to turn on FVB writes and erases.  Shouldn't be needed anymore.
#define SPI_FVB_VERIFY  1

EFI_SPI_NOR_FLASH_PROTOCOL  *mSpiNorFlashProtocol;

UINT32  mSpiFlashOffset;         // For SPI ROM
UINT64  mNvStorageBase;          // This is the offset to the SPI ROM the region managed by FVB driver is located.
                                 // It is also considered as the base address of region in SPI ROM the
                                 // FVB driver manages.
EFI_LBA  mNvStorageLbaOffset;    // This is the LBA number from the starting of SPI ROM the region is located.
UINT64   mNvStorageSize;         // This is the total size of the region in SPI ROM the FVB driver manages.

/**
  Get the flash offset for SPI boot based on SoC.

  @param[in]  FlashSize -  Size of physical flash part
  @retval     SPI Offset of current ROM image.
**/
UINT32
GetSpiFlashOffset (
  IN UINT32  FlashSize
  )
{
  UINT32  Value32;
  UINT8   SpiRomPageXor;
  UINT32  RomPage;
  UINT32  RomBase;
  UINT32  RomOffset;

  if (FlashSize == 0) {
    ASSERT (FALSE);
    DEBUG ((
      DEBUG_ERROR,
      "%a - Error: Flash size should not be zero.\n",
      __func__
      ));
    return 0xFFFFFFFF;
  }

  // Check if SPI Flash Size is less than the BIOS Image Size
  if (FlashSize < FixedPcdGet32 (PcdRom3FlashAreaSize)) {
    ASSERT (FALSE);
    DEBUG ((
      DEBUG_ERROR,
      "%a - Error: Image size larger than flash size.\n",
      __func__
      ));
    return 0xFFFFFFFF;
  }

  RomPage = FixedPcdGet32 (PcdFlashAreaBaseAddress) >> 24;

  // Rom Base is at top of address space
  RomBase = (0xFFFFFFFF - FlashSize) + 1;

  // Determine Bit [24], [25] Address Override Settings
  // SPI ROM Addr[25:24] |= Bit Override Settings
  Value32 = MmioRead32 ((UINTN)(FCH_SPI_BASE_ADDRESS + FCH_SPI_MMIO_REG30));
  if (Value32 & FCH_SPI_R2MSK24) {
    RomPage &= ~(UINT32)FCH_SPI_R2VAL24;
    RomPage |= Value32 & FCH_SPI_R2VAL24;
  }

  if (Value32 & FCH_SPI_R2MSK25) {
    RomPage &= ~(UINT32)FCH_SPI_R2VAL25;
    RomPage |= Value32 & FCH_SPI_R2VAL25;
  }

  // Read SPI XOR Value from Register 0x5C
  // SPI ROM Addr[31:24] = SPI ROM Page [31:24] ^ Host Mem Addr [31:24]
  Value32 = MmioRead32 ((UINTN)(FCH_SPI_BASE_ADDRESS + FCH_SPI_MMIO_REG5C_ADDR32_CTRL3));
  DEBUG ((DEBUG_INFO, "%a - ROM 0x5C ADDR32Ctrl3 = 0x%X\n", __func__, Value32));

  // Compute SPI Address
  SpiRomPageXor = (UINT8)(Value32 & FCH_SPI_SPIROM_PAGE_MASK);
  RomPage      ^= SpiRomPageXor;

  // SPI ROM Addr = Current SPI address - SPI Address Base
  if (RomBase < (RomPage << 24)) {
    RomOffset = (RomPage << 24) - RomBase;
  } else {
    RomOffset = 0;
    ASSERT (FALSE);
  }

  return RomOffset;
}

/**
  Validate the Firmware Volume header stored in SPI flash.

  Reads the FV header from SPI flash at the NV storage location and checks
  the signature, revision, FvLength, HeaderLength, and 16-bit checksum.

  @param[in]  ExpectedFvLength  The expected FvLength value in the FV header.
                                For the Variable store this is typically
                                PcdFlashNvStorageVariableSize.

  @retval EFI_SUCCESS           The FV header is valid.
  @retval EFI_VOLUME_CORRUPTED  The FV header failed validation.
  @retval Other                 SPI read or memory allocation failure.

**/
EFI_STATUS
ValidateFvHeader (
  IN UINT64  ExpectedFvLength
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_STATUS                  Status;
  UINT16                      Checksum;
  UINT16                      HeaderLength;
  UINT32                      SpiAddress;

  Status     = EFI_SUCCESS;
  FvHeader   = NULL;
  SpiAddress = (UINT32)mNvStorageLbaOffset * BLOCK_SIZE + mSpiFlashOffset;

  //
  // Read the base FV header from SPI flash.
  //
  FvHeader = AllocatePool (sizeof (EFI_FIRMWARE_VOLUME_HEADER));
  if (FvHeader == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "%a - Failed to allocate FV header buffer\n",
      __func__
      ));
    goto return_handler;
  }

  Status = mSpiNorFlashProtocol->ReadData (
                                   mSpiNorFlashProtocol,
                                   SpiAddress,
                                   sizeof (EFI_FIRMWARE_VOLUME_HEADER),
                                   (UINT8 *)FvHeader
                                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - SPI read failed: %r\n",
      __func__,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto return_handler;
  }

  //
  // Validate signature, revision, FvLength, and HeaderLength.
  //
  if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
    Status = EFI_VOLUME_CORRUPTED;
    DEBUG ((
      DEBUG_ERROR,
      "%a - Bad FV signature: 0x%08X\n",
      __func__,
      FvHeader->Signature
      ));
    ASSERT_EFI_ERROR (Status);
    goto return_handler;
  }

  if (FvHeader->Revision != EFI_FVH_REVISION) {
    Status = EFI_INCOMPATIBLE_VERSION;
    DEBUG ((DEBUG_ERROR, "%a - Bad FV revision: 0x%02X\n", __func__, FvHeader->Revision));
    ASSERT_EFI_ERROR (Status);
    goto return_handler;
  }

  if (FvHeader->FvLength != ExpectedFvLength) {
    Status = EFI_VOLUME_CORRUPTED;
    DEBUG ((
      DEBUG_ERROR,
      "%a - FV length mismatch: header=0x%lX expected=0x%lX\n",
      __func__,
      FvHeader->FvLength,
      ExpectedFvLength
      ));
    ASSERT_EFI_ERROR (Status);
    goto return_handler;
  }

  if ((FvHeader->HeaderLength < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) ||
      ((FvHeader->HeaderLength & 0x01) != 0) ||
      (FvHeader->HeaderLength > FvHeader->FvLength))
  {
    Status = EFI_VOLUME_CORRUPTED;
    DEBUG ((
      DEBUG_ERROR,
      "%a - Bad FV HeaderLength: 0x%04X\n",
      __func__,
      FvHeader->HeaderLength
      ));
    ASSERT_EFI_ERROR (Status);
    goto return_handler;
  }

  //
  // A valid FV header checksums to zero over its entire HeaderLength.
  // If the header includes extended block map entries, re-read the full
  // header from SPI before computing the checksum.
  //
  if (FvHeader->HeaderLength > sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    HeaderLength = FvHeader->HeaderLength;
    FreePool (FvHeader);
    FvHeader = NULL;

    FvHeader = AllocatePool (HeaderLength);
    if (FvHeader == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((
        DEBUG_ERROR,
        "%a - Failed to allocate full FV header buffer\n",
        __func__
        ));
      ASSERT_EFI_ERROR (Status);
      goto return_handler;
    }

    Status = mSpiNorFlashProtocol->ReadData (
                                     mSpiNorFlashProtocol,
                                     SpiAddress,
                                     HeaderLength,
                                     (UINT8 *)FvHeader
                                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - SPI re-read failed: %r\n",
        __func__,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      goto return_handler;
    }

    Checksum = CalculateSum16 ((CONST UINT16 *)FvHeader, HeaderLength);
  } else {
    Checksum = CalculateSum16 (
                 (CONST UINT16 *)FvHeader,
                 sizeof (EFI_FIRMWARE_VOLUME_HEADER)
                 );
  }

  if (Checksum != 0) {
    Status = EFI_CRC_ERROR;
    DEBUG ((
      DEBUG_ERROR,
      "%a - FV header checksum failed: 0x%04X\n",
      __func__,
      Checksum
      ));
    ASSERT_EFI_ERROR (Status);
    goto return_handler;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a - FV header validation passed\n",
    __func__
    ));

return_handler:
  if (FvHeader != NULL) {
    FreePool (FvHeader);
    FvHeader = NULL;
  }

  return Status;
}

#if SPI_FVB_VERIFY

/**
  Verify the write operation.

  @param[in]  Address     -  Address to verify
  @param[in]  WriteBytes  -  Number of bytes to verify
  @param[in]  WriteBuffer -  Buffer to verify
  @retval     EFI_STATUS
**/
EFI_STATUS
EFIAPI
VerifyWrite (
  IN      UINT32  Address,
  IN      UINT32  WriteBytes,
  IN      UINT8   *WriteBuffer
  )
{
  EFI_STATUS  Status;
  INTN        Index;
  UINT8       *VerifyBuffer;

  VerifyBuffer = AllocateZeroPool (WriteBytes);
  // Compare Write request with data read back
  Status = mSpiNorFlashProtocol->ReadData (mSpiNorFlashProtocol, Address, WriteBytes, VerifyBuffer);
  if (!EFI_ERROR (Status)) {
    Index = CompareMem (VerifyBuffer, WriteBuffer, WriteBytes);
    if (Index != 0) {
      Status = EFI_DEVICE_ERROR;
      DEBUG ((
        DEBUG_ERROR,
        "%a: Comparison Failure: Address=0x%X, WriteBytes=0x%X, WriteBuffer=0x%lX, VerifyBuffer=0x%lX\n",
        __func__,
        Address,
        WriteBytes,
        WriteBuffer,
        VerifyBuffer
        ));
      for (Index = 0; Index < WriteBytes; Index++) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Address=0x%X, WriteBuffer[0x%X]=0x%X, VerifyBuffer[0x%X]=0x%X",
          __func__,
          Address + Index,
          Index,
          WriteBuffer[Index],
          Index,
          VerifyBuffer[Index]
          ));
        if (WriteBuffer[Index] != VerifyBuffer[Index]) {
          DEBUG ((DEBUG_ERROR, " *** FAILED ***\n"));
        } else {
          DEBUG ((DEBUG_ERROR, "\n"));
        }
      }

      ASSERT (FALSE);
    }
  }

  if (VerifyBuffer != NULL) {
    FreePool (VerifyBuffer);
  }

  return Status;
}

/**
  Verify the erase operation.

  @param[in]  Address -  Address to verify
  @param[in]  Length  -  Number of bytes to verify
  @retval     EFI_STATUS
**/
EFI_STATUS
EFIAPI
VerifyErase (
  IN      UINT32  Address,
  IN      UINT32  Length
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  UINT8       *VerifyBuffer;

  VerifyBuffer = AllocateZeroPool (Length);

  Status = mSpiNorFlashProtocol->ReadData (mSpiNorFlashProtocol, Address, Length, VerifyBuffer);
  if (!EFI_ERROR (Status)) {
    // Compare Write request with data read back
    for (Index = 0; Index < Length; Index++) {
      if (VerifyBuffer[Index] != 0xFF) {
        Status = EFI_DEVICE_ERROR;
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failure: SpiAddress=0x%X + 0x%X, VerifyBuffer[0x%X]=0x%X *** FAILED ***\n",
          __func__,
          Address,
          Index,
          Index,
          VerifyBuffer[Index]
          ));
        ASSERT (FALSE);
      }
    }
  }

  if (VerifyBuffer != NULL) {
    FreePool (VerifyBuffer);
  }

  return Status;
}

#endif // SPI_FVB_VERIFY

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
STATIC
EFI_STATUS
EFIAPI
SpiFvbGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  *Attributes = EFI_FVB2_READ_ENABLED_CAP   | // Reads may be enabled
                EFI_FVB2_READ_STATUS        | // Reads are currently enabled
                EFI_FVB2_WRITE_STATUS       | // Writes are currently enabled
                EFI_FVB2_WRITE_ENABLED_CAP  | // Writes may be enabled
                EFI_FVB2_STICKY_WRITE       | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
                EFI_FVB2_MEMORY_MAPPED      | // It is memory mapped
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
STATIC
EFI_STATUS
EFIAPI
SpiFvbSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
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
STATIC
EFI_STATUS
EFIAPI
SpiFvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  )
{
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
STATIC
EFI_STATUS
EFIAPI
SpiFvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  if ((UINTN)mNvStorageSize / BLOCK_SIZE > (UINTN)Lba) {
    *BlockSize      = BLOCK_SIZE;
    *NumberOfBlocks = (UINTN)(mNvStorageSize / BLOCK_SIZE) - (UINTN)Lba;
  } else {
    return EFI_INVALID_PARAMETER;
  }

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
STATIC
EFI_STATUS
EFIAPI
SpiFvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      SpiOffset;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(Lba=%lX, Offset=%lX, *NumBytes=%lX, Buffer=%lX)\n",
    __func__,
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  if ((Lba > MAX_UINT32) || (NumBytes == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Offset >= BLOCK_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes > (MAX_UINTN - Offset)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Offset + *NumBytes > BLOCK_SIZE) {
    *NumBytes = ((Offset + *NumBytes) & ~(BLOCK_SIZE - 1)) - Offset;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(AfterBlockBoundary Lba=%lX, Offset=%lX, *NumBytes=%lX, Buffer=%lX)\n",
    __func__,
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  if (mNvStorageLbaOffset > MAX_UINT32) {
    return EFI_INVALID_PARAMETER;
  }

  SpiOffset = ((UINT32)mNvStorageLbaOffset + (UINT32)(Lba))
              * BLOCK_SIZE + (UINT32)Offset;

  if (SpiOffset > (MAX_UINT32 - mSpiFlashOffset)) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    SpiOffset += mSpiFlashOffset;

    if (*NumBytes > MAX_UINT32) {
      return EFI_BAD_BUFFER_SIZE;
    }

    Status = mSpiNorFlashProtocol->ReadData (
                                     mSpiNorFlashProtocol,
                                     SpiOffset,
                                     (UINT32)*NumBytes,
                                     Buffer
                                     );
  }

  return Status;
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
STATIC
EFI_STATUS
EFIAPI
SpiFvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      SpiOffset;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(Lba=%lX, Offset=%lX, *NumBytes=%lX, Buffer=%lX)\n",
    __func__,
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  if ((Lba > MAX_UINT32) || (NumBytes == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Offset >= BLOCK_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes > (MAX_UINTN - Offset)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Offset + *NumBytes > BLOCK_SIZE) {
    *NumBytes = ((Offset + *NumBytes) & ~(BLOCK_SIZE - 1)) - Offset;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(AfterBlockBoundary Lba=%lX, Offset=%lX, *NumBytes=%lX, Buffer=%lX)\n",
    __func__,
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  if (mNvStorageLbaOffset > MAX_UINT32) {
    return EFI_INVALID_PARAMETER;
  }

  SpiOffset = ((UINT32)mNvStorageLbaOffset + (UINT32)(Lba))
              * BLOCK_SIZE + (UINT32)Offset;

  if (SpiOffset > (MAX_UINT32 - mSpiFlashOffset)) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    SpiOffset += mSpiFlashOffset;

    if (*NumBytes > MAX_UINT32) {
      return EFI_BAD_BUFFER_SIZE;
    }

    Status = mSpiNorFlashProtocol->WriteData (
                                     mSpiNorFlashProtocol,
                                     SpiOffset,
                                     (UINT32)*NumBytes,
                                     Buffer
                                     );

 #if SPI_FVB_VERIFY
    Status = VerifyWrite (SpiOffset, (UINT32)*NumBytes, Buffer);
 #endif // SPI_FVB_VERIFY
  }

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
STATIC
EFI_STATUS
EFIAPI
SpiFvbErase (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  )
{
  VA_LIST     Args;
  EFI_LBA     Start;
  UINTN       Length;
  EFI_STATUS  Status;
  UINT32      SpiOffset;

  Status = EFI_SUCCESS;
  Args   = NULL;
  VA_START (Args, This);

  for (
       /* coverity[VARARGS] */
       Start = VA_ARG (Args, EFI_LBA);
       Start != EFI_LBA_LIST_TERMINATOR;
       /* coverity[VARARGS] */
       Start = VA_ARG (Args, EFI_LBA)
       )
  {
    Length = VA_ARG (Args, UINTN);
    DEBUG ((
      DEBUG_VERBOSE,
      "%a(StartLba=%lX, NumBlocks=%lX)\n",
      __func__,
      Start,
      Length
      ));
    if (BLOCK_SIZE < (MAX_UINTN/Length)) {
      Length *= BLOCK_SIZE;
    } else {
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    if (mNvStorageLbaOffset > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    SpiOffset = ((UINT32)Start + (UINT32)mNvStorageLbaOffset) * BLOCK_SIZE;

    if (SpiOffset > (MAX_UINT32 - mSpiFlashOffset)) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    SpiOffset += mSpiFlashOffset;

    Status = mSpiNorFlashProtocol->Erase (
                                     mSpiNorFlashProtocol,
                                     SpiOffset,
                                     (UINT32)Length / SIZE_4KB
                                     );
    if (EFI_ERROR (Status)) {
      break;
    }

 #if SPI_FVB_VERIFY
    Status = VerifyErase (SpiOffset, (UINT32)Length);
 #endif // SPI_FVB_VERIFY
  }

  VA_END (Args);

  return Status;
}

EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  mSpiFvbProtocol = {
  SpiFvbGetAttributes,
  SpiFvbSetAttributes,
  SpiFvbGetPhysicalAddress,
  SpiFvbGetBlockSize,
  SpiFvbRead,
  SpiFvbWrite,
  SpiFvbErase
};
