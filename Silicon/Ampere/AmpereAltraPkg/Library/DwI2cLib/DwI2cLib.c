/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/I2cLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#define I2cSync()  { asm volatile ("dmb ish" : : : "memory"); }

//
// Runtime needs to be 64K alignment
//
#define RUNTIME_ADDRESS_MASK    (~(SIZE_64KB - 1))
#define RUNTIME_ADDRESS_LENGTH  SIZE_64KB

//
// Private I2C bus data
//
typedef struct {
  UINTN      Base;
  UINT32     BusSpeed;
  UINT32     RxFifo;
  UINT32     TxFifo;
  UINT32     PollingTime;
  UINT32     Enabled;
  BOOLEAN    IsSmbus;
  BOOLEAN    PecCheck;
} DW_I2C_CONTEXT_T;

//
// I2C SCL counter macros
//
typedef enum {
  I2cSpeedModeStandard = 0,
  I2cSpeedModeFast,
} I2C_SPEED_MODE;

#define DW_I2C_MAXIMUM_SPEED_HZ  400000

typedef enum {
  I2cSclSpkLen = 0,
  I2cSclHcnt,
  I2cSclLcnt,
} I2C_SCL_PARAM;

STATIC UINT32  I2cSclParam[][3] = {
  /* SPK_LEN, HCNT, LCNT */
  [I2cSpeedModeStandard] = { 10, 0x3E2, 0x47D },   // SS (Standard Speed)
  [I2cSpeedModeFast]     = { 10, 0xA4,  0x13F },   // FS (Fast Speed)
};

STATIC BOOLEAN           mI2cRuntimeEnableArray[AC01_I2C_MAX_BUS_NUM] = { FALSE };
STATIC UINTN             mI2cBaseArray[AC01_I2C_MAX_BUS_NUM]          = { AC01_I2C_BASE_ADDRESS_LIST };
STATIC DW_I2C_CONTEXT_T  mI2cBusList[AC01_I2C_MAX_BUS_NUM];
STATIC UINTN             mI2cClock                  = 0;
STATIC EFI_EVENT         mVirtualAddressChangeEvent = NULL;

//
// Registers
//
#define DW_IC_CON                   0x0
#define DW_IC_CON_MASTER            BIT0
#define DW_IC_CON_SPEED_STD         BIT1
#define DW_IC_CON_SPEED_FAST        BIT2
#define DW_IC_CON_10BITADDR_MASTER  BIT4
#define DW_IC_CON_RESTART_EN        BIT5
#define DW_IC_CON_SLAVE_DISABLE     BIT6
#define DW_IC_TAR                   0x4
#define DW_IC_TAR_10BITS            BIT12
#define DW_IC_SAR                   0x8
#define DW_IC_DATA_CMD              0x10
#define DW_IC_DATA_CMD_RESTART      BIT10
#define DW_IC_DATA_CMD_STOP         BIT9
#define DW_IC_DATA_CMD_CMD          BIT8
#define DW_IC_DATA_CMD_DAT_MASK     0xFF
#define DW_IC_SS_SCL_HCNT           0x14
#define DW_IC_SS_SCL_LCNT           0x18
#define DW_IC_FS_SCL_HCNT           0x1c
#define DW_IC_FS_SCL_LCNT           0x20
#define DW_IC_HS_SCL_HCNT           0x24
#define DW_IC_HS_SCL_LCNT           0x28
#define DW_IC_INTR_STAT             0x2c
#define DW_IC_INTR_MASK             0x30
#define DW_IC_INTR_RX_UNDER         BIT0
#define DW_IC_INTR_RX_OVER          BIT1
#define DW_IC_INTR_RX_FULL          BIT2
#define DW_IC_INTR_TX_EMPTY         BIT4
#define DW_IC_INTR_TX_ABRT          BIT6
#define DW_IC_INTR_ACTIVITY         BIT8
#define DW_IC_INTR_STOP_DET         BIT9
#define DW_IC_INTR_START_DET        BIT10
#define DW_IC_ERR_CONDITION \
                (DW_IC_INTR_RX_UNDER | DW_IC_INTR_RX_OVER | DW_IC_INTR_TX_ABRT)
