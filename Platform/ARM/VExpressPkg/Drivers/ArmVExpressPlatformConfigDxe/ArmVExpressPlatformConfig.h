/** @file
*
*  Copyright (c) 2025, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_VEXPRESS_PLATFORM_CONFIG_H_
#define ARM_VEXPRESS_PLATFORM_CONFIG_H_

#include <Uefi.h>

#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>

#include <Guid/MdeModuleHii.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

#include <PlatformConfigStructs.h>

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  PlatformConfigVfrBin[];

#define ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_SIGNATURE  SIGNATURE_32 ('V', 'P', 'p', 's')

typedef struct {
  UINTN                              Signature;

  EFI_HANDLE                         DriverHandle;
  EFI_HII_HANDLE                     HiiHandle;

  //
  // Consumed protocol
  //
  EFI_HII_CONFIG_ROUTING_PROTOCOL    *HiiConfigRouting;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL     ConfigAccess;

  PLATFORM_CONFIG_DATA               PlatformConfig;
} ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_DATA;

#define ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_DATA, ConfigAccess, ARM_VEXPRESS_PLATFORM_CONFIG_PRIVATE_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

#endif // ARM_VEXPRESS_PLATFORM_CONFIG_H_
