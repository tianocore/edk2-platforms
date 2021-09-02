/** @file

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NorFlashInfoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "NorFlash.h"

STATIC CONST NOR_FLASH_INSTANCE  mNorFlashInstanceTemplate = {
  NOR_FLASH_SIGNATURE, // Signature
  NULL,                // Handle

  FALSE, // Initialized
  NULL,  // Initialize

  0, // HostRegisterBaseAddress
  0, // DeviceBaseAddress
  0, // RegionBaseAddress
  0, // Size
  0, // BlockSize
  0, // LastBlock
  0, // StartLba
  0, // OffsetLba

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
      },
    },
    0, // Index

    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
  }, // DevicePath
  0  // Flags
};

/**
  Execute Flash cmd ctrl and Read Status.

  @param[in]      Instance         NOR flash Instance.
  @param[in]      Val              Value to be written to Flash cmd ctrl Register.

  @retval         EFI_SUCCESS      Request is executed successfully.

**/
STATIC
EFI_STATUS
CdnsQspiExecuteCommand (
  IN  NOR_FLASH_INSTANCE  *Instance,
  IN  UINT32              Val
  )
{
  // Set the command
  MmioWrite32 (
    Instance->HostRegisterBaseAddress + CDNS_QSPI_FLASH_CMD_CTRL_REG_OFFSET,
    Val
    );
  // Execute the command
  MmioWrite32 (
    Instance->HostRegisterBaseAddress + CDNS_QSPI_FLASH_CMD_CTRL_REG_OFFSET,
    Val | CDNS_QSPI_FLASH_CMD_CTRL_REG_EXECUTE
    );

  // Wait until command has been executed
  while ((MmioRead32 (Instance->HostRegisterBaseAddress + CDNS_QSPI_FLASH_CMD_CTRL_REG_OFFSET)
          & CDNS_QSPI_FLASH_CMD_CTRL_REG_STATUS_BIT) == CDNS_QSPI_FLASH_CMD_CTRL_REG_STATUS_BIT)
  {
    continue;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS          Status;
  NOR_FLASH_INSTANCE  *Instance;
  NOR_FLASH_INFO      *FlashInfo;
  UINT8               JedecId[3];

  ASSERT (NorFlashInstance != NULL);
  Instance = AllocateRuntimeCopyPool (
               sizeof (mNorFlashInstanceTemplate),
               &mNorFlashInstanceTemplate
               );
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->HostRegisterBaseAddress = HostRegisterBase;
  Instance->DeviceBaseAddress       = NorFlashDeviceBase;
  Instance->RegionBaseAddress       = NorFlashRegionBase;
  Instance->Size                    = NorFlashSize;
  Instance->BlockSize               = BlockSize;
  Instance->LastBlock               = (NorFlashSize / BlockSize) - 1;

  Instance->OffsetLba = (NorFlashRegionBase - NorFlashDeviceBase) / BlockSize;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)Index;

  Status = NorFlashReadID (Instance, JedecId);
  if (EFI_ERROR (Status)) {
    goto FreeInstance;
  }

  Status = NorFlashGetInfo (JedecId, &FlashInfo, TRUE);
  if (EFI_ERROR (Status)) {
    goto FreeInstance;
  }

  NorFlashPrintInfo (FlashInfo);

  Instance->Flags = 0;
  if (FlashInfo->Flags & NOR_FLASH_WRITE_FSR) {
    Instance->Flags = NOR_FLASH_POLL_FSR;
  }

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);
  if (Instance->ShadowBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeInstance;
  }

  if (HasVarStore) {
    Instance->Initialize = NorFlashFvbInitialize;
  }

  *NorFlashInstance = Instance;
  FreePool (FlashInfo);
  return EFI_SUCCESS;

FreeInstance:
  FreePool (Instance);
  return Status;
}

/**
  Check whether NOR flash opertions are Locked.

  @param[in]     Instance         NOR flash Instance.
  @param[in]     BlockAddress     BlockAddress in NOR flash device.

  @retval        FALSE            If NOR flash is not locked.
**/
STATIC
BOOLEAN
NorFlashBlockIsLocked (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  )
{
  return FALSE;
}

/**
  Unlock NOR flash operations on given block.

  @param[in]      Instance         NOR flash instance.
  @param[in]      BlockAddress     BlockAddress in NOR flash device.

  @retval         EFI_SUCCESS      NOR flash operations is unlocked.
**/
STATIC
EFI_STATUS
NorFlashUnlockSingleBlock (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  )
{
  return EFI_SUCCESS;
}

/**
  Unlock NOR flash operations if it is necessary.

  @param[in]      Instance         NOR flash instance.
  @param[in]      BlockAddress     BlockAddress in NOR flash device.

  @retval         EFI_SUCCESS      Request is executed successfully.
**/
STATIC
EFI_STATUS
NorFlashUnlockSingleBlockIfNecessary (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (!NorFlashBlockIsLocked (Instance, BlockAddress)) {
    Status = NorFlashUnlockSingleBlock (Instance, BlockAddress);
  }

  return Status;
}

/**
  Enable write to NOR flash device.

  @param[in]      Instance         NOR flash instance.

  @retval         EFI_SUCCESS      Request is executed successfully.
**/
STATIC
EFI_STATUS
NorFlashEnableWrite (
  IN  NOR_FLASH_INSTANCE  *Instance
  )
{
  UINT32  val;

  DEBUG ((DEBUG_INFO, "NorFlashEnableWrite()\n"));
  val = (SPINOR_OP_WREN << CDNS_QSPI_FLASH_CMD_CTRL_REG_OPCODE_BIT_POS);
  if (EFI_ERROR (CdnsQspiExecuteCommand (Instance, val))) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  The following function presumes that the block has already been unlocked.

  @param[in]      Instance         NOR flash instance.
  @param[in]      BlockAddress     Block address within the variable region.

  @retval         EFI_SUCCESS      Request is executed successfully.
 **/
EFI_STATUS
NorFlashEraseSingleBlock (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               BlockAddress
  )
{
  UINT32  DevConfigVal;
  UINT32  EraseOffset;

  EraseOffset = 0x0;

  DEBUG ((
    DEBUG_INFO,
    "NorFlashEraseSingleBlock(BlockAddress=0x%08x)\n",
    BlockAddress
    ));

  if (EFI_ERROR (NorFlashEnableWrite (Instance))) {
    return EFI_DEVICE_ERROR;
  }

  EraseOffset = BlockAddress - Instance->DeviceBaseAddress;

  MmioWrite32 (
    Instance->HostRegisterBaseAddress + CDNS_QSPI_FLASH_CMD_ADDR_REG_OFFSET,
    EraseOffset
    );

  DevConfigVal = SPINOR_OP_BE_4K << CDNS_QSPI_FLASH_CMD_CTRL_REG_OPCODE_BIT_POS |
                 CDNS_QSPI_FLASH_CMD_CTRL_REG_ADDR_ENABLE << CDNS_QSPI_FLASH_CMD_CTRL_REG_ADDR_BIT_POS |
                 CDNS_QSPI_FLASH_CMD_CTRL_REG_ADDR_BYTE_3B << CDNS_QSPI_FLASH_CMD_CTRL_REG_ADDR_BYTE_BIT_POS;

  if (EFI_ERROR (CdnsQspiExecuteCommand (Instance, DevConfigVal))) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  NOR_FLASH_LOCK_CONTEXT  Lock;

  NorFlashLock (&Lock);

  Index = 0;
  do {
    // Unlock the block if we have to
    Status = NorFlashUnlockSingleBlockIfNecessary (Instance, BlockAddress);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = NorFlashEraseSingleBlock (Instance, BlockAddress);
    if (EFI_ERROR (Status)) {
      break;
    }

    Index++;
  } while ((Index < NOR_FLASH_ERASE_RETRY) && (Status == EFI_WRITE_PROTECTED));

  if (Index == NOR_FLASH_ERASE_RETRY) {
    DEBUG ((
      DEBUG_ERROR,
      "EraseSingleBlock(BlockAddress=0x%08x: Block Locked Error (try to erase %d times)\n",
      BlockAddress,
      Index
      ));
  }

  NorFlashUnlock (&Lock);

  return Status;
}

/**
  Write a single word to given location.

  @param[in]    Instance     NOR flash Instance of variable store region.
  @param[in]    WordAddress  The address in NOR flash to write given word.
  @param[in]    WriteData    The data to write into NOR flash location.

  @retval       EFI_SUCCESS  The write is completed.
**/
STATIC
EFI_STATUS
NorFlashWriteSingleWord (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN UINTN               WordAddress,
  IN UINT32              WriteData
  )
{
  DEBUG ((
    DEBUG_INFO,
    "NorFlashWriteSingleWord(WordAddress=0x%08x, WriteData=0x%08x)\n",
    WordAddress,
    WriteData
    ));

  if (EFI_ERROR (NorFlashEnableWrite (Instance))) {
    return EFI_DEVICE_ERROR;
  }

  MmioWrite32 (WordAddress, WriteData);
  return EFI_SUCCESS;
}

/**
  Write a full block to given location.

  @param[in]    Instance           NOR flash Instance of variable store region.
  @param[in]    Lba                The logical block address in NOR flash.
  @param[in]    DataBuffer         The data to write into NOR flash location.
  @param[in]    BlockSizeInWords   The number of bytes to write.

  @retval       EFI_SUCCESS        The write is completed.
**/
STATIC
EFI_STATUS
NorFlashWriteFullBlock (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINT32              *DataBuffer,
  IN UINT32              BlockSizeInWords
  )
{
  EFI_STATUS              Status;
  UINTN                   WordAddress;
  UINT32                  WordIndex;
  UINTN                   BlockAddress;
  NOR_FLASH_LOCK_CONTEXT  Lock;

  Status = EFI_SUCCESS;

  // Get the physical address of the block
  BlockAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   BlockSizeInWords * 4
                   );

  // Start writing from the first address at the start of the block
  WordAddress = BlockAddress;

  NorFlashLock (&Lock);

  Status = NorFlashUnlockAndEraseSingleBlock (Instance, BlockAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "WriteSingleBlock: ERROR - Failed to Unlock and Erase the single block at 0x%X\n",
      BlockAddress
      ));
    goto EXIT;
  }

  for (WordIndex = 0;
       WordIndex < BlockSizeInWords;
       WordIndex++, DataBuffer++, WordAddress += 4)
  {
    Status = NorFlashWriteSingleWord (Instance, WordAddress, *DataBuffer);
    if (EFI_ERROR (Status)) {
      goto EXIT;
    }
  }

