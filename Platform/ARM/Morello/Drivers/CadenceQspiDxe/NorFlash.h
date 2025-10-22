/** @file

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NOR_FLASH_DXE_H_
#define NOR_FLASH_DXE_H_

#include <Guid/EventGroup.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <PiDxe.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include "CadenceQspiReg.h"

#define NOR_FLASH_ERASE_RETRY  10

#define GET_NOR_BLOCK_ADDRESS(BaseAddr, Lba, LbaSize) \
                                      ((BaseAddr) + (UINTN)((Lba) * (LbaSize)))

#define NOR_FLASH_SIGNATURE  SIGNATURE_32('S', 'n', 'o', 'r')
#define INSTANCE_FROM_FVB_THIS(a)  CR(a, NOR_FLASH_INSTANCE, FvbProtocol,   \
                                        NOR_FLASH_SIGNATURE)

#define NOR_FLASH_POLL_FSR  BIT0

typedef struct _NOR_FLASH_INSTANCE NOR_FLASH_INSTANCE;

typedef EFI_STATUS (*NOR_FLASH_INITIALIZE)        (
  NOR_FLASH_INSTANCE  *Instance
  );

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  UINT8                       Index;
  EFI_DEVICE_PATH_PROTOCOL    End;
} NOR_FLASH_DEVICE_PATH;
#pragma pack()

struct _NOR_FLASH_INSTANCE {
  UINT32                                 Signature;
  EFI_HANDLE                             Handle;

  BOOLEAN                                Initialized;
  NOR_FLASH_INITIALIZE                   Initialize;

  UINTN                                  HostRegisterBaseAddress;
  UINTN                                  DeviceBaseAddress;
  UINTN                                  RegionBaseAddress;
  UINTN                                  Size;
  UINTN                                  BlockSize;
  UINTN                                  LastBlock;
  EFI_LBA                                StartLba;
  EFI_LBA                                OffsetLba;

  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL    FvbProtocol;
  VOID                                   *ShadowBuffer;

  NOR_FLASH_DEVICE_PATH                  DevicePath;

  UINT32                                 Flags;
};

typedef struct {
  EFI_TPL    OriginalTPL;
  BOOLEAN    InterruptsEnabled;
} NOR_FLASH_LOCK_CONTEXT;

/**
  Lock all pending read/write to Nor flash device

  @param[in]     Context     Nor flash device context structure.
**/
VOID
EFIAPI
NorFlashLock (
  IN NOR_FLASH_LOCK_CONTEXT  *Context
  );

/**
  Unlock all pending read/write to Nor flash device

  @param[in]     Context     Nor flash device context structure.
**/
VOID
EFIAPI
NorFlashUnlock (
  IN NOR_FLASH_LOCK_CONTEXT  *Context
  );

extern UINTN  mFlashNvStorageVariableBase;

/**
  Create Nor flash Instance for given region.

  @param[in]    HostRegisterBase      Base address of Nor flash controller.
  @param[in]    NorFlashDeviceBase    Base address of flash device.
  @param[in]    NorFlashRegionBase    Base address of flash region on device.
  @param[in]    NorFlashSize          Size of flash region.
  @param[in]    Index                 Index of given flash region.
  @param[in]    BlockSize             Block size of NOR flash device.
  @param[in]    HasVarStore           Boolean set for VarStore on given region.
  @param[out]   NorFlashInstance      Instance of given flash region.

  @retval       EFI_SUCCESS           On successful creation of NOR flash instance.
**/
EFI_STATUS
NorFlashCreateInstance (
  IN UINTN                HostRegisterBase,
  IN UINTN                NorFlashDeviceBase,
  IN UINTN                NorFlashRegionBase,
  IN UINTN                NorFlashSize,
  IN UINT32               Index,
  IN UINT32               BlockSize,
  IN BOOLEAN              HasVarStore,
  OUT NOR_FLASH_INSTANCE  **NorFlashInstance
  );

/**
  Install Fv block on to variable store region

  @param[in]   Instance         Instance of Nor flash variable region.

  @retval      EFI_SUCCESS      The entry point is executed successfully.
**/
EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE  *Instance
  );

/**
  Check the integrity of firmware volume header.

  @param[in]  Instance        Instance of Nor flash variable region.

  @retval     EFI_SUCCESS     The firmware volume is consistent.
  @retval     EFI_NOT_FOUND   The firmware volume has been corrupted.

**/
EFI_STATUS
ValidateFvHeader (
  IN  NOR_FLASH_INSTANCE  *Instance
  );

