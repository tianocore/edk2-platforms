/** @file
  Definition of the MMC Host Protocol

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
  Copyright (c) Academy of Intelligent Innovation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef __MMC_HOST_PROTOCOL_H__
#define __MMC_HOST_PROTOCOL_H__

/*
 * Global ID for the MMC Host Protocol
 */
#define MMC_HOST_PROTOCOL_GUID \
  { 0x3e591c00, 0x9e4a, 0x11df, {0x92, 0x44, 0x00, 0x02, 0xA5, 0xF5, 0xF5, 0x1B } }

#define MMC_BLOCK_SIZE      512U
#define MMC_BLOCK_MASK      (MMC_BLOCK_SIZE - 1U)
#define MMC_BOOT_CLK_RATE   (400 * 1000)

/* Values in EXT CSD register */
#define MMC_BUS_WIDTH_1       0U
#define MMC_BUS_WIDTH_4       1U
#define MMC_BUS_WIDTH_8       2U
#define MMC_BUS_WIDTH_DDR_4   5U
#define MMC_BUS_WIDTH_DDR_8   6U

#define MMC_RSP_48          BIT0
#define MMC_RSP_136         BIT1    /* 136 bit response */
#define MMC_RSP_CRC         BIT2    /* expect valid crc */
#define MMC_RSP_CMD_IDX     BIT3    /* response contains cmd idx */
#define MMC_RSP_BUSY        BIT4    /* device may be busy */

/* JEDEC 4.51 chapter 6.12 */
#define MMC_RESPONSE_R1     (MMC_RSP_48 | MMC_RSP_CMD_IDX | MMC_RSP_CRC)
#define MMC_RESPONSE_R1B    (MMC_RESPONSE_R1 | MMC_RSP_BUSY)
#define MMC_RESPONSE_R2     (MMC_RSP_48 | MMC_RSP_136 | MMC_RSP_CRC)
#define MMC_RESPONSE_R3     (MMC_RSP_48)
#define MMC_RESPONSE_R4     (MMC_RSP_48)
#define MMC_RESPONSE_R5     (MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R6     (MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R7     (MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)

typedef UINT32  MMC_RESPONSE_TYPE;

typedef UINT32  MMC_IDX;

#define MMC_CMD_WAIT_RESPONSE      (1 << 16)
#define MMC_CMD_LONG_RESPONSE      (1 << 17)
#define MMC_CMD_NO_CRC_RESPONSE    (1 << 18)

#define MMC_INDX(Index)       ((Index) & 0xFFFF)
#define MMC_GET_INDX(MmcCmd)  ((MmcCmd) & 0xFFFF)

#define MMC_CMD0              (MMC_INDX(0))
#define MMC_CMD1              (MMC_INDX(1))
#define MMC_CMD2              (MMC_INDX(2))
#define MMC_CMD3              (MMC_INDX(3))
#define MMC_CMD5              (MMC_INDX(5))
#define MMC_CMD6              (MMC_INDX(6))
#define MMC_CMD7              (MMC_INDX(7))
#define MMC_CMD8              (MMC_INDX(8))
#define MMC_CMD9              (MMC_INDX(9))
#define MMC_CMD11             (MMC_INDX(11))
#define MMC_CMD12             (MMC_INDX(12))
#define MMC_CMD13             (MMC_INDX(13))
#define MMC_CMD16             (MMC_INDX(16))
#define MMC_CMD17             (MMC_INDX(17))
#define MMC_CMD18             (MMC_INDX(18))
#define MMC_CMD20             (MMC_INDX(20))
#define MMC_CMD23             (MMC_INDX(23))
#define MMC_CMD24             (MMC_INDX(24))
#define MMC_CMD25             (MMC_INDX(25))
#define MMC_CMD55             (MMC_INDX(55))
#define MMC_ACMD22            (MMC_INDX(22))
#define MMC_ACMD41            (MMC_INDX(41))
#define MMC_ACMD51            (MMC_INDX(51))

// Valid responses for CMD1 in eMMC
#define EMMC_CMD1_CAPACITY_LESS_THAN_2GB    0x00FF8080 // Capacity <= 2GB, byte addressing used
#define EMMC_CMD1_CAPACITY_GREATER_THAN_2GB 0x40FF8080 // Capacity > 2GB, 512-byte sector addressing used

#define MMC_STATUS_APP_CMD    (1 << 5)

