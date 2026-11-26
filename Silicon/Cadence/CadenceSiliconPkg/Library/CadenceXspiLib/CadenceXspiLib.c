/** @file

  Cadence xSPI controller library.

  Provides NOR flash read, write, and erase operations on top of the Cadence
  xSPI controller. Transactions are issued as STIG commands and data is moved
  through the controller's direct-access (SDMA) window.

  All flash operations are serialized through the optional bus lock callbacks
  so that the controller can be shared with another agent. The library keeps a
  single controller instance and supports being used at runtime.

  Copyright (c) 2025-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CadenceXspiLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

//
// NOR flash command set used by the library (4-byte address variants).
//
#define CADENCE_XSPI_FLASH_READ       0x13
#define CADENCE_XSPI_FLASH_WRITE      0x12
#define CADENCE_XSPI_FLASH_ERASE      0x21
#define CADENCE_XSPI_FLASH_WREN       0x06
#define CADENCE_XSPI_FLASH_RDSR       0x05
#define CADENCE_XSPI_FLASH_RDID       0x9F
#define CADENCE_XSPI_FLASH_PAGE_SIZE  0x100
#define CADENCE_XSPI_FLASH_WIP_BIT    1

STATIC CADENCE_XSPI_CONFIG  mXspiConfig;
STATIC BOOLEAN              mXspiInitialized;

/**
  Acquire the platform bus lock, if one was registered.

  @retval  EFI_SUCCESS  The bus is available, or no lock callback is set.
  @retval  Others       The value returned by the lock callback.
**/
STATIC
EFI_STATUS
XspiLock (
  VOID
  )
{
  if (mXspiConfig.BusOps.Lock == NULL) {
    return EFI_SUCCESS;
  }

  return mXspiConfig.BusOps.Lock (mXspiConfig.BusOps.Context);
}

/**
  Release the platform bus lock, if one was registered.
**/
STATIC
VOID
XspiUnlock (
  VOID
  )
{
  if (mXspiConfig.BusOps.Unlock != NULL) {
    mXspiConfig.BusOps.Unlock (mXspiConfig.BusOps.Context);
  }
}

/**
  Wait for the controller to return to idle state.

  @retval  EFI_SUCCESS  The controller is now idle.
  @retval  EFI_TIMEOUT  The controller is still busy.
**/
STATIC
EFI_STATUS
XspiWaitForCtrlIdle (
  VOID
  )
{
  CTRL_STATUS  CtrlStatus;
  INT32        Timeout;

  Timeout = SPI_OP_IDLE_TIMEOUT_MS * 100;
  do {
    CtrlStatus.Uint32 = XSPI_READ32 (mXspiConfig.MpiBase + CTRL_STAT_OFFSET);
    if (CtrlStatus.Bits.CtrlBusy) {
      if (!EfiAtRuntime ()) {
        gBS->Stall (10);
      }

      Timeout--;
    }
  } while (CtrlStatus.Bits.CtrlBusy && (Timeout > 0));

  return (Timeout > 0) ? EFI_SUCCESS : EFI_TIMEOUT;
}

/**
  Select the controller work mode.

  Waits for the controller to become idle before updating the work mode field.

  @param[in]  Mode  Controller work mode to select.

  @retval  EFI_SUCCESS  The controller is now operating in the given mode.
  @retval  EFI_TIMEOUT  The controller did not become idle.
**/
STATIC
EFI_STATUS
XspiSetMode (
  IN XSPI_MODE  Mode
  )
{
  EFI_STATUS   Status;
  CTRL_CONFIG  CtrlCfg;

  Status = XspiWaitForCtrlIdle ();
  if (!EFI_ERROR (Status)) {
    CtrlCfg.Uint32        = XSPI_READ32 (mXspiConfig.MpiBase + CTRL_CFG_OFFSET);
    CtrlCfg.Bits.WorkMode = Mode;
    XSPI_WRITE32 (mXspiConfig.MpiBase + CTRL_CFG_OFFSET, CtrlCfg.Uint32);
  }

  return Status;
}

/**
  Wait for a SDMA data transfer to complete.

  @retval  EFI_SUCCESS  The SDMA operation did complete.
  @retval  EFI_TIMEOUT  The SDMA operation did not complete.
**/
STATIC
EFI_STATUS
XspiWaitForSdma (
  VOID
  )
{
  UINT32  CtrlStatus;
  INT32   Timeout;

  Timeout = 10000;
  do {
    CtrlStatus = XSPI_READ32 (mXspiConfig.MpiBase + CTRL_CMD_INTR_STATUS);
  } while (!(CtrlStatus & SDMA_TRIGGER_MASK) && (--Timeout > 0));

  return ((CtrlStatus & SDMA_TRIGGER_MASK) != 0) ? EFI_SUCCESS : EFI_TIMEOUT;
}

