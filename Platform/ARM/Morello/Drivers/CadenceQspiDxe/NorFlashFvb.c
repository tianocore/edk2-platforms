/** @file

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <PiDxe.h>

#include "NorFlash.h"

UINTN  mFlashNvStorageVariableBase;

/**
  Initialize the FV Header and Variable Store Header
  to support variable operations.

  @param[in]  Instance      Location to initialise the headers.

  @retval     EFI_SUCCESS   Fv init is done.

**/
EFI_STATUS
InitializeFvAndVariableStoreHeaders (
  IN NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS                  Status;
  VOID                        *Headers;
  UINTN                       HeadersLength;
  EFI_FIRMWARE_VOLUME_HEADER  *FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;

  if (!Instance->Initialized && Instance->Initialize) {
    Instance->Initialize (Instance);
  }

  HeadersLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) +
                  sizeof (EFI_FV_BLOCK_MAP_ENTRY) +
                  sizeof (VARIABLE_STORE_HEADER);
  Headers = AllocateZeroPool (HeadersLength);

  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Headers;
  CopyGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
  FirmwareVolumeHeader->FvLength =
    PcdGet32 (PcdFlashNvStorageVariableSize) +
    PcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
    PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  FirmwareVolumeHeader->Signature  = EFI_FVH_SIGNATURE;
  FirmwareVolumeHeader->Attributes = EFI_FVB2_READ_ENABLED_CAP |
                                     EFI_FVB2_READ_STATUS |
                                     EFI_FVB2_STICKY_WRITE |
                                     EFI_FVB2_MEMORY_MAPPED |
                                     EFI_FVB2_ERASE_POLARITY |
                                     EFI_FVB2_WRITE_STATUS |
                                     EFI_FVB2_WRITE_ENABLED_CAP;

  FirmwareVolumeHeader->HeaderLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) +
                                       sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  FirmwareVolumeHeader->Revision              = EFI_FVH_REVISION;
  FirmwareVolumeHeader->BlockMap[0].NumBlocks = Instance->LastBlock + 1;
  FirmwareVolumeHeader->BlockMap[0].Length    = Instance->BlockSize;
  FirmwareVolumeHeader->BlockMap[1].NumBlocks = 0;
  FirmwareVolumeHeader->BlockMap[1].Length    = 0;
  FirmwareVolumeHeader->Checksum              = CalculateCheckSum16 (
                                                  (UINT16 *)FirmwareVolumeHeader,
                                                  FirmwareVolumeHeader->HeaderLength
                                                  );

  VariableStoreHeader = (VOID *)((UINTN)Headers +
                                 FirmwareVolumeHeader->HeaderLength);
  CopyGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
  VariableStoreHeader->Size = PcdGet32 (PcdFlashNvStorageVariableSize) -
                              FirmwareVolumeHeader->HeaderLength;
  VariableStoreHeader->Format = VARIABLE_STORE_FORMATTED;
  VariableStoreHeader->State  = VARIABLE_STORE_HEALTHY;

  // Install the combined super-header in the NorFlash
  Status = FvbWrite (&Instance->FvbProtocol, 0, 0, &HeadersLength, Headers);

  FreePool (Headers);
  return Status;
}

