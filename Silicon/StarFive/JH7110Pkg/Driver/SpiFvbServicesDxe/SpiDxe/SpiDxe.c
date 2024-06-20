/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include "SpiDxe.h"

SPI_MASTER  *mSpiMasterInstance;

STATIC
VOID
SpiControllerEnable (
  IN UINT32  RegBase
  )
{
  UINT32  Reg;

  Reg  = MmioRead32 (RegBase + SPI_REG_CONFIG);
  Reg |= SPI_REG_CONFIG_ENABLE;
  MmioWrite32 (RegBase + SPI_REG_CONFIG, Reg);
}

STATIC
VOID
SpiControllerDisable (
  IN UINT32  RegBase
  )
{
  UINT32  Reg;

  Reg  = MmioRead32 (RegBase + SPI_REG_CONFIG);
  Reg &= ~SPI_REG_CONFIG_ENABLE;
  MmioWrite32 (RegBase + SPI_REG_CONFIG, Reg);
}

STATIC
VOID
SpiWriteSpeed (
  IN SPI_DEVICE_PARAMS  *Slave,
  IN UINT32             SclkHz,
  IN SPI_TIMING_PARAMS  *Timing
  )
{
  UINT32  Reg, Div, RefClkNs, SclkNs;
  UINT32  Tshsl, Tchsh, Tslch, Tsd2d;

  SpiControllerDisable (Slave->RegBase);

  /* Configure baudrate */
  Reg  = MmioRead32 (Slave->RegBase + SPI_REG_CONFIG);
  Reg &= ~(SPI_REG_CONFIG_BAUD_MASK << SPI_REG_CONFIG_BAUD_LSB);

  Div = DIV_ROUND_UP (Timing->RefClkHz, SclkHz * 2) - 1;

  if (Div > SPI_REG_CONFIG_BAUD_MASK) {
    Div = SPI_REG_CONFIG_BAUD_MASK;
  }

  DEBUG (
         (DEBUG_INFO, "%a(): RefClk %dHz sclk %dHz Div 0x%x, actual %dHz\n", __func__,
          Timing->RefClkHz, SclkHz, Div, Timing->RefClkHz / (2 * (Div + 1)))
         );

  Reg |= (Div << SPI_REG_CONFIG_BAUD_LSB);
  MmioWrite32 (Slave->RegBase + SPI_REG_CONFIG, Reg);

  /* Configure delay timing */
  RefClkNs = DIV_ROUND_UP (1000000000, Timing->RefClkHz);
  SclkNs = DIV_ROUND_UP (1000000000, SclkHz);

  if (Timing->TshslNs >= SclkNs + RefClkNs) {
    Timing->TshslNs -= SclkNs + RefClkNs;
  }

  if (Timing->TchshNs >= SclkNs + 3 * RefClkNs) {
    Timing->TchshNs -= SclkNs + 3 * RefClkNs;
  }

  Tshsl = DIV_ROUND_UP (Timing->TshslNs, RefClkNs);
  Tchsh = DIV_ROUND_UP (Timing->TchshNs, RefClkNs);
  Tslch = DIV_ROUND_UP (Timing->TslchNs, RefClkNs);
  Tsd2d = DIV_ROUND_UP (Timing->Tsd2dNs, RefClkNs);

  Reg = ((Tshsl & SPI_REG_DELAY_TSHSL_MASK)
         << SPI_REG_DELAY_TSHSL_LSB);
  Reg |= ((Tchsh & SPI_REG_DELAY_TCHSH_MASK)
          << SPI_REG_DELAY_TCHSH_LSB);
  Reg |= ((Tslch & SPI_REG_DELAY_TSLCH_MASK)
          << SPI_REG_DELAY_TSLCH_LSB);
  Reg |= ((Tsd2d & SPI_REG_DELAY_TSD2D_MASK)
          << SPI_REG_DELAY_TSD2D_LSB);
  MmioWrite32 (Slave->RegBase + SPI_REG_DELAY, Reg);

  SpiControllerEnable (Slave->RegBase);
}

