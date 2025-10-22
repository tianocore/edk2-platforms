/** @file
  Cadence I2C controller register map

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MORELLO_CADENCEI2CDXE_REGISTERMAP_H_
#define MORELLO_CADENCEI2CDXE_REGISTERMAP_H_

/// Control Register (CR)
#define REG_CR  0x0u
/// Control Register (CR) Read/Write (RW)
#define CR_RW  BIT0
/// Control Register (CR) Master/Slave (MS)
#define CR_MS  BIT1
/// Control Register (CR) Normal/Extended Address (NEA)
#define CR_NEA  BIT2
/// Control Register (CR) Acknowledge Enable (ACLEN)
#define CR_ACKEN  BIT3
/// Control Register (CR) Hold mode (HOLD)
#define CR_HOLD  BIT4
/// Control Register (CR) Slave Monitor mode (SLVMON)
#define CR_SLVMON  BIT5
/// Control Register (CR) Clear FIFO (CLRFIFO)
#define CR_CLRFIFO  BIT6
/// Control Register (CR) Divisor A (DIV_A)
#define CR_DIV_A        ( BIT8 \
                        | BIT9  \
                        | BIT10 \
                        | BIT11 \
                        | BIT12 \
                        | BIT13 )
#define CR_DIV_A_SHIFT  8u

/// Control Register (CR) Divisor B (DIV_B)
#define CR_DIV_B        ( BIT14 \
                        | BIT15 )
#define CR_DIV_B_SHIFT  14u

/// Status Register (SR)
#define REG_SR  0x4u
/// Status Register (SR) RX Read/Write flag (RXRW)
#define SR_RXRW  BIT3
/// Status Register (SR) Receiver Data Valid (RXDV)
#define SR_RXDV  BIT5
/// Status Register (SR) Transmitter Data Valid (TXDX)
#define SR_TXDX  BIT6
/// Status Register (SR) Receiver Overflow (RXOVF)
#define SR_RXOVF  BIT7
/// Status Register (SR) Bus Active (BA)
#define SR_BA  BIT8

/// Address Register
#define REG_AR  0x8u
/// Address Register (AR) I2C Address (ADD)
#define AR_ADD  ( BIT0 \
                | BIT1 \
                | BIT2 \
                | BIT3 \
                | BIT4 \
                | BIT5 \
                | BIT6 \
                | BIT7 \
                | BIT8 \
                | BIT9 )

/// Data Register (DR)
#define REG_DR  0xCu
/// Data Register (DR) I2C Data (DAT)
#define DR_DAT  ( BIT0 \
                | BIT1 \
                | BIT2 \
                | BIT3 \
                | BIT4 \
                | BIT5 \
                | BIT6 \
                | BIT7 )

/// Interrupt Status Register (ISR)
#define REG_ISR  0x10u
/// Interrupt Status Register (ISR) Transfer Complete (COMP)
#define ISR_COMP  BIT0
/// Interrupt Status Register (ISR) More Data (DATA)
#define ISR_DATA  BIT1
/// Interrupt Status Register (ISR) Transfer Not Acknowledged (NACK)
#define ISR_NACK  BIT2
/// Interrupt Status Register (ISR) Transfer Time Out (TO)
#define ISR_TO  BIT3
/// Interrupt Status Register (ISR) Monitored Slave Ready (SLV_RDY)
#define ISR_SLV_RDY  BIT4
/// Interrupt Status Register (ISR) Receive Overlow (RX_OVF)
#define ISR_RX_OVF  BIT5
/// Interrupt Status Register (ISR) FIFO Transmit Overflow (TX_OVF)
#define ISR_TX_OVF  BIT6
/// Interrupt Status Register (ISR) FIFO Receive Underflow (RX_UNF)
#define ISR_RX_UNF  BIT7
/// Interrupt Status Register (ISR) Arbitration Lost (ARB_LOST)
#define ISR_ARB_LOST  BIT9

/// Transfer Size Register (TSR)
#define REG_TSR  0x14u
/// Transfer Size Register (TSR) Transfer Size (TS)
#define TSR_TS  ( BIT0  \
                | BIT1  \
                | BIT2  \
                | BIT3  \
                | BIT4  \
                | BIT5  \
                | BIT6  \
                | BIT7  \
                | BIT8  \
                | BIT9  \
                | BIT10 \
                | BIT11 \
                | BIT12 \
                | BIT13 \
                | BIT14 \
                | BIT15 )

/// Time Out Register (TOR)
#define REG_TOR  0x1Cu
/// Time Out Register (TOR) Time Out (TO)
#define TOR_TO  ( BIT0 \
                | BIT1 \
                | BIT2 \
                | BIT3 \
                | BIT4 \
                | BIT5 \
                | BIT6 \
                | BIT7 )

/// Interrupt Mask Register (IMR)
#define REG_IMR  0x20u

/// Interrupt Enable Register (IER)
#define REG_IER  0x24u

/// Interrupt Disable Register (IDR)
#define REG_IDR  0x28u

/// max valid register offset
#define MAX_REGISTER_OFFSET  0x28u

#endif // MORELLO_CADENCEI2CDXE_REGISTERMAP_H_
