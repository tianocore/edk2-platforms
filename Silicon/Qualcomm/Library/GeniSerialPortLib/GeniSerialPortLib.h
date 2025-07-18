/** @file
  GENI Serial I/O Port library functions

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GENI_SERIAL_PORT_LIB_H_
#define GENI_SERIAL_PORT_LIB_H_

#include <Library/PcdLib.h>

#define GENI_REG(OFFSET)             (FixedPcdGet64 (PcdSerialRegisterBase) + OFFSET)

#define GENI_FORCE_DEFAULT_REG       GENI_REG(0x20)
#define GENI_STATUS_REG              GENI_REG(0x40)
#define GENI_TX_TRANS_LEN_REG        GENI_REG(0x270)
#define GENI_RX_PACKING_CFG0_REG     GENI_REG(0x284)
#define GENI_RX_PACKING_CFG1_REG     GENI_REG(0x288)
#define GENI_M_CMD0_REG              GENI_REG(0x600)
#define GENI_M_IRQ_STATUS_REG        GENI_REG(0x610)
#define GENI_M_IRQ_EN_REG            GENI_REG(0x614)
#define GENI_M_IRQ_CLEAR_REG         GENI_REG(0x618)
#define GENI_S_CMD0_REG              GENI_REG(0x630)
#define GENI_S_CMD_CTRL_REG          GENI_REG(0x634)
#define GENI_S_IRQ_STATUS_REG        GENI_REG(0x640)
#define GENI_S_IRQ_EN_REG            GENI_REG(0x644)
#define GENI_S_IRQ_CLEAR_REG         GENI_REG(0x648)
#define GENI_TX_FIFO_REG             GENI_REG(0x700)
#define GENI_RX_FIFO_REG             GENI_REG(0x780)
#define GENI_RX_FIFO_STATUS_REG      GENI_REG(0x804)
#define GENI_TX_WATERMARK_REG        GENI_REG(0x80c)

#define GENI_FORCE_DEFAULT           BIT0
#define GENI_STATUS_REG_CMD_ACTIVE   BIT0
#define GENI_M_CMD_DONE_EN           BIT0
#define GENI_S_CMD_DONE_EN           BIT0
#define GENI_S_CMD_ABORT             BIT1
#define GENI_S_CMD_ABORT_EN          BIT5
#define GENI_M_RX_FIFO_WATERMARK_EN  BIT26
#define GENI_M_RX_FIFO_LAST_EN       BIT27
#define GENI_M_TX_FIFO_WATERMARK_EN  BIT30
#define GENI_M_SEC_IRQ_EN            BIT31
#define GENI_S_RX_FIFO_WATERMARK_EN  BIT26
#define GENI_S_RX_FIFO_LAST_EN       BIT27
#define GENI_DEF_TX_WM               2
#define GENI_M_CMD_TX                0x08000000
#define GENI_S_CMD_RX                0x08000000
#define GENI_RX_FIFO_WC_MASK         0x01ffffff
#define GENI_UART_PACKING_CFG0       0xf
#define GENI_UART_PACKING_CFG1       0x0

#endif /* GENI_SERIAL_PORT_LIB_H_ */
