/** @file
  Define a simple and generic interface to access SD-card devices.

  Copyright (c) 2018-2021, ARM Limited and Contributors. All rights reserved.
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Include/MmcHost.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>

#include "Mmc.h"

#define MMC_DEFAULT_MAX_RETRIES   5
#define SEND_OP_COND_MAX_RETRIES  100

#define MULT_BY_512K_SHIFT        19

STATIC UINT32  MmcOCR;
STATIC CSD     MmcCsd;
STATIC UINT8   MmcExtCsd[512] __attribute__ ((aligned(16)));
STATIC UINT32  MmcRCA;
STATIC UINT32  MmcSCR[2] __attribute__ ((aligned(16))) = { 0 };

typedef enum _MMC_DEVICE_TYPE {
  MMC_IS_EMMC,
  MMC_IS_SD,
  MMC_IS_SD_HC,
} MMC_DEVICE_TYPE;

typedef struct {
  UINT64           DeviceSize;  /* Size of device in bytes */
  UINT32           BlockSize;   /* Block size in bytes */
  UINT32           MaxBusFreq;  /* Max bus freq in Hz */
  UINT32           OCRVoltage;  /* OCR voltage */
  MMC_DEVICE_TYPE  MmcDevType;  /* Type of MMC */
} MMC_DEVICE_INFO;

STATIC MMC_DEVICE_INFO MmcDevInfo = {
  .MmcDevType = MMC_IS_SD_HC,
  .OCRVoltage = 0x00300000, // OCR 3.2~3.3 3.3~3.4
};

STATIC CONST UINT8 TranSpeedBase[16] = {
  0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80
};

STATIC CONST UINT8 SdTranSpeedBase[16] = {
  0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};

/**
  Get the current state of the MMC device.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.
  @param[out]    State                 Pointer to the variable to store the device state.

  @retval EFI_SUCCESS                  The device state was retrieved successfully.
  @retval EFI_DEVICE_ERROR             Failed to retrieve the device state.

**/
STATIC
EFI_STATUS
MmcDeviceState (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  IN UINT32             *State
  )
{
  EFI_STATUS Status;
  INT32      RetryCount;
  UINT32     Response[4];

  RetryCount = MMC_DEFAULT_MAX_RETRIES;

  do {
    if (RetryCount == 0) {
      DEBUG ((DEBUG_ERROR, "%a: CMD13 failed after %d retries\n", __func__, MMC_DEFAULT_MAX_RETRIES));
      return EFI_DEVICE_ERROR;
    }

    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD13, MmcRCA << RCA_SHIFT_OFFSET,
               MMC_RESPONSE_R1, Response);
    if (EFI_ERROR (Status)) {
      RetryCount--;
      continue;
    }

    if ((Response[0] & MMC_R0_SWITCH_ERROR) != 0U) {
      return EFI_DEVICE_ERROR;
    }

    RetryCount--;
  } while ((Response[0] & MMC_R0_READY_FOR_DATA) == 0U);

  // DEBUG ((DEBUG_INFO, "%a: sd state %x\n", __func__, MMC_R0_CURRENTSTATE(Response)));
  *State = MMC_R0_CURRENTSTATE (Response);

  return EFI_SUCCESS;
}

/**
  Set the value of the specified MMC extended CSD register.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.
  @param[in]     ExtCmd                The extended CSD command.
  @param[in]     Value                 The value to set.

  @retval EFI_SUCCESS                  The value was successfully set.
  @retval Other                        An error occurred while setting the value.

**/
STATIC
EFI_STATUS
MmcSetExtCsd (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  IN UINT32             ExtCmd,
  IN UINT32             Value
  )
{
  EFI_STATUS Status;
  UINT32     State;

  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD6,
             EXTCSD_WRITE_BYTES | EXTCSD_CMD(ExtCmd) |
             EXTCSD_VALUE(Value) | EXTCSD_CMD_SET_NORMAL,
             MMC_RESPONSE_R1B, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  do {
    Status = MmcDeviceState (MmcHostInstance, &State);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } while (State == MMC_R0_STATE_PROG);

  return EFI_SUCCESS;
}