#define DW_IC_RAW_INTR_STAT        0x34
#define DW_IC_CLR_INTR             0x40
#define DW_IC_CLR_RX_UNDER         0x44
#define DW_IC_CLR_RX_OVER          0x48
#define DW_IC_CLR_TX_ABRT          0x54
#define DW_IC_CLR_ACTIVITY         0x5c
#define DW_IC_CLR_STOP_DET         0x60
#define DW_IC_CLR_START_DET        0x64
#define DW_IC_ENABLE               0x6c
#define DW_IC_STATUS               0x70
#define DW_IC_STATUS_ACTIVITY      BIT0
#define DW_IC_STATUS_TFE           BIT2
#define DW_IC_STATUS_RFNE          BIT3
#define DW_IC_STATUS_MST_ACTIVITY  BIT5
#define DW_IC_TXFLR                0x74
#define DW_IC_RXFLR                0x78
#define DW_IC_SDA_HOLD             0x7c
#define DW_IC_TX_ABRT_SOURCE       0x80
#define DW_IC_ENABLE_STATUS        0x9c
#define DW_IC_COMP_PARAM_1         0xf4
#define  DW_IC_COMP_PARAM_1_RX_BUFFER_DEPTH(x) \
           ((((x) >> 8) & 0xFF) + 1)
#define  DW_IC_COMP_PARAM_1_TX_BUFFER_DEPTH(x) \
           ((((x) >> 16) & 0xFF) + 1)
#define DW_IC_COMP_TYPE          0xfc
#define SB_DW_IC_CON             0xa8
#define SB_DW_IC_SCL_TMO_CNT     0xac
#define SB_DW_IC_RX_PEC          0xb0
#define SB_DW_IC_ACK             0xb4
#define SB_DW_IC_FLG             0xb8
#define SB_DW_IC_FLG_CLR         0xbc
#define SB_DW_IC_INTR_STAT       0xc0
#define SB_DW_IC_INTR_STAT_MASK  0xc4
#define SB_DW_IC_DEBUG_SEL       0xec
#define SB_DW_IC_ACK_DEBUG       0xf0
#define DW_IC_FS_SPKLEN          0xa0
#define DW_IC_HS_SPKLEN          0xa4

//
// Timeout interval
//
// The interval is equal to the 10 times the signaling period
// for the highest I2C transfer speed used in the system.
//
#define DW_POLL_INTERVAL_US(x)  (10 * (1000000 / (x)))

//
// Maximum timeout count
//
#define DW_MAX_TRANSFER_POLL_COUNT  100000// Maximum timeout: 10s
#define DW_MAX_STATUS_POLL_COUNT    100

#define DW_POLL_MST_ACTIVITY_INTERVAL_US  1000// 1ms
#define DW_MAX_MST_ACTIVITY_POLL_COUNT    20

/**
 Initialize I2C Bus
 **/
VOID
I2cHWInit (
  UINT32  Bus
  )
{
  UINT32  Param;

  mI2cBusList[Bus].Base = mI2cBaseArray[Bus];

  Param = MmioRead32 (mI2cBusList[Bus].Base + DW_IC_COMP_PARAM_1);

  mI2cBusList[Bus].PollingTime = DW_POLL_INTERVAL_US (mI2cBusList[Bus].BusSpeed);
  mI2cBusList[Bus].RxFifo      = DW_IC_COMP_PARAM_1_RX_BUFFER_DEPTH (Param);
  mI2cBusList[Bus].TxFifo      = DW_IC_COMP_PARAM_1_TX_BUFFER_DEPTH (Param);
  mI2cBusList[Bus].Enabled     = 0;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Bus %d, Rx_Buffer %d, Tx_Buffer %d\n",
    __func__,
    Bus,
    mI2cBusList[Bus].RxFifo,
    mI2cBusList[Bus].TxFifo
    ));
}

/**
 Enable or disable I2C Bus
 */
