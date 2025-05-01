/** @file
 *
 * Mmc Driver for Arasan SD 3.0/SDIO 3.0/eMMC 4.51 Host Controller
 * This doesn't support high speed mode yet, or eMMC.
 *
 * Copyright (c) 2025, Linaro Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Derived from Platform/RaspberryPi/Drivers/ArasanMmcHostDxe/ArasanMmcHostDxe.h
 *
 **/

#ifndef ARASAN_MMC_HOST_DXE_H
#define ARASAN_MMC_HOST_DXE_H

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/MmcHost.h>

#define MAX_RETRY_COUNT          (20000UL)

/*
 * The following values are in microseconds
 */
#define STALL_AFTER_SEND_CMD_US  (FixedPcdGet64(PcdMmcStallAfterCmdSend))
#define STALL_AFTER_REC_RESP_US  (FixedPcdGet64(PcdMmcStallAfterResponseReceive))
#define STALL_AFTER_WRITE_US     (FixedPcdGet64(PcdMmcStallAfterDataWrite))
#define STALL_AFTER_READ_US      (FixedPcdGet64(PcdMmcStallAfterDataRead))
#define STALL_AFTER_REG_WRITE_US (FixedPcdGet64(PcdMmcStallAfterRegisterWrite))
#define STALL_AFTER_RETRY_US     (FixedPcdGet64(PcdMmcStallAfterRetry))

#define MAX_DIVISOR_VALUE        (1023UL)

#define DEBUG_MMCHOST_SD         DEBUG_VERBOSE

/*
 * MMC/SD/SDIO register definitions.
 * We don't support eMMC yet (MMCHS1_BASE).
 * Therefore, this implementation will force using
 * MMCHS2_BASE
 */
#define MMCHS_BASE               (FixedPcdGet64 (PcdSdhciBase))

/*
 * High speed mode is not supported yet, so this implementation
 * currently only runs at 25MHz speed
 */
#define MMCHS_BASE_FREQUENCY     (200000000UL)
#define MMCHS_STANDBY_FREQUENCY  (25000000UL)
#define MMCHS_INIT_FREQUENCY     (400000UL)

#define MMCHS_BLK                (MMCHS_BASE + 0x4)
#define  BLEN_8BYTES             (0x8UL)
#define  BLEN_512BYTES           (0x200UL)

#define MMCHS_ARG                (MMCHS_BASE + 0x8)

/*
 * This represents 2 registers (Transfer Mode & Command)
 * for simpler command representation
 */
#define MMCHS_CMD                 (MMCHS_BASE + 0xC)
#define  DDIR_READ                BIT4
#define  DDIR_WRITE               ~BIT4
#define  MSBS_MULTBLK             BIT5
#define  RSP_TYPE_MASK            (0x3UL << 16)
#define   RSP_TYPE_136BITS        (0x1UL << 16)
#define   RSP_TYPE_48BITS         (0x2UL << 16)
#define   RSP_TYPE_48BUSY         (0x3UL << 16)
#define  CCCE_ENABLE              BIT19
#define  CICE_ENABLE              BIT20
#define  DP_ENABLE                BIT21
#define  CMD_TYPE_MASK            (0x3UL  << 22)
#define   CMD_TYPE_NORMAL         (0x0UL  << 22)
#define   CMD_TYPE_ABORT          (0x3UL  << 22)

#define TYPE(CMD_TYPE)            (((CMD_TYPE) & (CMD_TYPE_MASK)))
#define CMD_INDX_MASK             (0x3FUL << 24)
#define INDX_TO_CMD(CMD_INDX)     ((CMD_INDX << 24) & CMD_INDX_MASK)
#define CMD_TO_INDX(CMD)          (((CMD) & CMD_INDX_MASK) >> 24)
#define NORMAL_CMD(CMD_INDX)      (TYPE(CMD_TYPE_NORMAL) | INDX_TO_CMD(CMD_INDX))
#define ABORT_CMD(CMD_INDX)       (TYPE(CMD_TYPE_ABORT) | INDX_TO_CMD(CMD_INDX))

