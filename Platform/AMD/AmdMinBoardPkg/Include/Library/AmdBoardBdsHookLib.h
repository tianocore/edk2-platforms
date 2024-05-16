/** @file
  Header file for BDS Hook Library

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_BOARD_BDS_HOOK_LIB_H_
#define AMD_BOARD_BDS_HOOK_LIB_H_

#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/SortLib.h>

#define AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL_GUID    \
  { 0x5806db97, 0x5303, 0x409f,                         \
    { 0x8f, 0x09, 0xab, 0x29, 0xd8, 0x07, 0xa3, 0xf1 }}

/*
  This protocol is introduced so the platform can give certain boot options
  a custom priority value. Useful in boot overrides, or when IPMI doesn't inherently
  support a specific boot override needed by the platform.
*/
struct _AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL {
  UINT8           IpmiBootDeviceSelectorType;
  SORT_COMPARE    Compare;
};

typedef struct _AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL;

extern EFI_GUID  gAmdBoardBdsBootOptionPriorityProtocolGuid;

/**
  Returns the boot option type of a device

  @param[in] DevicePath         The path of device whose boot option type
                                should be returned
  @retval -1                    Device type not found
  @retval > -1                  Device type found
**/
UINT8
BootOptionType (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  This is the callback function for Bds Ready To Boot event.

  @param[in]  Event   Pointer to this event
  @param[in]  Context Event handler private data

  @retval None.
**/
VOID
EFIAPI
BdsReadyToBootCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  This is the callback function for Smm Ready To Lock event.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsSmmReadyToLockCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This is the callback function for PCI ENUMERATION COMPLETE.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsPciEnumCompleteCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Before console after trusted console event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsBeforeConsoleAfterTrustedConsoleCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Before console before end of DXE event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsBeforeConsoleBeforeEndOfDxeGuidCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  After console ready before boot option event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsAfterConsoleReadyBeforeBootOptionCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

#endif
