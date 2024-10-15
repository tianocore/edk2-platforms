/** @file
 *
 * Mmc Driver for Arasan SD 3.0/SDIO 3.0/eMMC 4.51 Host Controller
 * This doesn't support high speed mode yet, or eMMC.
 *
 * Copyright (c) 2025, Linaro Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Derived from Platform/RaspberryPi/Drivers/ArasanMmcHostDxe/ArasanMmcHostDxe.c
 *
 **/

#include "ArasanMmcHostDxe.h"

STATIC BOOLEAN            mCardIsPresent   = FALSE;
STATIC CARD_DETECT_STATE  mCardDetectState = CardDetectRequired;
STATIC EFI_GUID           mArasanDevicePathGuid = EFI_CALLER_ID_GUID;

EFI_MMC_HOST_PROTOCOL  gMMCHost =
{
  MMC_HOST_PROTOCOL_REVISION,
  MMCIsCardPresent,
  MMCIsReadOnly,
  MMCBuildDevicePath,
  MMCNotifyState,
  MMCSendCommand,
  MMCReceiveResponse,
  MMCReadBlockData,
  MMCWriteBlockData,
  NULL, /* SetIos is NULL as we don't support bus width change or high speed mode yet */
  MMCIsMultiBlock
};

/**
   Write to SDHCI registers with delay to avoid failures
 **/
STATIC
UINT32
EFIAPI
SdMmioWrite32 (
  IN      UINTN   Address,
  IN      UINT32  Value
  )
{
  UINT32  ReturnValue = 0;

  ReturnValue = MmioWrite32 (Address, Value);
  gBS->Stall (STALL_AFTER_REG_WRITE_US);
  return ReturnValue;
}

STATIC
UINT32
EFIAPI
SdMmioOr32 (
  IN      UINTN   Address,
  IN      UINT32  OrData
  )
{
  return SdMmioWrite32 (Address, MmioRead32 (Address) | OrData);
}

STATIC
UINT32
EFIAPI
SdMmioAnd32 (
  IN      UINTN   Address,
  IN      UINT32  AndData
  )
{
  return SdMmioWrite32 (Address, MmioRead32 (Address) & AndData);
}

STATIC
UINT32
EFIAPI
SdMmioAndThenOr32 (
  IN      UINTN   Address,
  IN      UINT32  AndData,
  IN      UINT32  OrData
  )
{
  return SdMmioWrite32 (Address, (MmioRead32 (Address) & AndData) | OrData);
}

/**
   Ignore commands that are not supported yet
 **/
STATIC
BOOLEAN
IgnoreCommand (
  UINT32  Command
  )
{
  BOOLEAN  Result = FALSE;

  switch (Command) {
    /*
     * We don't support high speed mode in this implementation yet
     */
    case MMC_CMD6:
      Result = TRUE;
      break;

    /*
     * No other commands should be ignored
     */
    default:
      Result = FALSE;
      break;
  }

  return Result;
}

/**
   Translates a generic SD command into the format used by the Arasan SD Host Controller
**/
STATIC
UINT32
TranslateCommand (
  UINT32  Command,
  UINT32  Argument
  )
{
  UINT32  Translation = INVALID_CMD;

  switch (Command) {
    case MMC_CMD0:
      Translation = CMD0;
      break;
    case MMC_CMD1:
      Translation = CMD1;
      break;
    case MMC_CMD2:
      Translation = CMD2;
      break;
    case MMC_CMD3:
      Translation = CMD3;
      break;
    case MMC_CMD5:
      Translation = CMD5;
      break;
    case MMC_CMD6:
      Translation = CMD6;
      break;
    case MMC_CMD7:
      Translation = CMD7;
      break;
    case MMC_CMD8:
    {
      if (Argument == CMD8_SD_ARG) {
        Translation = CMD8_SD;
      } else {
        ASSERT (Argument == CMD8_MMC_ARG);
        Translation = CMD8_MMC;
      }

      break;
    }
    case MMC_CMD9:
      Translation = CMD9;
      break;
    case MMC_CMD11:
      Translation = CMD11;
      break;
    case MMC_CMD12:
      Translation = CMD12;
      break;
    case MMC_CMD13:
      Translation = CMD13;
      break;
    case MMC_CMD16:
      Translation = CMD16;
      break;
    case MMC_CMD17:
      Translation = CMD17;
      break;
    case MMC_CMD18:
      Translation = CMD18;
      break;
    case MMC_CMD23:
      Translation = CMD23;
      break;
    case MMC_CMD24:
      Translation = CMD24;
      break;
    case MMC_CMD25:
      Translation = CMD25;
      break;
    case MMC_CMD55:
      Translation = CMD55;
      break;
    case MMC_ACMD41:
      Translation = ACMD41;
      break;
    case MMC_ACMD51:
      Translation = ACMD51;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "ArasanMMCHost: TranslateCommand(): Unrecognized Command: %d\n", Command));
      break;
  }

  return Translation;
}

