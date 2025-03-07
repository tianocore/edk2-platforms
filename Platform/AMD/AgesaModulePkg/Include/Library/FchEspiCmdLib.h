/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FCH_ESPI_CMD_LIB_H_
#define FCH_ESPI_CMD_LIB_H_

#include <Uefi/UefiBaseType.h>

#define MAX_ESPI_RETRY  100000ul
//
// Cycle Type
//
#define CYCLE_TYPE_FLASH_READ   0
#define CYCLE_TYPE_FLASH_WRITE  1
#define CYCLE_TYPE_FLASH_ERASE  2
#define CYCLE_TYPE_RPMC_OP1     3
#define CYCLE_TYPE_RPMC_OP2     4

// RPMC OP1/OP2 Command Payload Max Length (512 Bits)
#define RPMC_CMD_PAYLOAD_MAX_LEN  0x40

//
// Master Registers
//
#define SET_CONFIGURATION   0
#define GET_CONFIGURATION   1
#define IN_BAND_RESET       2
#define PC_MSG_DOWN_STREAM  4
#define VM_DOWN_STREAM      5
#define OOB_DOWN_STREAM     6
#define FA_DOWN_STREAM      7

// ESPIx00
#define DNCMD_STATUS  BIT3

// ESPIx2C Master Capability
#define MASTER_FA_SUPPORT          BIT0
#define MASTER_OOB_SUPPORT         BIT1
#define MASTER_VW_SUPPORT          BIT2
#define MASTER_PERIPHERAL_SUPPORT  BIT3

// ESPIx68  Slave0 Configuration
#define SLAVE_FA_ENABLE   BIT0
#define SLAVE_OOB_ENABLE  BIT1
#define SLAVE_VW_ENABLE   BIT2
#define SLAVE_PC_ENABLE   BIT3

/// eSPIx00 eSPI Software Specific Register 0
#define ESPI_DN_TXHDR_0  0x00
typedef union {
  struct {
    UINT32    SWCommandType        : 3;
    UINT32    CommandStatus        : 1;
    UINT32    PutFlashNpTranActive : 1;
    UINT32    Reserved             : 3;
    UINT32    DnCmdHdata0          : 8;
    UINT32    DnCmdHdata1          : 8;
    UINT32    DnCmdHdata2          : 8;
  } Field;
  UINT32    Value;
} ESPIx00_DN_TXHDR0;

/// eSPIx04 eSPI Software Specific Register 1
#define ESPI_DN_TXHDR_1  0x04
typedef union {
  struct {
    UINT32    DnCmdHdata3 : 8;
    UINT32    DnCmdHdata4 : 8;
    UINT32    DnCmdHdata5 : 8;
    UINT32    DnCmdHdata6 : 8;
  } Field;
  UINT32    Value;
} ESPIx04_DN_TXHDR1;

#define ESPI_DN_TXHDR_2  0x08
typedef union {
  struct {
    UINT32    DnCmdHdata7 : 8;
    UINT32    Reserved    : 24;
  } Field;
  UINT32    Value;
} ESPIx08_DN_TXHDR2;

#define ESPI_DN_TXDATA_PORT  0x0C
typedef union {
  struct {
    UINT32    DnTxData0 : 8;
    UINT32    DnTxData1 : 8;
    UINT32    DnTxData2 : 8;
    UINT32    DnTxData3 : 8;
  } Field;
  UINT32    Value;
} ESPIx0C_DN_TXDATA_PORT;

#define ESPI_UP_RXHDR_0  0x10
typedef union {
  struct {
    UINT32    UpCommandType   : 3;
    UINT32    UpCommandStatus : 1;
    UINT32    SlaveSel        : 2;
    UINT32    Reserved        : 2;
    UINT32    UpCmdHdata0     : 8;
    UINT32    UpCmdHdata1     : 8;
    UINT32    UpCmdHdata2     : 8;
  } Field;
  UINT32    Value;
} ESPIx10_UP_RXHDR0;

