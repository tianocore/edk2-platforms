/** @file
  The header file that provides definitions and function declarations
  related to the SD Host Controller Interface (SDHCI) for SD card host controllers.

  Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SD_HCI_H_
#define _SD_HCI_H_

#define SDIO_BASE                       (FixedPcdGet64(PcdSG2042SDIOBase))
#define SDHCI_DMA_ADDRESS               0x00
#define SDHCI_BLOCK_SIZE                0x04
#define SDHCI_MAKE_BLKSZ(dma, blksz)    ((((dma) & 0x7) << 12) | ((blksz) & 0xFFF))
#define SDHCI_BLOCK_COUNT               0x06
#define SDHCI_ARGUMENT                  0x08
#define SDHCI_TRANSFER_MODE             0x0C
#define SDHCI_TRNS_DMA                  BIT0
#define SDHCI_TRNS_BLK_CNT_EN           BIT1
#define SDHCI_TRNS_ACMD12               BIT2
#define SDHCI_TRNS_READ                 BIT4
#define SDHCI_TRNS_MULTI                BIT5
#define SDHCI_TRNS_RESP_INT             BIT8
#define SDHCI_COMMAND                   0x0E
#define SDHCI_CMD_RESP_MASK             0x03
#define SDHCI_CMD_CRC                   0x08
#define SDHCI_CMD_INDEX                 0x10
#define SDHCI_CMD_DATA                  0x20
#define SDHCI_CMD_ABORTCMD              0xC0
#define SDHCI_CMD_RESP_NONE             0x00
#define SDHCI_CMD_RESP_LONG             0x01
#define SDHCI_CMD_RESP_SHORT            0x02
#define SDHCI_CMD_RESP_SHORT_BUSY       0x03
#define SDHCI_MAKE_CMD(c, f)            ((((c) & 0xff) << 8) | ((f) & 0xff))
#define SDHCI_RESPONSE_01               0x10
#define SDHCI_RESPONSE_23               0x14
#define SDHCI_RESPONSE_45               0x18
#define SDHCI_RESPONSE_67               0x1C
#define SDHCI_PSTATE                    0x24
#define SDHCI_CMD_INHIBIT               BIT0
#define SDHCI_CMD_INHIBIT_DAT           BIT1
#define SDHCI_BUF_WR_ENABLE             BIT10
#define SDHCI_BUF_RD_ENABLE             BIT11
#define SDHCI_CARD_INSERTED             BIT16
#define SDHCI_HOST_CONTROL              0x28
#define SDHCI_DAT_XFER_WIDTH            BIT1
#define SDHCI_EXT_DAT_XFER              BIT5
#define SDHCI_CTRL_DMA_MASK             0x18
#define SDHCI_CTRL_SDMA                 0x00
#define SDHCI_PWR_CONTROL               0x29
#define SDHCI_BUS_VOL_VDD1_1_8V         0xC
#define SDHCI_BUS_VOL_VDD1_3_0V         0xE
#define SDHCI_BUF_DATA_R                0x20
#define SDHCI_BLOCK_GAP_CONTROL         0x2A
#define SDHCI_CLK_CTRL                  0x2C
#define SDHCI_TOUT_CTRL                 0x2E
#define SDHCI_SOFTWARE_RESET            0x2F
#define SDHCI_RESET_CMD                 0x02
#define SDHCI_RESET_DATA                0x04
#define SDHCI_INT_STATUS                0x30
#define SDHCI_ERR_INT_STATUS            0x32
#define SDHCI_INT_CMD_COMPLETE          BIT0
#define SDHCI_INT_XFER_COMPLETE         BIT1
#define SDHCI_INT_DMA_END               BIT3
#define SDHCI_INT_BUF_WR_READY          BIT4
#define SDHCI_INT_BUF_RD_READY          BIT5
#define SDHCI_INT_ERROR                 BIT15
#define SDHCI_INT_STATUS_EN             0x34
#define SDHCI_ERR_INT_STATUS_EN         0x36
#define SDHCI_INT_CMD_COMPLETE_EN       BIT0
#define SDHCI_INT_XFER_COMPLETE_EN      BIT1
#define SDHCI_INT_DMA_END_EN            BIT3
#define SDHCI_INT_CARD_INSERTION_EN     BIT6
#define SDHCI_INT_ERROR_EN              BIT15
#define SDHCI_SIGNAL_ENABLE             0x38
#define SDHCI_HOST_CONTROL2             0x3E
#define SDHCI_HOST_VER4_ENABLE          BIT12
#define SDHCI_CAPABILITIES1             0x40
#define SDHCI_CAPABILITIES2             0x44
#define SDHCI_ADMA_SA_LOW               0x58
#define SDHCI_ADMA_SA_HIGH              0x5C
#define SDHCI_HOST_CNTRL_VERS           0xFE
#define SDHCI_UHS_2_TIMER_CNTRL         0xC2

#define P_VENDOR_SPECIFIC_AREA          0xE8
#define P_VENDOR2_SPECIFIC_AREA         0xEA
#define VENDOR_SD_CTRL                  0x2C

#define SDHCI_PHY_R_OFFSET              0x300

#define SDHCI_P_PHY_CNFG        (SDHCI_PHY_R_OFFSET + 0x00)
#define SDHCI_P_CMDPAD_CNFG     (SDHCI_PHY_R_OFFSET + 0x04)
#define SDHCI_P_DATPAD_CNFG     (SDHCI_PHY_R_OFFSET + 0x06)
#define SDHCI_P_CLKPAD_CNFG     (SDHCI_PHY_R_OFFSET + 0x08)
#define SDHCI_P_STBPAD_CNFG     (SDHCI_PHY_R_OFFSET + 0x0A)
#define SDHCI_P_RSTNPAD_CNFG    (SDHCI_PHY_R_OFFSET + 0x0C)
#define SDHCI_P_PADTEST_CNFG    (SDHCI_PHY_R_OFFSET + 0x0E)
#define SDHCI_P_PADTEST_OUT     (SDHCI_PHY_R_OFFSET + 0x10)
#define SDHCI_P_PADTEST_IN      (SDHCI_PHY_R_OFFSET + 0x12)
#define SDHCI_P_COMMDL_CNFG     (SDHCI_PHY_R_OFFSET + 0x1C)
#define SDHCI_P_SDCLKDL_CNFG    (SDHCI_PHY_R_OFFSET + 0x1D)
#define SDHCI_P_SDCLKDL_DC      (SDHCI_PHY_R_OFFSET + 0x1E)
#define SDHCI_P_SMPLDL_CNFG     (SDHCI_PHY_R_OFFSET + 0x20)
#define SDHCI_P_ATDL_CNFG       (SDHCI_PHY_R_OFFSET + 0x21)
#define SDHCI_P_DLL_CTRL        (SDHCI_PHY_R_OFFSET + 0x24)
#define SDHCI_P_DLL_CNFG1       (SDHCI_PHY_R_OFFSET + 0x25)
#define SDHCI_P_DLL_CNFG2       (SDHCI_PHY_R_OFFSET + 0x26)
#define SDHCI_P_DLLDL_CNFG      (SDHCI_PHY_R_OFFSET + 0x28)
#define SDHCI_P_DLL_OFFST       (SDHCI_PHY_R_OFFSET + 0x29)
#define SDHCI_P_DLLMST_TSTDC    (SDHCI_PHY_R_OFFSET + 0x2A)
#define SDHCI_P_DLLLBT_CNFG     (SDHCI_PHY_R_OFFSET + 0x2C)
#define SDHCI_P_DLL_STATUS      (SDHCI_PHY_R_OFFSET + 0x2E)
#define SDHCI_P_DLLDBG_MLKDC    (SDHCI_PHY_R_OFFSET + 0x30)
#define SDHCI_P_DLLDBG_SLKDC    (SDHCI_PHY_R_OFFSET + 0x32)

#define PHY_CNFG_PHY_RSTN       0
#define PHY_CNFG_PHY_PWRGOOD    1
#define PHY_CNFG_PAD_SP         16
#define PHY_CNFG_PAD_SP_MSK     0xf
#define PHY_CNFG_PAD_SN         20
#define PHY_CNFG_PAD_SN_MSK     0xf

#define PAD_CNFG_RXSEL                0
#define PAD_CNFG_RXSEL_MSK            0x7
#define PAD_CNFG_WEAKPULL_EN          3
#define PAD_CNFG_WEAKPULL_EN_MSK      0x3
#define PAD_CNFG_TXSLEW_CTRL_P        5
#define PAD_CNFG_TXSLEW_CTRL_P_MSK    0xf
#define PAD_CNFG_TXSLEW_CTRL_N        9
#define PAD_CNFG_TXSLEW_CTRL_N_MSK    0xf

#define COMMDL_CNFG_DLSTEP_SEL        0
#define COMMDL_CNFG_DLOUT_EN          1

#define SDCLKDL_CNFG_EXTDLY_EN        0
#define SDCLKDL_CNFG_BYPASS_EN        1
#define SDCLKDL_CNFG_INPSEL_CNFG      2
#define SDCLKDL_CNFG_INPSEL_CNFG_MSK  0x3
#define SDCLKDL_CNFG_UPDATE_DC        4

#define SMPLDL_CNFG_EXTDLY_EN         0
#define SMPLDL_CNFG_BYPASS_EN         1
#define SMPLDL_CNFG_INPSEL_CNFG       2
#define SMPLDL_CNFG_INPSEL_CNFG_MSK   0x3
#define SMPLDL_CNFG_INPSEL_OVERRIDE   4

#define ATDL_CNFG_EXTDLY_EN           0
#define ATDL_CNFG_BYPASS_EN           1
#define ATDL_CNFG_INPSEL_CNFG         2
#define ATDL_CNFG_INPSEL_CNFG_MSK     0x3

#define SD_USE_PIO                    0x1

/**
  card detect status
  -1: haven't check the card detect register
  0 : no card detected
  1 : card detected
**/
#define SDCARD_STATUS_UNKNOWN       (-1)
#define SDCARD_STATUS_INSERTED      (1)
#define SDCARD_STATUS_NOT_INSERTED  (0)