typedef enum _MMC_STATE {
  MmcInvalidState = 0,
  MmcHwInitializationState,
  MmcIdleState,
  MmcReadyState,
  MmcIdentificationState,
  MmcStandByState,
  MmcTransferState,
  MmcSendingDataState,
  MmcReceiveDataState,
  MmcProgrammingState,
  MmcDisconnectState,
} MMC_STATE;

typedef enum _CARD_DETECT_STATE {
  CardDetectRequired = 0,
  CardDetectInProgress,
  CardDetectCompleted
} CARD_DETECT_STATE;

#define EMMCBACKWARD         (0)
#define EMMCHS26             (1 << 0)      // High-Speed @26MHz at rated device voltages
#define EMMCHS52             (1 << 1)      // High-Speed @52MHz at rated device voltages
#define EMMCHS52DDR1V8       (1 << 2)      // High-Speed Dual Data Rate @52MHz 1.8V or 3V I/O
#define EMMCHS52DDR1V2       (1 << 3)      // High-Speed Dual Data Rate @52MHz 1.2V I/O
#define EMMCHS200SDR1V8      (1 << 4)      // HS200 Single Data Rate @200MHz 1.8V I/O
#define EMMCHS200SDR1V2      (1 << 5)      // HS200 Single Data Rate @200MHz 1.2V I/O
#define EMMCHS400DDR1V8      (1 << 6)      // HS400 Dual Data Rate @400MHz 1.8V I/O
#define EMMCHS400DDR1V2      (1 << 7)      // HS400 Dual Data Rate @400MHz 1.2V I/O

///
/// Forward declaration for EFI_MMC_HOST_PROTOCOL
///
typedef struct _EFI_MMC_HOST_PROTOCOL  EFI_MMC_HOST_PROTOCOL;

typedef
BOOLEAN
(EFIAPI *MMC_ISCARDPRESENT) (
  IN  EFI_MMC_HOST_PROTOCOL   *This
  );

typedef
BOOLEAN
(EFIAPI *MMC_ISREADONLY) (
  IN  EFI_MMC_HOST_PROTOCOL   *This
  );

typedef
EFI_STATUS
(EFIAPI *MMC_BUILDDEVICEPATH) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  );

typedef
EFI_STATUS
(EFIAPI *MMC_NOTIFYSTATE) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  IN  MMC_STATE                 State
  );

typedef
EFI_STATUS
(EFIAPI *MMC_SENDCOMMAND) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  IN  MMC_IDX                   Cmd,
  IN  UINT32                    Argument,
  IN  MMC_RESPONSE_TYPE         Type,
  IN  UINT32                    *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *MMC_READBLOCKDATA) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  IN  EFI_LBA                   Lba,
  IN  UINTN                     Length,
  OUT UINT32                    *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *MMC_WRITEBLOCKDATA) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  IN  EFI_LBA                   Lba,
  IN  UINTN                     Length,
  IN  UINT32                    *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *MMC_SETIOS) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  IN  UINT32                    BusClockFreq,
  IN  UINT32                    BusWidth
  );

typedef
EFI_STATUS
(EFIAPI *MMC_PREPARE) (
  IN  EFI_MMC_HOST_PROTOCOL     *This,
  IN  EFI_LBA                   Lba,
  IN  UINTN                     Length,
  IN  UINTN                     Buffer
  );

typedef
BOOLEAN
(EFIAPI *MMC_ISMULTIBLOCK) (
  IN  EFI_MMC_HOST_PROTOCOL     *This
  );

struct _EFI_MMC_HOST_PROTOCOL {
  UINT32                  Revision;
  MMC_ISCARDPRESENT       IsCardPresent;
  MMC_ISREADONLY          IsReadOnly;
  MMC_BUILDDEVICEPATH     BuildDevicePath;

  MMC_NOTIFYSTATE         NotifyState;

  MMC_SENDCOMMAND         SendCommand;

  MMC_READBLOCKDATA       ReadBlockData;
  MMC_WRITEBLOCKDATA      WriteBlockData;

  MMC_SETIOS              SetIos;
  MMC_PREPARE             Prepare;
  MMC_ISMULTIBLOCK        IsMultiBlock;
};

#define MMC_HOST_PROTOCOL_REVISION      0x00010002    // 1.2

#define MMC_HOST_HAS_SETIOS(Host)       (Host->Revision >= MMC_HOST_PROTOCOL_REVISION && \
                                         Host->SetIos != NULL)
#define MMC_HOST_HAS_ISMULTIBLOCK(Host) (Host->Revision >= MMC_HOST_PROTOCOL_REVISION && \
                                         Host->IsMultiBlock != NULL)

#endif /* __MMC_HOST_PROTOCOL_H__ */
