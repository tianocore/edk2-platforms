/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - GUID Partition Table (GPT) Disk Layout
    (https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html)

**/

#include <PiMm.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/MmServicesTableLib.h>

#include <Protocol/BlockIo.h>

#include "DiskGpt.h"

/**
  Calculate the extra size required to align a total size to a block boundary.

  @param[in] TotalSize  The current total size in bytes.
  @param[in] BlockSize  The block size to align to (must be non-zero).

  @retval The number of extra bytes required for alignment.

**/
#define GET_EXTRA_BLOCK_ALIGN(TotalSize, BlockSize) \
  (BlockSize - (TotalSize % BlockSize))

/**
  Retrieve the GPT partition header.

  @param[in]  GptHandle  GptHandle.
  @param[out] Header     A pointer to the buffer where the GPT header is written to.

  @retval EFI_SUCCESS  The GPT header was successfully written to Header.
  @retval Error        The GPT header was not correctly read.

**/
STATIC
EFI_STATUS
EFIAPI
GetGptPartitionHeader (
  IN     GPT_PARTITION_HANDLE        *GptHandle,
  OUT    EFI_PARTITION_TABLE_HEADER  *Header
  )
{
  EFI_STATUS  Status;

  if ((GptHandle == NULL) || (GptHandle->Instance == NULL) || (Header == NULL)) {
    DEBUG ((DEBUG_ERROR, "GetGptPartitionHeader: Invalid parameters\n"));
    return EFI_INVALID_PARAMETER;
  }

  Status = GptReadPartition (
             GptHandle,
             Header,
             sizeof (EFI_PARTITION_TABLE_HEADER),
             GPT_PROTECTIVE_MBR_SIZE,
             0
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetGptPartitionHeader: Failed to read GPT partition info\n"));
  }

  return Status;
}

/**
  Print the contents of the GPT header.

  @param[in]  Header   The GPT header to be printed.

**/
STATIC
VOID
EFIAPI
PrintGptHeader (
  EFI_PARTITION_TABLE_HEADER  *Header
  )
{
  DEBUG ((DEBUG_BLKIO, "--GPT Header--------------------------\n"));
  DEBUG ((DEBUG_BLKIO, "Signature %x\n", Header->Header.Signature));
  DEBUG ((DEBUG_BLKIO, "Revision %x\n", Header->Header.Revision));
  DEBUG ((DEBUG_BLKIO, "Header Lba %x\n", Header->MyLBA));
  DEBUG ((DEBUG_BLKIO, "Disk GUID %g\n", &Header->DiskGUID));
  DEBUG ((DEBUG_BLKIO, "Partition entry Lba %x\n", Header->PartitionEntryLBA));
  DEBUG ((DEBUG_BLKIO, "Num partition entries %x\n", Header->NumberOfPartitionEntries));
  DEBUG ((DEBUG_BLKIO, "Size of partition entry %x\n", Header->SizeOfPartitionEntry));
  DEBUG ((DEBUG_BLKIO, "--------------------------------------\n"));
}

/**
  Read "ReadSize" bytes from a partition at "ReadOffset" bytes from its start.

  @param[in]  GptHandle  GptHandle.
  @param[out] DestBuffer Pointer to the buffer that will store the data read.
  @param[in]  ReadSize   The number of bytes to read.
  @param[in]  ReadOffset The offset to read from within the partition.
  @param[in]  BaseLba    The start Lba of the partition.

  @retval EFI_SUCCESS  ReadSize bytes read successfully.
  @retval Error        No bytes read.

**/
EFI_STATUS
EFIAPI
GptReadPartition (
  IN     GPT_PARTITION_HANDLE  *GptHandle,
  OUT    VOID                  *DestBuffer,
  IN     UINTN                 ReadSize,
  IN     UINTN                 ReadOffset,
  IN     UINTN                 BaseLba
  )
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL  *Instance;
  EFI_BLOCK_IO_MEDIA     *Media;
  UINTN                  ReadAlignment;
  UINTN                  AuxBufferSize;
  VOID                   *AuxBuffer;
  UINTN                  Lba;
  UINTN                  GptOffset;         // Read data offset based on GPT block size
  UINTN                  DeviceBlockOffset; // Read data offset in the device block

  if ((GptHandle == NULL) || (GptHandle->Instance == NULL) ||
      (DestBuffer == NULL) || (ReadSize == 0))
  {
    DEBUG ((DEBUG_ERROR, "GptReadPartition: Invalid parameters\n"));
    return EFI_INVALID_PARAMETER;
  }

  Instance = GptHandle->Instance;
  Media    = Instance->Media;

  GptOffset         = ((BaseLba * GPT_PARTITION_LBA_SIZE) + ReadOffset);
  Lba               = GptOffset / Media->BlockSize;
  DeviceBlockOffset = GptOffset - (Lba * Media->BlockSize);
  ReadAlignment     = GET_EXTRA_BLOCK_ALIGN ((DeviceBlockOffset + ReadSize), Media->BlockSize);
  AuxBufferSize     = DeviceBlockOffset + ReadSize + ReadAlignment;

  Status = gMmst->MmAllocatePool (EfiRuntimeServicesData, AuxBufferSize, (VOID **)&AuxBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Not enough resources to complete read request\n"));
    return Status;
  }

  // read from flash
  Status = Instance->ReadBlocks (
                       Instance,
                       Media->MediaId,
                       Lba,
                       AuxBufferSize,
                       AuxBuffer
                       );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "GptReadPartition: Flash read failed Lba %x writeSize %x\n",
      Lba,
      AuxBufferSize
      ));
    goto ErrorHandler;
  }

  CopyMem (DestBuffer, AuxBuffer + DeviceBlockOffset, ReadSize);