EXIT:
  NorFlashUnlock (&Lock);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "NOR FLASH Programming [WriteSingleBlock] failed at address 0x%08x. Exit Status = %r.\n",
      WordAddress,
      Status
      ));
  }

  return Status;
}

/**
  Write a full  block.

  @param[in]    Instance           NOR flash Instance of variable store region.
  @param[in]    Lba                The starting logical block index.
  @param[in]    BufferSizeInBytes  The number of bytes to read.
  @param[in]    Buffer             The pointer to a caller-allocated buffer that
                                   contains the source for the write.

  @retval       EFI_SUCCESS        The write is completed.
**/
EFI_STATUS
NorFlashWriteBlocks (
  IN NOR_FLASH_INSTANCE  *Instance,
  IN EFI_LBA             Lba,
  IN UINTN               BufferSizeInBytes,
  IN VOID                *Buffer
  )
{
  UINT32      *pWriteBuffer;
  EFI_STATUS  Status;
  EFI_LBA     CurrentBlock;
  UINT32      BlockSizeInWords;
  UINT32      NumBlocks;
  UINT32      BlockCount;

  Status = EFI_SUCCESS;
  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We must have some bytes to read
  DEBUG ((
    DEBUG_INFO,
    "NorFlashWriteBlocks: BufferSizeInBytes=0x%x\n",
    BufferSizeInBytes
    ));
  if (BufferSizeInBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // The size of the buffer must be a multiple of the block size
  DEBUG ((
    DEBUG_INFO,
    "NorFlashWriteBlocks: BlockSize in bytes =0x%x\n",
    Instance->BlockSize
    ));
  if ((BufferSizeInBytes % Instance->BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // All blocks must be within the device
  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->BlockSize;

  DEBUG ((
    DEBUG_INFO,
    "NorFlashWriteBlocks: NumBlocks=%d, LastBlock=%ld, Lba=%ld.\n",
    NumBlocks,
    Instance->LastBlock,
    Lba
    ));

  if ((Lba + NumBlocks) > (Instance->LastBlock + 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "NorFlashWriteBlocks: ERROR - Write will exceed last block.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (((UINTN)Buffer % sizeof (UINT32)) == 0);

  BlockSizeInWords = Instance->BlockSize / 4;

  // Because the target *Buffer is a pointer to VOID, we must put
  // all the data into a pointer to a proper data type, so use *ReadBuffer
  pWriteBuffer = (UINT32 *)Buffer;

  CurrentBlock = Lba;
  for (BlockCount = 0;
       BlockCount < NumBlocks;
       BlockCount++, CurrentBlock++, pWriteBuffer += BlockSizeInWords)
  {
    DEBUG ((
      DEBUG_INFO,
      "NorFlashWriteBlocks: Writing block #%d\n",
      (UINTN)CurrentBlock
      ));

    Status = NorFlashWriteFullBlock (
               Instance,
               CurrentBlock,
               pWriteBuffer,
               BlockSizeInWords
               );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  DEBUG ((DEBUG_INFO, "NorFlashWriteBlocks: Exit Status = %r.\n", Status));
  return Status;
}

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
  )
{
  UINT32  NumBlocks;
  UINTN   StartAddress;

  DEBUG ((
    DEBUG_INFO,
    "NorFlashReadBlocks: BufferSize=0x%xB BlockSize=0x%xB LastBlock=%ld, Lba=%ld.\n",
    BufferSizeInBytes,
    Instance->BlockSize,
    Instance->LastBlock,
    Lba
    ));

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Return if we do not have any byte to read
  if (BufferSizeInBytes == 0) {
    return EFI_SUCCESS;
  }

  // The size of the buffer must be a multiple of the block size
  if ((BufferSizeInBytes % Instance->BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumBlocks = ((UINT32)BufferSizeInBytes) / Instance->BlockSize;

  if ((Lba + NumBlocks) > (Instance->LastBlock + 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "NorFlashReadBlocks: ERROR - Read will exceed last block\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   Instance->BlockSize
                   );

  // Readout the data
  CopyMem (Buffer, (UINTN *)StartAddress, BufferSizeInBytes);

  return EFI_SUCCESS;
}

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
  )
{
  UINTN  StartAddress;

  // The buffer must be valid
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Return if we do not have any byte to read
  if (BufferSizeInBytes == 0) {
    return EFI_SUCCESS;
  }

  if (((Lba * Instance->BlockSize) + Offset + BufferSizeInBytes) >
      Instance->Size)
  {
    DEBUG ((
      DEBUG_ERROR,
      "NorFlashRead: ERROR - Read will exceed device size.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Get the address to start reading from
  StartAddress = GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   Instance->BlockSize
                   );

  // Readout the data
  CopyMem (Buffer, (UINTN *)(StartAddress + Offset), BufferSizeInBytes);

  return EFI_SUCCESS;
}

/**
  Write a full or portion of a block.

  @param[in]         Instance     NOR flash Instance of variable store region.
  @param[in]         Lba          The starting logical block index to write to.
  @param[in]         Offset       Offset into the block at which to begin writing.
  @param[in, out]    NumBytes     The total size of the buffer.
  @param[in]         Buffer       The pointer to a caller-allocated buffer that
                                  contains the source for the write.

  @retval            EFI_SUCCESS  The write is completed.
**/
EFI_STATUS
NorFlashWriteSingleBlock (
  IN        NOR_FLASH_INSTANCE  *Instance,
  IN        EFI_LBA             Lba,
  IN        UINTN               Offset,
  IN OUT    UINTN               *NumBytes,
  IN        UINT8               *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      Tmp;
  UINT32      TmpBuf;
  UINT32      WordToWrite;
  UINT32      Mask;
  BOOLEAN     DoErase;
  UINTN       BytesToWrite;
  UINTN       CurOffset;
  UINTN       WordAddr;
  UINTN       BlockSize;
  UINTN       BlockAddress;
  UINTN       PrevBlockAddress;

  if (Buffer == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "NorFlashWriteSingleBlock: ERROR - Buffer is invalid\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  PrevBlockAddress = 0;
  if (!Instance->Initialized && Instance->Initialize) {
    Instance->Initialize (Instance);
  }

  DEBUG ((
    DEBUG_INFO,
    "NorFlashWriteSingleBlock(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%08x)\n",
    Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  // Cache the block size to avoid de-referencing pointers all the time
  BlockSize = Instance->BlockSize;

  // The write must not span block boundaries.
  // We need to check each variable individually because adding two large
  // values together overflows.
  if ((Offset               >= BlockSize) ||
      (*NumBytes            >  BlockSize) ||
      ((Offset + *NumBytes) >  BlockSize))
  {
    DEBUG ((
      DEBUG_ERROR,
      "NorFlashWriteSingleBlock: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n",
      Offset,
      *NumBytes,
      BlockSize
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  // We must have some bytes to write
  if (*NumBytes == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "NorFlashWriteSingleBlock: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n",
      Offset,
      *NumBytes,
      BlockSize
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  // Pick 128bytes as a good start for word operations as opposed to erasing the
  // block and writing the data regardless if an erase is really needed.
  // It looks like most individual NV variable writes are smaller than 128bytes.
  if (*NumBytes <= 128) {
    // Check to see if we need to erase before programming the data into NOR.
    // If the destination bits are only changing from 1s to 0s we can just write.
    // After a block is erased all bits in the block is set to 1.
    // If any byte requires us to erase we just give up and rewrite all of it.
    DoErase      = FALSE;
    BytesToWrite = *NumBytes;
    CurOffset    = Offset;

    while (BytesToWrite > 0) {
      // Read full word from NOR, splice as required. A word is the smallest
      // unit we can write.
      Status = NorFlashRead (
                 Instance,
                 Lba,
                 CurOffset & ~(0x3),
                 sizeof (Tmp),
                 &Tmp
                 );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      // Physical address of word in NOR to write.
      WordAddr = (CurOffset & ~(0x3)) +
                 GET_NOR_BLOCK_ADDRESS (
                   Instance->RegionBaseAddress,
                   Lba,
                   BlockSize
                   );

      // The word of data that is to be written.
      TmpBuf = ReadUnaligned32 ((UINT32 *)(Buffer + (*NumBytes - BytesToWrite)));

      // First do word aligned chunks.
      if ((CurOffset & 0x3) == 0) {
        if (BytesToWrite >= 4) {
          // Is the destination still in 'erased' state?
          if (~Tmp != 0) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Write this word to NOR
          WordToWrite   = TmpBuf;
          CurOffset    += sizeof (TmpBuf);
          BytesToWrite -= sizeof (TmpBuf);
        } else {
          // BytesToWrite < 4. Do small writes and left-overs
          Mask = ~((~0) << (BytesToWrite * 8));
          // Mask out the bytes we want.
          TmpBuf &= Mask;
          // Is the destination still in 'erased' state?
          if ((Tmp & Mask) != Mask) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Merge old and new data. Write merged word to NOR
          WordToWrite  = (Tmp & ~Mask) | TmpBuf;
          CurOffset   += BytesToWrite;
          BytesToWrite = 0;
        }
      } else {
        // Do multiple words, but starting unaligned.
        if (BytesToWrite > (4 - (CurOffset & 0x3))) {
          Mask = ((~0) << ((CurOffset & 0x3) * 8));
          // Mask out the bytes we want.
          TmpBuf &= Mask;
          // Is the destination still in 'erased' state?
          if ((Tmp & Mask) != Mask) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Merge old and new data. Write merged word to NOR
          WordToWrite   = (Tmp & ~Mask) | TmpBuf;
          BytesToWrite -= (4 - (CurOffset & 0x3));
          CurOffset    += (4 - (CurOffset & 0x3));
        } else {
          // Unaligned and fits in one word.
          Mask = (~((~0) << (BytesToWrite * 8))) << ((CurOffset & 0x3) * 8);
          // Mask out the bytes we want.
          TmpBuf = (TmpBuf << ((CurOffset & 0x3) * 8)) & Mask;
          // Is the destination still in 'erased' state?
          if ((Tmp & Mask) != Mask) {
            // Check to see if we are only changing bits to zero.
            if ((Tmp ^ TmpBuf) & TmpBuf) {
              DoErase = TRUE;
              break;
            }
          }

          // Merge old and new data. Write merged word to NOR
          WordToWrite  = (Tmp & ~Mask) | TmpBuf;
          CurOffset   += BytesToWrite;
          BytesToWrite = 0;
        }
      }

      BlockAddress = GET_NOR_BLOCK_ADDRESS (
                       Instance->RegionBaseAddress,
                       Lba,
                       BlockSize
                       );
      if (BlockAddress != PrevBlockAddress) {
        Status = NorFlashUnlockSingleBlockIfNecessary (Instance, BlockAddress);
        if (EFI_ERROR (Status)) {
          return EFI_DEVICE_ERROR;
        }

        PrevBlockAddress = BlockAddress;
      }

      Status = NorFlashWriteSingleWord (Instance, WordAddr, WordToWrite);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    // Exit if we got here and could write all the data. Otherwise do the
    // Erase-Write cycle.
    if (!DoErase) {
      return EFI_SUCCESS;
    }
  }

  // Check we did get some memory. Buffer is BlockSize.
  if (Instance->ShadowBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "FvbWrite: ERROR - Buffer not ready\n"));
    return EFI_DEVICE_ERROR;
  }

  // Read NOR Flash data into shadow buffer
  Status = NorFlashReadBlocks (
             Instance,
             Lba,
             BlockSize,
             Instance->ShadowBuffer
             );
  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  // Put the data at the appropriate location inside the buffer area
  CopyMem ((VOID *)((UINTN)Instance->ShadowBuffer + Offset), Buffer, *NumBytes);

  // Write the modified buffer back to the NorFlash
  Status = NorFlashWriteBlocks (
             Instance,
             Lba,
             BlockSize,
             Instance->ShadowBuffer
             );
  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  UINT32  val;

  if ((Instance == NULL) || (JedecId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  val = SPINOR_OP_RDID << CDNS_QSPI_FLASH_CMD_CTRL_REG_OPCODE_BIT_POS |
        CDNS_QSPI_FLASH_CMD_CTRL_REG_READ_ENABLE << CDNS_QSPI_FLASH_CMD_CTRL_REG_READEN_BIT_POS |
        CDNS_QSPI_FLASH_CMD_CTRL_REG_ADDR_BYTE_3B << CDNS_QSPI_FLASH_CMD_CTRL_REG_READBYTE_BIT_POS;

  if (EFI_ERROR (CdnsQspiExecuteCommand (Instance, val))) {
    return EFI_DEVICE_ERROR;
  }

  val = MmioRead32 (Instance->HostRegisterBaseAddress + CDNS_QSPI_FLASH_CMD_READ_DATA_REG_OFFSET);

  // Manu.ID field
  JedecId[0] = (UINT8)val;
  // Type field
  JedecId[1] = (UINT8)(val >> 8);
  // Capacity field
  JedecId[2] = (UINT8)(val >> 16);

  DEBUG ((
    DEBUG_INFO,
    "Nor flash detected, Jedec ID, Manu.Id=%x Type=%x Capacity=%x \n",
    JedecId[0],
    JedecId[1],
    JedecId[2]
    ));

  return EFI_SUCCESS;
}