VOID
I2cEnable (
  UINT32  Bus,
  UINT32  Enable
  )
{
  UINT32  I2cStatusCnt;
  UINTN   Base;

  Base                     = mI2cBusList[Bus].Base;
  I2cStatusCnt             = DW_MAX_STATUS_POLL_COUNT;
  mI2cBusList[Bus].Enabled = Enable;

  MmioWrite32 (Base + DW_IC_ENABLE, Enable);

  do {
    if ((MmioRead32 (Base + DW_IC_ENABLE_STATUS) & 0x01) == Enable) {
      break;
    }

    MicroSecondDelay (mI2cBusList[Bus].PollingTime);
  } while (I2cStatusCnt-- != 0);

  if (I2cStatusCnt == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Enable/disable timeout\n", __func__));
  }

  if ((Enable == 0) || (I2cStatusCnt == 0)) {
    /* Unset the target adddress */
    MmioWrite32 (Base + DW_IC_TAR, 0);
    mI2cBusList[Bus].Enabled = 0;
  }
}

/**
 Setup Slave address
 **/
VOID
I2cSetSlaveAddr (
  UINT32  Bus,
  UINT32  SlaveAddr
  )
{
  UINTN   Base;
  UINT32  OldEnableStatus;

  Base            = mI2cBusList[Bus].Base;
  OldEnableStatus = mI2cBusList[Bus].Enabled;

  I2cEnable (Bus, 0);
  MmioWrite32 (Base + DW_IC_TAR, SlaveAddr);
  if (OldEnableStatus != 0) {
    I2cEnable (Bus, 1);
  }
}

/**
 Check for errors on I2C Bus
 **/
UINT32
I2cCheckErrors (
  UINT32  Bus
  )
{
  UINTN   Base;
  UINT32  ErrorStatus;

  Base = mI2cBusList[Bus].Base;

  ErrorStatus = MmioRead32 (Base + DW_IC_RAW_INTR_STAT) & DW_IC_ERR_CONDITION;

  if ((ErrorStatus & DW_IC_INTR_RX_UNDER) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: RX_UNDER error on i2c bus %d error status %08x\n",
      __func__,
      Bus,
      ErrorStatus
      ));
    MmioRead32 (Base + DW_IC_CLR_RX_UNDER);
  }

  if ((ErrorStatus & DW_IC_INTR_RX_OVER) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: RX_OVER error on i2c bus %d error status %08x\n",
      __func__,
      Bus,
      ErrorStatus
      ));
    MmioRead32 (Base + DW_IC_CLR_RX_OVER);
  }

  if ((ErrorStatus & DW_IC_INTR_TX_ABRT) != 0) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: TX_ABORT at source %08x\n",
      __func__,
      MmioRead32 (Base + DW_IC_TX_ABRT_SOURCE)
      ));
    MmioRead32 (Base + DW_IC_CLR_TX_ABRT);
  }

  return ErrorStatus;
}

/**
 Waiting for bus to not be busy
 **/
BOOLEAN
I2cWaitBusNotBusy (
  UINT32  Bus
  )
{
  UINTN  Base;
  UINTN  PollCount;

  Base      = mI2cBusList[Bus].Base;
  PollCount = DW_MAX_MST_ACTIVITY_POLL_COUNT;

  while ((MmioRead32 (Base + DW_IC_STATUS) & DW_IC_STATUS_MST_ACTIVITY) != 0) {
    if (PollCount == 0) {
      DEBUG ((DEBUG_VERBOSE, "%a: Timeout while waiting for bus ready\n", __func__));
      return FALSE;
    }

    PollCount--;

    /*
     * A delay isn't absolutely necessary.
     * But to ensure that we don't hammer the bus constantly,
     * delay for DW_POLL_MST_ACTIVITY_INTERVAL_US as with other implementation.
     */
    MicroSecondDelay (DW_POLL_MST_ACTIVITY_INTERVAL_US);
  }

  return TRUE;
}

/**
 Waiting for TX FIFO buffer available
 **/
