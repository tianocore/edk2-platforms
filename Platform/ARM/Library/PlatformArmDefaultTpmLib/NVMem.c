/** @file
  Tpm Nv Storage part of PlatformTpmLib to use TpmLib.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformTpmLib.h>

#include <Protocol/BlockIo.h>

#define TPM_NV_BLOCK_SIZE   0x200
#define TPM_NV_MEMORY_SIZE  FixedPcdGet64 (PcdTpmNvMemorySize)
#define TPM_NV_BLOCK_COUNT  ((TPM_NV_MEMORY_SIZE) / (TPM_NV_BLOCK_SIZE))

//
// Shortcut for 'dirty'ing all NV blocks. Note the type.
//
#if NV_BLOCK_COUNT < 64
#define NV_DIRTY_ALL  ((UINT64)((0x1ULL << NV_BLOCK_COUNT) - 1))
#elif NV_BLOCK_COUNT == 64
#define NV_DIRTY_ALL  (~(0x0ULL))
#else
  #error "NV block count exceeds 64 bit block map. Adjust block or NV size."
#endif

#define TPM_NV_STATE_SIGNATURE  SIGNATURE_32('T', 'P', 'M', '2')

#define BITS_PER_UINT64  (sizeof (UINT64) * 8)
#define BIT_WORD(nr)  ((nr) / (BITS_PER_UINT64))
#define BIT_IDX(nr)   ((nr) % BITS_PER_UINT64)
#define BIT_MASK(nr)  (1ULL << (BIT_IDX(nr)))

typedef struct {
  UINT32    Signature;
  UINT32    FirmwareV1;
  UINT32    FirmwareV2;
} TPM_NV_STATE;

extern UINT32  s_evictNvEnd;

STATIC EFI_BLOCK_IO_PROTOCOL  *mNorFlashBlockIo = NULL;
STATIC UINT32                 mNvBlockSize;
STATIC VOID                   *mNvBlockBuffer;
STATIC UINT32                 mTpmBlockPerNvBlock;

STATIC VOID  *mTpmDataBuffer;
STATIC VOID  *mTpmStateBuffer;

STATIC UINT64   mDirtyBlockMap[(TPM_NV_BLOCK_COUNT / BITS_PER_UINT64) + 1];
STATIC BOOLEAN  mNvInitialized     = FALSE;
STATIC BOOLEAN  mNvNeedManufacture = FALSE;

/**
  This function opens Tpm Nv Storage

**/
STATIC
EFI_STATUS
EFIAPI
OpenNvStorage (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!FixedPcdGetBool (PcdTpmEmuNvMemory)) {
    Status = gMmst->MmLocateProtocol (
                      &gEdkiiTpmBlockIoProtocolGuid,
                      NULL,
                      (VOID **)&mNorFlashBlockIo
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get gEdkiiTpmBlockIoProtocolGuid... Status: %r\n",
        __func__,
        Status
        ));
      return Status;
    }

    mNvBlockSize = mNorFlashBlockIo->Media->BlockSize;
  } else {
    mNvBlockSize = TPM_NV_BLOCK_SIZE;
  }

  mTpmBlockPerNvBlock = mNvBlockSize / TPM_NV_BLOCK_SIZE;
  mNvBlockBuffer      = AllocateRuntimePool (mNvBlockSize);
  if (mNvBlockBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate mNvBlockBuffer...\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  This function closes Tpm Nv Storage

**/
STATIC
VOID
EFIAPI
CloseNvStorage (
  VOID
  )
{
  if (mNvBlockBuffer != NULL) {
    FreePool (mNvBlockBuffer);
    mNvBlockBuffer = NULL;
    mNvBlockSize   = 0;
  }

  mNorFlashBlockIo = NULL;
}

/**
  This function reads data from Tpm Nv Storage

  @param [in]     Lba   Logical block address
  @param [out]    Buffer Output buffer
  @param [in]     Size  Size to read

  @return EFI_SUCCESS   Success to read
  @return Others        Error

**/
STATIC
EFI_STATUS
EFIAPI
ReadNvStorage (
  IN EFI_LBA  Lba,
  OUT VOID    *Buffer,
  IN UINT64   Size
  )
{
  EFI_STATUS  Status;
  VOID        *NvEmuAddr;
  EFI_LBA     NvLba;
  UINT32      Offset;
  UINT32      CopySize;

  for (NvLba = Lba / mTpmBlockPerNvBlock; Size > 0; NvLba++) {
    Offset = (Lba % mTpmBlockPerNvBlock) * TPM_NV_BLOCK_SIZE;

    if (!FixedPcdGetBool (PcdTpmEmuNvMemory)) {
      Status = mNorFlashBlockIo->ReadBlocks (
                                   mNorFlashBlockIo,
                                   mNorFlashBlockIo->Media->MediaId,
                                   NvLba,
                                   mNvBlockSize,
                                   mNvBlockBuffer
                                   );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to read Tpm Nv Data... Lba:%ld, Size: %ld, Status: %r\n",
          __func__,
          Lba,
          Size,
          Status
          ));
        return Status;
      }
    } else {
      NvEmuAddr = (VOID *)(FixedPcdGet64 (PcdTpmNvMemoryBase) + (NvLba * mNvBlockSize));
      CopyMem (mNvBlockBuffer, NvEmuAddr, mNvBlockSize);
    }

    CopySize = (Size > mNvBlockSize - Offset) ? mNvBlockSize - Offset : Size;
    CopyMem (Buffer, mNvBlockBuffer + Offset, CopySize);
    Buffer += CopySize;
    Size   -= CopySize;
    Lba    += ((Offset != 0) ? (mTpmBlockPerNvBlock - (Lba % mTpmBlockPerNvBlock)) : mTpmBlockPerNvBlock);
  }

  return EFI_SUCCESS;
}

/**
  This function writes data to Tpm Nv Storage

  @param [in]     Lba   Logical block address
  @param [in]     Buffer Input data
  @param [in]     Size  Size to write

  @return EFI_SUCCESS   Success to write
  @return Others        Error

**/
STATIC
EFI_STATUS
EFIAPI
WriteNvStorage (
  IN EFI_LBA  Lba,
  IN VOID     *Buffer,
  IN UINT64   Size
  )
{
  EFI_STATUS  Status;
  VOID        *NvEmuAddr;
  EFI_LBA     NvLba;
  UINT32      Offset;
  UINT32      CopySize;

  for (NvLba = Lba / mTpmBlockPerNvBlock; Size > 0; NvLba++) {
    Offset = (Lba % mTpmBlockPerNvBlock) * TPM_NV_BLOCK_SIZE;

    if (!FixedPcdGetBool (PcdTpmEmuNvMemory)) {
      Status = mNorFlashBlockIo->ReadBlocks (
                                   mNorFlashBlockIo,
                                   mNorFlashBlockIo->Media->MediaId,
                                   NvLba,
                                   mNvBlockSize,
                                   mNvBlockBuffer
                                   );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to read Tpm Nv Data... Lba:%ld, Size: %ld, Status: %r\n",
          __func__,
          Lba,
          Size,
          Status
          ));
        return Status;
      }
    } else {
      NvEmuAddr = (VOID *)(FixedPcdGet64 (PcdTpmNvMemoryBase) + (NvLba * mNvBlockSize));
      CopyMem (mNvBlockBuffer, NvEmuAddr, mNvBlockSize);
    }

    CopySize = (Size > mNvBlockSize - Offset) ? mNvBlockSize - Offset : Size;
    CopyMem (mNvBlockBuffer + Offset, Buffer, CopySize);

    if (!FixedPcdGetBool (PcdTpmEmuNvMemory)) {
      Status = mNorFlashBlockIo->WriteBlocks (
                                   mNorFlashBlockIo,
                                   mNorFlashBlockIo->Media->MediaId,
                                   NvLba,
                                   mNvBlockSize,
                                   mNvBlockBuffer
                                   );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to write Tpm Nv Data... Lba:%ld, Size: %ld, Status: %r\n",
          __func__,
          Lba,
          Size,
          Status
          ));
        return Status;
      }
    } else {
      NvEmuAddr = (VOID *)(FixedPcdGet64 (PcdTpmNvMemoryBase) + (NvLba * mNvBlockSize));
      CopyMem (NvEmuAddr, mNvBlockBuffer, Size);
    }

    Buffer += CopySize;
    Size   -= CopySize;
    Lba    += ((Offset != 0) ? (mTpmBlockPerNvBlock - (Lba % mTpmBlockPerNvBlock)) : mTpmBlockPerNvBlock);
  }

  return EFI_SUCCESS;
}

