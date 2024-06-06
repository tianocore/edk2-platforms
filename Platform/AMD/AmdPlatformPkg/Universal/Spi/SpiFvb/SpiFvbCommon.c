/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Register/Cpuid.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/AmdPlatformSocLib.h>
#include <Protocol/SpiSmmNorFlash.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <FchRegistersCommon.h>

#define BLOCK_SIZE  (FixedPcdGet32 (PcdFlashNvStorageBlockSize))
// Set to 1 to turn on FVB writes and erases.  Shouldn't be needed anymore.
#define SPI_FVB_VERIFY  1

EFI_SPI_NOR_FLASH_PROTOCOL  *mSpiNorFlashProtocol;

extern UINT32  mSpiFlashOffset;

UINT64   mNvStorageBase;
EFI_LBA  mNvStorageLbaOffset;

STATIC CONST UINT64  mNvStorageSize = FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
                                      FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
                                      FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize);

/**
  Set the flash offset for SPI boot based on SoC.

  @param[in]  FlashSize -  Size of physical flash part
  @retval     SPI Offset of current ROM image.
**/
UINT32
SetSpiFlashOffset (
  IN UINT32  FlashSize
  )
{
  UINT32                  Value32;
  UINT8                   SpiRomPageXor;
  UINT32                  RomPage;
  UINT32                  RomBase;
  CPUID_VERSION_INFO_EAX  VersionInfoEax;

  RomPage = FixedPcdGet32 (PcdFlashAreaBaseAddress) >> 24;

  ///
  /// Set Base SPI address
  ///
  VersionInfoEax.Uint32 = 0;
  AsmCpuid (CPUID_VERSION_INFO, &VersionInfoEax.Uint32, NULL, NULL, NULL);
  switch (VersionInfoEax.Uint32) {
    case F1A_BRH_A0_RAW_ID:
    case F1A_BRH_B0_RAW_ID:
    case F1A_BRH_B1_RAW_ID:
    case F1A_BRHD_A0_RAW_ID:
    case F1A_BRHD_B0_RAW_ID:
      DEBUG ((DEBUG_VERBOSE, "Turin A/B Detected\n"));
      RomBase = (0xFFFFFFFF - FlashSize) + 1;
      break;
    default:
      RomBase = 0xFC000000;
      break;
  }

  // Determine Bit [24], [25] Address Override Settings
  // SPI ROM Addr[25:24] |= Bit Override Settings
  Value32 = MmioRead32 ((UINTN)(FCH_SPI_BASE_ADDRESS + FCH_SPI_MMIO_REG30));
  if (Value32 & FCH_SPI_R2MSK24) {
    RomPage &= ~FCH_SPI_R2VAL24;
    RomPage |= Value32 & FCH_SPI_R2VAL24;
  }

  if (Value32 & FCH_SPI_R2MSK25) {
    RomPage &= ~FCH_SPI_R2VAL25;
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
  return ((RomPage << 24) - RomBase);
}

/**
  Set the flash offset for Espi boot.

  @param[in]  FlashSize -  Size of physical flash part
  @retval     SPI Offset of current ROM image.
**/
UINT32
SetEspiFlashOffset (
  IN UINT32  FlashSize
  )
{
  return 0;
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
  UINTN       Index;
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
          "%a: Failure: SpiAddress=0x%X VerifyBuffer[0x%X]=0x%X *** FAILED ***\n",
          __func__,
          Address + Index,
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
  *BlockSize      = BLOCK_SIZE;
  *NumberOfBlocks = mNvStorageSize / BLOCK_SIZE - (UINTN)Lba;

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

  if (Offset >= BLOCK_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(Lba=%lX, Offset=%lX, *NumBytes=%lX, Buffer=%lX)\n",
    __func__,
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));
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

  SpiOffset = ((UINT32)mNvStorageLbaOffset + (UINT32)(Lba))
              * BLOCK_SIZE + (UINT32)Offset;
  SpiOffset += mSpiFlashOffset;

  Status = mSpiNorFlashProtocol->ReadData (
                                   mSpiNorFlashProtocol,
                                   SpiOffset,
                                   (UINT32)*NumBytes,
                                   Buffer
                                   );

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

  if (Offset >= BLOCK_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a(Lba=%lX, Offset=%lX, *NumBytes=%lX, Buffer=%lX)\n",
    __func__,
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));
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

  SpiOffset = ((UINT32)mNvStorageLbaOffset + (UINT32)(Lba))
              * BLOCK_SIZE + (UINT32)Offset;
  SpiOffset += mSpiFlashOffset;

  Status = mSpiNorFlashProtocol->WriteData (
                                   mSpiNorFlashProtocol,
                                   SpiOffset,
                                   (UINT32)*NumBytes,
                                   Buffer
                                   );

 #if SPI_FVB_VERIFY
  Status = VerifyWrite (SpiOffset, (UINT32)*NumBytes, Buffer);
 #endif // SPI_FVB_VERIFY

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
  VA_START (Args, This);

  for (Start = VA_ARG (Args, EFI_LBA);
       Start != EFI_LBA_LIST_TERMINATOR;
       Start = VA_ARG (Args, EFI_LBA))
  {
    Length = VA_ARG (Args, UINTN);
    DEBUG ((
      DEBUG_VERBOSE,
      "%a(StartLba=%lX, NumBlocks=%lX)\n",
      __func__,
      Start,
      Length
      ));
    Length *= BLOCK_SIZE;

    SpiOffset  = ((UINT32)Start + (UINT32)mNvStorageLbaOffset) * BLOCK_SIZE;
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