STATIC
EFI_STATUS
SpiWaitIdle (
  IN UINT32  RegBase
  )
{
  BOOLEAN IsIdle;
  UINT32  Count     = 0;
  UINT32  TimeoutMs = 5000000;

  do {
    IsIdle = (BOOLEAN)((MmioRead32(RegBase + SPI_REG_CONFIG) >>
                        SPI_REG_CONFIG_IDLE_LSB) & 0x1);
    Count = (IsIdle) ? (Count+1) : 0;

    /*
     * Make sure the QSPI controller is in really idle
     * for n period of time before proceed
     */
    if (Count >= SPI_POLL_IDLE_RETRY) {
      return EFI_SUCCESS;
    }

    gBS->Stall (1);
  } while (TimeoutMs);

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
SpiExecFlashCmd (
  IN UINT32  RegBase,
  IN UINT32  Reg
  )
{
  EFI_STATUS  Status;
  UINT32      Retry = SPI_REG_RETRY;

  /* Write the CMDCTRL without start execution */
  MmioWrite32 (RegBase + SPI_REG_CMDCTRL, Reg);
  /* Start execute */
  Reg |= SPI_REG_CMDCTRL_EXECUTE;
  MmioWrite32 (RegBase + SPI_REG_CMDCTRL, Reg);

  while (Retry--) {
    Reg = MmioRead32 (RegBase + SPI_REG_CMDCTRL);
    if ((Reg & SPI_REG_CMDCTRL_INPROGRESS) == 0) {
      break;
    }
    gBS->Stall (1);
  }

  if (!Retry) {
    DEBUG ((DEBUG_ERROR, "%a(): flash command execution Timeout\n", __func__));
    return EFI_TIMEOUT;
  }

  /* Polling QSPI idle status */
  Status = SpiWaitIdle (RegBase);
  if (EFI_ERROR (Status)) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/* For command RDID, RDSR. */
EFI_STATUS
SpiCommandRead (
  IN SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE_PARAMS  *Slave,
  IN OUT SPI_OP_PARAMS   *Cmds
  )
{
  UINT32      Reg;
  UINT32      ReadLen;
  EFI_STATUS  Status;
  SPI_MASTER *SpiMaster;
  UINT32      RxLen  = Cmds->Data.NBytes;
  VOID        *RxBuf = Cmds->Data.Buf.In;

  SpiMaster = SPI_MASTER_FROM_SPI_MASTER_PROTOCOL (This);
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (&SpiMaster->Lock);
  }

  if ((RxLen > SPI_STIG_DATA_LEN_MAX) || !RxBuf) {
    DEBUG ((DEBUG_ERROR, "%a(): Invalid input arguments RxLen %d\n", __func__, RxLen));
    Status = EFI_INVALID_PARAMETER;
    goto Fail;
  }

  Reg = Cmds->Cmd.OpCode << SPI_REG_CMDCTRL_OPCODE_LSB;
  Reg |= (0x1 << SPI_REG_CMDCTRL_RD_EN_LSB);

  /* 0 means 1 byte */
  Reg |= (((RxLen - 1) & SPI_REG_CMDCTRL_RD_BYTES_MASK)
          << SPI_REG_CMDCTRL_RD_BYTES_LSB);
  Status = SpiExecFlashCmd (Slave->RegBase, Reg);
  if (EFI_ERROR (Status)) {
    goto Fail;
  }

  Reg = MmioRead32 (Slave->RegBase + SPI_REG_CMDREADDATALOWER);

  /* Put the read value into rx_buf */
  ReadLen = (RxLen > 4) ? 4 : RxLen;
  CopyMem (RxBuf, &Reg, ReadLen);
  RxBuf += ReadLen;

  if (RxLen > 4) {
    Reg = MmioRead32 (Slave->RegBase + SPI_REG_CMDREADDATAUPPER);

    ReadLen = RxLen - ReadLen;
    CopyMem (RxBuf, &Reg, ReadLen);
  }

  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return EFI_SUCCESS;

Fail:
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return Status;
}

STATIC
EFI_STATUS
SpiGetReadSramLevel (
  IN UINT32   RegBase,
  OUT UINT16  *SramLvl
  )
{
  UINT32  Reg = MmioRead32 (RegBase + SPI_REG_SDRAMLEVEL);
  Reg >>= SPI_REG_SDRAMLEVEL_RD_LSB;
  *SramLvl = (UINT16)(Reg & SPI_REG_SDRAMLEVEL_RD_MASK);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SpiWaitForData (
  IN UINT32  RegBase,
  UINT16    *SramLvl
  )
{
  UINT32  Timeout = 10000;

  while (Timeout--) {
    SpiGetReadSramLevel (RegBase, SramLvl);
    if (SramLvl != 0) {
      return EFI_SUCCESS;
    }
    gBS->Stall (1);
  }

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
SpiWaitForBitLe32 (
  IN INT32          Reg,
  IN CONST UINT32   Mask,
  IN CONST BOOLEAN  Set,
  IN CONST UINT32   TimeoutMs
  )
{
  UINT32  Val;
  UINTN   Start = TimeoutMs*1000;

  while(1) {
    Val = MmioRead32 (Reg);

    if (!Set) {
      Val = ~Val;
    }

    if ((Val & Mask) == Mask) {
      return EFI_SUCCESS;
    }

    if (Start == 0) {
      break;
    } else {
      Start--;
    }

    gBS->Stall (1);
  }

  DEBUG ((DEBUG_ERROR, "Timeout (Reg=%lx Mask=%x wait_set=%d)\n", Reg, Mask, Set));

  return EFI_TIMEOUT;
}

STATIC
VOID
SpiReadByte (
  IN VOID  *Addr,
  IN VOID  *Data,
  IN UINT16 ByteLen
  )
{
  UINT8  *AddrPtr;
  UINT8  *DataPtr;

  AddrPtr  = (UINT8 *)Addr;
  DataPtr = (UINT8 *)Data;

  while (ByteLen) {
    *DataPtr = *AddrPtr;
    DataPtr++;
    ByteLen--;
  }
}

STATIC
VOID
SpiReadLong (
  VOID    *Addr,
  VOID    *Data,
  UINT16  LongLen
  )
{
  UINT32  *AddrPtr;
  UINT32  *DataPtr;

  AddrPtr = (UINT32 *)Addr;
  DataPtr = (UINT32 *)Data;

  while (LongLen) {
    *DataPtr = *AddrPtr;
    DataPtr++;
    LongLen--;
  }
}

EFI_STATUS
SpiDataRead (
  IN SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE_PARAMS  *Slave,
  IN OUT SPI_OP_PARAMS  *Cmds
  )
{
  SPI_MASTER  *SpiMaster;
  UINT8       *RxBuf      = Cmds->Data.Buf.In;
  UINT32      Remaining   = Cmds->Data.NBytes;
  UINT16      BytesToRead = 0;
  EFI_STATUS  Status;
  UINT32      Reg;

  SpiMaster = SPI_MASTER_FROM_SPI_MASTER_PROTOCOL (This);
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (&SpiMaster->Lock);
  }

  if (!Cmds->Addr.NBytes) {
    Status = EFI_ABORTED;
    goto Fail;
  }

  /* Setup the indirect trigger start address */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTRDSTARTADDR, Cmds->Addr.Val);

  /* Register command */
  Reg  = Cmds->Cmd.OpCode << SPI_REG_RD_INSTR_OPCODE_LSB;
  MmioWrite32 (Slave->RegBase + SPI_REG_RD_INSTR, Reg);

  /* Set device size */
  Reg  = MmioRead32 (Slave->RegBase + SPI_REG_SIZE);
  Reg &= ~SPI_REG_SIZE_ADDRESS_MASK;
  Reg |= (Cmds->Addr.NBytes - 1);
  MmioWrite32 (Slave->RegBase + SPI_REG_SIZE, Reg);

  /* Setup indirect read bytes */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTRDBYTES, Remaining);

  /* Start the indirect read transfer */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTRD, SPI_REG_INDIRECTRD_START);

  while (Remaining > 0) {
    Status = SpiWaitForData (Slave->RegBase, &BytesToRead);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Indirect write timed out\n", __func__));
      goto Fail;
    }

    while (BytesToRead != 0) {
      BytesToRead *= Slave->FifoWidth;
      if (BytesToRead > Remaining) {
        BytesToRead = Remaining;
      }

      if (((UINTN)RxBuf % 4) || (BytesToRead % 4)) {
        SpiReadByte (Slave->AhbBase, RxBuf, BytesToRead);
      } else {
        SpiReadLong (Slave->AhbBase, RxBuf, BytesToRead >> 2);
      }

      RxBuf        += BytesToRead;
      Remaining    -= BytesToRead;
      SpiGetReadSramLevel (Slave->RegBase, &BytesToRead);
    }
  }

  /* Check indirect done status */
  Status = SpiWaitForBitLe32 (
                              Slave->RegBase + SPI_REG_INDIRECTRD,
                              SPI_REG_INDIRECTRD_DONE,
                              1,
                              10
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Indirect read completion error\n"));
    goto Fail;
  }

  /* Clear indirect completion status */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTRD, SPI_REG_INDIRECTRD_DONE);

  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return EFI_SUCCESS;

