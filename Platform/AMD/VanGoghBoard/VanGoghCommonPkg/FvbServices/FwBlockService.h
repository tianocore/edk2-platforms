/** @file
  Implements FvbServicesSmm
  Firmware volume block driver for SPI device

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013-2015 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FW_BLOCK_SERVICE_SMM_H_
#define FW_BLOCK_SERVICE_SMM_H_

//
// The Library classes this module consumes
//
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>

#include <Protocol/Spi.h>
#include <Protocol/SpiCommon.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Pi/PiFirmwareVolume.h>

#define FVB_PHYSICAL  0
#define FVB_VIRTUAL   1

typedef struct {
  EFI_LOCK                      FvbDevLock;
  UINTN                         FvBase[2];
  UINTN                         NumOfBlocks;
  EFI_FIRMWARE_VOLUME_HEADER    VolumeHeader;
} EFI_FW_VOL_INSTANCE;

typedef struct {
  UINT32                 NumFv;
  EFI_FW_VOL_INSTANCE    *FvInstance[2];
  UINT8                  *FvbScratchSpace[2];
  EFI_SPI_PROTOCOL       *SpiProtocol;
} ESAL_FWB_GLOBAL;

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a)         CR (a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_EXTEND_DEVICE_FROM_THIS(a)  CR (a, EFI_FW_VOL_BLOCK_DEVICE, FvbExtension, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE  SIGNATURE_32 ('F', 'V', 'B', 'N')

typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH    FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
  UINTN                                 Signature;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  UINTN                                 Instance;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

/**
  Get Fvb information.

  @param[in] BaseAddress    The base address compare with NvStorageVariable base address.
  @param[out] FvbInfo        Fvb information.

  @retval EFI_SUCCESS       Get Fvb information successfully.
  @retval EFI_NOT_FOUND     Not find Fvb information.

**/
EFI_STATUS
GetFvbInfo (
  IN  UINT64                      FvLength,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FvbInfo
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Retrieves the physical address of the device.

  @param[in]   This        Calling context
  @param[out]  Address     Output buffer containing the address.

  @retval EFI_SUCCESS  Successfully returns

**/
EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                  Instance,
  OUT EFI_PHYSICAL_ADDRESS  *Address,
  IN ESAL_FWB_GLOBAL        *Global,
  IN BOOLEAN                Virtual
  );

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
  );

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
  );

/**
  Retrieves Volume attributes.  No polarity translations are done.

  @param[in]     This                 Calling context
  @param[out]    Attributes           Output buffer which contains attributes

  @retval  EFI_SUCCESS            Successfully returns

**/
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_FVB_ATTRIBUTES_2                     *Attributes
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif
