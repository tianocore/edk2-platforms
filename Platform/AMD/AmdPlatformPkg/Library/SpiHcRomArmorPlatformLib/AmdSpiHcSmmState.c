/** @file

  SPI HC SMM state registration function definitions

  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

**/
#include <Base.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Protocol/AmdSpiSmmHcState.h>
#include "AmdSpiHcSmmState.h"
#include "AmdSpiHcInternal.h"

extern BOOLEAN               mPspMailboxSpiMode;
extern EFI_PHYSICAL_ADDRESS  mHcAddress;
extern BOOLEAN               mSmmAlreadySavedState;
extern VOID                  *mState;
extern UINT32                mStateSize;
extern UINT32                mStateRecordCount;

CONST SPI_HC_REGISTER_STATE  mSpiHcState[] = {
  // {Register, Size, Count}
  { 0x04, 0x4, 0x1 }, // SPI_RestrictedCmd
  { 0x08, 0x4, 0x1 }, // SPI_RestrictedCmd2
  { 0x0D, 0x1, 0x1 }, // SPI_Cntrl1[15:8]
  { 0x0E, 0x2, 0x1 }, // SPI_Cntrl1[31:16]
  { 0x10, 0x4, 0x1 }, // SPI_CmdValue0
  { 0x14, 0x4, 0x1 }, // SPI_CmdValue1
  { 0x18, 0x4, 0x1 }, // SPI_CmdValue2
  { 0x1D, 0x1, 0x1 }, // Alt_SPI_CS
  { 0x20, 0x1, 0x1 }, // SPI100 Enable
  { 0x22, 0x2, 0x1 }, // SPI100 Speed Config
  { 0x24, 0x4, 0x1 }, // SPI100 Clock Config
  { 0x32, 0x2, 0x1 }, // SPI100 Dummy Cycle Config
  { 0x34, 0x2, 0x1 }, // SPI100 RX Timing Config 1
  { 0x44, 0x1, 0x1 }, // ModeByte
  { 0x45, 0x1, 0x1 }, // CmdCode
  { 0x48, 0x1, 0x1 }, // TxByteCount
  { 0x4B, 0x1, 0x1 }, // RxByteCount
  { 0x80, 0x1, 70  }, // FIFO [70:0]
  { 0x00, 0x4, 0x1 }  // SpiCntrl0  ** Save last so restore last **
};

/**
  Allocate the save state space and update the instance structure.

  @retval EFI_SUCCESS            The Save State space was allocated.
  @retval EFI_OUT_OF_RESOURCES   The Save State space failed to allocate.
  @retval EFI_BAD_BUFFER_SIZE    Integer overflow detected in state size calculation.
**/
EFI_STATUS
EFIAPI
AllocateState (
  )
{
  EFI_STATUS  Status;
  UINT32      NumRecords;
  UINT32      Record;

  NumRecords = sizeof (mSpiHcState) / sizeof (SPI_HC_REGISTER_STATE);

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return EFI_SUCCESS;
  }

  // calculate space needed
  mStateSize = 0;
  for (Record = 0; Record < NumRecords; Record++) {
    if ((mSpiHcState[Record].Count != 0) &&
        (((MAX_UINT32 - mStateSize) / mSpiHcState[Record].Count)
         >= mSpiHcState[Record].Size))
    {
      mStateSize += mSpiHcState[Record].Size * mSpiHcState[Record].Count;
    } else {
      return EFI_BAD_BUFFER_SIZE;
    }
  }

  mStateRecordCount = NumRecords;
  mState            = AllocateZeroPool (mStateSize);
  if (mState == NULL) {
    mStateRecordCount = 0;
    Status            = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Save the Host controller state to restore after transaction is complete.

  @param[in] This           SPI host controller Preserve State Protocol.

  @retval EFI_SUCCESS       SPI host controller state saved successfully.
  @retval EFI_DEVICE_ERROR  SPI controller is in PSP mode or address is invalid.
**/
EFI_STATUS
EFIAPI
SaveState (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;
  UINT32      NumRecords;
  UINT32      Record;
  UINT32      Count;
  VOID        *State;
  UINT8       *TrackState;

  Status = EFI_SUCCESS;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return Status;
  }

  if (mHcAddress > 0xFFFFFF00) {
    return EFI_DEVICE_ERROR;
  }

  Status = FchSpiControllerNotBusy ();
  if (!EFI_ERROR (Status)) {
    State      = mState;
    TrackState = (UINT8 *)State;
    NumRecords = mStateRecordCount;
    if (!mSmmAlreadySavedState) {
      for (Record = 0; Record < NumRecords; Record++) {
        switch (mSpiHcState[Record].Size) {
          case 0x1:
            for (Count = 0; Count < mSpiHcState[Record].Count; Count++) {
              *(UINT8 *)State = MmioRead8 (
                                  mHcAddress
                                  + mSpiHcState[Record].Register
                                  );
              TrackState += 1;
            }

            break;
          case 0x2:
            for (Count = 0; Count < mSpiHcState[Record].Count; Count++) {
              *(UINT16 *)State = MmioRead16 (
                                   mHcAddress
                                   + mSpiHcState[Record].Register
                                   );
              TrackState += 2;
            }

            break;
          case 0x4:
            for (Count = 0; Count < mSpiHcState[Record].Count; Count++) {
              *(UINT32 *)State = MmioRead32 (
                                   mHcAddress
                                   + mSpiHcState[Record].Register
                                   );
              TrackState += 4;
            }

            break;
        }

        State = TrackState;
      }

      mSmmAlreadySavedState = TRUE;
    }
  }

  return Status;
}