#define ESPI_UP_RXHDR_1  0x14
typedef union {
  struct {
    UINT32    UpCmdHdata3 : 8;
    UINT32    UpCmdHdata4 : 8;
    UINT32    UpCmdHdata5 : 8;
    UINT32    UpCmdHdata6 : 8;
  } Field;
  UINT32    Value;
} ESPIx14_UP_RXHDR1;

#define ESPI_UP_RXDATA_PORT  0x18

/// eSPIx2C eSPI Master Capability
#define ESPI_MASTER_CAP  0x2C
typedef union {
  struct {
    UINT32    FlashAccessChannelSupport    : 1;
    UINT32    OOBMessageChannelSupport     : 1;
    UINT32    VWChannelSupport             : 1;
    UINT32    PChannelSupport              : 1;
    UINT32    MasterVersion                : 3;
    UINT32    FlashAccessChannelMaxPayload : 3;
    UINT32    OOBMessageChannelMaxPayload  : 3;
    UINT32    OperatingMaxVWCount          : 6;
    UINT32    PChannelMaxPayloadSize       : 3;
    UINT32    NumberOfSlave                : 3;
    UINT32    OperatingSupportFreq         : 3;
    UINT32    IOMode                       : 2;
    UINT32    AlertMode                    : 1;
    UINT32    CRCCheck                     : 1;
  } Field;
  UINT32    Value;
} ESPIx2C_MASTER_CAP;

/// eSPIx30 eSPI Global Control 0
#define ESPI_GLOBAL_CTRL0  0x30
typedef union {
  struct {
    UINT32    WdgEn            : 1;
    UINT32    WaitChkEn        : 1;
    UINT32    PrClkgatEn       : 1;
    UINT32    AlStopEn         : 1;
    UINT32    AlIdleTimer      : 3;
    UINT32    RgDbgclkGatingEn : 1;
    UINT32    WdgCnt           : 16;
    UINT32    WaitCnt          : 6;
    UINT32    PrRstEnPltrst    : 1;
    UINT32    SafsClkGateEn    : 1;
  } Field;
  UINT32    Value;
} ESPIx30_GLOBAL_CTRL0;

/// eSPIx68 eSPI Slave0 Configuration
#define ESPI_SLAVE0_CONFIG  0x68
typedef union {
  struct {
    UINT32    FlashAccessChannelEnable : 1;
    UINT32    OOBMessageChannelEnable  : 1;
    UINT32    VWChannelEnable          : 1;
    UINT32    PChannelEnable           : 1;
    UINT32    FlashSharingMode         : 1;
    UINT32    FlashMaxPayloadSize      : 3;
    UINT32    PutFlashNpHeaderDataEn   : 1;
    UINT32    PutFlashNpHeaderEn       : 1;
    UINT32    SafsDeferValidEn         : 1;
    UINT32    FlashModifierEn          : 1;
    UINT32    Reserved_24_12           : 13;
    UINT32    OperatingFreq            : 3;
    UINT32    IOModeSelect             : 2;
    UINT32    AlertMode                : 1;
    UINT32    CRCCheckingEnable        : 1;
  } Field;
  UINT32    Value;
} ESPIx68_SLAVE0_CONFIG;

/// eSPIx70 eSPI Slave0 Interrupt Status
#define ESPI_SLAVE0_INT_STS  0x70
typedef union {
  struct {
    UINT32    BusErrInt          : 1;
    UINT32    WaitTimeoutInt     : 1;
    UINT32    CrcErrInt          : 1;
    UINT32    Reserved_3         : 1;
    UINT32    NoRspInt           : 1;
    UINT32    FatalErrInt        : 1;
    UINT32    NonFatalErrInt     : 1;
    UINT32    UnknownRspInt      : 1;
    UINT32    UnknownCtInt       : 1;
    UINT32    UnsucssCplInt      : 1;
    UINT32    IllegalTagInt      : 1;
    UINT32    IllegalLenInt      : 1;
    UINT32    RxOobOverflowInt   : 1;
    UINT32    RxMsgOverflowInt   : 1;
    UINT32    RxFlashOverflowInt : 1;
    UINT32    ProtocolErrInt     : 1;
    UINT32    Reserved_16        : 1;
    UINT32    UpFifoWdgTo        : 1;
    UINT32    MstAbortInt        : 1;
    UINT32    WdgTimeoutInt      : 1;
    UINT32    Reserved_23_20     : 4;
    UINT32    RxVwGrp0Int        : 1;
    UINT32    RxVwGrp1Int        : 1;
    UINT32    RxVwGrp2Int        : 1;
    UINT32    RxVwGrp3Int        : 1;
    UINT32    DnCmdInt           : 1;
    UINT32    RxMsgInt           : 1;
    UINT32    RxOobInt           : 1;
    UINT32    FlashReqInt        : 1;
  } Field;
  UINT32    Value;
} ESPIx70_SLAVE0_INT_STS;