/**
  Wait for a STIG command to complete.

  @retval  EFI_SUCCESS  The STIG operation did complete.
  @retval  EFI_TIMEOUT  The STIG operation did not complete.
**/
STATIC
EFI_STATUS
XspiWaitForStigReady (
  VOID
  )
{
  CTRL_STATUS  CtrlStatus;
  INT32        Timeout;

  Timeout = SPI_OP_IDLE_TIMEOUT_MS * 10000;
  do {
    CtrlStatus.Uint32 = XSPI_READ32 (mXspiConfig.MpiBase + CTRL_STAT_OFFSET);
    if (CtrlStatus.Bits.GcmdEngBusy) {
      if (!EfiAtRuntime ()) {
        gBS->Stall (10);
      }

      Timeout--;
    }
  } while (CtrlStatus.Bits.GcmdEngBusy && (Timeout > 0));

  return (Timeout > 0) ? EFI_SUCCESS : EFI_TIMEOUT;
}

/**
  Move the received data out of the direct-access window.

  The transfer size is taken from the controller's SDMA size register.

  @param[out]  Buffer  Destination buffer.
  @param[out]  Length  Receives the number of bytes read.

  @retval  EFI_SUCCESS       All data was copied.
  @retval  EFI_DEVICE_ERROR  The controller reports a zero transfer size.
**/
STATIC
EFI_STATUS
XspiReadSdma (
  OUT VOID    *Buffer,
  OUT UINT32  *Length
  )
{
  UINT32  Size;
  UINT64  Tmp64;
  UINT8   *Ptr;

  Size = XSPI_READ32 (mXspiConfig.MpiBase + CTRL_CFG_SDMA_SIZE);
  if (Size == 0) {
    *Length = 0;
    return EFI_DEVICE_ERROR;
  }

  *Length = Size;
  Ptr     = Buffer;
  while (Size >= XFER_QWORD_BYTECOUNT) {
    Tmp64 = XSPI_READ64 (mXspiConfig.DirectBase);
    CopyMem (Ptr, &Tmp64, sizeof (Tmp64));
    Size -= sizeof (Tmp64);
    Ptr  += sizeof (Tmp64);
  }

  if (Size != 0) {
    Tmp64 = XSPI_READ64 (mXspiConfig.DirectBase);
    CopyMem (Ptr, &Tmp64, Size);
  }

  return EFI_SUCCESS;
}

/**
  Move data into the direct-access window for an SDMA write.

  The transfer size is taken from the controller's SDMA size register.

  @param[in]      Buffer  Source buffer.
  @param[in,out]  Length  On input, the number of bytes to write. Receives the
                          number of bytes actually written.

  @retval  EFI_SUCCESS       All data was copied.
  @retval  EFI_DEVICE_ERROR  The controller reports a zero transfer size.
**/
STATIC
EFI_STATUS
XspiWriteSdma (
  IN VOID        *Buffer,
  IN OUT UINT32  *Length
  )
{
  UINT32  Size;
  UINT64  Tmp64;
  UINT8   *Ptr;

  Size = XSPI_READ32 (mXspiConfig.MpiBase + CTRL_CFG_SDMA_SIZE);
  if (Size == 0) {
    *Length = 0;
    return EFI_DEVICE_ERROR;
  }

  if (*Length > Size) {
    *Length = Size;
  } else {
    Size = *Length;
  }

  Ptr = Buffer;
  while (Size >= XFER_QWORD_BYTECOUNT) {
    CopyMem (&Tmp64, Ptr, sizeof (Tmp64));
    XSPI_WRITE64 (mXspiConfig.DirectBase, Tmp64);
    Size -= sizeof (Tmp64);
    Ptr  += sizeof (Tmp64);
  }

  if (Size != 0) {
    Tmp64 = 0;
    CopyMem (&Tmp64, Ptr, Size);
    XSPI_WRITE64 (mXspiConfig.DirectBase, Tmp64);
  }

  return EFI_SUCCESS;
}

