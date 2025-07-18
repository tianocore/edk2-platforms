/** @file
  GENI Serial I/O Port library functions

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>

#include "GeniSerialPortLib.h"

STATIC VOID
GeniSerialPollBit (
  IN UINTN      Reg,
  IN UINT32     Bit,
  IN BOOLEAN    Set
  )
{
  UINT32  TimeOutUs = 10000;

  do {
    if ((MmioRead32(Reg) & Bit) == Set) {
      break;
    }
    MicroSecondDelay(10);
    TimeOutUs -= 10;
  } while (TimeOutUs > 0);
}

/**
  Initialize GENI serial port.

  @return    Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  MmioWrite32(GENI_TX_WATERMARK_REG, GENI_DEF_TX_WM);
  MmioWrite32(GENI_M_IRQ_EN_REG, MmioRead32(GENI_M_IRQ_EN_REG) |
              GENI_M_CMD_DONE_EN | GENI_M_TX_FIFO_WATERMARK_EN |
              GENI_M_RX_FIFO_WATERMARK_EN | GENI_M_RX_FIFO_LAST_EN);

  MmioWrite32(GENI_S_CMD_CTRL_REG, GENI_S_CMD_ABORT);
  GeniSerialPollBit(GENI_S_CMD_CTRL_REG, GENI_S_CMD_ABORT, FALSE);
  MmioWrite32(GENI_S_IRQ_CLEAR_REG, GENI_S_CMD_DONE_EN | GENI_S_CMD_ABORT_EN);

  MmioWrite32(GENI_FORCE_DEFAULT_REG, GENI_FORCE_DEFAULT);
  MmioWrite32(GENI_RX_PACKING_CFG0_REG, GENI_UART_PACKING_CFG0);
  MmioWrite32(GENI_RX_PACKING_CFG1_REG, GENI_UART_PACKING_CFG1);
  MmioWrite32(GENI_S_CMD0_REG, GENI_S_CMD_RX);
  MmioWrite32(GENI_S_IRQ_EN_REG, MmioRead32(GENI_S_IRQ_EN_REG) |
              GENI_S_RX_FIFO_WATERMARK_EN | GENI_S_RX_FIFO_LAST_EN);

  return EFI_SUCCESS;
}

/**
  Write data to GENI serial port.

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
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while (MmioRead32(GENI_STATUS_REG) & GENI_STATUS_REG_CMD_ACTIVE);
    MmioWrite32(GENI_TX_TRANS_LEN_REG, 1);
    MmioWrite32(GENI_M_CMD0_REG, GENI_M_CMD_TX);
    MmioWrite32(GENI_TX_FIFO_REG, *Buffer);
  }

  return NumberOfBytes;
}


/**
  Read data from GENI serial port.

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
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++) {
    MmioWrite32(GENI_S_CMD0_REG, GENI_S_CMD_RX);

    GeniSerialPollBit(GENI_M_IRQ_STATUS_REG, GENI_M_SEC_IRQ_EN, TRUE);
    MmioWrite32(GENI_M_IRQ_CLEAR_REG, MmioRead32(GENI_M_IRQ_STATUS_REG));
    MmioWrite32(GENI_S_IRQ_CLEAR_REG, MmioRead32(GENI_S_IRQ_STATUS_REG));

    GeniSerialPollBit(GENI_RX_FIFO_STATUS_REG, GENI_RX_FIFO_WC_MASK, TRUE);
    if ((MmioRead32(GENI_RX_FIFO_STATUS_REG) & GENI_RX_FIFO_WC_MASK) == 0) {
	    return Count;
    }

    Buffer[Count] = (UINT8)MmioRead32(GENI_RX_FIFO_REG);
  }

  return NumberOfBytes;
}


/**
  Check to see if any data is avaiable on GENI serial port.

  @retval bool

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  if (MmioRead32(GENI_RX_FIFO_STATUS_REG) & GENI_RX_FIFO_WC_MASK) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Sets the control bits on the GENI serial port.

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
  Retrieve the status of the control bits on the GENI serial port.

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