#define MMCHS_RSP10               (MMCHS_BASE + 0x10)
#define MMCHS_RSP32               (MMCHS_BASE + 0x14)
#define MMCHS_RSP54               (MMCHS_BASE + 0x18)
#define MMCHS_RSP76               (MMCHS_BASE + 0x1C)
#define MMCHS_DATA                (MMCHS_BASE + 0x20)

#define MMCHS_PRES_STATE          (MMCHS_BASE + 0x24)
#define  CMDI                     BIT0
#define  DATI                     BIT1
#define  WRITE_PROTECT_OFF        BIT19

/*
 * This represents 2 registers (Host Control 1 & Power Control)
 * for simpler implementation
 */
#define MMCHS_HPCTL               (MMCHS_BASE + 0x28)
#define  CDTL                     BIT6
#define  CDSS                     BIT7
#define  SDBP                     BIT8
#define  SDVS_MASK                (0x7UL << 9)
#define   SDVS_1_8_V              (0x5UL << 9)
#define   SDVS_3_0_V              (0x6UL << 9)
#define   SDVS_3_3_V              (0x7UL << 9)

/*
 * This represents 2 registers (Clock and Timeout Control)
 * for simpler implementation
 */
#define MMCHS_SYSCTL              (MMCHS_BASE + 0x2C)
#define  ICE                      BIT0
#define  ICS                      BIT1
#define  CEN                      BIT2
#define  CLKD_MASK                (0x3FFUL << 6)
#define   CLKD_80KHZ              (0x258UL)        /* (96*1000/80)/2 */
#define   CLKD_400KHZ             (0xF0UL)
#define   CLKD_12500KHZ           (0x200UL)
#define  DTO_MASK                 (0xFUL << 16)
#define   DTO_VAL                 (0xEUL << 16)
#define  SRA                      BIT24
#define  SRC                      BIT25
#define  SRD                      BIT26

#define MMCHS_INT_STAT            (MMCHS_BASE + 0x30)
#define  CC                       BIT0
#define  BWR                      BIT4
#define  BRR                      BIT5
#define  CARD_INS                 BIT6
#define  ERRI                     BIT15

#define MMCHS_IE                  (MMCHS_BASE + 0x34)
#define  ALL_EN                   (0xFFFFFFFFUL)

#define MMCHS_ISE                 (MMCHS_BASE + 0x38)
#define MMCHS_AC12                (MMCHS_BASE + 0x3C)
#define MMCHS_HC2R                (MMCHS_BASE + 0x3E)
#define MMCHS_CAPA                (MMCHS_BASE + 0x40)
#define MMCHS_CUR_CAPA            (MMCHS_BASE + 0x48)
#define MMCHS_REV                 (MMCHS_BASE + 0xFC)

/*
 * Command types & Commands
 */
#define CMD_R1                    (RSP_TYPE_48BITS | CCCE_ENABLE | CICE_ENABLE)
#define CMD_R1B                   (RSP_TYPE_48BUSY | CCCE_ENABLE | CICE_ENABLE)
#define CMD_R2                    (RSP_TYPE_136BITS | CCCE_ENABLE)
#define CMD_R3                    (RSP_TYPE_48BITS)
#define CMD_R6                    (RSP_TYPE_48BITS | CCCE_ENABLE | CICE_ENABLE)
#define CMD_R7                    (RSP_TYPE_48BITS | CCCE_ENABLE | CICE_ENABLE)
#define CMD_R1_ADTC               (CMD_R1 | DP_ENABLE)
#define CMD_R1_ADTC_READ          (CMD_R1_ADTC | DDIR_READ)
#define CMD_R1_ADTC_WRITE         (CMD_R1_ADTC & DDIR_WRITE)