EFI_STATUS
I2cWaitTxData (
  UINT32  Bus
  )
{
  UINTN  Base;
  UINTN  PollCount;

  Base      = mI2cBusList[Bus].Base;
  PollCount = 0;

  while (MmioRead32 (Base + DW_IC_TXFLR) == mI2cBusList[Bus].TxFifo) {
    if (PollCount++ >= DW_MAX_TRANSFER_POLL_COUNT) {
      DEBUG ((DEBUG_ERROR, "%a: Timeout waiting for TX buffer available\n", __func__));
      return EFI_TIMEOUT;
    }

    if ((I2cCheckErrors (Bus) & DW_IC_INTR_TX_ABRT) != 0) {
      return EFI_ABORTED;
    }

    MicroSecondDelay (mI2cBusList[Bus].PollingTime);
  }

  return EFI_SUCCESS;
}

/**
 Waiting for RX FIFO buffer available
 **/
EFI_STATUS
I2cWaitRxData (
  UINT32  Bus
  )
{
  UINTN  Base;
  UINTN  PollCount;

  Base      = mI2cBusList[Bus].Base;
  PollCount = 0;

  while ((MmioRead32 (Base + DW_IC_STATUS) & DW_IC_STATUS_RFNE) == 0) {
    if (PollCount++ >= DW_MAX_TRANSFER_POLL_COUNT) {
      DEBUG ((DEBUG_ERROR, "%a: Timeout waiting for RX buffer available\n", __func__));
      return EFI_TIMEOUT;
    }

    if ((I2cCheckErrors (Bus) & DW_IC_INTR_TX_ABRT) != 0) {
      return EFI_ABORTED;
    }

    MicroSecondDelay (mI2cBusList[Bus].PollingTime);
  }

  return EFI_SUCCESS;
}

/**
 Initialize the Designware I2C SCL Counts

 This functions configures SCL clock Count for Standard Speed (SS) and Fast Speed (FS) mode.
 **/
VOID
I2cSclInit (
  UINT32  Bus,
  UINT32  I2cClkFreq,
  UINT32  I2cSpeed
  )
{
  UINT16  IcCon;
  UINTN   Base;
  UINT32  I2cSpeedKhz;

  Base        = mI2cBusList[Bus].Base;
  I2cSpeedKhz = I2cSpeed / 1000;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Bus %d I2cClkFreq %d I2cSpeed %d\n",
    __func__,
    Bus,
    I2cClkFreq,
    I2cSpeed
    ));

  IcCon = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE | DW_IC_CON_RESTART_EN;

  if (I2cSpeedKhz <= 100) {
    IcCon |= DW_IC_CON_SPEED_STD;
    // Standard speed mode
    MmioWrite32 (Base + DW_IC_FS_SPKLEN, I2cSclParam[I2cSpeedModeStandard][I2cSclSpkLen]);
    MmioWrite32 (Base + DW_IC_SS_SCL_HCNT, I2cSclParam[I2cSpeedModeStandard][I2cSclHcnt]);
    MmioWrite32 (Base + DW_IC_SS_SCL_LCNT, I2cSclParam[I2cSpeedModeStandard][I2cSclLcnt]);
  } else if ((I2cSpeedKhz > 100) && (I2cSpeedKhz <= 400)) {
    IcCon |= DW_IC_CON_SPEED_FAST;
    // Fast speed mode
    MmioWrite32 (Base + DW_IC_FS_SPKLEN, I2cSclParam[I2cSpeedModeFast][I2cSclSpkLen]);
    MmioWrite32 (Base + DW_IC_FS_SCL_HCNT, I2cSclParam[I2cSpeedModeFast][I2cSclHcnt]);
    MmioWrite32 (Base + DW_IC_FS_SCL_LCNT, I2cSclParam[I2cSpeedModeFast][I2cSclLcnt]);
  }

  MmioWrite32 (Base + DW_IC_CON, IcCon);
}

/**
 Initialize the designware i2c master hardware
 **/
