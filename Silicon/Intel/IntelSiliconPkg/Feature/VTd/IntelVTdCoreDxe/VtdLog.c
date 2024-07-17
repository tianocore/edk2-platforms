/** @file

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

UINT8  *mVtdLogBuffer = NULL;

UINT8  *mVtdLogDxeFreeBuffer = NULL;
UINT32 mVtdLogDxeBufferUsed = 0;

UINT32 mVtdLogPeiPostMemBufferUsed = 0;

UINT8  mVtdLogPeiError = 0;
UINT16 mVtdLogDxeError = 0;

/**
  Allocate memory buffer for VTd log items.

  @param[in] MemorySize    Required memory buffer size.

  @retval Buffer address

**/
UINT8 *
EFIAPI
VTdLogAllocMemory (
  IN  CONST UINT32          MemorySize
  )
{
  UINT8                     *Buffer;

  Buffer = NULL;
  if (mVtdLogDxeFreeBuffer != NULL) {
    if ((mVtdLogDxeBufferUsed + MemorySize) <= PcdGet32 (PcdVTdDxeLogBufferSize)) {
      Buffer = mVtdLogDxeFreeBuffer;

      mVtdLogDxeFreeBuffer += MemorySize;
      mVtdLogDxeBufferUsed += MemorySize;
    } else {
      mVtdLogDxeError |= VTD_LOG_ERROR_BUFFER_FULL;
    }
  }
  return Buffer;
}

/**
  Add a new VTd log event.

  @param[in] EventType   Event type
  @param[in] Data1       First parameter
  @param[in] Data2       Second parameter

**/
VOID
EFIAPI
VTdLogAddEvent (
  IN  CONST VTDLOG_EVENT_TYPE EventType,
  IN  CONST UINT64            Data1,
  IN  CONST UINT64            Data2
  )
{
  VTDLOG_EVENT_2PARAM         *Item;

  if (PcdGet8 (PcdVTdLogLevel) == 0) {
    return;
  } else if ((PcdGet8 (PcdVTdLogLevel) == 1) && (EventType >= VTDLOG_DXE_ADVANCED)) {
    return;
  }

  Item = (VTDLOG_EVENT_2PARAM *) VTdLogAllocMemory (sizeof (VTDLOG_EVENT_2PARAM));
  if (Item != NULL) {
    Item->Data1 = Data1;
    Item->Data2 = Data2;

    Item->Header.DataSize  = sizeof (VTDLOG_EVENT_2PARAM);
    Item->Header.LogType   = LShiftU64 (1, EventType);
    Item->Header.Timestamp = AsmReadTsc ();
  }
}

/**
  Add a new VTd log event with data.

  @param[in] EventType   Event type
  @param[in] Param       parameter
  @param[in] Data        Data
  @param[in] DataSize    Data size

**/
VOID
EFIAPI
VTdLogAddDataEvent (
  IN  CONST VTDLOG_EVENT_TYPE EventType,
  IN  CONST UINT64            Param,
  IN  CONST VOID              *Data,
  IN  CONST UINT32            DataSize
  )
{
  VTDLOG_EVENT_CONTEXT        *Item;
  UINT32                      EventSize;

  if (PcdGet8 (PcdVTdLogLevel) == 0) {
    return;
  } else if ((PcdGet8 (PcdVTdLogLevel) == 1) && (EventType >= VTDLOG_DXE_ADVANCED)) {
    return;
  }

  EventSize = sizeof (VTDLOG_EVENT_CONTEXT) + DataSize - 1;

  Item = (VTDLOG_EVENT_CONTEXT *) VTdLogAllocMemory (EventSize);
  if (Item != NULL) {
    Item->Param = Param;
    CopyMem (Item->Data, Data, DataSize);

    Item->Header.DataSize  = EventSize;
    Item->Header.LogType   = LShiftU64 (1, EventType);
    Item->Header.Timestamp = AsmReadTsc ();
  }
}