#define CMD0                      (NORMAL_CMD(0))                                     /* Go idle */
#define CMD1                      (NORMAL_CMD(1) | CMD_R3)                            /* MMC: Send Op Cond */
#define CMD2                      (NORMAL_CMD(2) | CMD_R2)                            /* Send CID */
#define CMD3                      (NORMAL_CMD(3) | CMD_R6)                            /* Set Relative Addr */
#define CMD4                      (NORMAL_CMD(4))                                     /* Set DSR */
#define CMD5                      (NORMAL_CMD(5) | CMD_R1B)                           /* SDIO: Sleep/Awake */
#define CMD6                      (NORMAL_CMD(6) | CMD_R1_ADTC_READ)                  /* Switch */
#define CMD7                      (NORMAL_CMD(7) | CMD_R1B)                           /* Select/Deselect */
#define CMD8_SD                   (NORMAL_CMD(8) | CMD_R7)                            /* Send If Cond */
#define  CMD8_SD_ARG              (0x0UL << 12 | BIT8 | 0xCEUL << 0)
#define CMD8_MMC                  (NORMAL_CMD(8) | CMD_R1_ADTC_READ)                  /* Send Ext Csd */
#define  CMD8_MMC_ARG             (0)
#define CMD9                      (NORMAL_CMD(9) | CMD_R2)                            /* Send CSD */
#define CMD10                     (NORMAL_CMD(10) | CMD_R2)                           /* Send CID */
#define CMD11                     (NORMAL_CMD(11) | CMD_R1)                           /* Voltage Switch */
#define CMD12                     (ABORT_CMD(12) | CMD_R1B)                           /* Stop Transmission */
#define CMD13                     (NORMAL_CMD(13) | CMD_R1)                           /* Send Status */
#define CMD15                     (NORMAL_CMD(15))                                    /* Go inactive state */
#define CMD16                     (NORMAL_CMD(16) | CMD_R1)                           /* Set Blocklen */
#define CMD17                     (NORMAL_CMD(17) | CMD_R1_ADTC_READ)                 /* Read Single Block */
#define CMD18                     (NORMAL_CMD(18) | CMD_R1_ADTC_READ | MSBS_MULTBLK)  /* Read Multiple Blocks */
#define CMD19                     (NORMAL_CMD(19) | CMD_R1_ADTC_READ)                 /* SD: Send Tuning Block (64 bytes) */
#define CMD20                     (NORMAL_CMD(20) | CMD_R1B)                          /* SD: Speed Class Control */
#define CMD23                     (NORMAL_CMD(23) | CMD_R1)                           /* Set Block Count for CMD18 and CMD25 */
#define CMD24                     (NORMAL_CMD(24) | CMD_R1_ADTC_WRITE)                /* Write Block */
#define CMD25                     (NORMAL_CMD(25) | CMD_R1_ADTC_WRITE | MSBS_MULTBLK) /* Write Multiple Blocks */
#define CMD55                     (NORMAL_CMD(55) | CMD_R1)                           /* App Cmd */
#define ACMD6                     (NORMAL_CMD(6)  | CMD_R1)                           /* Set Bus Width */
#define ACMD41                    (NORMAL_CMD(41) | CMD_R3)                           /* Send Op Cond */
#define ACMD51                    (NORMAL_CMD(51) | CMD_R1_ADTC_READ)                 /* Send SCR */
#define INVALID_CMD               (0xFFFFFFFFUL)

/* User-friendly command names */
#define CMD_IO_SEND_OP_COND       CMD5
#define CMD_SEND_CSD              CMD9 /* CSD: Card-Specific Data */
#define CMD_STOP_TRANSMISSION     CMD12
#define CMD_SEND_STATUS           CMD13
#define CMD_READ_SINGLE_BLOCK     CMD17
#define CMD_READ_MULTIPLE_BLOCK   CMD18
#define CMD_SET_BLOCK_COUNT       CMD23
#define CMD_WRITE_SINGLE_BLOCK    CMD24
#define CMD_WRITE_MULTIPLE_BLOCK  CMD25

typedef enum _CARD_DETECT_STATE {
  CardDetectRequired = 0,
  CardDetectInProgress,
  CardDetectCompleted
} CARD_DETECT_STATE;

/**
 * @brief Checks if card is write-protected
 *
 * @param This Pointer to Mmc host protocol instance
 * @return BOOLEAN
 * @retval TRUE,  card is write-protected
 * @retval FALSE, card is not write-protected
 */
STATIC
BOOLEAN
EFIAPI
MMCIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL  *This
  );

/**
 * @brief Adds device path node for Mmc
 *
 * @param This Pointer to Mmc host protocol instance
 * @param DevicePath On input,  points to current device path node.
 *                   On output, points to next device path node.
 * @return EFI_STATUS
 * @retval EFI_SUCCESS New device path node is added
 * @retval EFI_OUT_OF_RESOURCES Failed to create device path node
 */
