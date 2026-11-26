/** @file
  Main Header file for the MMC DXE driver

  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
  Copyright (c) 2023, Academy of Intelligent Innovation, Shandong Universiy, China.P.R. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MMC_H
#define __MMC_H

#include <Uefi.h>
#include <Include/MmcHost.h>
#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define BIT_32(nr)          (1U << (nr))
#define BIT_64(nr)          (1ULL << (nr))
#define UINT64_C(c)         (c ## UL)
#define GENMASK_64(h,l)     (((~UINT64_C(0)) << (l)) & (~UINT64_C(0) >> (64 - 1 - (h))))
#define GENMASK(h,l)        GENMASK_64(h,l)

#define MMC_TRACE(txt)      DEBUG((DEBUG_BLKIO, "MMC: " txt "\n"))

#define MMC_IOBLOCKS_READ   0
#define MMC_IOBLOCKS_WRITE  1

/* Value randomly chosen for eMMC RCA, it should be > 1 */
#define MMC_FIX_RCA         6
#define RCA_SHIFT_OFFSET    16

#define MMC_OCR_POWERUP        BIT31
#define MMC_OCR_ACCESS_MASK    0x3     /* bit[30-29] */
#define MMC_OCR_ACCESS_BYTE    0x1     /* bit[29] */
#define MMC_OCR_ACCESS_SECTOR  0x2     /* bit[30] */
#define OCR_HCS                BIT30
#define OCR_BYTE_MODE          (0U << 29)
#define OCR_SECTOR_MODE        (2U << 29)
#define OCR_ACCESS_MODE_MASK   (3U << 29)
#define OCR_VDD_MIN_2V7        GENMASK(23, 15)
#define OCR_VDD_MIN_2V0        GENMASK(14, 8)
#define OCR_VDD_MIN_1V7        BIT7

/* Value randomly chosen for eMMC RCA, it should be > 1 */
#define MMC_FIX_RCA                  6
#define RCA_SHIFT_OFFSET             16

#define CMD_EXTCSD_PARTITION_CONFIG  179
#define CMD_EXTCSD_BUS_WIDTH         183
#define CMD_EXTCSD_HS_TIMING         185
#define CMD_EXTCSD_PART_SWITCH_TIME  199
#define CMD_EXTCSD_SEC_CNT           212

#define EXTCSD_SET_CMD               (0U << 24)
#define EXTCSD_SET_BITS              (1U << 24)
#define EXTCSD_CLR_BITS              (2U << 24)
#define EXTCSD_WRITE_BYTES           (3U << 24)
#define EXTCSD_CMD(x)                (((x) & 0xff) << 16)
#define EXTCSD_VALUE(x)              (((x) & 0xff) << 8)
#define EXTCSD_CMD_SET_NORMAL        1U

#define CSD_TRAN_SPEED_UNIT_MASK     GENMASK(2, 0)
#define CSD_TRAN_SPEED_MULT_MASK     GENMASK(6, 3)
#define CSD_TRAN_SPEED_MULT_SHIFT    3

#define MMC_CSD_GET_CCC(Response)            (Response[2] >> 20)
#define MMC_CSD_GET_TRANSPEED(Response)      (Response[3] & 0xFF)
#define MMC_CSD_GET_READBLLEN(Response)      ((Response[2] >> 16) & 0xF)
#define MMC_CSD_GET_WRITEBLLEN(Response)     ((Response[0] >> 22) & 0xF)
#define MMC_CSD_GET_FILEFORMAT(Response)     ((Response[0] >> 10) & 0x3)
#define MMC_CSD_GET_FILEFORMATGRP(Response)  ((Response[0] >> 15) & 0x1)
#define MMC_CSD_GET_DEVICESIZE(csd)          (((Response[1] >> 30) & 0x3) | ((Response[2] & 0x3FF) << 2))
#define HC_MMC_CSD_GET_DEVICESIZE(Response)  ((Response[1] >> 16) | ((Response[2] & 0x3F) << 16));
#define MMC_CSD_GET_DEVICESIZEMULT(csd)      ((Response[1] >> 15) & 0x7)

#define MMC_R0_READY_FOR_DATA          (1U << 8)
#define MMC_R0_SWITCH_ERROR            (1U << 7)
#define MMC_R0_CURRENTSTATE(Response)  ((Response[0] >> 9) & 0xF)
#define MMC_R0_STATE_IDLE              0
#define MMC_R0_STATE_READY             1
#define MMC_R0_STATE_IDENT             2
#define MMC_R0_STATE_STDBY             3
#define MMC_R0_STATE_TRAN              4
#define MMC_R0_STATE_DATA              5
#define MMC_R0_STATE_RECV              6
#define MMC_R0_STATE_PROG              7
#define MMC_R0_STATE_DIS               8