/**
   Repeatedly polls a register until its value becomes correct, or until MAX_RETRY_COUNT polls is reached
**/
STATIC
EFI_STATUS
PollRegisterWithMask (
  IN UINTN  Register,
  IN UINTN  Mask,
  IN UINTN  ExpectedValue
  )
{
  UINT32  RetryCount = 0;

  while (RetryCount < MAX_RETRY_COUNT) {
    if ((MmioRead32 (Register) & Mask) != ExpectedValue) {
      RetryCount++;
      gBS->Stall (STALL_AFTER_RETRY_US);
    } else {
      break;
    }
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
   Perform soft reset
**/
STATIC
EFI_STATUS
SoftReset (
  IN UINT32  Mask
  )
{
  DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: SoftReset with mask 0x%x\n", Mask));

  SdMmioOr32 (MMCHS_SYSCTL, Mask);

  if (PollRegisterWithMask (MMCHS_SYSCTL, Mask, 0) == EFI_TIMEOUT) {
    DEBUG ((DEBUG_ERROR, "ArasanMMCHost: Failed to SoftReset with mask 0x%x\n", Mask));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
   Calculate the clock divisor
**/
STATIC
EFI_STATUS
CalculateClockFrequencyDivisor (
  IN  UINTN   TargetFrequency,
  OUT UINT32  *DivisorValue,
  OUT UINTN   *ActualFrequency
  )
{
  UINT32  Divisor       = 0;
  UINT32  BaseFrequency = MMCHS_BASE_FREQUENCY;

  ASSERT (BaseFrequency != 0);
  ASSERT (TargetFrequency != 0);
  Divisor = BaseFrequency / TargetFrequency;

  /*
   * Arasan controller is based on 3.0 spec so the div is multiple of 2
   * Actual Frequency = BaseFequency/(Div*2)
   */
  Divisor /= 2;

  if ((TargetFrequency < BaseFrequency) &&
      (TargetFrequency * 2 * Divisor != BaseFrequency))
  {
    Divisor += 1;
  }

  if (Divisor > MAX_DIVISOR_VALUE) {
    Divisor = MAX_DIVISOR_VALUE;
  }

  DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: BaseFrequency 0x%x Divisor 0x%x\n", BaseFrequency, Divisor));

  *DivisorValue  = (Divisor & 0xFF) << 8;
  Divisor      >>= 8;
  *DivisorValue |= (Divisor & 0x03) << 6;

  if (ActualFrequency) {
    if (Divisor == 0) {
      *ActualFrequency = BaseFrequency;
    } else {
      *ActualFrequency   = BaseFrequency / Divisor;
      *ActualFrequency >>= 1;
    }

    DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: *ActualFrequency 0x%x\n", *ActualFrequency));
  }

  DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: *DivisorValue 0x%x\n", *DivisorValue));

  return EFI_SUCCESS;
}

/******************** Protocol Member Functions ********************/

STATIC
BOOLEAN
EFIAPI
MMCIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL  *This
  )
{
  BOOLEAN  IsReadOnly = FALSE;

  if (PcdGetBool (PcdEnableMmcWPDetection)) {
    IsReadOnly = !((MmioRead32 (MMCHS_PRES_STATE) & WRITE_PROTECT_OFF) == WRITE_PROTECT_OFF);
    DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: MMCIsReadOnly(): %d\n", IsReadOnly));
  } else {
    IsReadOnly = FALSE;
  }

  return IsReadOnly;
}

STATIC
EFI_STATUS
EFIAPI
MMCBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL         *This,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  EFI_STATUS                Status             = EFI_SUCCESS;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePathNode = NULL;

  DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: MMCBuildDevicePath()\n"));

  NewDevicePathNode = CreateDeviceNode (HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof (VENDOR_DEVICE_PATH));

  if (NewDevicePathNode == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    CopyGuid (&((VENDOR_DEVICE_PATH *)NewDevicePathNode)->Guid, &mArasanDevicePathGuid);
    *DevicePath = NewDevicePathNode;
    Status      = EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
MMCSendCommand (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN MMC_CMD                MmcCmd,
  IN UINT32                 Argument
  )
{
  EFI_STATUS  Status        = EFI_SUCCESS;
  UINT32      MmcStatus     = 0;
  UINT32      CmdSendOKMask = 0;
  UINT32      RetryCount    = 0;
  BOOLEAN     IsDATCmd      = FALSE;

  DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: MMCSendCommand(MmcCmd: %08x, Argument: %08x)\n", MmcCmd, Argument));

  if (IgnoreCommand (MmcCmd)) {
    return EFI_SUCCESS;
  }

  MmcCmd = TranslateCommand (MmcCmd, Argument);

  if (MmcCmd == INVALID_CMD) {
    return EFI_UNSUPPORTED;
  }

  if (
      /* R1b commands use the busy signal on DAT[0] line, except for CMD12 */
      (((MmcCmd & CMD_R1B) == CMD_R1B) && (MmcCmd != CMD_STOP_TRANSMISSION)) ||
      /* ADTC commands are data transfer commands (using DAT line) */
      ((MmcCmd & CMD_R1_ADTC) == CMD_R1_ADTC)
     )
  {
    IsDATCmd = TRUE;
  }

  CmdSendOKMask = CMDI;

  if (IsDATCmd) {
    CmdSendOKMask |= DATI;
  }

  /* Poll until command line is available */
  if (PollRegisterWithMask (
        MMCHS_PRES_STATE,
        CmdSendOKMask,
        0
        ) == EFI_TIMEOUT)
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a(%u): Timeout reached for MMC_CMD%u PresState 0x%x MmcStatus 0x%x\n",
      __func__,
      __LINE__,
      CMD_TO_INDX (MmcCmd),
      MmioRead32 (MMCHS_PRES_STATE),
      MmioRead32 (MMCHS_INT_STAT)
      ));
    return EFI_TIMEOUT;
  }

  /* Provide the block size */
  if (MmcCmd == ACMD51) {
    SdMmioWrite32 (MMCHS_BLK, BLEN_8BYTES);
  } else {
    SdMmioWrite32 (MMCHS_BLK, BLEN_512BYTES);
  }

  /* Set Data timeout counter value to max value */
  SdMmioAndThenOr32 (MMCHS_SYSCTL, (UINT32) ~DTO_MASK, DTO_VAL);

  /*
   * Clear Interrupt Status Register, but not the Card Inserted bit
   * to avoid messing with card detection logic.
   */
  SdMmioWrite32 (MMCHS_INT_STAT, ALL_EN & ~(CARD_INS));

  /* Set command argument register */
  SdMmioWrite32 (MMCHS_ARG, Argument);

  /* Send the command */
  SdMmioWrite32 (MMCHS_CMD, MmcCmd);

  /* Check for the command status */
  while (RetryCount < MAX_RETRY_COUNT) {
    MmcStatus = MmioRead32 (MMCHS_INT_STAT);

    /* Check for errors */
    if ((MmcStatus & ERRI) != 0) {
      /*
       * CMD5 (CMD_IO_SEND_OP_COND) is only valid for SDIO
       * cards and thus expected to fail.
       */
      if (MmcCmd != CMD_IO_SEND_OP_COND) {
        DEBUG ((
          DEBUG_ERROR,
          "%a(%u): MMC_CMD%u ERRI MmcStatus 0x%x\n",
          __func__,
          __LINE__,
          CMD_TO_INDX (MmcCmd),
          MmcStatus
          ));
      }

      SoftReset (SRC);

      Status = EFI_DEVICE_ERROR;
      break;
    }
    /* Check if command is completed */
    else if ((MmcStatus & CC) == CC) {
      SdMmioWrite32 (MMCHS_INT_STAT, CC);
      break;
    }

    RetryCount++;
    gBS->Stall (STALL_AFTER_RETRY_US);
  }

  gBS->Stall (STALL_AFTER_SEND_CMD_US);

  if (RetryCount == MAX_RETRY_COUNT) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(%u): MMC_CMD%u completion TIMEOUT PresState 0x%x MmcStatus 0x%x\n",
      __func__,
      __LINE__,
      CMD_TO_INDX (MmcCmd),
      MmioRead32 (MMCHS_PRES_STATE),
      MmcStatus
      ));
    Status = EFI_TIMEOUT;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