/**
  Restore the Host Controller state.

  @param[in] This           SPI host controller Preserve State Protocol.

  @retval EFI_SUCCESS       SPI host controller state restored successfully.
  @retval EFI_DEVICE_ERROR  SPI controller is in PSP mode or address is invalid.
**/
EFI_STATUS
EFIAPI
RestoreState (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;
  UINT32      NumRecords;
  UINT32      Record;
  UINT32      Count;
  VOID        *State;
  UINT8       *TrackState;

  Status = EFI_SUCCESS;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return Status;
  }

  if (mHcAddress > 0xFFFFFF00) {
    return EFI_DEVICE_ERROR;
  }

  State      = mState;
  TrackState = (UINT8 *)State;
  NumRecords = mStateRecordCount;
  if (mSmmAlreadySavedState) {
    for (Record = 0; Record < NumRecords; Record++) {
      switch (mSpiHcState[Record].Size) {
        case 0x1:
          for (Count = 0; Count < mSpiHcState[Record].Count; Count++) {
            MmioWrite8 (
              mHcAddress + mSpiHcState[Record].Register,
              *(UINT8 *)State
              );
            TrackState += 1;
          }

          break;
        case 0x2:
          for (Count = 0; Count < mSpiHcState[Record].Count; Count++) {
            MmioWrite16 (
              mHcAddress + mSpiHcState[Record].Register,
              *(UINT16 *)State
              );
            TrackState += 2;
          }

          break;
        case 0x4:
          for (Count = 0; Count < mSpiHcState[Record].Count; Count++) {
            MmioWrite32 (
              mHcAddress + mSpiHcState[Record].Register,
              *(UINT32 *)State
              );
            TrackState += 4;
          }

          break;
      }

      State = TrackState;
    }

    mSmmAlreadySavedState = FALSE;
  }

  return Status;
}

/**
  Block SPI Flash Write Enable Opcode.  This will block anything that requires
  the Opcode equivalent to the SPI Flash Memory Write Enable Opcode.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04).

  @param[in] This    SPI host controller Preserve State Protocol.
  @param[in] Opcode  SPI opcode to block.

  @retval EFI_SUCCESS  Opcode blocked successfully.
**/
EFI_STATUS
EFIAPI
FchSpiBlockOpcode (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This,
  IN UINT8                                Opcode
  )
{
  EFI_STATUS  Status;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return EFI_SUCCESS;
  }

  Status = InternalFchSpiBlockOpcode (mHcAddress, Opcode);
  return Status;
}

/**
  Un-Block SPI Flash Write Enable Opcode.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04).

  @param[in] This    SPI host controller Preserve State Protocol.
  @param[in] Opcode  SPI opcode to unblock.

  @retval EFI_SUCCESS  Opcode unblocked successfully.
**/
EFI_STATUS
EFIAPI
FchSpiUnblockOpcode (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This,
  IN UINT8                                Opcode
  )
{
  EFI_STATUS  Status;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return EFI_SUCCESS;
  }

  Status = InternalFchSpiUnblockOpcode (mHcAddress, Opcode);
  return Status;
}

/**
  Un-Block any blocked SPI Opcodes.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04).

  @param[in] This  SPI host controller Preserve State Protocol.

  @retval EFI_SUCCESS  All opcodes unblocked successfully.
**/
EFI_STATUS
EFIAPI
FchSpiUnblockAllOpcodes (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return EFI_SUCCESS;
  }

  Status = InternalFchSpiUnblockAllOpcodes (mHcAddress);
  return Status;
}

/**
  Lock SPI host controller registers.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04).

  @param[in] This  SPI host controller Preserve State Protocol.

  @retval EFI_SUCCESS       SPI controller registers locked successfully.
  @retval EFI_DEVICE_ERROR  Lock verification read-back failed.
**/
EFI_STATUS
EFIAPI
FchSpiLockSpiHostControllerRegisters (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return EFI_SUCCESS;
  }

  Status = InternalFchSpiLockSpiHostControllerRegisters (mHcAddress);
  return Status;
}

/**
  Unlock SPI host controller registers.  This unlock function will only work in
  SMM.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04).

  @param[in] This  SPI host controller Preserve State Protocol.

  @retval EFI_SUCCESS       SPI controller registers unlocked successfully.
  @retval EFI_DEVICE_ERROR  Unlock verification read-back failed.
**/
EFI_STATUS
EFIAPI
FchSpiUnlockSpiHostControllerRegisters (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  // If PSP is in control, cannot save state
  if (mPspMailboxSpiMode) {
    return EFI_SUCCESS;
  }

  Status = InternalFchSpiUnlockSpiHostControllerRegisters (mHcAddress);
  return Status;
}