/**
  This function marks dirty block.

  @param [in]     StartOffset   Start offset where data updated
  @param [in]     Size          Size of updated data

**/
STATIC
VOID
MarkDirtyBlock (
  UINT32  StartOffset,
  UINT32  Size
  )
{
  UINT64  DirtyStartIdx;
  UINT64  DirtyStartWord;
  UINT64  DirtyEndIdx;
  UINT64  DirtyEndWord;
  UINTN   Idx;

  DirtyStartIdx  = BIT_IDX (StartOffset / TPM_NV_BLOCK_SIZE);
  DirtyStartWord = BIT_WORD (StartOffset / TPM_NV_BLOCK_SIZE);
  DirtyEndIdx    = BIT_IDX ((StartOffset + Size) / TPM_NV_BLOCK_SIZE);
  DirtyEndWord   = BIT_WORD ((StartOffset + Size) / TPM_NV_BLOCK_SIZE);

  if (DirtyStartWord != DirtyEndWord) {
    for (Idx = DirtyStartIdx; Idx < BITS_PER_UINT64; Idx++) {
      mDirtyBlockMap[DirtyStartWord] |= BIT_MASK (Idx);
    }

    for (Idx = DirtyStartWord + 1; Idx < DirtyEndWord - 1; Idx++) {
      mDirtyBlockMap[Idx] = 0xFFFFFFFFFFFFFFFFULL;
    }

    for (Idx = 0; Idx <= DirtyEndIdx; Idx++) {
      mDirtyBlockMap[DirtyEndWord] |= BIT_MASK (Idx);
    }
  } else {
    for (Idx = DirtyStartIdx; Idx <= DirtyEndIdx; Idx++) {
      mDirtyBlockMap[DirtyStartWord] |= BIT_MASK (Idx);
    }
  }
}

/**
  _plat__NVNeedsManufacture()

  This function checks whether TPM's NV state needs to be manufactured

**/
BOOLEAN
EFIAPI
PlatformTpmLibNVNeedsManufacture (
  VOID
  )
{
  return mNvNeedManufacture;
}

/**
  _plat__NVEnable()

  Enable NV memory.

  the NV state would be read in, decrypted and integrity checked if needs.

  The recovery from an integrity failure depends on where the error occurred. It
  it was in the state that is discarded by TPM Reset, then the error is
  recoverable if the TPM is reset. Otherwise, the TPM must go into failure mode.

  @param [in] PlatParameter   Platform parameter to enable NV storage
  @param [in] ParamSize

  @return 0        if success
  @return > 0      if receive recoverable error
  @return < 0      if unrecoverable error

**/
INT32
EFIAPI
PlatformTpmLibNVEnable (
  IN VOID   *PlatParameter, // IN: platform specific parameters
  IN UINTN  ParamSize
  )
{
  EFI_STATUS    Status;
  TPM_NV_STATE  *TpmNvState;

  if (mNvInitialized) {
    return 0;
  }

  Status = OpenNvStorage ();
  if (EFI_ERROR (Status)) {
    goto ErrorHandler;
  }

  mTpmDataBuffer = AllocateRuntimePool (TPM_NV_MEMORY_SIZE + TPM_NV_BLOCK_SIZE);
  if (mTpmDataBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate NvBuffer...\n", __func__));
    goto ErrorHandler;
  }

  mTpmStateBuffer = mTpmDataBuffer + TPM_NV_MEMORY_SIZE;

  Status = ReadNvStorage (TPM_NV_BLOCK_COUNT, mTpmStateBuffer, TPM_NV_BLOCK_SIZE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to read Tpm Nv State... Status: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  TpmNvState = (TPM_NV_STATE *)mTpmStateBuffer;
  if (TpmNvState->Signature != TPM_NV_STATE_SIGNATURE) {
    mNvNeedManufacture     = TRUE;
    TpmNvState->Signature  = TPM_NV_STATE_SIGNATURE;
    TpmNvState->FirmwareV1 = 0x20240601;
    TpmNvState->FirmwareV2 = 0x20240601;
  } else {
    Status = ReadNvStorage (0, mTpmDataBuffer, TPM_NV_MEMORY_SIZE);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to read Tpm Nv Data... Status: %r\n",
        __func__,
        Status
        ));
      goto ErrorHandler;
    }
  }

  mNvInitialized = TRUE;
  s_evictNvEnd   = TPM_NV_MEMORY_SIZE;

  return 0;

ErrorHandler:
  CloseNvStorage ();

  if (mTpmDataBuffer != NULL) {
    FreePool (mTpmDataBuffer);
    mTpmDataBuffer  = NULL;
    mTpmStateBuffer = NULL;
  }

  return -1;
}

/**
  _plat__NVDisable()

  Disable NV memory.

**/
VOID
EFIAPI
PlatformTpmLibNVDisable (
  VOID
  )
{
  if (mTpmDataBuffer != NULL) {
    FreePool (mTpmDataBuffer);
    mTpmDataBuffer  = NULL;
    mTpmStateBuffer = NULL;
  }

  CloseNvStorage ();

  mNvInitialized = FALSE;

  return;
}

/**
  _plat__GetNvReadyState()

  Check if NV is available

  @return    0               NV is available
  @return    1               NV is not available due to write failure
  @return    2               NV is not available due to rate limit

**/
INT32
EFIAPI
PlatformTpmLibGetNvReadyState (
  VOID
  )
{
  return 0;
}

/**
  _plat__NvMemoryRead()

  Read a chunk of NV memory

  @param [in] StartOffset   Read start offset
  @param [in] Size          Size to read
  @param [out] Data         Data buffer

  @return 1 Success to read
  @return 0 Failed to read

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryRead (
  IN  UINT32  StartOffset,
  IN  UINT32  Size,
  OUT VOID    *Data
  )
{
  if (StartOffset + Size > TPM_NV_MEMORY_SIZE) {
    return 0;
  }

  CopyMem (Data, mTpmDataBuffer + StartOffset, Size);

  return 1;
}

/**
  _plat__NvGetChangedStatus()

  This function checks to see if the NV is different from the test value
  so that NV will not be written if it has not changed.

  @param [in] StartOffset Start offset to compare
  @param [in] Size        Size for compare
  @param [in] Data        Data to be compared

  @return NV_HAS_CHANGED(1)       the NV location is different from the test value
  @return NV_IS_SAME(0)           the NV location is the same as the test value
  @return NV_INVALID_LOCATION(-1) the NV location is invalid; also triggers failure mode
**/
INT32
EFIAPI
PlatformTpmLibNvGetChangedStatus (
  IN  UINT32  StartOffset,  // IN: read start
  IN  UINT32  Size,         // IN: size of bytes to read
  IN  VOID    *Data         // IN: data buffer
  )
{
  INTN  Result;

  if (StartOffset + Size > TPM_NV_MEMORY_SIZE) {
    return -1;
  }

  Result = CompareMem (mTpmDataBuffer + StartOffset, Data, Size);

  return (Result == 0) ? 0 : 1;
}