Fail:
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTRD, SPI_REG_INDIRECTRD_CANCEL);
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return EFI_ABORTED;
}

/* For commands: WRSR, WREN, WRDI, CHIP_ERASE, BE, etc. */
EFI_STATUS
SpiCommandWrite (
  IN SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE_PARAMS  *Slave,
  IN OUT SPI_OP_PARAMS  *Cmds
  )
{
  UINT32      Reg;
  UINT32      WriteData;
  UINT32      WriteLen;
  SPI_MASTER  *SpiMaster;
  UINT32      TxLen   = Cmds->Data.NBytes;
  CONST VOID  *TxBuf  = Cmds->Data.Buf.Out;
  UINT32      Addr;
  EFI_STATUS  Status;

  SpiMaster = SPI_MASTER_FROM_SPI_MASTER_PROTOCOL (This);
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (&SpiMaster->Lock);
  }

  /* Reorder address to SPI bus order if only transferring address */
  if (!TxLen) {
    Addr = SwapBytes32 (Cmds->Addr.Val);
    if (Cmds->Addr.NBytes == 3) {
      Addr >>= 8;
    }

    TxBuf = &Addr;
    TxLen = Cmds->Addr.NBytes;
  }

  if (TxLen > SPI_STIG_DATA_LEN_MAX) {
    DEBUG ((DEBUG_ERROR, "QSPI: Invalid input arguments TxLen %d\n", TxLen));
    Status = EFI_INVALID_PARAMETER;
    goto Fail;
  }

  Reg = Cmds->Cmd.OpCode << SPI_REG_CMDCTRL_OPCODE_LSB;

  if (TxLen) {
    Reg |= (0x1 << SPI_REG_CMDCTRL_WR_EN_LSB);
    Reg |= ((TxLen - 1) & SPI_REG_CMDCTRL_WR_BYTES_MASK)
           << SPI_REG_CMDCTRL_WR_BYTES_LSB;

    WriteLen = TxLen > 4 ? 4 : TxLen;
    CopyMem (&WriteData, TxBuf, WriteLen);
    MmioWrite32 (Slave->RegBase + SPI_REG_CMDWRITEDATALOWER, WriteData);

    if (TxLen > 4) {
      TxBuf   += WriteLen;
      WriteLen = TxLen - WriteLen;
      CopyMem (&WriteData, TxBuf, WriteLen);
      MmioWrite32 (Slave->RegBase + SPI_REG_CMDWRITEDATAUPPER, WriteData);
    }
  }

  Status = SpiExecFlashCmd (Slave->RegBase, Reg);
  if (EFI_ERROR (Status)) {
    goto Fail;
  }

  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return EFI_SUCCESS;

