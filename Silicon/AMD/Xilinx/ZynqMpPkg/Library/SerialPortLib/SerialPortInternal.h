/**
 * @file
 *
 * Serial Port library header for ZynqMP (Cadence)
 *
 * Copyright (c) 2025, Linaro Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 */

#ifndef SERIAL_PORT_LIB_H
#define SERIAL_PORT_LIB_H

#define ZYNQ_UART_SR_TXACTIVE          BIT11                   /* TX active */
#define ZYNQ_UART_SR_TXFULL            BIT4                    /* TX FIFO full */
#define ZYNQ_UART_SR_TXEMPTY           BIT3                    /* TX FIFO empty */
#define ZYNQ_UART_SR_RXEMPTY           BIT1                    /* RX FIFO empty */
#define ZYNQ_UART_CR_TX_EN             BIT4                    /* TX enabled */
#define ZYNQ_UART_CR_RX_EN             BIT2                    /* RX enabled */
#define ZYNQ_UART_CR_TXRST             BIT1                    /* TX logic reset */
#define ZYNQ_UART_CR_RXRST             BIT0                    /* RX logic reset */
#define ZYNQ_UART_MR_STOPMODE_2_BIT    (0x00000080UL)          /* 2 stop bits */
#define ZYNQ_UART_MR_STOPMODE_1_5_BIT  (0x00000040UL)          /* 1.5 stop bits */
#define ZYNQ_UART_MR_STOPMODE_1_BIT    (0x00000000UL)          /* 1 stop bit */
#define ZYNQ_UART_MR_PARITY_NONE       (0x00000020UL)          /* No parity mode */
#define ZYNQ_UART_MR_PARITY_ODD        (0x00000008UL)          /* Odd parity mode */
#define ZYNQ_UART_MR_PARITY_EVEN       (0x00000000UL)          /* Even parity mode */
#define ZYNQ_UART_MR_CHARLEN_6_BIT     (0x00000006UL)          /* 6 bits data */
#define ZYNQ_UART_MR_CHARLEN_7_BIT     (0x00000004UL)          /* 7 bits data */
#define ZYNQ_UART_MR_CHARLEN_8_BIT     (0x00000000UL)          /* 8 bits data */

/*  Check here:
 *  https://docs.amd.com/r/en-US/ug1087-zynq-ultrascale-registers/UART-Module
 */
typedef struct {
  UINT32    Control;         /* 0x0 - Control Register [8:0] */
  UINT32    Mode;            /* 0x4 - Mode Register [10:0] */
  UINT32    Reserved1[4];
  UINT32    BaudRateGen;     /* 0x18 - Baud Rate Generator [15:0] */
  UINT32    Reserved2[4];
  UINT32    ChannelSts;      /* 0x2c - Channel Status [11:0] */
  UINT32    TxRxFIFO;        /* 0x30 - FIFO [7:0] */
  UINT32    BaudRateDivider; /* 0x34 - Baud Rate Divider [7:0] */
} ZYNQ_UART_PERIPHERAL;

#endif