#define EMMC_CMD6_ARG_ACCESS(x)        (((x) & 0x3) << 24)
#define EMMC_CMD6_ARG_INDEX(x)         (((x) & 0xFF) << 16)
#define EMMC_CMD6_ARG_VALUE(x)         (((x) & 0xFF) << 8)
#define EMMC_CMD6_ARG_CMD_SET(x)       (((x) & 0x7) << 0)

#define SWITCH_CMD_DATA_LENGTH         64
#define SD_HIGH_SPEED_SUPPORTED        0x200
#define SD_DEFAULT_SPEED               25000000
#define SD_HIGH_SPEED                  50000000
#define SWITCH_CMD_SUCCESS_MASK        0xf
#define CMD8_CHECK_PATTERN             0xAAU
#define VHS_2_7_3_6_V                  BIT8

#define SD_SCR_BUS_WIDTH_1             BIT8
#define SD_SCR_BUS_WIDTH_4             BIT10

typedef enum {
  UNKNOWN_CARD,
  MMC_CARD,              //MMC card
  MMC_CARD_HIGH,         //MMC Card with High capacity
  EMMC_CARD,             //eMMC 4.41 card
  SD_CARD,               //SD 1.1 card
  SD_CARD_2,             //SD 2.0 or above standard card
  SD_CARD_2_HIGH         //SD 2.0 or above high capacity card
} CARD_TYPE;

typedef struct {
  UINT32  Reserved0:   7; // 0
  UINT32  V170_V195:   1; // 1.70V - 1.95V
  UINT32  V200_V260:   7; // 2.00V - 2.60V
  UINT32  V270_V360:   9; // 2.70V - 3.60V
  UINT32  RESERVED_1:  5; // Reserved
  UINT32  AccessMode:  2; // 00b (byte mode), 10b (sector mode)
  UINT32  PowerUp:     1; // This bit is set to LOW if the card has not finished the power up routine
} OCR;

typedef struct {
  UINT8   SD_SPEC:               4; // SD Memory Card - Spec. Version [59:56]
  UINT8   SCR_STRUCTURE:         4; // SCR Structure [63:60]
  UINT8   SD_BUS_WIDTHS:         4; // DAT Bus widths supported [51:48]
  UINT8   DATA_STAT_AFTER_ERASE: 1; // Data Status after erases [55]
  UINT8   SD_SECURITY:           3; // CPRM Security Support [54:52]
  UINT8   EX_SECURITY_1:         1; // Extended Security Support [43]
  UINT8   SD_SPEC4:              1; // Spec. Version 4.00 or higher [42]
  UINT8   RESERVED_1:            2; // Reserved [41:40]
  UINT8   SD_SPEC3:              1; // Spec. Version 3.00 or higher [47]
  UINT8   EX_SECURITY_2:         3; // Extended Security Support [46:44]
  UINT8   CMD_SUPPORT:           4; // Command Support bits [35:32]
  UINT8   RESERVED_2:            4; // Reserved [39:36]
  UINT32  RESERVED_3;               // Manufacturer Usage [31:0]
} SCR;

typedef struct {
  UINT32  NOT_USED;   // 1 [0:0]
  UINT32  CRC;        // CRC7 checksum [7:1]

  UINT32  MDT;        // Manufacturing date [19:8]
  UINT32  RESERVED_1; // Reserved [23:20]
  UINT32  PSN;        // Product serial number [55:24]
  UINT8   PRV;        // Product revision [63:56]
  UINT8   PNM[5];     // Product name [64:103]
  UINT16  OID;        // OEM/Application ID [119:104]
  UINT8   MID;        // Manufacturer ID [127:120]
} CID;

/*
 * designware can't read out response bit 0-7, it only returns
 * bit 8-135, so we shift 8 bits here.
 */
