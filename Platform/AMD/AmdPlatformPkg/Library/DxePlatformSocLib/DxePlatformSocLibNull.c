/** @file
  Implements AMD Platform SoC Library.
  Provides interface to Get/Set platform specific data.

  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Acpi65.h>
#include <Library/AmdPlatformSocLib.h>
#include <IndustryStandard/SmBios.h>

/**
  Get the platform specific IOAPIC information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  IoApicInfo     The IOAPIC information
  @param[in, out]  IoApicCount    Number of IOAPIC present

  @retval EFI_SUCCESS             Successfully retrieve the IOAPIC information.
  @retval EFI_INVALID_PARAMETERS  Incorrect parameters provided.
  @retval EFI_UNSUPPORTED         Platform do not support this function.
  @retval Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetIoApicInfo (
  IN OUT EFI_ACPI_6_5_IO_APIC_STRUCTURE  **IoApicInfo,
  IN OUT UINT8                           *IoApicCount
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the platform PCIe configuration information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  RootBridge              The root bridge information
  @param[in, out]  RootBridgeCount         Number of root bridges present

  @retval EFI_SUCCESS             Successfully retrieve the root bridge information.
  @retval EFI_INVALID_PARAMETERS  Incorrect parameters provided.
  @retval EFI_UNSUPPORTED         Platform do not support this function.
  @retval Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetPcieInfo (
  IN OUT AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  **RootBridge,
  IN OUT UINTN                                *RootBridgeCount
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the ECRC capability setting from PCIe core topology for a root bridge.
  @param[in]      Segment      PCI segment number.
  @param[in]      Bus          PCI bus number.
  @param[out]     EcrcSupport  PCIe core ECRC support value.

  @retval EFI_SUCCESS             Successfully retrieved ECRC support.
  @retval EFI_INVALID_PARAMETER   Incorrect parameters provided.
  @retval EFI_NOT_FOUND           Matching PCIe core was not found in topology.
  @retval Other value             Returns other EFI_STATUS in case of failure.
**/
EFI_STATUS
EFIAPI
GetPcieEcrcSupport (
  IN  UINTN  Segment,
  IN  UINTN  Bus,
  OUT UINTN  *EcrcSupport
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the platform specific System Power Supply information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  PowerSupplyInfo          The System Power Supply information

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetSystemPowerSupplyInfo (
  IN OUT SMBIOS_TABLE_TYPE39  **PowerSupplyInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the platform specific System Slot information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  SystemSlotInfo          The System Slot information
  @param[in, out]  SystemSlotCount         Number of System Slot present

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetSystemSlotInfo (
  IN OUT SMBIOS_TABLE_TYPE9  **SystemSlotInfo,
  IN OUT UINTN               *SystemSlotCount
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the platform specific System Slot information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  PortingDeviceBuffer         The System Porting Device information.
  @param[in, out]  PortingDeviceCounts         Number of System Porting Device.

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetPortingDeviceInfo (
  IN OUT SMBIOS_TABLE_TYPE21  **PortingDeviceBuffer,
  IN OUT UINTN                *PortingDeviceCounts
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the platform specific System Voltage Probe information.

  NOTE: Caller will need to free structure once finished.

  @param[in, out]  VoltageProbeBuffer         The System Voltage Probe information.
  @param[in, out]  VoltageProbeCounts         Number of System Voltage Probe.

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetVoltageProbeInfo (
  IN OUT VOID   **VoltageProbeBuffer,
  IN OUT UINTN  *VoltageProbeCounts
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the BIOS information for SMBIOS type 0.

  @param[out]  SmBiosType0Info         The structure to hold BIOS information for SMBIOS type0.

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetBoardBiosInfo (
  OUT AMD_SMBIOS_TYPE0_BIOS_INFO  *SmBiosType0Info
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Update the platform specific Configuration Manager information.
  PlatformRepo needs to be typecasted to the platform specific Configuration Manager information.
  e.g EDKII_PLATFORM_REPOSITORY_INFO *PlatRepo = (EDKII_PLATFORM_REPOSITORY_INFO *)PlatformRepo;

  @param[in, out]  PlatformRepo   The platform specific Configuration Manager information.
  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
UpdatePlatformCmInfo (
  IN OUT VOID  *PlatformRepo
  )
{
  return EFI_UNSUPPORTED;
}
