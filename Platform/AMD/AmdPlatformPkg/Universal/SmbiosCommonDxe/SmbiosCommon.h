/** @file
  AMD Smbios common header file.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_COMMON_DRIVER_H_
#define SMBIOS_COMMON_DRIVER_H_

#include <PiDxe.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/PciSegmentLib.h>

/**
  Add an SMBIOS record.

  @param[in]  Smbios                The EFI_SMBIOS_PROTOCOL instance.
  @param[out] SmbiosHandle          A unique handle will be assigned to the SMBIOS record.
  @param[in]  Record                The data for the fixed portion of the SMBIOS record. The format of the record is
                                determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined
                                by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or
                                a set of null terminated strings and a null.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.

**/
EFI_STATUS
AddCommonSmbiosRecord (
  IN EFI_SMBIOS_PROTOCOL      *Smbios,
  OUT EFI_SMBIOS_HANDLE       *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER  *Record
  );

/**
  This function gets the Bus, Device and Segment number of a PCI device when Vendor ID, Device ID and instance
  are provided.

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.
  @param[in]  VendorId                   Vendor ID of the PCI device to be provided.
  @param[in]  DeviceId                   Device ID of the PCI device to be provided
  @param[out]  Instance                   Instance of the PCI device. If more than one devices with same vendor
                                     and device ID is present, instance number is used.
  @param[out]  Segment                    Segment number of the PCI device is assigned.
  @param[out]  Bus                        Bus number of the PCI device is assigned.
  @param[out]  Device                     Device number of the PCI device is assigned.
  @param[out]  Functions                  Bits 0-7 of the Functions variable correspond to respective function numbers.
  @param[out]  DeviceFound                Set to 1 if the device is found.

  @retval EFI_SUCCESS                All parameters were valid.
**/
EFI_STATUS
EFIAPI
GetBusDeviceInfo (
  IN  UINT16  *VendorId,
  IN  UINT16  *DeviceId,
  IN  UINT8   *Instance,
  OUT UINT16  *Segment,
  OUT UINT8   *Bus,
  OUT UINT8   *Device,
  OUT UINT8   *Functions,
  OUT UINT8   *DeviceFound
  );

/**
  PciEnumerationComplete Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
OnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This function updates IPMI Device information changes to the contents of the
  Table Type 38.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
**/
EFI_STATUS
EFIAPI
IpmiDeviceInformation (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  This function checks for system slot info and adds smbios record (Type 9).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.

**/
EFI_STATUS
EFIAPI
SystemSlotInfoFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  This function adds port connector information smbios record (Type 8).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
PortConnectorInfoFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  This function adds OEM strings smbios record (Type 11).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
OemStringsFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  This function adds System Configuration Options record (Type 12).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
SystemCfgOptionsFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  This function adds bios language information smbios record (Type 13).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
  @retval EFI_NOT_FOUND              Not able to locate PlatformLanguage.

**/
EFI_STATUS
EFIAPI
BiosLanguageInfoFunction (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  This function adds onboard devices extended information smbios record (Type 41).

  @param[in]  Smbios                     The EFI_SMBIOS_PROTOCOL instance.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_OUT_OF_RESOURCES       Resource not available.
**/
EFI_STATUS
EFIAPI
OnboardDevExtInfoFunction (
  IN EFI_SMBIOS_PROTOCOL  *Smbios
  );

typedef
EFI_STATUS
(EFIAPI EFI_COMMON_SMBIOS_DATA_FUNCTION)(
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

typedef struct {
  EFI_COMMON_SMBIOS_DATA_FUNCTION    *Function;
} EFI_COMMON_SMBIOS_DATA;
#endif // SMBIOS_COMMON_DRIVER_H_
