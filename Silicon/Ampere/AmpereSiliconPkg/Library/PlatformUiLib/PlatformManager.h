/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_MANAGER_H_
#define PLATFORM_MANAGER_H_

#include <Uefi.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformManagerHii.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>

#include "PlatformManagerVfr.h"

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  PlatformManagerVfrBin[];
extern UINT8  PlatformManagerUiLibStrings[];

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  ///
  /// Platform Manager HII relative handles
  ///
  EFI_HII_HANDLE    HiiHandle;
  EFI_HANDLE        DriverHandle;
} PLATFORM_MANAGER_CALLBACK_DATA;

#endif