ErrorHandler:
  gMmst->MmFreePool (AuxBuffer);
  return Status;
}

/**
  Write "BufferSize" bytes to a partition at "WriteOffset" bytes from its start.

  @param[in]     GptHandle  GptHandle.
  @param[in]     SrcBuffer   Pointer to the buffer with the data to be written.
  @param[in]     BufferSize  The number of bytes to write.
  @param[in]     WriteOffset The offset to write to within the partition.
  @param[in]     BaseLba     The start Lba of the partition.

  @retval EFI_SUCCESS  BufferSize bytes written successfully.
  @retval Error        No bytes written.

**/
EFI_STATUS
EFIAPI
GptWritePartition (
  IN  GPT_PARTITION_HANDLE  *GptHandle,
  IN  VOID                  *SrcBuffer,
  IN  UINTN                 BufferSize,
  IN  UINTN                 WriteOffset,
  IN  UINTN                 BaseLba
  )
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL  *Instance;
  EFI_BLOCK_IO_MEDIA     *Media;
  UINTN                  WriteAlignment;
  UINTN                  AuxBufferSize;
  VOID                   *AuxBuffer;
  UINTN                  Lba;
  UINTN                  GptOffset;         // Write data offset based on GPT block size
  UINTN                  DeviceBlockOffset; // Write data offset in the device block

  if ((GptHandle == NULL) || (GptHandle->Instance == NULL) ||
      (SrcBuffer == NULL) || (BufferSize == 0))
  {
    DEBUG ((DEBUG_INFO, "GptWritePartition: Invalid parameters\n"));
    return EFI_INVALID_PARAMETER;
  }

  Instance = GptHandle->Instance;
  Media    = Instance->Media;

  GptOffset         = ((BaseLba * GPT_PARTITION_LBA_SIZE) + WriteOffset);
  Lba               = GptOffset / Media->BlockSize;
  DeviceBlockOffset = GptOffset - (Lba * Media->BlockSize);
  WriteAlignment    = GET_EXTRA_BLOCK_ALIGN ((DeviceBlockOffset + BufferSize), Media->BlockSize);
  AuxBufferSize     = DeviceBlockOffset + BufferSize + WriteAlignment;

  Status = gMmst->MmAllocatePool (EfiRuntimeServicesData, AuxBufferSize, (VOID **)&AuxBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Not enough resources to complete write request\n"));

    return Status;
  }

  // read from flash
  Status = Instance->ReadBlocks (
                       Instance,
                       Media->MediaId,
                       Lba,
                       AuxBufferSize,
                       AuxBuffer
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "GptWritePartition: Flash read failed Lba %x writeSize %x\n",
      Lba,
      AuxBufferSize
      ));

    goto ErrorHandler;
  }

  // Apply the changes to the aux buffer, commit these to flash.
  CopyMem (AuxBuffer + DeviceBlockOffset, SrcBuffer, BufferSize);

  // Write to flash
  Status = Instance->WriteBlocks (
                       Instance,
                       Media->MediaId,
                       Lba,
                       AuxBufferSize,
                       AuxBuffer
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GptWritePartition: Flash write failed Lba %x writeSize %x\n", Lba, BufferSize));
  }

ErrorHandler:
  gMmst->MmFreePool (AuxBuffer);
  return Status;
}