/**
  _plat__NvMemoryWrite()

  This function is used to update NV memory. The "write" is to a memory copy of
  NV. At the end of the current command, any changes are written to
  the actual NV memory.

  NOTE: A useful optimization would be for this code to compare the current
  contents of NV with the local copy and note the blocks that have changed. Then
  only write those blocks when _plat__NvCommit() is called.

  @param [in] StartOffset   Start Offset to write
  @param [in] Size          Size to wrrite
  @param [in] Data          Data

  @return    0               Failed to write
  @return    1               Success to write

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryWrite (
  IN  UINT32  StartOffset,
  IN  UINT32  Size,
  IN  VOID    *Data
  )
{
  ASSERT (StartOffset + Size <= TPM_NV_MEMORY_SIZE);

  MarkDirtyBlock (StartOffset, Size);
  CopyMem (mTpmDataBuffer + StartOffset, Data, Size);

  return 1;
}

/**
  _plat__NvMemoryClear()

  Function is used to set a range of NV memory bytes to an implementation-dependent
  value. The value represents the erase state of the memory.

  @param [in] StartOffset   Start offset to clear
  @param [in] SIze          Size to be clear


  @return 0 Failed to clear
  @return 1 Success

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryClear (
  IN  UINT32  StartOffset, // IN: read start
  IN  UINT32  Size         // IN: size of bytes to read
  )
{
  if (StartOffset + Size > TPM_NV_MEMORY_SIZE) {
    return 0;
  }

  MarkDirtyBlock (StartOffset, Size);
  SetMem (mTpmDataBuffer + StartOffset, 0x00, Size);

  return 1;
}

/**
  _plat__NvMemoryMove()

  Function: Move a chunk of NV memory from source to destination
  This function should ensure that if there overlap, the original data is
  copied before it is written

  @param [in] SourceOffset  Source offset to move
  @param [in] DestOffset    Destination offset to move
  @param [in] Size          Size to be moved

  @return 0 Failed to move
  @return 1 Success

**/
INT32
EFIAPI
PlatformTpmLibNvMemoryMove (
  IN   UINT32  SourceOffset,  // IN: source offset
  IN   UINT32  DestOffset,    // IN: destination offset
  IN   UINT32  Size           // IN: size of data being moved
  )
{
  if ((SourceOffset + Size > TPM_NV_MEMORY_SIZE) ||
      (DestOffset + Size > TPM_NV_MEMORY_SIZE))
  {
    return 0;
  }

  MarkDirtyBlock (SourceOffset, Size);
  MarkDirtyBlock (DestOffset, Size);

  CopyMem (mTpmDataBuffer + DestOffset, mTpmDataBuffer+ SourceOffset, Size);

  return 1;
}

/**
  _plat__NvCommit()

  This function writes the local copy of NV to NV for permanent store.
  It will write TPM_NV_MEMORY_SIZE bytes to NV.

   @return 0       NV write success
   @return non-0   NV write fail

**/
INT32
EFIAPI
PlatformTpmLibNvCommit (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      WordIdx;
  UINT64      BitIdx;
  UINT64      BlockNum;

  for (WordIdx = 0; WordIdx < (TPM_NV_BLOCK_COUNT / BITS_PER_UINT64) + 1; WordIdx++) {
    if (mDirtyBlockMap[WordIdx] == 0x00) {
      continue;
    }

    for (BitIdx = 0; BitIdx < BITS_PER_UINT64; BitIdx++) {
      if ((mDirtyBlockMap[WordIdx] & BIT_MASK (BitIdx)) == 0x00) {
        continue;
      }

      BlockNum = (WordIdx * BITS_PER_UINT64) + BitIdx;

      Status = WriteNvStorage (
                 BlockNum,
                 mTpmDataBuffer + (BlockNum * TPM_NV_BLOCK_SIZE),
                 TPM_NV_BLOCK_SIZE
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to write TpmNvData... Status: %r\n",
          __func__,
          Status
          ));
        return -1;
      }
    }
  }

  if (mNvNeedManufacture) {
    Status = WriteNvStorage (TPM_NV_BLOCK_COUNT, mTpmStateBuffer, TPM_NV_BLOCK_SIZE);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to write Tpm State Data... Status: %r\n",
        __func__,
        Status
        ));
      return -1;
    }

    mNvNeedManufacture = FALSE;
  }

  return 0;
}

/**
  _plat__SetNvAvail()

   Set the current NV state to available.
   This function is for testing purpose only.
   It is not part of the platform NV logic

**/
VOID
EFIAPI
PlatformTpmLibSetNvAvail (
  VOID
  )
{
  // Not support.
  return;
}

/**
  _plat__ClearNvAvail()

  Set the current NV state to unavailable.
  This function is for testing purpose only.
  It is not part of the platform NV logic

**/
VOID
EFIAPI
PlatformTpmLibClearNvAvail (
  VOID
  )
{
  // Not support.
  return;
}
