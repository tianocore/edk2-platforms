/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_CONFIG_H_
#define CPU_CONFIG_H_

#include <CpuConfigNVDataStruc.h>

//
// This is the generated IFR binary data for each formset defined in VFR.
//
extern UINT8  CpuConfigVfrBin[];

//
// This is the generated String package data for all .UNI files.
//
extern UINT8  CpuConfigDxeStrings[];

#define CPU_CONFIG_PRIVATE_SIGNATURE  SIGNATURE_32 ('C', 'P', 'U', '_')

typedef struct {
  UINTN                              Signature;

  EFI_HANDLE                         DriverHandle;
  EFI_HII_HANDLE                     HiiHandle;
  CPU_VARSTORE_DATA                  Configuration;

  EFI_HII_CONFIG_ROUTING_PROTOCOL    *HiiConfigRouting;

  EFI_HII_CONFIG_ACCESS_PROTOCOL     ConfigAccess;
} CPU_CONFIG_PRIVATE_DATA;

#define CPU_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, CPU_CONFIG_PRIVATE_DATA, ConfigAccess, CPU_CONFIG_PRIVATE_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

#endif /* CPU_CONFIG_H_ */
