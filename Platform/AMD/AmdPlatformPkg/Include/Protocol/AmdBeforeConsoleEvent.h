/** @file
  The header file for the event before console.

  Copyright (C) 2021 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_BEFORE_CONSOLE_EVENT_H_
#define AMD_BEFORE_CONSOLE_EVENT_H_

typedef struct AMD_BEFORE_CONSOLE_EVENT_PROTOCOL AMD_BEFORE_CONSOLE_EVENT_PROTOCOL;

/**
  The callback function that will be executed in PlatformBootManagerBeforeConsole

  This routine is called at TPL_APPLICATION.

  @retval EFI_SUCCESS       The callback function is executed successfully.
  @retval others            Error occurred.

**/
typedef EFI_STATUS
(EFIAPI *AMD_BEFORE_CONSOLE_EVENT_CALLBACK)(
  VOID
  );

///
/// Define the Before Console Event Protocol to provide a hook for other dxe drivers.
///
struct AMD_BEFORE_CONSOLE_EVENT_PROTOCOL {
  ///
  /// callback function
  ///
  AMD_BEFORE_CONSOLE_EVENT_CALLBACK    Callback;
  ///
  /// The priority of the callback comparing to others, 0 is highest, 0xff is lowest
  ///
  UINT8                                Order;
};

#endif // AMD_BEFORE_CONSOLE_EVENT_H_
