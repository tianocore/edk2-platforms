/** @file
  Main file for NULL named library for the Serial Port Terminal Redirection library.

  This library adds a Terminal Device connected to SerialDxe to the UEFI Console
  Variables. This allows BIOS Setup, UEFI Shell, etc. to be used on a headless
  system via a null modem and terminal
  emulator.

  Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SerialPortTerminalLib.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID  *mTerminalType[] = {
  &gEfiPcAnsiGuid,
  &gEfiVT100Guid,
  &gEfiVT100PlusGuid,
  &gEfiVTUTF8Guid,
  &gEfiTtyTermGuid
};

GLOBAL_REMOVE_IF_UNREFERENCED SERIAL_DEVICE_PATH mSerialDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) sizeof (VENDOR_DEVICE_PATH),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8) sizeof (UART_DEVICE_PATH),
        (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,                  // Reserved
    0,                  // BaudRate
    0,                  // DataBits
    0,                  // Parity
    0                   // StopBits
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
      }
    },
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

/**
  Updates the ConOut, ConIn, ErrOut variables with the serial terminal device path
  @param                        none
  @retval                       none
**/
VOID
AddSerialTerminal (
  VOID
  )
{
  UINT8   DefaultTerminalType;

  //
  // Update the Terminal Device Configuration Parameters
  //
  mSerialDevicePath.Uart.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  mSerialDevicePath.Uart.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  mSerialDevicePath.Uart.Parity   = PcdGet8 (PcdUartDefaultParity);
  mSerialDevicePath.Uart.StopBits = PcdGet8 (PcdUartDefaultStopBits);
  DefaultTerminalType             = PcdGet8 (PcdDefaultTerminalType);
  DEBUG ((DEBUG_INFO, "[AddSerialPortTerminal] [%d, %d, %d, %d, %d]\n",
      (int) mSerialDevicePath.Uart.BaudRate,
      (int) mSerialDevicePath.Uart.DataBits,
      (int) mSerialDevicePath.Uart.Parity,
      (int) mSerialDevicePath.Uart.StopBits,
      (int) DefaultTerminalType));

  if (DefaultTerminalType < (sizeof (mTerminalType) / sizeof (mTerminalType[0]))) {
    CopyMem (
      (VOID *) &(mSerialDevicePath.TerminalType.Guid),
      (VOID *) mTerminalType[DefaultTerminalType],
      sizeof (EFI_GUID)
      );
  } else {
    DEBUG ((DEBUG_WARN, "PcdDefaultTerminalType has invalid value: %d\n", (int) DefaultTerminalType));
  }

  //
  // Append Serial Terminal into "ConIn", "ConOut", and "ErrOut"
  //
  EfiBootManagerUpdateConsoleVariable (ConOut, (EFI_DEVICE_PATH_PROTOCOL *) &mSerialDevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, (EFI_DEVICE_PATH_PROTOCOL *) &mSerialDevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, (EFI_DEVICE_PATH_PROTOCOL *) &mSerialDevicePath, NULL);
}


/**
  Constructor for the Serial Port Terminal Device library.

  @param ImageHandle    The Image Handle of the process
  @param SystemTable    The EFI System Table pointer

  @retval EFI_SUCCESS   The Serial Port Terminal Device was installed successfully
**/
EFI_STATUS
EFIAPI
SerialPortTerminalLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEBUG ((DEBUG_INFO, "[SerialPortTerminalLibConstructor]\n"));

  AddSerialTerminal();

  return EFI_SUCCESS;
}