/**
  Perform an SD switch to set the bus width for the MMC/SD device.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.
  @param[in]     BusWidth              The desired bus width.

  @retval EFI_SUCCESS                   The bus width was successfully set.
  @retval Other                         An error occurred while setting the bus width.

**/
STATIC
EFI_STATUS
MmcSdSwitch (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  IN UINT32             BusWidth
  )
{
  EFI_STATUS  Status;
  UINT32      State;
  INT32       RetryCount;
  UINT32      BusWidthArg;

  RetryCount  = MMC_DEFAULT_MAX_RETRIES;
  BusWidthArg = 0;

  Status = MmcHostInstance->MmcHost->Prepare (MmcHostInstance->MmcHost, 0, sizeof(MmcSCR), (UINTN)&MmcSCR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // CMD55: Application Specific Command
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD55, MmcRCA << RCA_SHIFT_OFFSET,
             MMC_RESPONSE_R5, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // ACMD51: SEND_SCR
  do {
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_ACMD51, 0, MMC_RESPONSE_R1, NULL);
    if ((EFI_ERROR (Status)) && (RetryCount == 0)) {
      DEBUG ((DEBUG_ERROR, "%a: ACMD51 failed after %d retries (Status=%r)\n", __func__, MMC_DEFAULT_MAX_RETRIES, Status));
      return Status;
    }

    RetryCount--;
  } while (EFI_ERROR (Status));

  Status = MmcHostInstance->MmcHost->ReadBlockData (MmcHostInstance->MmcHost, 0, sizeof(MmcSCR), MmcSCR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (((MmcSCR[0] & SD_SCR_BUS_WIDTH_4) != 0U) && (BusWidth == MMC_BUS_WIDTH_4)) {
    BusWidthArg = 2;
  }

  // CMD55: Application Specific Command
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD55, MmcRCA << RCA_SHIFT_OFFSET,
             MMC_RESPONSE_R5, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // ACMD6: SET_BUS_WIDTH
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD6, BusWidthArg, MMC_RESPONSE_R1, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  do {
    Status = MmcDeviceState (MmcHostInstance, &State);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } while (State == MMC_R0_STATE_PROG);

  return EFI_SUCCESS;
}

/**
  Set the I/O settings for the MMC/SD device.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.
  @param[in]     Clk                   The desired clock frequency.
  @param[in]     BusWidth              The desired bus width.

  @retval EFI_SUCCESS                   The I/O settings were successfully set.
  @retval Other                         An error occurred while setting the I/O settings.

**/
STATIC
EFI_STATUS
MmcSetIos (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  IN UINT32             Clk,
  IN UINT32             BusWidth
  )
{
  EFI_STATUS  Status;
  UINT32      Width;

  Width = BusWidth;

  if (MmcDevInfo.MmcDevType != MMC_IS_EMMC) {
    if (Width == MMC_BUS_WIDTH_8) {
      DEBUG ((DEBUG_INFO, "%a: Wrong bus config for SD-card, force to 4\n", __func__));
      Width = MMC_BUS_WIDTH_4;
    }

    Status = MmcSdSwitch (MmcHostInstance, Width);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else if (MmcCsd.SPEC_VERS == 4U) {
    Status = MmcSetExtCsd (MmcHostInstance, CMD_EXTCSD_BUS_WIDTH, (UINT32)Width);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a: Wrong MMC type or spec version\n", __func__));
  }

  return MmcHostInstance->MmcHost->SetIos (MmcHostInstance->MmcHost, Clk, Width);
}

/**
  Fill the MMC device information.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.

  @retval EFI_SUCCESS                   The MMC device information was successfully filled.
  @retval EFI_DEVICE_ERROR              Failed to fill the MMC device information.
  @retval Other                         An error occurred while filling the MMC device information.

**/
STATIC
EFI_STATUS
MmcFillDeviceInfo (
  IN MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  EFI_STATUS  Status;
  UINTN       CardSize;
  UINT32      SpeedIdx;
  UINT32      NumBlocks;
  UINT32      FreqUnit;
  UINT32      State;
  ECSD        *CsdSdV2;

  Status = EFI_SUCCESS;

  switch (MmcDevInfo.MmcDevType) {
    case MMC_IS_EMMC:
      MmcDevInfo.BlockSize = MMC_BLOCK_SIZE;

      Status = MmcHostInstance->MmcHost->Prepare (MmcHostInstance->MmcHost, 0, sizeof(MmcExtCsd), (UINTN)&MmcExtCsd);

      if (EFI_ERROR (Status)) {
        return Status;
      }

      /* MMC CMD8: SEND_EXT_CSD */
      Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD8, 0, MMC_RESPONSE_R1, NULL);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = MmcHostInstance->MmcHost->ReadBlockData (MmcHostInstance->MmcHost, 0, sizeof(MmcExtCsd), (UINT32*)MmcExtCsd);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      do {
        Status = MmcDeviceState (MmcHostInstance, &State);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      } while (State != MMC_R0_STATE_TRAN);

      NumBlocks = (MmcExtCsd[CMD_EXTCSD_SEC_CNT] << 0) |
            (MmcExtCsd[CMD_EXTCSD_SEC_CNT + 1] << 8) |
            (MmcExtCsd[CMD_EXTCSD_SEC_CNT + 2] << 16) |
            (MmcExtCsd[CMD_EXTCSD_SEC_CNT + 3] << 24);

      MmcDevInfo.DeviceSize = (UINT64)NumBlocks * MmcDevInfo.BlockSize;

      break;

    case MMC_IS_SD:
      /*
      * Use the same MmcCsd struct, as required fields here
      * (READ_BL_LEN, C_SIZE, CSIZE_MULT) are common with eMMC.
      */
      MmcDevInfo.BlockSize = BIT_32(MmcCsd.READ_BL_LEN);

      CardSize = ((UINT64)MmcCsd.C_SIZEHigh10 << 2U) |
        (UINT64)MmcCsd.C_SIZELow2;
      ASSERT(CardSize != 0xFFFU);

      MmcDevInfo.DeviceSize = (CardSize + 1U) *
                BIT_64(MmcCsd.C_SIZE_MULT + 2U) *
                MmcDevInfo.BlockSize;

      break;

    case MMC_IS_SD_HC:
        MmcHostInstance->CardInfo.CardType = SD_CARD_2_HIGH;

      ASSERT (MmcCsd.CSD_STRUCTURE == 1U);

      MmcDevInfo.BlockSize = MMC_BLOCK_SIZE;

      /* Need to use ECSD struct */
      CsdSdV2 = (ECSD *)&MmcCsd;
      CardSize = ((UINT64)CsdSdV2->C_SIZEHigh6 << 16) |
        (UINT64)CsdSdV2->C_SIZELow16;

      MmcDevInfo.DeviceSize = (CardSize + 1U) << MULT_BY_512K_SHIFT;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  SpeedIdx = (MmcCsd.TRAN_SPEED & CSD_TRAN_SPEED_MULT_MASK) >>
        CSD_TRAN_SPEED_MULT_SHIFT;

  ASSERT (SpeedIdx > 0U);

  if (MmcDevInfo.MmcDevType == MMC_IS_EMMC) {
    MmcDevInfo.MaxBusFreq = TranSpeedBase[SpeedIdx];
  } else {
    MmcDevInfo.MaxBusFreq = SdTranSpeedBase[SpeedIdx];
  }

  FreqUnit = MmcCsd.TRAN_SPEED & CSD_TRAN_SPEED_UNIT_MASK;
  while (FreqUnit != 0U) {
    MmcDevInfo.MaxBusFreq *= 10U;
    --FreqUnit;
  }

  MmcDevInfo.MaxBusFreq *= 10000U;

  return EFI_SUCCESS;
}

/**
  Send the SD_SEND_OP_COND command to initialize the SD card.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.

  @retval EFI_SUCCESS                   The SD_SEND_OP_COND command was successfully sent.
  @retval EFI_DEVICE_ERROR              Failed to send the SD_SEND_OP_COND command.
  @retval Other                         An error occurred while sending the SD_SEND_OP_COND command.

**/
STATIC
EFI_STATUS
SdSendOpCond (
  IN MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  EFI_STATUS Status;
  INT32      I;
  UINT32     Response[4];

  for (I = 0; I < SEND_OP_COND_MAX_RETRIES; I++) {
    // CMD55: Application Specific Command
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD55, 0, MMC_RESPONSE_R1, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // ACMD41: SD_SEND_OP_COND
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_ACMD41, OCR_HCS |
      MmcDevInfo.OCRVoltage, MMC_RESPONSE_R3, Response);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Response[0] & MMC_OCR_POWERUP) != 0U) {
      MmcOCR = Response[0];

      if ((MmcOCR & OCR_HCS) != 0U) {
        MmcDevInfo.MmcDevType = MMC_IS_SD_HC;
        MmcHostInstance->CardInfo.OCRData.AccessMode = 0x2;
      } else {
        MmcDevInfo.MmcDevType = MMC_IS_SD;
        MmcHostInstance->CardInfo.OCRData.AccessMode = 0x0;
      }

      return EFI_SUCCESS;
    }

    gBS->Stall (10000);
  }

  DEBUG ((DEBUG_ERROR, "%a: ACMD41 failed after %d retries\n", __func__, SEND_OP_COND_MAX_RETRIES));

  return EFI_DEVICE_ERROR;
}