/**
  Retrieve first matched GPT partition entry information.

  @param[in]  GptHandle   GptHandle.
  @param[in]  Match       Matching function to find out specific GPT partition.
  @param[in]  Data        Data used in @Match to find out specific GPT partition.
  @param[out] StartLBA    The StartLBA of matched partition.
  @param[out] PartSize    Size of matched partition in bytes.

  @retval EFI_SUCCESS  The GPT partition array was correctly retrieved.
  @retval Error        No information was correctly retrieved.

**/
EFI_STATUS
EFIAPI
GptGetMatchedPartitionStats (
  IN  GPT_PARTITION_HANDLE  *GptHandle,
  IN     MATCH_FUNC_T       Match,
  IN     VOID               *Data,
  OUT    EFI_LBA            *StartLBA,
  OUT    UINTN              *PartSize
  )
{
  EFI_STATUS                 Status;
  EFI_BLOCK_IO_PROTOCOL      *Instance;
  UINTN                      Idx;
  CONST EFI_PARTITION_ENTRY  *Entry;
  GPT_PARTITION_TABLE        *GptTable;

  if ((GptHandle == NULL) ||
      (GptHandle->Instance == NULL) ||
      (GptHandle->GptTable == NULL) ||
      (Match == NULL) ||
      (Data == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status   = EFI_NOT_FOUND;
  Instance = GptHandle->Instance;
  GptTable = GptHandle->GptTable;

  for (Idx = 0; Idx < GptTable->Header.NumberOfPartitionEntries; Idx++) {
    if (Match (&GptTable->PartitionTable[Idx], Data)) {
      Status = EFI_SUCCESS;
      Entry  = &GptTable->PartitionTable[Idx];
      if (StartLBA != NULL) {
        *StartLBA = Entry->StartingLBA;
      }

      if (PartSize != NULL) {
        *PartSize = (Entry->EndingLBA - Entry->StartingLBA + 1) * Instance->Media->BlockSize;
      }

      break;
    }
  }

  return Status;
}

/**
  Release GptHandle.

  @param[in] GptHandle    Gpt Handle.

**/
VOID
EFIAPI
GptReleasePartition (
  IN    GPT_PARTITION_HANDLE  *GptHandle
  )
{
  if (GptHandle != NULL) {
    if (GptHandle->GptTable != NULL) {
      gMmst->MmFreePool (GptHandle->GptTable);
      GptHandle->GptTable = NULL;
    }

    GptHandle->Instance = NULL;
    gMmst->MmFreePool (GptHandle);
  }
}

/**
  Open GptHandle.

  @param[in]  Instance    The block IO protocol instance containing a GPT.
  @param[out] GptHandle   Gpt Handle.

  @retval EFI_SUCCESS               Success.
  @retval EFI_INVALID_PARAMETER     Invalid Parameter.
  @retval EFI_ABORT                 Fail to read Gpt Table Header or
                                    Invalid Gpt Table.
  @retval EFI_OUT_OF_RESOURCES      Out of memory.

**/
EFI_STATUS
EFIAPI
GptOpenPartition (
  IN     EFI_BLOCK_IO_PROTOCOL  *Instance,
  OUT    GPT_PARTITION_HANDLE   **GptHandle
  )
{
  EFI_STATUS                  Status;
  EFI_PARTITION_TABLE_HEADER  Header;
  UINTN                       GptTableSize;
  UINTN                       GptPartitionArraySize;
  GPT_PARTITION_HANDLE        *TmpGptHandle;

  if ((Instance == NULL) || (GptHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (GPT_PARTITION_HANDLE),
                    (VOID **)&TmpGptHandle
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TmpGptHandle->Instance = Instance;
  TmpGptHandle->GptTable = NULL;

  // Retrieve partition header
  Status = GetGptPartitionHeader (TmpGptHandle, &Header);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Init fail, cannot retrieve GPT header\n"));
    return EFI_ABORTED;
  }

  PrintGptHeader (&Header);

  // The copy in-place of the GPT partition entry below assumes that the size of each
  // Partition entry == sizeof(EFI_PARTITION_ENTRY).
  // Abort if that assumption does not hold.
  if (Header.SizeOfPartitionEntry != sizeof (EFI_PARTITION_ENTRY)) {
    Status = EFI_ABORTED;
    DEBUG ((DEBUG_ERROR, "Init fail, partition size mismatch (%x)\n", Header.SizeOfPartitionEntry));
    goto ErrorHandler;
  }

  GptPartitionArraySize = Header.NumberOfPartitionEntries * Header.SizeOfPartitionEntry;
  GptTableSize          = GptPartitionArraySize + sizeof (EFI_PARTITION_TABLE_HEADER);

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    GptTableSize,
                    (VOID **)&TmpGptHandle->GptTable
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Init fail, cannot allocate memory for cached GPT header\n"));
    goto ErrorHandler;
  }

  CopyMem (TmpGptHandle->GptTable, &Header, sizeof (Header));

  // Read The GPT partition array from flash (the read size must be a block size multiple)
  Status = GptReadPartition (
             TmpGptHandle,
             TmpGptHandle->GptTable->PartitionTable,
             GptPartitionArraySize,
             0,
             Header.PartitionEntryLBA
             );
  if (Header.SizeOfPartitionEntry != sizeof (EFI_PARTITION_ENTRY)) {
    Status = EFI_ABORTED;
    DEBUG ((DEBUG_ERROR, "GptInit: Failed to read GPT partition table\n"));
    goto ErrorHandler;
  }

  *GptHandle = TmpGptHandle;

  return EFI_SUCCESS;

ErrorHandler:
  if (TmpGptHandle) {
    if (TmpGptHandle->GptTable) {
      gMmst->MmFreePool (TmpGptHandle->GptTable);
    }

    gMmst->MmFreePool (TmpGptHandle);
  }

  *GptHandle = NULL;

  return Status;
}
