/** @file
  Smbios basic entry point.

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosBasic.h"

EFI_STATUS
EFIAPI
BiosVendorFunction( // Type 0
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
SystemManufacturerFunction( // Type 1
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
BaseBoardManufacturerFunction( // Type 2
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ChassisManufacturerFunction( // Type 3
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ProcessorInformationFunction( // Type 4
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
MemoryControllerFunction( // Type 5
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
MemoryModuleInformationFunction( // Type 6
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
CacheInformationFunction( // Type 7
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
PortConnectorInformationFunction( // Type 8
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
SystemSlotsFunction( // Type 9
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
OnBoardDevicesInformationFunction( // Type 10
	IN  EFI_SMBIOS_PROTOCOL* Smbios
);

EFI_STATUS
EFIAPI
OEMStringsFunction( // Type 11
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
  EFIAPI
  SystemConfigurationOptionsFunction( // Type 12
     IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
BiosLanguageFunction( // Type 13
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
GroupAssociationsFunction( // Type 14
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
SystemEventLogFunction( // Type 15
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
PhysicalMemoryArrayFunction( // Type 16
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
MemoryDeviceFunction( // Type 17
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
g32bitMemoryErrorInformationFunction( // Type 18
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
MemoryArrayMappedAddressFunction( // Type 19
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
MemoryDeviceMappedAddressFunction( // Type 20
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
PointingDeviceFunction( // Type 21
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
PortableBatterynFunction( // Type 22
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
SystemResetFunction( // Type 23
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
HardwareSecurityFunction( // Type 24
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
SystemPowerControlsFunction( // Type 25
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
VoltageProbeFunction( // Type 26
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
CoolingDeviceFunction( // Type 27
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
TemperatureProbeFunction( // Type 28
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ElectricalCurrentProbeFunction( // Type 29
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
OutOfBoundRemoteAccessFunction(
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
BISEntryPointFunction( // Type 31
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
BootInfoStatusFunction( // Type 32
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
BitMemoryErrorInformationFunction( // Type 33
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ManagementDeviceFunction( // Type 34
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ManagementDeviceComponentFunction( // Type 35
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ManagementDeviceThresholdDataFunction( // Type 36
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
MemoryChannelInformationFunction( // Type 37
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
PimiDeviceInformationFunction( // Type 38
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
SystemPowerSupplyFunction( // Type 39
  IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
AdditionalInformationFunction( // Type 40
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
OnboardDevicesExtendedInformationFunction( // Type 41
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ManagementControllerHostInterfaceFunction( // Type 42
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
TMPDeviceFunction( // Type 43
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
ProcessorAdditionalInformationFunction( // Type 44
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

EFI_STATUS
EFIAPI
InactiveFunction( // Type 126
    IN  EFI_SMBIOS_PROTOCOL   *Smbios
);

typedef
EFI_STATUS
(EFIAPI EFI_BASIC_SMBIOS_DATA_FUNCTION) (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
);

typedef struct {
  EFI_BASIC_SMBIOS_DATA_FUNCTION *Function;
} EFI_BASIC_SMBIOS_DATA;

EFI_BASIC_SMBIOS_DATA mSmbiosBasicDataFuncTable[] = {
  {&BiosVendorFunction},
  {&SystemManufacturerFunction},
  {&BaseBoardManufacturerFunction},
  {&ChassisManufacturerFunction},
  {&ProcessorInformationFunction},
  {&MemoryControllerFunction},
  {&MemoryModuleInformationFunction},
  {&CacheInformationFunction},
  {&PortConnectorInformationFunction},
  {&SystemSlotsFunction},
  {&OnBoardDevicesInformationFunction},
  {&OEMStringsFunction},
  {&SystemConfigurationOptionsFunction},
  {&BiosLanguageFunction},
  {&GroupAssociationsFunction},
  {&SystemEventLogFunction},
  {&PhysicalMemoryArrayFunction},
  {&MemoryDeviceFunction},
  {&g32bitMemoryErrorInformationFunction},
  {&MemoryArrayMappedAddressFunction},
  {&MemoryDeviceMappedAddressFunction},
  {&PointingDeviceFunction},
  {&PortableBatterynFunction},
  {&SystemResetFunction},
  {&HardwareSecurityFunction},
  {&SystemPowerControlsFunction},
  {&VoltageProbeFunction},
  {&CoolingDeviceFunction},
  {&TemperatureProbeFunction},
  {&ElectricalCurrentProbeFunction},
  {&OutOfBoundRemoteAccessFunction},
  {&BISEntryPointFunction},
  {&BootInfoStatusFunction},
  {&BitMemoryErrorInformationFunction},
  {&ManagementDeviceFunction},
  {&ManagementDeviceComponentFunction},
  {&ManagementDeviceThresholdDataFunction},
  {&MemoryChannelInformationFunction},
  {&PimiDeviceInformationFunction},
  {&SystemPowerSupplyFunction},
  {&AdditionalInformationFunction},
  {&OnboardDevicesExtendedInformationFunction},
  {&ManagementControllerHostInterfaceFunction},
  {&TMPDeviceFunction},
  {&ProcessorAdditionalInformationFunction},
  {&InactiveFunction},
};

/**
  Standard EFI driver point.  This driver parses the mSmbiosMiscDataTable
  structure and reports any generated data using SMBIOS protocol.

  @param  ImageHandle     Handle for the image of this driver
  @param  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

**/
EFI_STATUS
EFIAPI
SmbiosBasicEntryPoint(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  UINTN                Index;
  EFI_STATUS           EfiStatus;
  EFI_SMBIOS_PROTOCOL  *Smbios;

  EfiStatus = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  if (EFI_ERROR(EfiStatus)) {
    DEBUG((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
    return EfiStatus;
  }

  for (Index = 0; Index < sizeof(mSmbiosBasicDataFuncTable)/sizeof(mSmbiosBasicDataFuncTable[0]); ++Index) {
    EfiStatus = (*mSmbiosBasicDataFuncTable[Index].Function) (Smbios);
    if (EFI_ERROR(EfiStatus)) {
      DEBUG((DEBUG_ERROR, "Basic smbios store error.  Index=%d, ReturnStatus=%r\n", Index, EfiStatus));
      return EfiStatus;
    }
  }

  return EfiStatus;
}

/**
  Add an SMBIOS record.

  @param  Smbios                The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          A unique handle will be assigned to the SMBIOS record.
  @param  Record                The data for the fixed portion of the SMBIOS record. The format of the record is
                                determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined
                                by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or
                                a set of null terminated strings and a null.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.

**/
EFI_STATUS
AddSmbiosRecord (
  IN EFI_SMBIOS_PROTOCOL        *Smbios,
  OUT EFI_SMBIOS_HANDLE         *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER    *Record
  )
{
  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  return Smbios->Add (
                   Smbios,
                   NULL,
                   SmbiosHandle,
                   Record
                   );
}