///
/// Slave Registers
///
#define SLAVE_REG_ID          0x04
#define SLAVE_GENERAL_CAPCFG  0x08
#define SLAVE_PC_CAPCFG       0x10
#define SLAVE_VW_CAPCFG       0x20
#define SLAVE_OOB_CAPCFG      0x30
#define SLAVE_FA_CAPCFG       0x40
#define SLAVE_FA_CAPCFG2      0x44

/// Offset 04h: Device Identification
typedef union {
  struct {
    UINT32    RO_VersionID  : 8;
    UINT32    Reserved_31_8 : 24;
  } Field;
  UINT32    Value;
} ESPI_SL04_DEVICE_ID;

// SLAVE offset 0x08   SLAVE_GENERAL_CAPCFG
#define SLAVE_FA_SUPPORT          BIT3
#define SLAVE_OOB_SUPPORT         BIT2
#define SLAVE_VW_SUPPORT          BIT1
#define SLAVE_PERIPHERAL_SUPPORT  BIT0
/// Offset 08h: General Capabilities and Configurations
typedef union {
  struct {
    UINT32    RO_PCSupported             : 1;
    UINT32    RO_VWSupported             : 1;
    UINT32    RO_OOBMsgSupported         : 1;
    UINT32    RO_FASupported             : 1;
    UINT32    Reserved_7_3               : 4;
    UINT32    Reserved_11_8              : 4;
    UINT32    RO_MaxWaitStateAllowed     : 4;
    UINT32    RO_MaxFreqSupported        : 3;
    UINT32    RO_OpenDrainAlertSupported : 1;
    UINT32    OperatingFreq              : 3;
    UINT32    OpenDrainAlertSelect       : 1;
    UINT32    RO_IOModeSupported         : 2;
    UINT32    IOModeSelect               : 2;
    UINT32    AlertMode                  : 1;
    UINT32    Reserved_29                : 1;
    UINT32    ResponseModifierEn         : 1;
    UINT32    CRCCheckingEn              : 1;
  } Field;
  UINT32    Value;
} ESPI_SL08_SLAVE_GENERAL_CAPCFG;

/// Offset 10h: Channel 0 Capabilities and Configurations
typedef union {
  struct {
    UINT32    PCEn                         : 1;
    UINT32    RO_PCReady                   : 1;
    UINT32    BusMasterEn                  : 1;
    UINT32    Reserved_3                   : 1;
    UINT32    RO_PCMaxPayloadSizeSupported : 3;
    UINT32    Reserved_7                   : 1;
    UINT32    PCMaxPayloadSizeSelected     : 3;
    UINT32    Reserved_11                  : 1;
    UINT32    PCMaxReadRequestSize         : 3;
    UINT32    Reserved_31_15               : 17;
  } Field;
  UINT32    Value;
} ESPI_SL10_SLAVE_PC_CAPCFG;

/// Offset 20h: Channel 1 Capabilities and Configurations
typedef union {
  struct {
    UINT32    VWEn                 : 1;
    UINT32    RO_VWReady           : 1;
    UINT32    Reserved_7_2         : 6;
    UINT32    RO_MaxVWCntSupported : 6;
    UINT32    Reserved_15_14       : 2;
    UINT32    OpMaxVWCnt           : 6;
    UINT32    Reserved_31_22       : 10;
  } Field;
  UINT32    Value;
} ESPI_SL20_SLAVE_VW_CAPCFG;