STATIC
EFI_STATUS
EFIAPI
MMCBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL         *This,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  );

/**
 * @brief Send a command
 *
 * @param This Pointer to Mmc host protocol instance
 * @param MmcCmd The command to be sent (MMC_CMDx)
 * @param Argument Command arguments
 * @return EFI_STATUS
 * @retval EFI_SUCCESS Command is sent successfully or ignored
 * @retval EFI_UNSUPPORTED Command is invalid
 * @retval EFI_TIMEOUT Timeout occured
 * @retval EFI_DEVICE_ERROR An error occured while sending the command. Check verbose logs for details.
 */
STATIC
EFI_STATUS
EFIAPI
MMCSendCommand (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN MMC_CMD                MmcCmd,
  IN UINT32                 Argument
  );

/**
 * @brief Notify the Mmc host with current state
 *
 * @param This Pointer to Mmc host protocol instance
 * @param State Only states defined in MMC_STATE (MmcHostProtocol) are accepted
 * @return EFI_STATUS
 * @retval EFI_SUCCESS Notification and subsequent operations succeeded
 * Otherwise, an error value according to which state/operation failed
 */
STATIC
EFI_STATUS
EFIAPI
MMCNotifyState (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN MMC_STATE              State
  );

/**
 * @brief Check if a card exists
 *
 * @param This Pointer to Mmc host protocol instance
 * @return BOOLEAN
 * TRUE Card is detected
 * FALSE No card present, or failed to detect one
 */
STATIC
BOOLEAN
EFIAPI
MMCIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL  *This
  );

/**
 * @brief Receive a command response
 *
 * @param This Pointer to Mmc host protocol instance
 * @param Type Command type
 * @param Buffer Buffer to get the response data
 * @return EFI_STATUS
 * EFI_SUCCESS Data read successfully
 */
STATIC
EFI_STATUS
EFIAPI
MMCReceiveResponse (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN MMC_RESPONSE_TYPE      Type,
  OUT UINT32                *Buffer
  );

/**
 * @brief Read from card
 *
 * @param This Pointer to Mmc host protocol instance
 * @param Lba Logical block address (not used)
 * @param Length Length of data to be read
 * @param Buffer Buffer to get data
 * @return EFI_STATUS
 * @retval EFI_SUCCESS Data read successfully
 * @retval EFI_INVALID_PARAMETER One of the parameters is invalid. Check verbose logs for details.
 * @retval EFI_TIMEOUT Timeout occured
 */
STATIC
EFI_STATUS
EFIAPI
MMCReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN EFI_LBA                Lba,
  IN UINTN                  Length,
  OUT UINT32                *Buffer
  );

/**
 * @brief Write to card
 *
 * @param This Pointer to Mmc host protocol instance
 * @param Lba Logical block address (not used)
 * @param Length Length of data to be written
 * @param Buffer Buffer that holds data
 * @return EFI_STATUS
 * @retval EFI_SUCCESS Data read successfully
 * @retval EFI_INVALID_PARAMETER One of the parameters is invalid. Check verbose logs for details.
 * @retval EFI_TIMEOUT Timeout occured
 */
STATIC
EFI_STATUS
EFIAPI
MMCWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL  *This,
  IN EFI_LBA                Lba,
  IN UINTN                  Length,
  IN UINT32                 *Buffer
  );

/**
 * @brief Check if the Mmc Host supports multi-block
 *
 * @param This Pointer to Mmc host protocol instance
 * @return BOOLEAN
 * TRUE Multi-block is supported
 */
STATIC
BOOLEAN
EFIAPI
MMCIsMultiBlock (
  IN EFI_MMC_HOST_PROTOCOL  *This
  );

/**
 * @brief Initialize Mmc Host (driver entry point)
 *
 * @param ImageHandle A handle for the image that is initializing this driver
 * @param SystemTable A pointer to the EFI system table
 * @return EFI_STATUS
 * @retval EFI_SUCCESS or same errors as CoreInstallMultipleProtocolInterfaces
 */
EFI_STATUS
MMCInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