/**
  Get Event Items From Pei Pre-Mem Buffer

  @param[in]     Buffer           Pre-Memory data buffer.
  @param[in]     Context          Event context
  @param[in out] CallbackHandle   Callback function for each VTd log event
**/
UINT64
EFIAPI
VTdGetEventItemsFromPeiPreMemBuffer (
  IN     VTDLOG_PEI_PRE_MEM_INFO     *InfoBuffer,
  IN     VOID                        *Context,
  IN OUT EDKII_VTD_LOG_HANDLE_EVENT  CallbackHandle
  )
{
  UINTN                            Index;
  UINT64                           EventCount;
  VTDLOG_EVENT_2PARAM              Event;

  if (InfoBuffer == NULL) {
    return 0;
  }

  EventCount = 0;
  for (Index = 0; Index < VTD_LOG_PEI_PRE_MEM_BAR_MAX; Index++) {
    if (InfoBuffer[Index].Mode == VTD_LOG_PEI_PRE_MEM_NOT_USED) {
        continue;
    }
    if (CallbackHandle) {
      Event.Header.DataSize = sizeof (VTDLOG_EVENT_2PARAM);
      Event.Header.Timestamp = 0;

      Event.Header.LogType = LShiftU64 (1, VTDLOG_PEI_PRE_MEM_DMA_PROTECT);
      Event.Data1 = InfoBuffer[Index].BarAddress;
      Event.Data2 = InfoBuffer[Index].Mode;
      Event.Data2 |= LShiftU64 (InfoBuffer[Index].Status, 8);
      CallbackHandle (Context, &Event.Header);
    }
    EventCount++;
  }

  return EventCount;
}

/**
  Get Event Items From Pei Post-Mem/Dxe Buffer

  @param[in]      Buffer            Data buffer.
  @param[in]      BufferUsed        Data buffer used.
  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback function for each VTd log event
**/
UINT64
EFIAPI
VTdGetEventItemsFromBuffer (
  IN     UINT8                       *Buffer,
  IN     UINT32                      BufferUsed,
  IN     VOID                        *Context,
  IN OUT EDKII_VTD_LOG_HANDLE_EVENT  CallbackHandle
  )
{
  UINT64                      Count;
  VTDLOG_EVENT_HEADER         *Header;

  Count = 0;
  if (Buffer != NULL) {
    while (BufferUsed > 0) {
      Header = (VTDLOG_EVENT_HEADER *) Buffer;
      if (BufferUsed >= Header->DataSize) {
        if (CallbackHandle) {
          CallbackHandle (Context, Header);
        }
        Buffer += Header->DataSize;
        BufferUsed -= Header->DataSize;
        Count++;
      } else {
        BufferUsed = 0;
      }
    }
  }

  return Count;
}

/**
  Generate the VTd log state.

  @param[in]      EventType         Event type
  @param[in]      Data1             First parameter
  @param[in]      Data2             Second parameter
  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback function for each VTd log event
**/
VOID
EFIAPI
VTdGenerateStateEvent (
  IN     VTDLOG_EVENT_TYPE          EventType,
  IN     UINT64                     Data1,
  IN     UINT64                     Data2,
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LOG_HANDLE_EVENT CallbackHandle
  )
{
  VTDLOG_EVENT_2PARAM         Item;

  Item.Data1 = Data1;
  Item.Data2 = Data2;

  Item.Header.DataSize  = sizeof (VTDLOG_EVENT_2PARAM);
  Item.Header.LogType   = LShiftU64 (1, EventType);
  Item.Header.Timestamp = 0;

  if (CallbackHandle) {
    CallbackHandle (Context, &Item.Header);
  }
}

/**
  Get the VTd log events.
  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback function for each VTd log event

  @retval     UINT32            Number of events
**/
UINT64
EFIAPI
VTdLogGetEvents (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LOG_HANDLE_EVENT CallbackHandle
  )
{
  UINT64                      CountPeiPreMem;
  UINT64                      CountPeiPostMem;
  UINT64                      CountDxe;
  UINT8                       *Buffer;

  if (mVtdLogBuffer == NULL) {
    return 0;
  }

  //
  // PEI pre-memory phase
  //
  Buffer = &mVtdLogBuffer[PcdGet32 (PcdVTdDxeLogBufferSize) + PcdGet32 (PcdVTdPeiPostMemLogBufferSize)];
  CountPeiPreMem = VTdGetEventItemsFromPeiPreMemBuffer ((VTDLOG_PEI_PRE_MEM_INFO *) Buffer, Context, CallbackHandle);
  DEBUG ((DEBUG_INFO, "Find %d in PEI pre mem phase\n", CountPeiPreMem));

  //
  // PEI post memory phase
  //
  Buffer = &mVtdLogBuffer[PcdGet32 (PcdVTdDxeLogBufferSize)];
  CountPeiPostMem = VTdGetEventItemsFromBuffer (Buffer, mVtdLogPeiPostMemBufferUsed, Context, CallbackHandle);
  if (mVtdLogPeiError != 0) {
    VTdGenerateStateEvent (VTDLOG_PEI_BASIC, mVtdLogPeiError, 0, Context, CallbackHandle);
    CountPeiPostMem++;
  }
  DEBUG ((DEBUG_INFO, "Find %d in PEI post mem phase\n", CountPeiPostMem));

  //
  // DXE phase
  //
  Buffer = &mVtdLogBuffer[0];
  CountDxe = VTdGetEventItemsFromBuffer (Buffer, mVtdLogDxeBufferUsed, Context, CallbackHandle);
  if (mVtdLogDxeError != 0) {
    VTdGenerateStateEvent (VTDLOG_DXE_BASIC, mVtdLogDxeError, 0, Context, CallbackHandle);
    CountDxe++;
  }
  DEBUG ((DEBUG_INFO, "Find %d in DXE phase\n", CountDxe));

  return CountPeiPreMem + CountPeiPostMem + CountDxe;
}