typedef struct {
  UINT32  CmdIdx;
  UINT32  CmdArg;
  UINT32  ResponseType;
  UINT32  Response[4];
} MMC_CMD;

typedef struct {
  UINTN   RegBase;
  UINTN   VendorBase;
  UINTN   DescBase;
  UINTN   DescSize;
  INT32   ClkRate;
  INT32   BusWidth;
  UINT32  Flags;
  INT32   CardIn;
} BM_SD_PARAMS;

extern BM_SD_PARAMS BmParams;

/**
  SD card sends command.

  @param[in]  Idx       Command ID.
  @param[in]  Arg       Command argument.
  @param[in]  RespType  Type of response data.
  @param[out] Response  Response data.

  @retval  EFI_SUCCESS             The command was sent successfully.
  @retval  EFI_DEVICE_ERROR        There was an error during the command transmission or response handling.
  @retval  EFI_TIMEOUT             The command transmission or response handling timed out.

**/
EFI_STATUS
EFIAPI
BmSdSendCmd (
  IN  UINT32 Idx,
  IN  UINT32 Arg,
  IN  UINT32 RespType,
  OUT UINT32 *Response
  );

/**
  Detect the status of the SD card.

  @return The status of the SD card:
          - SDCARD_STATUS_INSERTED:      The SD card is inserted.
          - SDCARD_STATUS_NOT_INSERTED:  The SD card is not inserted.
          - SDCARD_STATUS_UNKNOWN:       The status of the SD card is unknown.

**/
INT32
BmSdCardDetect (
  VOID
  );

