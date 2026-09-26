/** @file
  Header file for BDS Hook Library

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024 - 2025, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/AmdBootOptionPriorityProtocol.h>

#include <Guid/GlobalVariable.h>
#include <Guid/MemoryOverwriteControl.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/PerformanceLib.h>

#include <IndustryStandard/Pci30.h>
#include <Protocol/PciEnumerationComplete.h>

///
/// For boot order override.
///
#define IS_FIRST_BOOT_VAR_NAME  L"IsFirstBoot"
#define UEFI_HARD_DRIVE_NAME    L"UEFI Hard Drive"
#define BOOT_DEVICE_LIST_STR    L"Boot Device List"
#define ENTER_SETUP_STR         L"Enter Setup"

///
/// CERT-C scan flags all the macros starting with 'E' as DCL37-C issues, even
/// though these macros are not defined in errno.h and other C Standard library files.
/// These issues are false positives.
///
// coverity[cert_dcl37_c_violation]
#define END_ENTIRE_DEVICE_PATH \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { END_DEVICE_PATH_LENGTH, 0 } \
  }

extern EFI_GUID       gUefiShellFileGuid;
extern EFI_BOOT_MODE  gBootMode;

//
// Below is the boot option device path
//

#define CLASS_HID          3
#define SUBCLASS_BOOT      1
#define PROTOCOL_KEYBOARD  1

typedef struct {
  USB_CLASS_DEVICE_PATH       UsbClass;
  EFI_DEVICE_PATH_PROTOCOL    End;
} USB_CLASS_FORMAT_DEVICE_PATH;

extern USB_CLASS_FORMAT_DEVICE_PATH  gUsbClassKeyboardDevicePath;

//
// Platform BDS Functions
//

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param[in]  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
MemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL  Level
  );

/**
  Connect with predeined platform connect sequence,
  the OEM/IBV can customize with their own connect sequence.

  @param[in] BootMode          Boot mode of this boot.
**/
VOID
ConnectSequence (
  IN EFI_BOOT_MODE  BootMode
  );

/**
   Compares boot priorities of two boot options

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @return           The difference between the Left and Right
                    boot options
 **/
INTN
EFIAPI
CompareBootOption (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  );

/**
   Compares boot priorities of two boot options, while giving PXE the highest priority

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @return           The difference between the Left and Right
                    boot options
**/
INTN
EFIAPI
CompareBootOptionPxePriority (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  );

/**
   Compares boot priorities of two boot options, while giving HDD the highest priority

  @param[in] Left       The left boot option
  @param[in] Right      The right boot option

  @return           The difference between the Left and Right
                    boot options
**/
INTN
EFIAPI
CompareBootOptionHddPriority (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  );

/**
  This function is called after all the boot options are enumerated and ordered properly.
**/
VOID
RegisterStaticHotkey (
  VOID
  );

/**
  Registers/Unregisters boot option hotkey
**/
VOID
RegisterDefaultBootOption (
  VOID
  );

/**
  Add console variable device paths

  @param[in] ConsoleType         ConIn or ConOut
  @param[in] ConsoleDevicePath   Device path to be added
**/
VOID
AddConsoleVariable (
  IN CONSOLE_TYPE     ConsoleType,
  IN EFI_DEVICE_PATH  *ConsoleDevicePath
  );

/**
  Returns the boot option type of a device.

  @param[in] DevicePath         The path of device whose boot option type
                                should be returned.
  @retval MAX_UINT8             Device type not found.
  @retval < MAX_UINT8           Device type found.
**/
UINT8
EFIAPI
BootOptionType (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Handles possible IPMI boot overrides by modifying the current list of boot options.
  Uses sorting function installed in BootOptionPriorityProtocol if the protocol is installed
  and a valid IPMI override is detected.

  @retval  EFI_SUCCESS              Successfully updated the current boot order, or not necessary.
  @retval  EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval  Other errors             Failed to apply the override to the current boot order.
**/
EFI_STATUS
HandleIpmiBootOverride (
  VOID
  );