Fail:
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return Status;
}

STATIC
VOID
SpiDelayNanoSec (
  IN UINTN  nsec
  )
{
  UINT32  Timeout = DIV_ROUND_UP (nsec, 1000);

  do {
    Timeout--;
    gBS->Stall (1);
  } while (Timeout);
}

STATIC
VOID
SpiWriteLong (
  IN VOID        *Addr,
  IN CONST VOID  *Data,
  IN INTN        LongLen
  )
{
  UINT32  *AddrPtr;
  UINT32  *DataPtr;

  AddrPtr  = (UINT32 *)Addr;
  DataPtr = (UINT32 *)Data;

  while (LongLen) {
    *AddrPtr = *DataPtr;
    DataPtr++;
    LongLen--;
  }
}

STATIC
VOID
SpiWriteByte (
  IN VOID        *Addr,
  IN CONST VOID  *Data,
  IN INTN        ByteLen
  )
{
  UINT8  *AddrPtr;
  UINT8  *DataPtr;

  AddrPtr  = (UINT8 *)Addr;
  DataPtr  = (UINT8 *)Data;

  while (ByteLen) {
    *AddrPtr = *DataPtr;
    DataPtr++;
    ByteLen--;
  }
}

EFI_STATUS
SpiDataWrite (
  IN SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE_PARAMS   *Slave,
  IN OUT SPI_OP_PARAMS  *Cmds
  )
{
  UINT32       Reg;
  SPI_MASTER   *SpiMaster;
  UINT32       PageSize  = Slave->Info->PageSize;
  UINT32       Remaining = Cmds->Data.NBytes;
  CONST UINT8  *TxBuf = Cmds->Data.Buf.Out;
  UINT32       WriteBytes;
  EFI_STATUS   Status;

  SpiMaster = SPI_MASTER_FROM_SPI_MASTER_PROTOCOL (This);
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (&SpiMaster->Lock);
  }

  if (!Cmds->Addr.NBytes) {
    return EFI_ABORTED;
  }

  /* Write opcode to write instruction register */
  Reg  = Cmds->Cmd.OpCode << SPI_REG_WR_INSTR_OPCODE_LSB;
  MmioWrite32 (Slave->RegBase + SPI_REG_WR_INSTR, Reg);

  /* Set buffer address */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTWRSTARTADDR, Cmds->Addr.Val);

  /* Configure device size */
  Reg  = MmioRead32 (Slave->RegBase + SPI_REG_SIZE);
  Reg &= ~SPI_REG_SIZE_ADDRESS_MASK;
  Reg |= (Cmds->Addr.NBytes - 1);
  MmioWrite32 (Slave->RegBase + SPI_REG_SIZE, Reg);

  /* Configure the indirect read transfer bytes */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTWRBYTES, Remaining);

  /* Start the indirect write transfer */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTWR, SPI_REG_INDIRECTWR_START);

  /* Delay is required for QSPI module to synchronized internally */
  SpiDelayNanoSec (Slave->WriteDelay);

  while (Remaining > 0) {
    WriteBytes = Remaining > PageSize ? PageSize : Remaining;
    SpiWriteLong (Slave->AhbBase, TxBuf, WriteBytes >> 2);
    if (WriteBytes % 4) {
      SpiWriteByte (
                    Slave->AhbBase,
                    TxBuf + ROUND_DOWN (WriteBytes, 4),
                    WriteBytes % 4
                    );
    }

    Status = SpiWaitForBitLe32 (
                                Slave->RegBase + SPI_REG_SDRAMLEVEL,
                                SPI_REG_SDRAMLEVEL_WR_MASK <<
                                SPI_REG_SDRAMLEVEL_WR_LSB,
                                0,
                                10
                                );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(): Indirect write timed out (%d)\n", __func__, Status));
      goto FailWrite;
    }

    TxBuf  += WriteBytes;
    Remaining -= WriteBytes;
  }

  /* Check indirect done status */
  Status = SpiWaitForBitLe32 (
                              Slave->RegBase + SPI_REG_INDIRECTWR,
                              SPI_REG_INDIRECTWR_DONE,
                              1,
                              10
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Indirect write completion error (%d)\n", Status));
    goto FailWrite;
  }

  /* Clear indirect completion status */
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTWR, SPI_REG_INDIRECTWR_DONE);
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return EFI_SUCCESS;