/**
  Set the input/output settings for the SD card.

  @param[in] Clk     The clock frequency for the SD card.
  @param[in] Width   The bus width for data transfer.

  @retval EFI_SUCCESS             The input/output settings were set successfully.
  @retval EFI_UNSUPPORTED         The specified bus width is not supported.

**/
EFI_STATUS
BmSdSetIos (
  IN UINT32 Clk,
  IN UINT32 Width
  );

/**
  Prepare the SD card for data transfer.
  Set the number and size of data blocks before sending IO commands to the SD card.

  @param[in]  Lba       Logical Block Address.
  @param[in]  Buf       Buffer Address.
  @param[in]  Size      Size of Data Blocks.

  @retval EFI_SUCCESS             The SD card was prepared successfully.
  @retval Other                   An error occurred during the preparation of the SD card.

**/
EFI_STATUS
BmSdPrepare (
  IN INT32 Lba,
  IN UINTN Buf,
  IN UINTN Size
  );

/**
  SD card sends command to read data blocks.

  @param[in]  Lba       Logical Block Address.
  @param[in]  Buf       Buffer Address.
  @param[in]  Size      Size of Data Blocks.

  @retval  EFI_SUCCESS             The command to read data blocks was sent successfully.
  @retval  EFI_TIMEOUT             The command transmission or data transfer timed out.

**/
EFI_STATUS
BmSdRead (
  IN INT32   Lba,
  IN UINT32* Buf,
  IN UINTN   Size
  );

/**
  SD card sends commands to write data blocks.

  @param[in]  Lba       Logical Block Address.
  @param[in]  Buf       Buffer Address.
  @param[in]  Size      Size of Data Blocks.

  @retval  EFI_SUCCESS             The command to write data blocks was sent successfully.
  @retval  EFI_TIMEOUT             The command transmission or data transfer timed out.

**/
EFI_STATUS
BmSdWrite (
  IN INT32   Lba,
  IN UINT32* Buf,
  IN UINTN   Size
  );

/**
  Initialize the SD card.

  This function performs the initialization of the SD card hardware and settings.

  @param[in] Flags     Initialization flags.

  @retval EFI_SUCCESS  The SD card was initialized successfully.

**/
EFI_STATUS
SdInit (
  IN UINT32  flags
  );

#endif