/**
  Reset the MMC/SD card to the idle state.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.

  @retval EFI_SUCCESS                   The MMC/SD card was successfully reset to the idle state.
  @retval Other                         An error occurred while resetting the MMC/SD card to the idle state.

**/
STATIC
EFI_STATUS
MmcResetToIdle(
  IN MMC_HOST_INSTANCE     *MmcHostInstance
  )
{
  EFI_STATUS Status;

  /* CMD0: reset to IDLE */
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD0, 0, 0, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->Stall (2000);

  return EFI_SUCCESS;
}

/**
  Send the Operation Condition (CMD1) to the MMC/SD card.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.

  @retval EFI_SUCCESS                   The Operation Condition was successfully sent to the MMC/SD card.
  @retval EFI_DEVICE_ERROR              Failed to send the Operation Condition to the MMC/SD card.
  @retval Other                         An error occurred while sending the Operation Condition to the MMC/SD card.

**/
STATIC
EFI_STATUS
MmcSendOpCond (
  IN MMC_HOST_INSTANCE     *MmcHostInstance
  )
{
  INT32       I;
  EFI_STATUS  Status;
  UINT32      Response[4];

  Status = MmcResetToIdle (MmcHostInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (I = 0; I < SEND_OP_COND_MAX_RETRIES; I++) {
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD1, OCR_SECTOR_MODE |
            OCR_VDD_MIN_2V7 | OCR_VDD_MIN_1V7,
            MMC_RESPONSE_R3, Response);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Response[0] & MMC_OCR_POWERUP) != 0U) {
      MmcOCR = Response[0];
      return EFI_SUCCESS;
    }

    gBS->Stall (10000);
  }

  DEBUG ((DEBUG_ERROR, "%a: CMD1 failed after %d retries\n", __func__, SEND_OP_COND_MAX_RETRIES));

  return EFI_DEVICE_ERROR;
}