FailWrite:
  MmioWrite32 (Slave->RegBase + SPI_REG_INDIRECTWR, SPI_REG_INDIRECTWR_CANCEL);
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }
  return Status;
}

STATIC
VOID
SpiConfigGetDataCapture (
  IN UINT32  RegBase,
  IN UINT32  ByPass,
  IN UINT32  Delay
  )
{
  UINT32  Reg;

  SpiControllerDisable (RegBase);

  Reg = MmioRead32 (RegBase + SPI_REG_RD_DATA_CAPTURE);

  if (ByPass) {
    Reg |= SPI_REG_RD_DATA_CAPTURE_BYPASS;
  } else {
    Reg &= ~SPI_REG_RD_DATA_CAPTURE_BYPASS;
  }

  Reg &= ~(SPI_REG_RD_DATA_CAPTURE_DELAY_MASK
           << SPI_REG_RD_DATA_CAPTURE_DELAY_LSB);

  Reg |= (Delay & SPI_REG_RD_DATA_CAPTURE_DELAY_MASK)
         << SPI_REG_RD_DATA_CAPTURE_DELAY_LSB;

  MmioWrite32 (RegBase + SPI_REG_RD_DATA_CAPTURE, Reg);

  SpiControllerEnable (RegBase);
}

STATIC
EFI_STATUS
SpiSpeedCalibration (
  IN SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE_PARAMS  *Slave,
  IN SPI_TIMING_PARAMS  *Timing
  )
{
  UINT8         IdLen = 3;
  UINT32        IdInit = 0, IdCali = 0;
  INTN          RangeLow = -1, RangeHigh = -1;
  SPI_OP_PARAMS  CmdsInitialId = SPI_READID_OP ((UINT8 *)&IdInit, IdLen);
  SPI_OP_PARAMS  CmdsCalibrateId = SPI_READID_OP ((UINT8 *)&IdCali, IdLen);
  EFI_STATUS    Status;

  /* Start calibration with slowest clock speed at 1 MHz */
  SpiWriteSpeed (Slave, SPI_MIN_HZ, Timing);

  /* Set the read data capture delay register to 0 */
  SpiConfigGetDataCapture (Slave->RegBase, 1, 0);

  /* Get flash ID value as reference */
  Status = SpiCommandRead (This, Slave, &CmdsInitialId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Spi: Calibration failed (read id)\n"));
    return EFI_ABORTED;
  }

  /* Use the input speed */
  SpiWriteSpeed (Slave, SPI_MAX_HZ, Timing);

  /* Find high and low range */
  for (UINT8 i = 0; i < SPI_READ_CAPTURE_MAX_DELAY; i++) {
    /* Change the read data capture delay register */
    SpiConfigGetDataCapture (Slave->RegBase, 1, i);

    /* Read flash ID for comparison later */
    Status = SpiCommandRead (This, Slave, &CmdsCalibrateId);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Spi: Calibration failed (read)\n"));
      return EFI_ABORTED;
    }

    /* Verify low range */
    if ((RangeLow == -1) && (IdCali == IdInit)) {
      RangeLow = i;
      continue;
    }

    /* Verify high range */
    if ((RangeLow != -1) && (IdCali != IdInit)) {
      RangeHigh = i - 1;
      break;
    }

    RangeHigh = i;
  }

  if (RangeLow == -1) {
    DEBUG ((DEBUG_ERROR, "Spi: Calibration failed\n"));
    return EFI_ABORTED;
  }

  /*
  * Set the final value for read data capture delay register based
  * on the calibrated value
  */
  SpiConfigGetDataCapture (Slave->RegBase, 1, (RangeHigh + RangeLow) / 2);
  DEBUG (
         (DEBUG_INFO, "Spi: Read data capture delay calibrated to %d (%d - %d)\n",
          (RangeHigh + RangeLow) / 2, RangeLow, RangeHigh)
         );

  return EFI_SUCCESS;
}

