/** @file
  GENI Serial I/O Port library functions

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>

#define GENI_FORCE_DEFAULT_REG       0x20
#define GENI_STATUS_REG              0x40
#define GENI_TX_TRANS_LEN_REG        0x270
#define GENI_RX_PACKING_CFG0_REG     0x284
#define GENI_RX_PACKING_CFG1_REG     0x288
#define GENI_M_CMD0_REG              0x600
#define GENI_M_IRQ_STATUS_REG        0x610
#define GENI_M_IRQ_EN_REG            0x614
#define GENI_M_IRQ_CLEAR_REG         0x618
#define GENI_S_CMD0_REG              0x630
#define GENI_S_CMD_CTRL_REG          0x634
#define GENI_S_IRQ_STATUS_REG        0x640
#define GENI_S_IRQ_EN_REG            0x644
#define GENI_S_IRQ_CLEAR_REG         0x648
#define GENI_TX_FIFO_REG             0x700
#define GENI_RX_FIFO_REG             0x780
#define GENI_RX_FIFO_STATUS_REG      0x804
#define GENI_TX_WATERMARK_REG        0x80c

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

VOID
GeniSerialPollBit (
  IN UINT32     Offset,
  IN UINT32     Bit,
  IN BOOLEAN    Set
  )
{
  UINT32  TimeOutUs = 10000;
  UINT32  REG = (UINTN)PcdGet64 (PcdSerialRegisterBase) + Offset;

  do {
    if ((MmioRead32(REG) & Bit) == Set) {
      break;
    }
    MicroSecondDelay(10);
    TimeOutUs -= 10;
  } while (TimeOutUs > 0);
}

/*

  Programmed hardware of Serial port.

  @return    Always return RETURN_SUCCESS.

**/
EFI_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINT32  FDR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_FORCE_DEFAULT_REG;
  UINT32  SCMDR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_CMD0_REG;
  UINT32  SCTRL = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_CMD_CTRL_REG;
  UINT32  TWMR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_TX_WATERMARK_REG;
  UINT32  MIRQR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_M_IRQ_EN_REG;
  UINT32  SIRQR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_IRQ_EN_REG;
  UINT32  SIRCR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_IRQ_CLEAR_REG;
  UINT32  RXC0R = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_RX_PACKING_CFG0_REG;
  UINT32  RXC1R = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_RX_PACKING_CFG1_REG;

  MmioWrite32(TWMR, GENI_DEF_TX_WM);
  MmioWrite32(MIRQR, MmioRead32(MIRQR) | GENI_M_CMD_DONE_EN |
              GENI_M_TX_FIFO_WATERMARK_EN | GENI_M_RX_FIFO_WATERMARK_EN |
              GENI_M_RX_FIFO_LAST_EN);

  MmioWrite32(SCTRL, GENI_S_CMD_ABORT);
  GeniSerialPollBit(GENI_S_CMD_CTRL_REG, GENI_S_CMD_ABORT, FALSE);
  MmioWrite32(SIRCR, GENI_S_CMD_DONE_EN | GENI_S_CMD_ABORT_EN);

  MmioWrite32(FDR, GENI_FORCE_DEFAULT);
  MmioWrite32(RXC0R, GENI_UART_PACKING_CFG0);
  MmioWrite32(RXC1R, GENI_UART_PACKING_CFG1);
  MmioWrite32(SCMDR, GENI_S_CMD_RX);
  MmioWrite32(SIRQR, MmioRead32(SIRQR) | GENI_S_RX_FIFO_WATERMARK_EN |
              GENI_S_RX_FIFO_LAST_EN);

  return EFI_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINT32  MCMDR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_M_CMD0_REG;
  UINT32  STATR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_STATUS_REG;
  UINT32  TFIFOR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_TX_FIFO_REG;
  UINT32  LENR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_TX_TRANS_LEN_REG;
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while (MmioRead32(STATR) & GENI_STATUS_REG_CMD_ACTIVE);
    MmioWrite32(LENR, 1);
    MmioWrite32(MCMDR, GENI_M_CMD_TX);
    MmioWrite32(TFIFOR, *Buffer);
  }

  return NumberOfBytes;
}


/**
  Read data from serial device and save the datas in buffer.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  UINT32  SCMDR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_CMD0_REG;
  UINT32  RFIFO = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_RX_FIFO_REG;
  UINT32  RFIFOS = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_RX_FIFO_STATUS_REG;
  UINT32  MICR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_M_IRQ_CLEAR_REG;
  UINT32  MISR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_M_IRQ_STATUS_REG;
  UINT32  SICR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_IRQ_CLEAR_REG;
  UINT32  SISR = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_S_IRQ_STATUS_REG;
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    MmioWrite32(SCMDR, GENI_S_CMD_RX);

    GeniSerialPollBit(GENI_M_IRQ_STATUS_REG, GENI_M_SEC_IRQ_EN, TRUE);
    MmioWrite32(MICR, MmioRead32(MISR));
    MmioWrite32(SICR, MmioRead32(SISR));

    GeniSerialPollBit(GENI_RX_FIFO_STATUS_REG, GENI_RX_FIFO_WC_MASK, TRUE);
    if ((MmioRead32(RFIFOS) & GENI_RX_FIFO_WC_MASK) == 0)
	    return Count;

    *Buffer = (UINT8)MmioRead32(RFIFO);
  }

  return NumberOfBytes;
}


/**
  Check to see if any data is avaiable to be read from the debug device.

  @retval bool
**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  UINT32 RFIFOS = (UINTN)PcdGet64 (PcdSerialRegisterBase) + GENI_RX_FIFO_STATUS_REG;

  if (MmioRead32(RFIFOS) & GENI_RX_FIFO_WC_MASK) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Sets the control bits on a serial device.

  @param[in] Control         Sets the bits of Control that are settable.

  @retval EFI_SUCCESS        The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED    The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR   The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32 Control
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param[out] Control       A pointer to return the current control signals from the serial device.

  @retval EFI_SUCCESS       The control bits were read from the serial device.
  @retval EFI_UNSUPPORTED   The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR  The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32 *Control
  )
{
  *Control = 0;
  if (!SerialPortPoll ()) {
    *Control = EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }
  return EFI_SUCCESS;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receice time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
                            On output, the value actually set.
  @param ReveiveFifoDepth   The requested depth of the FIFO on the receive side of the
                            serial interface. A ReceiveFifoDepth value of 0 will use
                            the device's default FIFO depth.
                            On output, the value actually set.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
                            On output, the value actually set.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
                            On output, the value actually set.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            vaule of 0 will use the device's default data bit setting.
                            On output, the value actually set.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.
                            On output, the value actually set.

  @retval EFI_SUCCESS            The new attributes were set on the serial device.
  @retval EFI_UNSUPPORTED        The serial device does not support this operation.
  @retval EFI_INVALID_PARAMETER  One or more of the attributes has an unsupported value.
  @retval EFI_DEVICE_ERROR       The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64             *BaudRate,
  IN OUT UINT32             *ReceiveFifoDepth,
  IN OUT UINT32             *Timeout,
  IN OUT EFI_PARITY_TYPE    *Parity,
  IN OUT UINT8              *DataBits,
  IN OUT EFI_STOP_BITS_TYPE *StopBits
  )
{
  return EFI_UNSUPPORTED;
}