EDKII_VTD_LOG_PROTOCOL mIntelVTdLog = {
  EDKII_VTD_LOG_PROTOCOL_REVISION,
  VTdLogGetEvents
};

/**
  Initializes the VTd Log.

**/
VOID
EFIAPI
VTdLogInitialize(
  VOID
  )
{
  UINT32                  TotalBufferSize;
  EFI_STATUS              Status;
  VOID                    *HobPtr;
  VTDLOG_PEI_BUFFER_HOB   *HobPeiBuffer;
  EFI_HANDLE              Handle;
  UINT32                  BufferOffset;

  if (PcdGet8 (PcdVTdLogLevel) == 0) {
    return;
  }

  if (mVtdLogBuffer != NULL) {
    return;
  }

  TotalBufferSize = PcdGet32 (PcdVTdDxeLogBufferSize) + PcdGet32 (PcdVTdPeiPostMemLogBufferSize) + sizeof (VTDLOG_PEI_PRE_MEM_INFO) * VTD_LOG_PEI_PRE_MEM_BAR_MAX;

  Status = gBS->AllocatePool (EfiBootServicesData, TotalBufferSize, (VOID **) &mVtdLogBuffer);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // DXE Buffer
  //
  if (PcdGet32 (PcdVTdDxeLogBufferSize) > 0) {
    mVtdLogDxeFreeBuffer = mVtdLogBuffer;
    mVtdLogDxeBufferUsed = 0;
  }

  //
  // Get PEI pre-memory buffer offset
  //
  BufferOffset = PcdGet32 (PcdVTdDxeLogBufferSize) + PcdGet32 (PcdVTdPeiPostMemLogBufferSize);

  HobPtr = GetFirstGuidHob (&gVTdLogBufferHobGuid);
  if (HobPtr != NULL) {
    HobPeiBuffer = GET_GUID_HOB_DATA (HobPtr);

    //
    // Copy PEI pre-memory phase VTd log.
    //
    CopyMem (&mVtdLogBuffer[BufferOffset], &HobPeiBuffer->PreMemInfo, sizeof (VTDLOG_PEI_PRE_MEM_INFO) * VTD_LOG_PEI_PRE_MEM_BAR_MAX);

    //
    // Copy PEI post-memory pase VTd log.
    //
    BufferOffset = PcdGet32 (PcdVTdDxeLogBufferSize);
    if (PcdGet32 (PcdVTdPeiPostMemLogBufferSize) > 0) {
      if (HobPeiBuffer->PostMemBufferUsed > 0) {
        mVtdLogPeiPostMemBufferUsed = HobPeiBuffer->PostMemBufferUsed;
        CopyMem (&mVtdLogBuffer[BufferOffset], (UINT8 *) (UINTN) HobPeiBuffer->PostMemBuffer, mVtdLogPeiPostMemBufferUsed);
      }
    }

    mVtdLogPeiError = HobPeiBuffer->VtdLogPeiError;
  } else {
    //
    // Do not find PEI Vtd log, clear PEI pre-memory phase buffer.
    //
    ZeroMem (&mVtdLogBuffer[BufferOffset], sizeof (VTDLOG_PEI_PRE_MEM_INFO) * VTD_LOG_PEI_PRE_MEM_BAR_MAX);
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiVTdLogProtocolGuid,
                  &mIntelVTdLog,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}