EFI_STATUS
I2cInit (
  UINT32  Bus,
  UINTN   BusSpeed
  )
{
  UINTN  Base;

  ASSERT (mI2cClock != 0);

  mI2cBusList[Bus].BusSpeed = BusSpeed;
  I2cHWInit (Bus);

  Base = mI2cBusList[Bus].Base;

  /* Disable the adapter and interrupt */
  I2cEnable (Bus, 0);
  MmioWrite32 (Base + DW_IC_INTR_MASK, 0);

  /* Set standard and fast speed divider for high/low periods */
  I2cSclInit (Bus, mI2cClock, BusSpeed);
  MmioWrite32 (Base + DW_IC_SDA_HOLD, 0x4b);

  return EFI_SUCCESS;
}

/**
 Wait the transaction finished
 **/
EFI_STATUS
I2cFinish (
  UINT32  Bus
  )
{
  UINTN  Base;
  UINTN  PollCount;

  Base      = mI2cBusList[Bus].Base;
  PollCount = 0;

  /* Wait for TX FIFO empty */
  do {
    if ((MmioRead32 (Base + DW_IC_STATUS) & DW_IC_STATUS_TFE) != 0) {
      break;
    }

    MicroSecondDelay (mI2cBusList[Bus].PollingTime);
  } while (PollCount++ < DW_MAX_TRANSFER_POLL_COUNT);

  if (PollCount >= DW_MAX_TRANSFER_POLL_COUNT) {
    DEBUG ((DEBUG_ERROR, "%a: Timeout waiting for TX FIFO empty\n", __func__));
    return EFI_TIMEOUT;
  }

  /* Wait for STOP signal detected on the bus */
  PollCount = 0;
  do {
    if ((MmioRead32 (Base + DW_IC_RAW_INTR_STAT) & DW_IC_INTR_STOP_DET) != 0) {
      MmioRead32 (Base + DW_IC_CLR_STOP_DET);
      return EFI_SUCCESS;
    }

    MicroSecondDelay (mI2cBusList[Bus].PollingTime);
  } while (PollCount++ < DW_MAX_TRANSFER_POLL_COUNT);

  DEBUG ((DEBUG_ERROR, "%a: Timeout waiting for transaction finished\n", __func__));
  return EFI_TIMEOUT;
}

EFI_STATUS
InternalI2cWrite (
  UINT32  Bus,
  UINT8   *Buf,
  UINT32  *Length
  )
{
  EFI_STATUS  Status;
  UINTN       WriteCount;
  UINTN       Base;

  Status = EFI_SUCCESS;
  Base   = mI2cBusList[Bus].Base;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Write Bus %d Buf %p Length %d\n",
    __func__,
    Bus,
    Buf,
    *Length
    ));
  I2cEnable (Bus, 1);

  WriteCount = 0;
  while ((*Length - WriteCount) != 0) {
    Status = I2cWaitTxData (Bus);
    if (EFI_ERROR (Status)) {
      MmioWrite32 (Base + DW_IC_DATA_CMD, DW_IC_DATA_CMD_STOP);
      I2cSync ();
      goto Exit;
    }

    if (WriteCount == *Length - 1) {
      MmioWrite32 (
        Base + DW_IC_DATA_CMD,
        (Buf[WriteCount] & DW_IC_DATA_CMD_DAT_MASK) | DW_IC_DATA_CMD_STOP
        );
    } else {
      MmioWrite32 (
        Base + DW_IC_DATA_CMD,
        Buf[WriteCount] & DW_IC_DATA_CMD_DAT_MASK
        );
    }

    I2cSync ();
    WriteCount++;
  }

Exit:
  *Length = WriteCount;
  I2cFinish (Bus);
  I2cWaitBusNotBusy (Bus);
  I2cEnable (Bus, 0);

  return Status;
}