/**
  Initialize the FV Header and Variable Store Header
  to support variable operations.

  @param[in]  Instance      Location to Initialize the headers

  @retval     EFI_SUCCESS   Fv init is done

**/
EFI_STATUS
InitializeFvAndVariableStoreHeaders (
  IN NOR_FLASH_INSTANCE  *Instance
  );

/**
 Retrieves the attributes and current settings of the block.

 @param[in]   This         Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param[out]  Attributes   Pointer to EFI_FVB_ATTRIBUTES_2 in which the attributes and
                           current settings are returned.
                           Type EFI_FVB_ATTRIBUTES_2 is defined in
                           EFI_FIRMWARE_VOLUME_HEADER.

 @retval      EFI_SUCCESS  The firmware volume attributes were returned.

**/
EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                 *Attributes
  );

/**
 Sets configurable firmware volume attributes and returns the
 new settings of the firmware volume.


 @param[in]         This                     EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param[in, out]    Attributes               On input, Attributes is a pointer to
                                             EFI_FVB_ATTRIBUTES_2 that contains the desired
                                             firmware volume settings.
                                             On successful return, it contains the new
                                             settings of the firmware volume.

 @retval            EFI_UNSUPPORTED          The firmware volume attributes are not supported.

**/
EFI_STATUS
EFIAPI
FvbSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  );

/**
 Retrieves the base address of a memory-mapped firmware volume.
 This function should be called only for memory-mapped firmware volumes.

 @param[in]     This               EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param[out]    Address            Pointer to a caller-allocated
                                   EFI_PHYSICAL_ADDRESS that, on successful
                                   return from GetPhysicalAddress(), contains the
                                   base address of the firmware volume.

 @retval        EFI_SUCCESS        The firmware volume base address was returned.

**/
EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  );

/**
 Retrieves the size of the requested block.
 It also returns the number of additional blocks with the identical size.
 The GetBlockSize() function is used to retrieve the block map
 (see EFI_FIRMWARE_VOLUME_HEADER).


 @param[in]     This                     EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param[in]     Lba                      Indicates the block whose size to return

 @param[out]    BlockSize                Pointer to a caller-allocated UINTN in which
                                         the size of the block is returned.

 @param[out]    NumberOfBlocks           Pointer to a caller-allocated UINTN in
                                         which the number of consecutive blocks,
                                         starting with Lba, is returned. All
                                         blocks in this range have a size of
                                         BlockSize.

 @retval        EFI_SUCCESS              The firmware volume base address was returned.

 @retval        EFI_INVALID_PARAMETER    The requested LBA is out of range.

**/
EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  );

/**
 Reads the specified number of bytes into a buffer from the specified block.

 The Read() function reads the requested number of bytes from the
 requested block and stores them in the provided buffer.

 @param[in]       This                 EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param[in]       Lba                  The starting logical block index from which to read

 @param[in]       Offset               Offset into the block at which to begin reading.

 @param[in, out]  NumBytes             Pointer to a UINTN.
                                       At entry, *NumBytes contains the total size of the
                                       buffer. *NumBytes should have a non zero value.
                                       At exit, *NumBytes contains the total number of
                                       bytes read.

 @param[in out]   Buffer               Pointer to a caller-allocated buffer that will be
                                       used to hold the data that is read.

 @retval          EFI_SUCCESS          The firmware volume was read successfully, and
                                       contents are in Buffer.

 @retval          EFI_BAD_BUFFER_SIZE  Read attempted across an LBA boundary.

 @retval          EFI_DEVICE_ERROR     The block device is not functioning correctly and
                                       could not be read.

**/
EFI_STATUS
EFIAPI
FvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  );

/**
 Writes the specified number of bytes from the input buffer to the block.

 @param[in]        This                 EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

 @param[in]        Lba                  The starting logical block index to write to.

 @param[in]        Offset               Offset into the block at which to begin writing.

 @param[in, out]   NumBytes             The pointer to a UINTN.
                                        At entry, *NumBytes contains the total size of the
                                        buffer.
                                        At exit, *NumBytes contains the total number of
                                        bytes actually written.

 @param[in]        Buffer               The pointer to a caller-allocated buffer that
                                        contains the source for the write.

 @retval           EFI_SUCCESS          The firmware volume was written successfully.

**/
EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );

/**
 Erases and initialises a firmware volume block.

 @param[in]   This                     EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL

 @param[in]   ...                      The variable argument list is a list of tuples.
                                       Each tuple describes a range of LBAs to erase
                                       and consists of the following:
                                       - An EFI_LBA that indicates the starting LBA
                                       - A UINTN that indicates the number of blocks
                                       to erase.

                                       The list is terminated with an
                                       EFI_LBA_LIST_TERMINATOR.

 @retval      EFI_SUCCESS              The erase request successfully completed.

 @retval      EFI_ACCESS_DENIED        The firmware volume is in the WriteDisabled
                                       state.

 @retval      EFI_DEVICE_ERROR         The block device is not functioning correctly
                                       and could not be written.
                                       The firmware device may have been partially
                                       erased.

 @retval      EFI_INVALID_PARAMETER    One or more of the LBAs listed in the variable
                                       argument list do not exist in the firmware
                                       volume.

**/
EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  );

/**
  This function unlock and erase an entire NOR Flash block.

  @param[in]     Instance       NOR flash Instance of variable store region.
  @param[in]     BlockAddress   Block address within the variable store region.

  @retval        EFI_SUCCESS    The erase and unlock successfully completed.
**/
EFI_STATUS
NorFlashUnlockAndEraseSingleBlock (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  );

/**
  Write a full or portion of a block.

  @param[in]        Instance     NOR flash Instance of variable store region.
  @param[in]        Lba          The starting logical block index to write to.
  @param[in]        Offset       Offset into the block at which to begin writing.
  @param[in,out]    NumBytes     The total size of the buffer.
  @param[in]        Buffer       The pointer to a caller-allocated buffer that
                                 contains the source for the write.

  @retval           EFI_SUCCESS  The write is completed.
**/
EFI_STATUS
NorFlashWriteSingleBlock (
  IN        NOR_FLASH_INSTANCE  *Instance,
  IN        EFI_LBA             Lba,
  IN        UINTN               Offset,
  IN OUT    UINTN               *NumBytes,
  IN        UINT8               *Buffer
  );

/**
  Write a full  block.

  @param[in]    Instance             NOR flash Instance of variable store region.
  @param[in]    Lba                  The starting logical block index to write to.
  @param[in]    BufferSizeInBytes    The number of bytes to write.
  @param[in]    Buffer               The pointer to a caller-allocated buffer that
                                     contains the source for the write.

  @retval       EFI_SUCCESS          The write is completed.
**/
EFI_STATUS
NorFlashWriteBlocks (
  IN  NOR_FLASH_INSTANCE  *Instance,
  IN  EFI_LBA             Lba,
  IN  UINTN               BufferSizeInBytes,
  IN  VOID                *Buffer
  );

/**
  Read a full  block.

  @param[in]     Instance           NOR flash Instance of variable store region.
  @param[in]     Lba                The starting logical block index to read from.
  @param[in]     BufferSizeInBytes  The number of bytes to read.
  @param[out]    Buffer             The pointer to a caller-allocated buffer that
                                    should be copied with read data.

  @retval        EFI_SUCCESS        The read is completed.
**/
EFI_STATUS
NorFlashReadBlocks (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINTN               BufferSizeInBytes,
  OUT VOID               *Buffer
  );

/**
  Read from nor flash.

  @param[in]     Instance           NOR flash Instance of variable store region.
  @param[in]     Lba                The starting logical block index to read from.
  @param[in]     Offset             Offset into the block at which to begin reading.
  @param[in]     BufferSizeInBytes  The number of bytes to read.
  @param[out]    Buffer             The pointer to a caller-allocated buffer that
                                    should copied with read data.

  @retval        EFI_SUCCESS        The read is completed.
**/
EFI_STATUS
NorFlashRead (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINTN               Offset,
  IN UINTN               BufferSizeInBytes,
  OUT VOID               *Buffer
  );

/**
  Read JEDEC ID of NOR flash device.

  @param[in]     Instance     NOR flash Instance of variable store region.
  @param[out]    JedecId      JEDEC ID of NOR flash device.

  @retval        EFI_SUCCESS  The write is completed.
**/
EFI_STATUS
NorFlashReadID (
  IN  NOR_FLASH_INSTANCE  *Instance,
  OUT UINT8               JedecId[3]
  );

#define SPINOR_SR_WIP  BIT0                 // Write in progress

#define SPINOR_OP_WREN   0x06               // Write enable
#define SPINOR_OP_BE_4K  0x20               // Erase 4KiB block
#define SPINOR_OP_RDID   0x9f               // Read JEDEC ID
#define SPINOR_OP_RDSR   0x05               // Read status register

#define SPINOR_SR_WIP_POLL_TIMEOUT_MS  1000u // Status Register read timeout

#endif /* NOR_FLASH_DXE_H_ */
