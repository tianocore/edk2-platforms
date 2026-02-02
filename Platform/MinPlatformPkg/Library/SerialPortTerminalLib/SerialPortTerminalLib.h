/** @file
  Header file for NULL named library for the Serial Port Terminal Redirection library.

  This library adds a Terminal Device connected to SerialDxe to the UEFI Console
  Variables. This allows BIOS Setup, UEFI Shell, etc. to be used on a headless
  system via a null modem and terminal
  emulator.

  Copyright (c) 2020 -2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SERIAL_PORT_TERMINAL_LIB_H_
#define _SERIAL_PORT_TERMINAL_LIB_H_

#include <Uefi.h>
#include <Guid/SerialPortLibVendor.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>

//
// Below is the platform console device path
//
typedef struct {
  VENDOR_DEVICE_PATH        Guid;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} SERIAL_DEVICE_PATH;

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { END_DEVICE_PATH_LENGTH, 0 } \
  }

#endif