/**
  Trigger a STIG command.

  @param[in]  CmdReg  Pointer to the array of values corresponding to the STIG
                      command and its parameters.
**/
STATIC
VOID
XspiTriggerCommand (
  IN UINT32  *CmdReg
  )
{
  XSPI_WRITE32 (mXspiConfig.MpiBase + CMD_REG5_OFFSET, CmdReg[5]);
  XSPI_WRITE32 (mXspiConfig.MpiBase + CMD_REG4_OFFSET, CmdReg[4]);
  XSPI_WRITE32 (mXspiConfig.MpiBase + CMD_REG3_OFFSET, CmdReg[3]);
  XSPI_WRITE32 (mXspiConfig.MpiBase + CMD_REG2_OFFSET, CmdReg[2]);
  XSPI_WRITE32 (mXspiConfig.MpiBase + CMD_REG1_OFFSET, CmdReg[1]);
  XSPI_WRITE32 (mXspiConfig.MpiBase + CMD_REG0_OFFSET, CmdReg[0]);
}

EFI_STATUS
CadenceXspiInit (
  IN CONST CADENCE_XSPI_CONFIG  *Config
  )
{
  if ((Config == NULL) || (Config->MpiBase == 0) || (Config->DirectBase == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&mXspiConfig, Config, sizeof (mXspiConfig));
  mXspiInitialized = TRUE;

  return EFI_SUCCESS;
}

VOID
CadenceXspiVirtualNotify (
  VOID
  )
{
  if (!mXspiInitialized) {
    return;
  }

  EfiConvertPointer (0x0, (VOID **)&mXspiConfig.MpiBase);
  EfiConvertPointer (0x0, (VOID **)&mXspiConfig.DirectBase);

  if (mXspiConfig.BusOps.Lock != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mXspiConfig.BusOps.Lock);
    EfiConvertPointer (0x0, (VOID **)&mXspiConfig.BusOps.Unlock);
  }

  if (mXspiConfig.BusOps.Context != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mXspiConfig.BusOps.Context);
  }
}

