/** @file

  Copyright (c) 2024, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - GUID Partition Table (GPT) Disk Layout
    (https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html)

**/

#ifndef DISK_GPT_H_
#define DISK_GPT_H_

#include <Uefi/UefiGpt.h>

#define GPT_PROTECTIVE_MBR_SIZE  512
#define GPT_PARTITION_LBA_SIZE   512

/**
  Compare function to find out specific Gpt Partition.

  @param [in]   PartitionEntry       GPT partition.
  @param [in]   Data                 User Data used to compare GPT partition.

  @retval       TRUE                 Found
  @retval       FALSE                No found
**/
typedef BOOLEAN (EFIAPI *MATCH_FUNC_T)(
  CONST EFI_PARTITION_ENTRY *PartitionEntry,
  CONST VOID *Data
  );

#pragma pack(1)

/** GPT partition table.
 */
typedef struct {
  /// GPT header.
  EFI_PARTITION_TABLE_HEADER    Header;

  /// Gpt Partition Entry.
  EFI_PARTITION_ENTRY           PartitionTable[];
} GPT_PARTITION_TABLE;

/**
 * GPT partition handle.
 */
typedef struct {
  /// Block device instance.
  EFI_BLOCK_IO_PROTOCOL    *Instance;

  /// Gpt Partition Table.
  GPT_PARTITION_TABLE      *GptTable;
} GPT_PARTITION_HANDLE;

#pragma pack()

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
  );

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
GptWritePartition (
  IN  GPT_PARTITION_HANDLE  *GptHandle,
  IN  VOID                  *SrcBuffer,
  IN  UINTN                 BufferSize,
  IN  UINTN                 WriteOffset,
  IN  UINTN                 BaseLba
  );

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
  );

/**
  Release GptHandle.

  @param[in] GptHandle    Gpt Handle.

**/
VOID
EFIAPI
GptReleasePartition (
  IN    GPT_PARTITION_HANDLE  *GptHandle
  );

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
  );

#endif /* _DISK_GPT_H_ */