typedef struct {
#ifdef FULL_CSD
  UINT8   NOT_USED:           1; // Not used, always 1 [0:0]
  UINT8   CRC:                7; // CRC [7:1]
#endif
  UINT8   RESERVED_1:         2; // Reserved [9:8]
  UINT8   FILE_FORMAT:        2; // File format [11:10]
  UINT8   TMP_WRITE_PROTECT:  1; // Temporary write protection [12:12]
  UINT8   PERM_WRITE_PROTECT: 1; // Permanent write protection [13:13]
  UINT8   COPY:               1; // Copy flag (OTP) [14:14]
  UINT8   FILE_FORMAT_GRP:    1; // File format group [15:15]

  UINT16  RESERVED_2:         5; // Reserved [20:16]
  UINT16  WRITE_BL_PARTIAL:   1; // Partial blocks for write allowed [21:21]
  UINT16  WRITE_BL_LEN:       4; // Max. write data block length [25:22]
  UINT16  R2W_FACTOR:         3; // Write speed factor [28:26]
  UINT16  RESERVED_3:         2; // Reserved [30:29]
  UINT16  WP_GRP_ENABLE:      1; // Write protect group enable [31:31]

  UINT32  WP_GRP_SIZE:        7; // Write protect group size [38:32]
  UINT32  SECTOR_SIZE:        7; // Erase sector size [45:39]
  UINT32  ERASE_BLK_EN:       1; // Erase single block enable [46:46]
  UINT32  C_SIZE_MULT:        3; // Device size multiplier [49:47]
  UINT32  VDD_W_CURR_MAX:     3; // Max. write current @ VDD max [52:50]
  UINT32  VDD_W_CURR_MIN:     3; // Max. write current @ VDD min [55:53]
  UINT32  VDD_R_CURR_MAX:     3; // Max. read current @ VDD max [58:56]
  UINT32  VDD_R_CURR_MIN:     3; // Max. read current @ VDD min [61:59]
  UINT32  C_SIZELow2:         2; // Device size [63:62]

  UINT32  C_SIZEHigh10:       10;// Device size [73:64]
  UINT32  RESERVED_4:         2; // Reserved [75:74]
  UINT32  DSR_IMP:            1; // DSR implemented [76:76]
  UINT32  READ_BLK_MISALIGN:  1; // Read block misalignment [77:77]
  UINT32  WRITE_BLK_MISALIGN: 1; // Write block misalignment [78:78]
  UINT32  READ_BL_PARTIAL:    1; // Partial blocks for read allowed [79:79]
  UINT32  READ_BL_LEN:        4; // Max. read data block length [83:80]
  UINT32  CCC:                12;// Card command classes [95:84]

  UINT8   TRAN_SPEED          ;  // Max. bus clock frequency [103:96]
  UINT8   NSAC                ;  // Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8   TAAC                ;  // Data read access-time 1 [119:112]

  UINT8   RESERVED_5:         2; // Reserved [121:120]
  UINT8   SPEC_VERS:          4; // System specification version [125:122]
  UINT8   CSD_STRUCTURE:      2; // CSD structure [127:126]
} CSD;

typedef struct {
#ifdef FULL_CSD
  UINT8   NOT_USED:           1; // Not used, always 1 [0:0]
  UINT8   CRC:                7; // CRC [7:1]
#endif
  UINT8   RESERVED_1:         2; // Reserved [9:8]
  UINT8   FILE_FORMAT:        2; // File format [11:10]
  UINT8   TMP_WRITE_PROTECT:  1; // Temporary write protection [12:12]
  UINT8   PERM_WRITE_PROTECT: 1; // Permanent write protection [13:13]
  UINT8   COPY:               1; // Copy flag (OTP) [14:14]
  UINT8   FILE_FORMAT_GRP:    1; // File format group [15:15]

  UINT16  RESERVED_2:         5; // Reserved [20:16]
  UINT16  WRITE_BL_PARTIAL:   1; // Partial blocks for write allowed [21:21]
  UINT16  WRITE_BL_LEN:       4; // Max. write data block length [25:22]
  UINT16  R2W_FACTOR:         3; // Write speed factor [28:26]
  UINT16  RESERVED_3:         2; // Reserved [30:29]
  UINT16  WP_GRP_ENABLE:      1; // Write protect group enable [31:31]

  UINT32  WP_GRP_SIZE:        7; // Write protect group size [38:32]
  UINT32  SECTOR_SIZE:        7; // Erase sector size [45:39]
  UINT32  ERASE_BLK_EN:       1; // Erase single block enable [46:46]
  UINT32  RESERVED_4:         1; // Reserved [47]
  UINT32  C_SIZELow16:        16; // Device size [63:48]

  UINT32  C_SIZEHigh6:        6; // Device size [69:64]
  UINT32  RESERVED_5:         6; // Reserved [75:70]
  UINT32  DSR_IMP:            1; // DSR implemented [76:76]
  UINT32  READ_BLK_MISALIGN:  1; // Read block misalignment [77:77]
  UINT32  WRITE_BLK_MISALIGN: 1; // Write block misalignment [78:78]
  UINT32  READ_BL_PARTIAL:    1; // Partial blocks for read allowed [79:79]
  UINT32  READ_BL_LEN:        4; // Max. read data block length [83:80]
  UINT32  CCC:                12;// Card command classes [95:84]

  UINT8   TRAN_SPEED:         8;  // Max. bus clock frequency [103:96]
  UINT8   NSAC:               8;  // Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8   TAAC:               8;  // Data read access-time 1 [119:112]

  UINT8   RESERVED_6:         6; // Reserved [121:120]
  UINT8   CSD_STRUCTURE:      2; // CSD structure [127:126]
} ECSD;