/**
  Enumerate and initialize the MMC/SD card.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.
  @param[in]     Clk                   Clock frequency for the MMC/SD card.
  @param[in]     BusWidth              Bus width for the MMC/SD card.

  @retval EFI_SUCCESS                   The MMC/SD card was successfully enumerated and initialized.
  @retval Other                         An error occurred while enumerating and initializing the MMC/SD card.

**/
STATIC
EFI_STATUS
MmcEnumerte (
  IN MMC_HOST_INSTANCE     *MmcHostInstance,
  IN UINT32                Clk,
  IN UINT32                BusWidth
  )
{
  EFI_STATUS  Status;
  UINT32      State;
  UINT32      Response[4];

  Status = MmcResetToIdle (MmcHostInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (MmcDevInfo.MmcDevType == MMC_IS_EMMC) {
    Status = MmcSendOpCond (MmcHostInstance);
  } else {
    // CMD8: Send Interface Condition Command
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD8, VHS_2_7_3_6_V | CMD8_CHECK_PATTERN,
            MMC_RESPONSE_R5, Response);

    if ((Status == EFI_SUCCESS) && ((Response[0] & 0xffU) == CMD8_CHECK_PATTERN)) {
      Status = SdSendOpCond (MmcHostInstance);
    }
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // CMD2: Card Identification
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD2, 0, MMC_RESPONSE_R2, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // CMD3: Set Relative Address
  if (MmcDevInfo.MmcDevType == MMC_IS_EMMC) {
    MmcRCA = MMC_FIX_RCA;
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD3, MmcRCA << RCA_SHIFT_OFFSET,
            MMC_RESPONSE_R1, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD3, 0,
            MMC_RESPONSE_R6, Response);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    MmcRCA = (Response[0] & 0xFFFF0000U) >> 16;
  }

  // CMD9: CSD Register
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD9, MmcRCA << RCA_SHIFT_OFFSET,
          MMC_RESPONSE_R2, Response);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem(&MmcCsd, &Response, sizeof(Response));

  // CMD7: Select Card
  Status = MmcHostInstance->MmcHost->SendCommand (MmcHostInstance->MmcHost, MMC_CMD7, MmcRCA << RCA_SHIFT_OFFSET,
          MMC_RESPONSE_R1, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  do {
    Status = MmcDeviceState (MmcHostInstance, &State);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } while (State != MMC_R0_STATE_TRAN);

  Status = MmcSetIos (MmcHostInstance, Clk, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return MmcFillDeviceInfo (MmcHostInstance);
}

/**
  Perform the MMC Identification Mode.

  @param[in]     MmcHostInstance       Pointer to the MMC_HOST_INSTANCE structure.

  @retval EFI_SUCCESS                   The MMC Identification Mode was performed successfully.
  @retval EFI_INVALID_PARAMETER         MmcHost is NULL.
  @retval Other                         An error occurred while performing the MMC Identification Mode.

**/
STATIC
EFI_STATUS
EFIAPI
MmcIdentificationMode (
  IN MMC_HOST_INSTANCE     *MmcHostInstance
  )
{
  EFI_STATUS              Status;
  UINTN                   CmdArg;
  BOOLEAN                 IsHCS;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;

  MmcHost = MmcHostInstance->MmcHost;
  CmdArg  = 0;
  IsHCS   = FALSE;

  if (MmcHost == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We can get into this function if we restart the identification mode
  if (MmcHostInstance->State == MmcHwInitializationState) {
    // Initialize the MMC Host HW
    Status = MmcNotifyState (MmcHostInstance, MmcHwInitializationState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcHwInitializationState, Status=%r.\n", Status));
      return Status;
    }
  }

  Status = MmcEnumerte (MmcHostInstance, 50 * 1000 * 1000, MMC_BUS_WIDTH_4);

  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcEnumerte, Status=%r.\n", Status));
      return Status;
  }

  MmcHostInstance->CardInfo.RCA                = MmcRCA;
  MmcHostInstance->BlockIo.Media->LastBlock    = ((MmcDevInfo.DeviceSize >> 9) - 1);
  MmcHostInstance->BlockIo.Media->BlockSize    = MmcDevInfo.BlockSize;
  MmcHostInstance->BlockIo.Media->ReadOnly     = MmcHost->IsReadOnly (MmcHost);
  MmcHostInstance->BlockIo.Media->MediaPresent = TRUE;
  MmcHostInstance->BlockIo.Media->MediaId++;

  return EFI_SUCCESS;
}

/**
  Initialize the MMC device.

  @param[in] MmcHostInstance   MMC host instance

  @retval EFI_SUCCESS          MMC device initialized successfully
  @retval Other                MMC device initialization failed

**/
EFI_STATUS
InitializeMmcDevice (
  IN  MMC_HOST_INSTANCE   *MmcHostInstance
  )
{
  EFI_STATUS              Status;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;
  UINTN                   BlockCount;

  BlockCount = 1;
  MmcHost    = MmcHostInstance->MmcHost;

  Status = MmcIdentificationMode (MmcHostInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(): Error in Identification Mode, Status=%r\n", Status));
    return Status;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcTransferState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(): Error MmcTransferState, Status=%r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}
