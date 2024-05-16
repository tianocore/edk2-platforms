/** @file
  Header file for BDS Hook Library

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BOARD_BDS_HOOK_H_
#define BOARD_BDS_HOOK_H_

#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/LoadFile.h>
#include <Protocol/PciIo.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/GlobalVariable.h>
#include <Guid/MemoryOverwriteControl.h>
#include <Guid/FileInfo.h>
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
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/CapsuleLib.h>
#include <Library/PerformanceLib.h>

#include <IndustryStandard/Pci30.h>
#include <IndustryStandard/PciCodeId.h>
#include <Protocol/PciEnumerationComplete.h>

///
/// For boot order override.
///
#define IPMI_BOOT_OVERRIDE_VAR_NAME  L"IpmiBootOverride"
#define IS_FIRST_BOOT_VAR_NAME       L"IsFirstBoot"
#define UEFI_HARD_DRIVE_NAME         L"UEFI Hard Drive"

///
/// ConnectType
///
#define CONSOLE_OUT  0x00000001
#define STD_ERROR    0x00000002
#define CONSOLE_IN   0x00000004
#define CONSOLE_ALL  (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)
#define END_ENTIRE_DEVICE_PATH \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { END_DEVICE_PATH_LENGTH, 0 } \
  }

extern EFI_GUID       gUefiShellFileGuid;
extern EFI_BOOT_MODE  gBootMode;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  UINTN                       ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

//
// Platform Root Bridge
//
typedef struct {
  ACPI_HID_DEVICE_PATH        PciRootBridge;
  EFI_DEVICE_PATH_PROTOCOL    End;
} PLATFORM_ROOT_BRIDGE_DEVICE_PATH;

//
// Below is the platform console device path
//
typedef struct {
  ACPI_HID_DEVICE_PATH        PciRootBridge;
  PCI_DEVICE_PATH             IsaBridge;
  ACPI_HID_DEVICE_PATH        Keyboard;
  EFI_DEVICE_PATH_PROTOCOL    End;
} PLATFORM_KEYBOARD_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH        PciRootBridge;
  PCI_DEVICE_PATH             PciDevice;
  EFI_DEVICE_PATH_PROTOCOL    End;
} PLATFORM_ONBOARD_CONTROLLER_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH        PciRootBridge;
  PCI_DEVICE_PATH             Pci0Device;
  EFI_DEVICE_PATH_PROTOCOL    End;
} PLATFORM_PEG_ROOT_CONTROLLER_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH        PciRootBridge;
  PCI_DEVICE_PATH             PciBridge;
  PCI_DEVICE_PATH             PciDevice;
  EFI_DEVICE_PATH_PROTOCOL    End;
} PLATFORM_PCI_CONTROLLER_DEVICE_PATH;

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

#endif //BOARD_BDS_HOOK_H_