typedef struct {
  UINT16    RCA;
  CARD_TYPE CardType;
  OCR       OCRData;
  CID       CIDData;
  CSD       CSDData;
  ECSD      *ECSDData;                         // MMC V2 extended card specific
} CARD_INFO;

typedef struct _MMC_HOST_INSTANCE {
  UINTN                     Signature;
  LIST_ENTRY                Link;
  EFI_HANDLE                MmcHandle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  MMC_STATE                 State;
  EFI_BLOCK_IO_PROTOCOL     BlockIo;
  CARD_INFO                 CardInfo;
  EFI_MMC_HOST_PROTOCOL     *MmcHost;

  BOOLEAN                   Initialized;
} MMC_HOST_INSTANCE;

#define MMC_HOST_INSTANCE_SIGNATURE                 SIGNATURE_32('m', 'm', 'c', 'h')
#define MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(a)     CR (a, MMC_HOST_INSTANCE, BlockIo, MMC_HOST_INSTANCE_SIGNATURE)
#define MMC_HOST_INSTANCE_FROM_LINK(a)              CR (a, MMC_HOST_INSTANCE, Link, MMC_HOST_INSTANCE_SIGNATURE)


EFI_STATUS
EFIAPI
MmcGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
MmcGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

extern EFI_COMPONENT_NAME_PROTOCOL  gMmcComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gMmcComponentName2;

extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gMmcDriverDiagnostics2;

extern LIST_ENTRY mMmcHostPool;

/**
  Reset the block device.

  This function implements EFI_BLOCK_IO_PROTOCOL.Reset().
  It resets the block device hardware.
  ExtendedVerification is ignored in this implementation.

  @param  This                   Indicates a pointer to the calling context.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive
                                 verification operation of the device during reset.

  @retval EFI_SUCCESS            The block device was reset.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
MmcReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  );

/**
  Reads the requested number of blocks from the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks().
  It reads the requested number of blocks from the device.
  All the blocks are read, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the read request is for.
  @param  Lba                    The starting logical block address to read from on the device.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller is
                                 responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS            The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the read operation.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
MmcReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  );

/**
  Writes a specified number of blocks to the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks().
  It writes a specified number of blocks to the device.
  All blocks are written, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the write request is for.
  @param  Lba                    The starting logical block address to be written.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED    The device cannot be written to.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic
                                 block size of the device.
  @retval EFI_INVALID_PARAMETER  The write request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
MmcWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  );

/**
  Flushes all modified data to a physical block device.

  @param  This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS            All outstanding data were written correctly to the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to write data.
  @retval EFI_NO_MEDIA           There is no media in the device.

**/
EFI_STATUS
EFIAPI
MmcFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  );

/**
  Sets the state of the MMC host instance and invokes the
  NotifyState function of the MMC host, passing the updated state.

  @param  MmcHostInstance        Pointer to the MMC host instance.
  @param  State                  The new state to be set for the MMC host instance.

  @retval EFI_STATUS

**/
EFI_STATUS
MmcNotifyState (
  IN MMC_HOST_INSTANCE      *MmcHostInstance,
  IN MMC_STATE               State
  );

/**
  Initialize the MMC device.

  @param[in] MmcHostInstance   MMC host instance

  @retval EFI_SUCCESS          MMC device initialized successfully
  @retval Other                MMC device initialization failed

**/
EFI_STATUS
InitializeMmcDevice (
  IN  MMC_HOST_INSTANCE     *MmcHost
  );

/**
  Callback function to check MMC cards.

  @param[in] Event    The event that is being triggered
  @param[in] Context  The context passed to the event

**/
VOID
EFIAPI
CheckCardsCallback (
  IN  EFI_EVENT   Event,
  IN  VOID        *Context
  );

/**
  Print the Card Specific Data (CSD).

  @param[in] Csd    Pointer to the CSD array

**/
VOID
PrintCSD (
  IN UINT32* Csd
  );

/**
  Print the Relative Card Address (RCA).

  @param[in] Rca    The Relative Card Address (RCA) value

**/
VOID
PrintRCA (
  IN UINT32 Rca
  );

/**
  Print the Operation Condition Register (OCR).

  @param[in] Ocr    The Operation Condition Register (OCR) value.

**/
VOID
PrintOCR (
  IN UINT32 Ocr
  );

/**
  Print the R1 response.

  @param[in] Response   The R1 response value.

**/
VOID
PrintResponseR1 (
  IN  UINT32 Response
  );

/**
  Print the Card Identification (CID) register.

  @param[in] Cid    Pointer to the CID array.

**/
VOID
PrintCID (
  IN UINT32* Cid
  );

#endif