EFI_STATUS
SpiSetupDevice (
  IN SPI_MASTER_PROTOCOL *This,
  OUT SPI_DEVICE_PARAMS  *Slave
  )
{
  SPI_TIMING_PARAMS  *Timing;
  EFI_STATUS  Status;

  if (!Slave) {
    Slave = AllocateZeroPool (sizeof (SPI_DEVICE_PARAMS));
    if (Slave == NULL) {
      DEBUG ((DEBUG_ERROR, "%a(): Cannot allocate memory Slave\n", __func__));
      Status = EFI_OUT_OF_RESOURCES;
      goto Fail;
    }
  }

  if (!Slave->Info) {
    Slave->Info = AllocateZeroPool (sizeof (NOR_FLASH_INFO));
    if (Slave->Info == NULL) {
      DEBUG ((DEBUG_ERROR, "%a(): Cannot allocate memory Slave->Info\n", __func__));
      Status = EFI_OUT_OF_RESOURCES;
      goto Fail;
    }
  }

  Timing = AllocateZeroPool (sizeof (SPI_TIMING_PARAMS));
  if (Timing == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot allocate memory Timing\n", __func__));
    FreePool(Slave);
    Status = EFI_OUT_OF_RESOURCES;
    goto Fail;
  }

  Slave->RegBase    = PcdGet32 (PcdSpiFlashRegBase);
  Slave->AhbBase    = (VOID *)(UINTN)PcdGet64 (PcdSpiFlashAhbBase);
  Slave->FifoWidth  = PcdGet8 (PcdSpiFlashFifoWidth);
  Timing->RefClkHz  = PcdGet32 (PcdSpiFlashRefClkHz);
  Timing->TshslNs   = PcdGet32 (PcdSpiFlashTshslNs);
  Timing->Tsd2dNs   = PcdGet32 (PcdSpiFlashTsd2dNs);
  Timing->TchshNs   = PcdGet32 (PcdSpiFlashTchshNs);
  Timing->TslchNs   = PcdGet32 (PcdSpiFlashTslchNs);

  Slave->WriteDelay = 50 * DIV_ROUND_UP (NSEC_PER_SEC, Timing->RefClkHz);

  Status = SpiSpeedCalibration (This, Slave, Timing);
  if (EFI_ERROR (Status)) {
    goto Fail;
  }

  FreePool(Timing);
  return EFI_SUCCESS;

Fail:
  FreePool(Slave);
  FreePool(Timing);
  return Status;
}