MMCNotifyState (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN MMC_STATE              State
  )
{
  EFI_STATUS  Status         = EFI_SUCCESS;
  UINTN       ClockFrequency = 0;
  UINT32      Divisor        = 0;

  DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: MMCNotifyState(State: %d)\n", State));

  /* Stall all operations except init until card detection has occurred */
  if ((State != MmcHwInitializationState) &&
      (mCardDetectState != CardDetectCompleted))
  {
    return EFI_NOT_READY;
  }

  switch (State) {
    case MmcHwInitializationState:
    {
      DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: current divisor %x\n", MmioRead32 (MMCHS_SYSCTL)));

      Status = SoftReset (SRA);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      /* Switch to card detect test mode */
      SdMmioOr32 (MMCHS_HPCTL, CDSS|CDTL);

      /* Set card voltage */
      SdMmioAnd32 (MMCHS_HPCTL, ~SDBP);
      SdMmioAndThenOr32 (MMCHS_HPCTL, (UINT32) ~SDBP, SDVS_3_3_V);
      SdMmioOr32 (MMCHS_HPCTL, SDBP);

      /* First turn off the clock */
      SdMmioAnd32 (MMCHS_SYSCTL, ~CEN);

      /* Attempt to set the clock to 400Khz which is the expected initialization speed */
      Status = CalculateClockFrequencyDivisor (MMCHS_INIT_FREQUENCY, &Divisor, NULL);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "ArasanMMCHost: MMCNotifyState(): Fail to initialize SD clock\n"));
        return Status;
      }

      /* Set data timeout counter value + clock frequency, and enable internal clock */
      SdMmioOr32 (MMCHS_SYSCTL, DTO_VAL | Divisor | CEN | ICS | ICE);
      SdMmioOr32 (MMCHS_HPCTL, SDBP);

      /* Wait for ICS */
      while ((MmioRead32 (MMCHS_SYSCTL) & ICS) != ICS) {
      }

      /* Enable interrupts */
      SdMmioWrite32 (MMCHS_IE, ALL_EN);
      break;
    }
    case MmcIdleState:
      break;
    case MmcReadyState:
      break;
    case MmcIdentificationState:
      break;
    case MmcStandByState:
    {
      ClockFrequency = MMCHS_STANDBY_FREQUENCY;

      /* Turn off the SD clock */
      SdMmioAnd32 (MMCHS_SYSCTL, ~CEN);

      Status = CalculateClockFrequencyDivisor (ClockFrequency, &Divisor, NULL);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ArasanMMCHost: MmcStandByState(): Fail to initialize SD clock to %u Hz\n",
          ClockFrequency
          ));
        return Status;
      }

      /* Setup new divisor */
      SdMmioAndThenOr32 (MMCHS_SYSCTL, (UINT32) ~CLKD_MASK, Divisor);

      /* Wait for the clock to stabilise */
      while ((MmioRead32 (MMCHS_SYSCTL) & ICS) != ICS) {
      }

      /* Turn on the SD clock */
      SdMmioOr32 (MMCHS_SYSCTL, CEN);
      break;
    }
    case MmcTransferState:
      break;
    case MmcSendingDataState:
      break;
    case MmcReceiveDataState:
      break;
    case MmcProgrammingState:
      break;
    case MmcDisconnectState:
    case MmcInvalidState:
    default:
      DEBUG ((DEBUG_ERROR, "ArasanMMCHost: MMCNotifyState(): Invalid State: %d\n", State));
      ASSERT (0);
  }

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
EFIAPI
MMCIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL  *This
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  ASSERT (This != NULL);

  /*
   * If we are already in progress (we may get concurrent calls)
   * or completed the detection, just return the current value.
   */
  if (mCardDetectState != CardDetectRequired) {
    return mCardIsPresent;
  }

  mCardDetectState = CardDetectInProgress;
  mCardIsPresent   = FALSE;

  /*
   * The two following commands should succeed even if no card is present.
   */
  Status = MMCNotifyState (This, MmcHwInitializationState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MMCIsCardPresent: Error MmcHwInitializationState, Status=%r.\n", Status));
    /* If we failed init, go back to requiring card detection */
    mCardDetectState = CardDetectRequired;
    return mCardIsPresent;
  }

  /*
   * Reset card to idle state
   */
  if (EFI_ERROR (MMCSendCommand (This, MMC_CMD0, 0))) {
    DEBUG ((DEBUG_ERROR, "MMCIsCardPresent: Failed to reset card (CMD0 Error), Status=%r.\n", Status));
    mCardIsPresent = FALSE;
  }

  /*
   * Check SD Card
   */
  else if (!EFI_ERROR (MMCSendCommand (This, MMC_CMD8, CMD8_SD_ARG))) {
    DEBUG ((DEBUG_INFO, "MMCIsCardPresent: SD card detected.\n"));
    mCardIsPresent = TRUE;
  }

  /*
   * Check MMC/eMMC.
   */
  else if (!EFI_ERROR (MMCSendCommand (This, MMC_CMD1, EMMC_CMD1_CAPACITY_GREATER_THAN_2GB))) {
    DEBUG ((DEBUG_INFO, "MMCIsCardPresent: MMC card detected.\n"));
    mCardIsPresent = TRUE;
  }

  /*
   * Check SDIO.
   */
  else if (!EFI_ERROR (MMCSendCommand (This, MMC_CMD5, 0))) {
    DEBUG ((DEBUG_INFO, "MMCIsCardPresent: SDIO card detected.\n"));
    mCardIsPresent = TRUE;
  }

  /*
   * No card detected.
   */
  else {
    DEBUG ((DEBUG_INFO, "MMCIsCardPresent: No card detected, Status=%r.\n", Status));
  }

  mCardDetectState = CardDetectCompleted;
  return mCardIsPresent;
}

