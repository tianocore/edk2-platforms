/** @file

  Cadence xSPI controller register definitions.

  Copyright (c) 2025-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/IoLib.h>

typedef union {
  struct {
    UINT32    SdmaBusy      : 1;
    UINT32    MdmaBusy      : 1;
    UINT32    AcmdEngBusy   : 1;
    UINT32    GcmdEngBusy   : 1;
    UINT32    GcmdEngMcBusy : 1;
    UINT32    Rsvd          : 1;
    UINT32    DiscoveryBusy : 1;
    UINT32    CtrlBusy      : 1;
    UINT32    InitFail      : 2;
    UINT32    Rsvd2         : 6;
    UINT32    InitComplete  : 1;
    UINT32    Rsvd3         : 15;
  } Bits;
  UINT32    Uint32;
} CTRL_STATUS;

typedef union {
  struct {
    UINT32    Rsvd        : 3;
    UINT32    ContOnError : 1;
    UINT32    Rsvd2       : 1;
    UINT32    WorkMode    : 2;
    UINT32    Rsvd3       : 26;
  } Bits;
  UINT32    Uint32;
} CTRL_CONFIG;

typedef enum {
  XspiModeDirect = 0x00,
  XspiModeAuto   = 0x03,
  XspiModeStig   = 0x01,
} XSPI_MODE;

typedef enum {
  StigInstType0,
  StigInstType1,
  StigInstTypeData = 127,
} STIG_INST_TYPE;

//
// Instruction phase type of a STIG command.
//
typedef enum {
  CadenceXspiOpRead,
  CadenceXspiOpWrite,
  CadenceXspiOpErase,
  CadenceXspiOpRegRead,
  CadenceXspiOpRegWrite,
  CadenceXspiOpNoData
} CADENCE_XSPI_OP;

#define SPI_DIRECT_ACC_BASE   0x10000000
#define SPI_DIRECT_ACC_SIZE   (SIZE_64KB * 8)
#define CMD_REG0_OFFSET       0x00
#define CMD_REG1_OFFSET       0x04
#define CMD_REG2_OFFSET       0x08
#define CMD_REG3_OFFSET       0x0C
#define CMD_REG4_OFFSET       0x10
#define CMD_REG5_OFFSET       0x14
#define CMD_STAT_OFFSET       0x44
#define CTRL_STAT_OFFSET      0x100
#define CTRL_CMD_INTR_STATUS  0x110
#define CTRL_CFG_OFFSET       0x230
#define CTRL_CFG_SDMA_SIZE    0x240
#define SPI_CLK_CTRL_OFFSET   0x4020

#define SDMA_TRIGGER_MASK   (1 << 21)
#define STIG_COMPLETE_MASK  (1 << 15)
#define STIG_FAIL_MASK      (1 << 14)

#define XSPI_READ32(Offset)          MmioRead32 ((Offset))
#define XSPI_WRITE32(Offset, Value)  MmioWrite32 ((Offset), (Value))
#define XSPI_READ64(Offset)          MmioRead64 ((Offset))
#define XSPI_WRITE64(Offset, Value)  MmioWrite64 ((Offset), (Value))

#define STIG_DONE_FLAG          1
#define XFER_QWORD_BYTECOUNT    8
#define SPI_OP_IDLE_TIMEOUT_MS  100