EFI_STATUS
SpiFreeDevice (
  IN SPI_DEVICE_PARAMS  *Slave
  )
{
  FreePool (Slave);

  return EFI_SUCCESS;
}

EFI_STATUS
SpiMasterInitProtocol (
  IN SPI_MASTER_PROTOCOL  *SpiMasterProtocol
  )
{
  SpiMasterProtocol->SetupDevice = SpiSetupDevice;
  SpiMasterProtocol->FreeDevice  = SpiFreeDevice;
  SpiMasterProtocol->CmdRead     = SpiCommandRead;
  SpiMasterProtocol->DataRead    = SpiDataRead;
  SpiMasterProtocol->CmdWrite    = SpiCommandWrite;
  SpiMasterProtocol->DataWrite   = SpiDataWrite;

  return EFI_SUCCESS;
}

EFI_STATUS
SpiEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mSpiMasterInstance = AllocateRuntimeZeroPool (sizeof (SPI_MASTER));
  if (mSpiMasterInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock (&mSpiMasterInstance->Lock, TPL_NOTIFY);

  SpiMasterInitProtocol (&mSpiMasterInstance->SpiMasterProtocol);

  mSpiMasterInstance->Signature = SPI_MASTER_SIGNATURE;

  Status = gBS->InstallMultipleProtocolInterfaces (
                                                   &(mSpiMasterInstance->Handle),
                                                   &gJH7110SpiMasterProtocolGuid,
                                                   &(mSpiMasterInstance->SpiMasterProtocol),
                                                   NULL
                                                   );
  if (EFI_ERROR (Status)) {
    FreePool (mSpiMasterInstance);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