EFI_STATUS
CadenceXspiSendCommand (
  IN     UINT8            OpCode,
  IN     UINT32           Address,
  IN     UINT32           Size,
  IN     BOOLEAN          Data,
  IN OUT VOID             *Buffer,
  IN     CADENCE_XSPI_OP  Op
  )
{
  EFI_STATUS  Status;
  UINT32      CmdReg[6];
  UINT32      CmdStatus;

  if (!mXspiInitialized) {
    return EFI_INVALID_PARAMETER;
  }

  Status = XspiWaitForCtrlIdle ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = XspiSetMode (XspiModeStig);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (CmdReg, sizeof (CmdReg));

  CmdReg[1]  = Data ? StigInstType1 : StigInstType0;
  CmdReg[1] |= ((Address & 0xFF) << 24);
  CmdReg[2]  = ((Address >> 8) & 0xFF);
  CmdReg[2] |= (((Address >> 16) & 0xFF) << 8);
  CmdReg[2] |= (((Address >> 24) & 0xFF) << 16);

  CmdReg[3] = OpCode << 16;
  if ((Op == CadenceXspiOpRead) || (Op == CadenceXspiOpWrite) ||
      (Op == CadenceXspiOpErase))
  {
    CmdReg[3] |= 4 << 28;
  }

  CmdReg[4] = mXspiConfig.ChipSelect << 12;

  XspiTriggerCommand (CmdReg);

  if (Data) {
    CmdReg[0] = STIG_DONE_FLAG;
    CmdReg[1] = StigInstTypeData;
    CmdReg[2] = (Size & 0xFFFF) << 16;
    CmdReg[3] = (Size >> 16) & 0xFFFF;

    if ((Op == CadenceXspiOpRead) || (Op == CadenceXspiOpRegRead)) {
      CmdReg[4] = CadenceXspiOpRead << 4;
    } else if ((Op == CadenceXspiOpWrite) || (Op == CadenceXspiOpRegWrite)) {
      CmdReg[4] = CadenceXspiOpWrite << 4;
    }

    CmdReg[4] |= mXspiConfig.ChipSelect << 12;

    XspiTriggerCommand (CmdReg);

    Status = XspiWaitForSdma ();
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  if (Size != 0) {
    if ((Op == CadenceXspiOpRead) || (Op == CadenceXspiOpRegRead)) {
      Status = XspiReadSdma (Buffer, &Size);
    } else if ((Op == CadenceXspiOpWrite) || (Op == CadenceXspiOpRegWrite)) {
      Status = XspiWriteSdma (Buffer, &Size);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = XspiWaitForStigReady ();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  CmdStatus = XSPI_READ32 (mXspiConfig.MpiBase + CMD_STAT_OFFSET);
  if (((CmdStatus & STIG_COMPLETE_MASK) == 0) ||
      ((CmdStatus & STIG_FAIL_MASK) != 0))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Poll the flash status register until the write or erase in progress bit
  clears.

  @retval  EFI_SUCCESS       The device reported completion.
  @retval  Others            The status read failed, or the device stayed busy.
**/
STATIC
EFI_STATUS
XspiWaitForWriteCompletion (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      Register;
  INT32       Timeout;

  Register = 0;
  Timeout  = 10000;

  do {
    Status = CadenceXspiSendCommand (
               CADENCE_XSPI_FLASH_RDSR,
               0,
               sizeof (Register),
               TRUE,
               &Register,
               CadenceXspiOpRegRead
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } while ((Register & CADENCE_XSPI_FLASH_WIP_BIT) && (--Timeout > 0));

  return ((Register & CADENCE_XSPI_FLASH_WIP_BIT) != 0) ?
         EFI_DEVICE_ERROR : EFI_SUCCESS;
}

EFI_STATUS
CadenceXspiFlashReadId (
  OUT UINT8  *Id
  )
{
  EFI_STATUS  Status;

  if ((Id == NULL) || !mXspiInitialized) {
    return EFI_INVALID_PARAMETER;
  }

  Status = XspiLock ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CadenceXspiSendCommand (
             CADENCE_XSPI_FLASH_RDID,
             0,
             CADENCE_XSPI_FLASH_ID_SIZE,
             TRUE,
             Id,
             CadenceXspiOpRegRead
             );
  XspiUnlock ();

  return Status;
}

EFI_STATUS
CadenceXspiFlashRead (
  IN  UINT32  Address,
  IN  UINTN   Length,
  OUT VOID    *Buffer
  )
{
  EFI_STATUS  Status;

  if ((Buffer == NULL) || !mXspiInitialized) {
    return EFI_INVALID_PARAMETER;
  }

  Status = XspiLock ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CadenceXspiSendCommand (
             CADENCE_XSPI_FLASH_READ,
             Address,
             (UINT32)Length,
             TRUE,
             Buffer,
             CadenceXspiOpRead
             );
  XspiUnlock ();

  return Status;
}

EFI_STATUS
CadenceXspiFlashWrite (
  IN UINT32  Address,
  IN UINTN   Length,
  IN VOID    *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       Chunk;
  UINT8       *Ptr;

  if ((Buffer == NULL) || !mXspiInitialized) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr    = Buffer;
  Status = XspiLock ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (Length != 0) {
    Chunk = MIN (
              Length,
              CADENCE_XSPI_FLASH_PAGE_SIZE -
              (Address % CADENCE_XSPI_FLASH_PAGE_SIZE)
              );

    Status = CadenceXspiSendCommand (
               CADENCE_XSPI_FLASH_WREN,
               0,
               0,
               FALSE,
               NULL,
               CadenceXspiOpNoData
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = CadenceXspiSendCommand (
               CADENCE_XSPI_FLASH_WRITE,
               Address,
               (UINT32)Chunk,
               TRUE,
               Ptr,
               CadenceXspiOpWrite
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = XspiWaitForWriteCompletion ();
    if (EFI_ERROR (Status)) {
      break;
    }

    Address += Chunk;
    Ptr     += Chunk;
    Length  -= Chunk;
  }

  XspiUnlock ();
  return Status;
}

EFI_STATUS
CadenceXspiFlashErase (
  IN UINT32  Address,
  IN UINTN   Length
  )
{
  EFI_STATUS  Status;

  if (!mXspiInitialized) {
    return EFI_INVALID_PARAMETER;
  }

  Status = XspiLock ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (Length != 0) {
    Status = CadenceXspiSendCommand (
               CADENCE_XSPI_FLASH_WREN,
               0,
               0,
               FALSE,
               NULL,
               CadenceXspiOpNoData
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = CadenceXspiSendCommand (
               CADENCE_XSPI_FLASH_ERASE,
               Address,
               0,
               FALSE,
               NULL,
               CadenceXspiOpErase
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = XspiWaitForWriteCompletion ();
    if (EFI_ERROR (Status)) {
      break;
    }

    Address += SIZE_4KB;
    Length  -= MIN (Length, (UINTN)SIZE_4KB);
  }

  XspiUnlock ();
  return Status;
}