STATIC
EFI_STATUS
EFIAPI
MMCReceiveResponse (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN MMC_RESPONSE_TYPE      Type,
  OUT UINT32                *Buffer
  )
{
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(%u): NULL Buffer\n", __func__, __LINE__));
    return EFI_INVALID_PARAMETER;
  }

  if (Type == MMC_RESPONSE_TYPE_R2) {
    /* 16-byte response */
    Buffer[0] = MmioRead32 (MMCHS_RSP10);
    Buffer[1] = MmioRead32 (MMCHS_RSP32);
    Buffer[2] = MmioRead32 (MMCHS_RSP54);
    Buffer[3] = MmioRead32 (MMCHS_RSP76);

    DEBUG ((
      DEBUG_MMCHOST_SD,
      "ArasanMMCHost: MMCReceiveResponse(Type: %x), Buffer[0-3]: %08x, %08x, %08x, %08x\n",
      Type,
      Buffer[0],
      Buffer[1],
      Buffer[2],
      Buffer[3]
      ));
  } else {
    /* 4-byte response */
    Buffer[0] = MmioRead32 (MMCHS_RSP10);
    DEBUG ((DEBUG_MMCHOST_SD, "ArasanMMCHost: MMCReceiveResponse(Type: %08x), Buffer[0]: %08x\n", Type, Buffer[0]));
  }

  gBS->Stall (STALL_AFTER_REC_RESP_US);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MMCReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN EFI_LBA                Lba,
  IN UINTN                  Length,
  OUT UINT32                *Buffer
  )
{
  UINT32  MmcStatus  = 0;
  UINT32  RetryCount = 0;
  UINTN   RemLength  = 0;
  UINTN   Count      = 0;
  UINTN   BlockLen   = 0;

  DEBUG ((
    DEBUG_MMCHOST_SD,
    "%a(%u): LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",
    __func__,
    __LINE__,
    Lba,
    Length,
    Buffer
    ));

  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(%u): NULL Buffer\n", __func__, __LINE__));
    return EFI_INVALID_PARAMETER;
  }

  if (Length % sizeof (UINT32) != 0) {
    DEBUG ((DEBUG_ERROR, "%a(%u): bad Length %u\n", __func__, __LINE__, Length));
    return EFI_INVALID_PARAMETER;
  }

  RemLength = Length;
  while (RemLength != 0) {
    RetryCount = 0;
    BlockLen   = MIN (RemLength, BLEN_512BYTES);

    while (RetryCount < MAX_RETRY_COUNT) {
      MmcStatus = MmioRead32 (MMCHS_INT_STAT);
      if ((MmcStatus & BRR) != 0) {
        SdMmioWrite32 (MMCHS_INT_STAT, BRR);

        /*
         * Data is ready.
         */
        for (Count = 0; Count < BlockLen; Count += 4, Buffer++) {
          *Buffer = MmioRead32 (MMCHS_DATA);
        }

        break;
      }

      gBS->Stall (STALL_AFTER_RETRY_US);
      RetryCount++;
    }

    if (RetryCount == MAX_RETRY_COUNT) {
      DEBUG ((
        DEBUG_ERROR,
        "%a(%u): %lu/%lu MMCHS_INT_STAT: %08x\n",
        __func__,
        __LINE__,
        Length - RemLength,
        Length,
        MmcStatus
        ));
      return EFI_TIMEOUT;
    }

    RemLength -= BlockLen;
    gBS->Stall (STALL_AFTER_READ_US);
  }

  SdMmioWrite32 (MMCHS_INT_STAT, BRR);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MMCWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN EFI_LBA                Lba,
  IN UINTN                  Length,
  IN UINT32                 *Buffer
  )
{
  UINT32  MmcStatus  = 0;
  UINT32  RetryCount = 0;
  UINTN   RemLength  = 0;
  UINTN   Count      = 0;
  UINTN   BlockLen   = 0;

  DEBUG ((
    DEBUG_MMCHOST_SD,
    "%a(%u): LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",
    __func__,
    __LINE__,
    Lba,
    Length,
    Buffer
    ));

  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(%u): NULL Buffer\n", __func__, __LINE__));
    return EFI_INVALID_PARAMETER;
  }

  if (Length % sizeof (UINT32) != 0) {
    DEBUG ((DEBUG_ERROR, "%a(%u): bad Length %u\n", __func__, __LINE__, Length));
    return EFI_INVALID_PARAMETER;
  }

  RemLength = Length;
  while (RemLength != 0) {
    RetryCount = 0;
    BlockLen   = MIN (RemLength, BLEN_512BYTES);

    while (RetryCount < MAX_RETRY_COUNT) {
      MmcStatus = MmioRead32 (MMCHS_INT_STAT);
      if ((MmcStatus & BWR) != 0) {
        SdMmioWrite32 (MMCHS_INT_STAT, BWR);

        /*
         * Can write data.
         */
        for (Count = 0; Count < BlockLen; Count += 4, Buffer++) {
          SdMmioWrite32 (MMCHS_DATA, *Buffer);
        }

        break;
      }

      gBS->Stall (STALL_AFTER_RETRY_US);
      RetryCount++;
    }

    if (RetryCount == MAX_RETRY_COUNT) {
      DEBUG ((
        DEBUG_ERROR,
        "%a(%u): %lu/%lu MMCHS_INT_STAT: %08x\n",
        __func__,
        __LINE__,
        Length - RemLength,
        Length,
        MmcStatus
        ));
      return EFI_TIMEOUT;
    }

    RemLength -= BlockLen;
    gBS->Stall (STALL_AFTER_WRITE_US);
  }

  SdMmioWrite32 (MMCHS_INT_STAT, BWR);
  return EFI_SUCCESS;
}

STATIC
BOOLEAN
EFIAPI
MMCIsMultiBlock (
  IN EFI_MMC_HOST_PROTOCOL  *This
  )
{
  return TRUE;
}

/******************** Entry Point ********************/
EFI_STATUS
MMCInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  EFI_HANDLE  Handle = NULL;

  DEBUG ((DEBUG_MMCHOST_SD, "----- ArasanMMCHost: MMCInitialize() Start -----\n"));

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEmbeddedMmcHostProtocolGuid,
                  &gMMCHost,
                  NULL
                  );

  ASSERT_EFI_ERROR (Status);

  return Status;
}
