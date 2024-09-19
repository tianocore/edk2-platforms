/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_CONFIG_DXE_H_
#define ACPI_CONFIG_DXE_H_

//
// This is the generated IFR binary data for each formset defined in VFR.
//
extern UINT8  AcpiConfigVfrBin[];

//
// This is the generated String package data for all .UNI files.
//
extern UINT8  AcpiConfigDxeStrings[];

//
// Signature: Ampere Computing ACPI Configuration
//
#define ACPI_CONFIG_PRIVATE_SIGNATURE  SIGNATURE_32 ('A', 'C', 'A', 'C')

typedef struct {
  UINTN                              Signature;

  EFI_HANDLE                         DriverHandle;
  EFI_HII_HANDLE                     HiiHandle;
  ACPI_CONFIG_VARSTORE_DATA          Configuration;
  PLATFORM_INFO_HOB                  *PlatformHob;
  EFI_ACPI_SDT_PROTOCOL              *AcpiSdtProtocol;
  EFI_ACPI_HANDLE                    AcpiTableHandle;

  //
  // Consumed protocol
  //
  EFI_HII_CONFIG_ROUTING_PROTOCOL    *HiiConfigRouting;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL     ConfigAccess;
} ACPI_CONFIG_PRIVATE_DATA;

#define ACPI_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, ACPI_CONFIG_PRIVATE_DATA, ConfigAccess, ACPI_CONFIG_PRIVATE_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

#endif
