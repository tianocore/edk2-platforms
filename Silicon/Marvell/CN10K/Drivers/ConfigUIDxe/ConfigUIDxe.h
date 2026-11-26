/** @file

  Header file for the CN10K configuration UI driver.

  Copyright (c) 2022-2026 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Library/DevicePathLib.h>
#include "NvDataStruct.h"

#define DISABLE_WATCHDOG_VALUE  1
#define ENABLE_WATCHDOG_VALUE   0
#define CONFIG_VAR_STR          L"CN10KConfigVar"

#define CN10K_CINFIG_PRIVATE_SIGNATURE  SIGNATURE_32 ('C', 'N', '1', '0')

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  ConfigUIBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  ConfigUIStrings[];

//
// Driver private data, including the current configuration and the HII
// handles and protocols used by the configuration callbacks.
//
typedef struct {
  // CN10K configuration private data signature.
  UINTN                              Signature;

  // Handle on which the HII protocols are installed.
  EFI_HANDLE                         DriverHandle;
  // Handle of the published HII package list.
  EFI_HII_HANDLE                     HiiHandle;
  // Current configuration values.
  CN10K_CONFIG_NV_DATA               Configuration;

  //
  // Consumed protocol
  //
  EFI_HII_CONFIG_ROUTING_PROTOCOL    *HiiConfigRouting;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL     ConfigAccess;
} CN10K_CONFIG_PRIVATE_DATA;

#define CN10K_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, CN10K_CONFIG_PRIVATE_DATA, ConfigAccess, CN10K_CINFIG_PRIVATE_SIGNATURE)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;