/**
  Check the integrity of firmware volume header.

  @param[in]  Instance        Instance of Nor flash variable region.

  @retval     EFI_SUCCESS     The firmware volume is consistent.
  @retval     EFI_NOT_FOUND   The firmware volume has been corrupted.

**/
EFI_STATUS
ValidateFvHeader (
  IN  NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINTN                       VariableStoreLength;
  UINTN                       FvLength;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Instance->RegionBaseAddress;

  FvLength = PcdGet32 (PcdFlashNvStorageVariableSize) +
             PcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
             PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  if (  (FwVolHeader->Revision  != EFI_FVH_REVISION)
     || (FwVolHeader->Signature != EFI_FVH_SIGNATURE)
     || (FwVolHeader->FvLength  != FvLength)
        )
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Firmware Volume header present\n",
      __FUNCTION__
      ));
    return EFI_NOT_FOUND;
  }

  // Check the Firmware Volume Guid
  if (!CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Firmware Volume Guid non-compatible\n",
      __FUNCTION__
      ));
    return EFI_NOT_FOUND;
  }

  VariableStoreHeader = (VOID *)((UINTN)FwVolHeader +
                                 FwVolHeader->HeaderLength);

  // Check the Variable Store Guid
  if (!CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) &&
      !CompareGuid (
         &VariableStoreHeader->Signature,
         &gEfiAuthenticatedVariableGuid
         ))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Variable Store Guid non-compatible\n",
      __FUNCTION__
      ));
    return EFI_NOT_FOUND;
  }

  VariableStoreLength = PcdGet32 (PcdFlashNvStorageVariableSize) -
                        FwVolHeader->HeaderLength;
  if (VariableStoreHeader->Size != VariableStoreLength) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Variable Store Length does not match\n",
      __FUNCTION__
      ));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_FVB_ATTRIBUTES_2  FlashFvbAttributes;
  NOR_FLASH_INSTANCE    *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  FlashFvbAttributes = EFI_FVB2_READ_ENABLED_CAP | EFI_FVB2_READ_STATUS |
                       EFI_FVB2_WRITE_ENABLED_CAP | EFI_FVB2_WRITE_STATUS |
                       EFI_FVB2_STICKY_WRITE | EFI_FVB2_MEMORY_MAPPED |
                       EFI_FVB2_ERASE_POLARITY;

  *Attributes = FlashFvbAttributes;

  DEBUG ((DEBUG_INFO, "FvbGetAttributes(0x%X)\n", *Attributes));

  return EFI_SUCCESS;
}

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
  )
{
  DEBUG ((
    DEBUG_INFO,
    "FvbSetAttributes(0x%X) is not supported\n",
    *Attributes
    ));
  return EFI_UNSUPPORTED;
}

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
  )
{
  NOR_FLASH_INSTANCE  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_INFO,
    "FvbGetPhysicalAddress(BaseAddress=0x%08x)\n",
    Instance->RegionBaseAddress
    ));

  ASSERT (Address != NULL);

  *Address = Instance->RegionBaseAddress;
  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS          Status;
  NOR_FLASH_INSTANCE  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_INFO,
    "FvbGetBlockSize(Lba=%ld, BlockSize=0x%x, LastBlock=%ld)\n",
    Lba,
    Instance->BlockSize,
    Instance->LastBlock
    ));

  if (Lba > Instance->LastBlock) {
    DEBUG ((
      DEBUG_ERROR,
      "FvbGetBlockSize: ERROR - Parameter LBA %ld is beyond the last Lba (%ld).\n",
      Lba,
      Instance->LastBlock
      ));
    Status = EFI_INVALID_PARAMETER;
  } else {
    // This is easy because in this platform each NorFlash device has equal sized blocks.
    *BlockSize      = (UINTN)Instance->BlockSize;
    *NumberOfBlocks = (UINTN)(Instance->LastBlock - Lba + 1);

    DEBUG ((
      DEBUG_INFO,
      "FvbGetBlockSize: *BlockSize=0x%x, *NumberOfBlocks=0x%x.\n",
      *BlockSize,
      *NumberOfBlocks
      ));

    Status = EFI_SUCCESS;
  }

  return Status;
}

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

 @param[in, out]  Buffer               Pointer to a caller-allocated buffer that will be
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
  )
{
  EFI_STATUS          Status;
  UINTN               BlockSize;
  NOR_FLASH_INSTANCE  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((
    DEBUG_INFO,
    "FvbRead(Parameters: Lba=%ld, Offset=0x%x, *NumBytes=0x%x, Buffer @ 0x%08x)\n",
    Instance->StartLba + Lba,
    Offset,
    *NumBytes,
    Buffer
    ));

  if (!Instance->Initialized && Instance->Initialize) {
    Instance->Initialize (Instance);
  }

  BlockSize = Instance->BlockSize;

  DEBUG ((
    DEBUG_INFO,
    "FvbRead: Check if (Offset=0x%x + NumBytes=0x%x) <= BlockSize=0x%x\n",
    Offset,
    *NumBytes,
    BlockSize
    ));

  // The read must not span block boundaries.
  // We need to check each variable individually because adding two large
  // values together overflows.
  if ((Offset               >= BlockSize) ||
      (*NumBytes            >  BlockSize) ||
      ((Offset + *NumBytes) >  BlockSize))
  {
    DEBUG ((
      DEBUG_ERROR,
      "FvbRead: ERROR - EFI_BAD_BUFFER_SIZE: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n",
      Offset,
      *NumBytes,
      BlockSize
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  // We must have some bytes to read
  if (*NumBytes == 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  // Decide if we are doing full block reads or not.
  if (*NumBytes % BlockSize != 0) {
    Status = NorFlashRead (
               Instance,
               Instance->StartLba + Lba,
               Offset,
               *NumBytes,
               Buffer
               );
  } else {
    // Read NOR Flash data into shadow buffer
    Status = NorFlashReadBlocks (
               Instance,
               Instance->StartLba + Lba,
               BlockSize,
               Buffer
               );
  }

  if (EFI_ERROR (Status)) {
    // Return one of the pre-approved error statuses
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  NOR_FLASH_INSTANCE  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  return NorFlashWriteSingleBlock (
           Instance,
           Instance->StartLba + Lba,
           Offset,
           NumBytes,
           Buffer
           );
}

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

 @retval      EFI_INVALID_PARAMETER   One or more of the LBAs listed in the variable
                                      argument list do not exist in the firmware
                                      volume.

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
  NOR_FLASH_INSTANCE  *Instance;

  Instance = INSTANCE_FROM_FVB_THIS (This);

  DEBUG ((DEBUG_INFO, "FvbEraseBlocks()\n"));

  Status = EFI_SUCCESS;

  // Before erasing, check the entire list of parameters to ensure
  // all specified blocks are valid

  VA_START (Args, This);
  do {
    // Get the Lba from which we start erasing
    StartingLba = VA_ARG (Args, EFI_LBA);

    // Have we reached the end of the list?
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    // How many Lba blocks are we requested to erase?
    NumOfLba = VA_ARG (Args, UINT32);

    // All blocks must be within range
    DEBUG ((
      DEBUG_INFO,
      "FvbEraseBlocks: Check if: ( StartingLba=%ld + NumOfLba=%d - 1 ) > LastBlock=%ld.\n",
      Instance->StartLba + StartingLba,
      NumOfLba,
      Instance->LastBlock
      ));
    if ((NumOfLba == 0) ||
        ((Instance->StartLba + StartingLba + NumOfLba - 1) >
         Instance->LastBlock))
    {
      VA_END (Args);
      DEBUG ((
        DEBUG_ERROR,
        "FvbEraseBlocks: ERROR - Lba range goes past the last Lba.\n"
        ));
      return EFI_INVALID_PARAMETER;
    }
  } while (TRUE);

  VA_END (Args);

  VA_START (Args, This);
  do {
    // Get the Lba from which we start erasing
    StartingLba = VA_ARG (Args, EFI_LBA);

    // Have we reached the end of the list?
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      // Exit the while loop
      break;
    }

    // How many Lba blocks are we requested to erase?
    NumOfLba = VA_ARG (Args, UINT32);

    // Go through each one and erase it
    while (NumOfLba > 0) {
      // Get the physical address of Lba to erase
      BlockAddress = GET_NOR_BLOCK_ADDRESS (
                       Instance->RegionBaseAddress,
                       Instance->StartLba + StartingLba,
                       Instance->BlockSize
                       );

      // Erase it
      DEBUG ((
        DEBUG_INFO,
        "FvbEraseBlocks: Erasing Lba=%ld @ 0x%08x.\n",
        Instance->StartLba + StartingLba,
        BlockAddress
        ));
      Status = NorFlashUnlockAndEraseSingleBlock (Instance, BlockAddress);
      if (EFI_ERROR (Status)) {
        VA_END (Args);
        return EFI_DEVICE_ERROR;
      }

      // Move to the next Lba
      StartingLba++;
      NumOfLba--;
    }
  } while (TRUE);

  VA_END (Args);

  return Status;
}