/**
  This extracts the data length from the initial byte of the SMBUS transaction. This allows
  the driver to accurately read the SMBUS response with the exact length, rather than
  consistently reading 32-byte block of data.

  @param[in]  Bus      I2C bus Id.
  @param[out] BusSpeed Pointer to the buffer to store the read length.

  @retval EFI_SUCCESS  The operation is successful.
  @retval Others       An error occurred.

**/
EFI_STATUS
InternalSmbusReadDataLength (
  UINT32  Bus,
  UINT32  *Length
  )
{
  EFI_STATUS  Status;
  UINTN       Base;
  UINT32      CmdSend;

  Base = mI2cBusList[Bus].Base;

  CmdSend = DW_IC_DATA_CMD_CMD;
  MmioWrite32 (Base + DW_IC_DATA_CMD, CmdSend);
  I2cSync ();

  if (I2cCheckErrors (Bus) != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Sending reading command error\n", __func__));
    return EFI_CRC_ERROR;
  }

  Status = I2cWaitRxData (Bus);
  if (EFI_ERROR (Status)) {
    //
    // If the SMBUS target is not ready to handle the request
    // or is busy with preparing the response data, it will response
    // NACK, and the error status TX_ABRT is triggered to indicate that
    // the RX FIFO is not ready for reading. Thus, the following message
    // serves more as verbose alert rather than an error.
    //
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Reading Smbus data length failed to wait data\n",
      __func__
      ));

    if (Status != EFI_ABORTED) {
      MmioWrite32 (Base + DW_IC_DATA_CMD, DW_IC_DATA_CMD_STOP);
      I2cSync ();
    }

    return Status;
  }

  *Length = MmioRead32 (Base + DW_IC_DATA_CMD) & DW_IC_DATA_CMD_DAT_MASK;
  I2cSync ();

  if (I2cCheckErrors (Bus) != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Sending reading command error\n", __func__));
    return EFI_CRC_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalI2cRead (
  UINT32  Bus,
  UINT8   *BufCmd,
  UINT32  CmdLength,
  UINT8   *Buf,
  UINT32  *Length
  )
{
  EFI_STATUS  Status;
  UINTN       Base;
  UINT32      CmdSend;
  UINT32      TxLimit, RxLimit;
  UINTN       Idx;
  UINTN       Count;
  UINTN       ReadCount;
  UINTN       WriteCount;
  UINT32      ResponseLen;

  Status    = EFI_SUCCESS;
  Base      = mI2cBusList[Bus].Base;
  Count     = 0;
  ReadCount = 0;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Read Bus %d Buf %p Length:%d\n",
    __func__,
    Bus,
    Buf,
    *Length
    ));

  I2cEnable (Bus, 1);

  /* Write command data */
  WriteCount = 0;
  while (CmdLength != 0) {
    TxLimit = mI2cBusList[Bus].TxFifo - MmioRead32 (Base + DW_IC_TXFLR);
    Count   = CmdLength > TxLimit ? TxLimit : CmdLength;

    for (Idx = 0; Idx < Count; Idx++ ) {
      CmdSend = BufCmd[WriteCount++] & DW_IC_DATA_CMD_DAT_MASK;
      MmioWrite32 (Base + DW_IC_DATA_CMD, CmdSend);
      I2cSync ();

      if (I2cCheckErrors (Bus) != 0) {
        Status = EFI_CRC_ERROR;
        goto Exit;
      }

      CmdLength--;
    }

    Status = I2cWaitTxData (Bus);
    if (EFI_ERROR (Status)) {
      MmioWrite32 (Base + DW_IC_DATA_CMD, DW_IC_DATA_CMD_STOP);
      I2cSync ();
      goto Exit;
    }
  }

  WriteCount = 0;
  if (mI2cBusList[Bus].IsSmbus) {
    //
    // Read Smbus Data Length, first byte of the Smbus response data.
    //
    Status = InternalSmbusReadDataLength (Bus, &ResponseLen);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    WriteCount++;
    Buf[ReadCount++] = ResponseLen;

    //
    // Abort the transaction when the requested length is shorter than the actual response data
    // or if there is no response data when PEC disabled.
    //
    if ((*Length < (ResponseLen + 2)) || (!mI2cBusList[Bus].PecCheck && (ResponseLen == 0))) {
      MmioWrite32 (Base + DW_IC_DATA_CMD, DW_IC_DATA_CMD_CMD | DW_IC_DATA_CMD_STOP);
      I2cSync ();
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    *Length = ResponseLen + 1; // Response Data Length + 8-bit Byte Count field
    if (mI2cBusList[Bus].PecCheck) {
      *Length += 1; // ++ 8-bit PEC field
    }
  }

  while ((*Length - ReadCount) != 0) {
    TxLimit = mI2cBusList[Bus].TxFifo - MmioRead32 (Base + DW_IC_TXFLR);
    RxLimit = mI2cBusList[Bus].RxFifo - MmioRead32 (Base + DW_IC_RXFLR);
    Count   = *Length - ReadCount;
    Count   = Count > RxLimit ? RxLimit : Count;
    Count   = Count > TxLimit ? TxLimit : Count;

    for (Idx = 0; Idx < Count; Idx++ ) {
      CmdSend = DW_IC_DATA_CMD_CMD;
      if (WriteCount == *Length - 1) {
        CmdSend |= DW_IC_DATA_CMD_STOP;
      }

      MmioWrite32 (Base + DW_IC_DATA_CMD, CmdSend);
      I2cSync ();
      WriteCount++;

      if (I2cCheckErrors (Bus) != 0) {
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: Sending reading command remaining length %d CRC error\n",
          __func__,
          *Length
          ));
        Status = EFI_CRC_ERROR;
        goto Exit;
      }
    }

    for (Idx = 0; Idx < Count; Idx++ ) {
      Status = I2cWaitRxData (Bus);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: Reading remaining length %d failed to wait data\n",
          __func__,
          *Length
          ));

        if (Status != EFI_ABORTED) {
          MmioWrite32 (Base + DW_IC_DATA_CMD, DW_IC_DATA_CMD_STOP);
          I2cSync ();
        }

        goto Exit;
      }

      Buf[ReadCount++] = MmioRead32 (Base + DW_IC_DATA_CMD) & DW_IC_DATA_CMD_DAT_MASK;
      I2cSync ();

      if (I2cCheckErrors (Bus) != 0) {
        DEBUG ((
          DEBUG_VERBOSE,
          "%a: Reading remaining length %d CRC error\n",
          __func__,
          *Length
          ));
        Status = EFI_CRC_ERROR;
        goto Exit;
      }
    }
  }