/// Offset 30h: Channel 2 Capabilities and Configurations
typedef union {
  struct {
    UINT32    OOBEn                           : 1;
    UINT32    RO_OOBReady                     : 1;
    UINT32    Reserved_3_2                    : 2;
    UINT32    RO_MsgChMaxPayloadSizeSupported : 3;
    UINT32    Reserved_7                      : 1;
    UINT32    MsgChMaxPayloadSizeSelected     : 3;
    UINT32    Reserved_31_11                  : 21;
  } Field;
  UINT32    Value;
} ESPI_SL30_SLAVE_OOB_CAPCFG;

/// Offset 40h: Channel 3 Capabilities and Configurations
typedef union {
  struct {
    UINT32    FAEn                               : 1;
    UINT32    RO_FAReady                         : 1;
    UINT32    FlashBlockEraseSize                : 3;
    UINT32    RO_ChMaxPayloadSizeSupported       : 3;
    UINT32    ChMaxPayloadSizeSelected           : 3;
    UINT32    RO_FlashSharingMode                : 1;
    UINT32    ChMaxReadReqSize                   : 3;
    UINT32    Reserved_15                        : 1;
    UINT32    RO_FlashSharingCapabilitySupported : 2;
    UINT32    Reserved_19_18                     : 2;
    UINT32    RO_RPMCCounterOn1stDevice          : 4;
    UINT32    RO_RPMCOp1On1stDevice              : 8;
  } Field;
  UINT32    Value;
} ESPI_SL40_SLAVE_FA_CAPCFG;

/// Offset 44h: Channel 3 Capabilities and Configurations2
typedef union {
  struct {
    UINT32    RO_TargetMaxReadReqSizeSupported : 3;
    UINT32    Reserved_7_3                     : 5;
    UINT32    RO_TargetFlashEraseBlockSize     : 8;
    UINT32    RO_TargetRPMCSupported           : 6;
    UINT32    RO_NumOfRPMCdevices              : 2;
    UINT32    Reserved_31_24                   : 8;
  } Field;
  UINT32    Value;
} ESPI_SL44_SLAVE_FA_CAPCFG2;

//
// eSPI Command functions
//
VOID
FchEspiCmd_InBandRst  (
  IN  UINT32  EspiBase
  );

UINT32
FchEspiCmd_GetConfiguration  (
  IN  UINT32  EspiBase,
  IN  UINT32  RegAddr
  );

VOID
FchEspiCmd_SetConfiguration  (
  IN  UINT32  EspiBase,
  IN  UINT32  RegAddr,
  IN  UINT32  Value
  );

EFI_STATUS
FchEspiCmd_SafsFlashRead  (
  IN  UINT32  EspiBase,
  IN  UINT32  Address,
  IN  UINT32  Length,
  OUT UINT8   *Buffer
  );

EFI_STATUS
FchEspiCmd_SafsFlashWrite  (
  IN  UINT32  EspiBase,
  IN  UINT32  Address,
  IN  UINT32  Length,
  IN  UINT8   *Value
  );

EFI_STATUS
FchEspiCmd_SafsFlashErase  (
  IN  UINT32  EspiBase,
  IN  UINT32  Address,
  IN  UINT32  Length
  );

EFI_STATUS
FchEspiCmd_SafsRpmcOp1  (
  IN  UINT32  EspiBase,
  IN  UINT8   RpmcFlashDev,
  IN  UINT32  Length,
  IN  UINT8   *Data
  );

EFI_STATUS
FchEspiCmd_SafsRpmcOp2  (
  IN  UINT32  EspiBase,
  IN  UINT8   RpmcFlashDev,
  IN  UINT32  Length,
  OUT UINT8   *Buffer
  );

#endif // FCH_ESPI_CMD_LIB_H_