Exit:
  *Length = ReadCount;
  I2cFinish (Bus);
  I2cWaitBusNotBusy (Bus);
  I2cEnable (Bus, 0);

  return Status;
}

/**
  Write to I2C bus.

  @param[in]     Bus          I2C bus Id.
  @param[in]     SlaveAddr    The address of slave device on the bus.
  @param[in,out] Buf          Buffer that holds data to write.
  @param[in,out] WriteLength  Pointer to length of buffer.

  @return EFI_SUCCESS            Write successfully.
  @return EFI_INVALID_PARAMETER  A parameter is invalid.
  @return EFI_UNSUPPORTED        The bus is not supported.
  @return EFI_NOT_READY          The device/bus is not ready.
  @return EFI_TIMEOUT            Timeout why transferring data.

**/
EFI_STATUS
EFIAPI
I2cWrite (
  IN     UINT32  Bus,
  IN     UINT32  SlaveAddr,
  IN OUT UINT8   *Buf,
  IN OUT UINT32  *WriteLength
  )
{
  if (  (Bus >= AC01_I2C_MAX_BUS_NUM)
     || (Buf == NULL)
     || (WriteLength == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  I2cSetSlaveAddr (Bus, SlaveAddr);

  return InternalI2cWrite (Bus, Buf, WriteLength);
}

/**
  Read data from I2C bus.

  @param[in]     Bus          I2C bus Id.
  @param[in]     SlaveAddr    The address of slave device on the bus.
  @param[in]     BufCmd       Buffer where to send the command.
  @param[in]     CmdLength    Pointer to length of BufCmd.
  @param[in,out] Buf          Buffer where to put the read data to.
  @param[in,out] ReadLength   Pointer to length of buffer.

  @return EFI_SUCCESS            Read successfully.
  @return EFI_INVALID_PARAMETER  A parameter is invalid.
  @return EFI_UNSUPPORTED        The bus is not supported.
  @return EFI_NOT_READY          The device/bus is not ready.
  @return EFI_TIMEOUT            Timeout why transferring data.
  @return EFI_CRC_ERROR          There are errors on receiving data.

**/
EFI_STATUS
EFIAPI
I2cRead (
  IN     UINT32  Bus,
  IN     UINT32  SlaveAddr,
  IN     UINT8   *BufCmd,
  IN     UINT32  CmdLength,
  IN OUT UINT8   *Buf,
  IN OUT UINT32  *ReadLength
  )
{
  if (  (Bus >= AC01_I2C_MAX_BUS_NUM)
     || (Buf == NULL)
     || (ReadLength == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  I2cSetSlaveAddr (Bus, SlaveAddr);

  return InternalI2cRead (Bus, BufCmd, CmdLength, Buf, ReadLength);
}

/**
 Setup new transaction with I2C slave device.

  @param[in] Bus      I2C bus Id.
  @param[in] BusSpeed I2C bus speed in Hz.
  @param[in] IsSmbus  Flag to indicate if the bus is used to execute an SMBus operation.
  @param[in] PecCheck If Packet Error Code (PEC) checking is required for the SMBUS operation
                      and is ignored when present in other operations.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.

**/
EFI_STATUS
EFIAPI
I2cProbe (
  IN UINT32   Bus,
  IN UINTN    BusSpeed,
  IN BOOLEAN  IsSmbus,
  IN BOOLEAN  PecCheck
  )
{
  if (  (Bus >= AC01_I2C_MAX_BUS_NUM)
     || (BusSpeed > DW_I2C_MAXIMUM_SPEED_HZ))
  {
    return EFI_INVALID_PARAMETER;
  }

  mI2cBusList[Bus].IsSmbus  = IsSmbus;
  mI2cBusList[Bus].PecCheck = PecCheck;

  return I2cInit (Bus, BusSpeed);
}

/**
 * Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.
 *
 * This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
 * It convers pointer to new virtual address.
 *
 * @param  Event        Event whose notification function is being invoked.
 * @param  Context      Pointer to the notification function's context.
 */
VOID
EFIAPI
I2cVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Count;

  EfiConvertPointer (0x0, (VOID **)&mI2cBusList);
  EfiConvertPointer (0x0, (VOID **)&mI2cBaseArray);
  EfiConvertPointer (0x0, (VOID **)&mI2cClock);
  for (Count = 0; Count < AC01_I2C_MAX_BUS_NUM; Count++) {
    if (!mI2cRuntimeEnableArray[Count]) {
      continue;
    }

    EfiConvertPointer (0x0, (VOID **)&mI2cBaseArray[Count]);
    EfiConvertPointer (0x0, (VOID **)&mI2cBusList[Count].Base);
  }
}

/**
 Setup a bus that to be used in runtime service.

  @param[in] Bus I2C bus Id.

  @retval EFI_SUCCESS  Success.
  @retval Otherwise    Error code.

**/
EFI_STATUS
EFIAPI
I2cSetupRuntime (
  IN UINT32  Bus
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  if (Bus >= AC01_I2C_MAX_BUS_NUM) {
    return EFI_INVALID_PARAMETER;
  }

  if (mVirtualAddressChangeEvent == NULL) {
    /*
     * Register for the virtual address change event
     */
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    I2cVirtualAddressChangeEvent,
                    NULL,
                    &gEfiEventVirtualAddressChangeGuid,
                    &mVirtualAddressChangeEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  Status = gDS->GetMemorySpaceDescriptor (
                  mI2cBaseArray[Bus] & RUNTIME_ADDRESS_MASK,
                  &Descriptor
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  mI2cBaseArray[Bus] & RUNTIME_ADDRESS_MASK,
                  RUNTIME_ADDRESS_LENGTH,
                  Descriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mI2cRuntimeEnableArray[Bus] = TRUE;

  return Status;
}

EFI_STATUS
EFIAPI
I2cLibConstructor (
  VOID
  )
{
  VOID               *Hob;
  PLATFORM_INFO_HOB  *PlatformHob;

  /* Get I2C Clock from the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);
  mI2cClock   = PlatformHob->AhbClk;
  ASSERT (mI2cClock != 0);

  return EFI_SUCCESS;
}
