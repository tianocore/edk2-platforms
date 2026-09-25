/** @file
  AMD Platform SoC Library.
  Provides interface to Get/Set/Update platform specific data.

  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <IndustryStandard/Acpi65.h>
#include <IndustryStandard/SmBios.h>
#include <Uefi/UefiBaseType.h>

#define PCIE_MAX_FUNCTIONS  8
#define PCIE_MAX_DEVICES    32
#define PCIE_MAX_ROOTPORT   (PCIE_MAX_DEVICES * PCIE_MAX_FUNCTIONS)

#define F1A_BRH_A0_RAW_ID   0x00B00F00ul
#define F1A_BRH_B0_RAW_ID   0x00B00F10ul
#define F1A_BRH_B1_RAW_ID   0x00B00F11ul
#define F1A_BRHD_A0_RAW_ID  0x00B10F00ul
#define F1A_BRHD_B0_RAW_ID  0x00B10F10ul

typedef struct {
  UINTN      Index;
  BOOLEAN    Enabled;
  UINT8      PortPresent;
  UINTN      Device;
  UINTN      Function;
  UINT8      LinkHotplug;
  UINTN      SlotNum;
  // Interrupts are relative to IOAPIC 0->n
  UINTN      BridgeInterrupt;           // Redirection table entry for mapped bridge interrupt
  UINTN      EndpointInterruptArray[4]; // Redirection table entries for mapped INT A/B/C/D
} AMD_PCI_ROOT_PORT_OBJECT;

typedef struct {
  UINTN    Index;
  UINT8    SocketId;
  UINTN    Segment;
  UINTN    BaseBusNumber;
} AMD_PCI_ROOT_BRIDGE_OBJECT;

/// Extended PCI address format
typedef struct {
  IN OUT  UINT32    Register : 12;                ///< Register offset
  IN OUT  UINT32    Function : 3;                 ///< Function number
  IN OUT  UINT32    Device   : 5;                 ///< Device number
  IN OUT  UINT32    Bus      : 8;                 ///< Bus number
  IN OUT  UINT32    Segment  : 4;                 ///< Segment
} AMD_EXT_PCI_ADDR;

/// Union type for PCI address
typedef union {
  IN  UINT32              AddressValue;             ///< Formal address
  IN  AMD_EXT_PCI_ADDR    Address;                  ///< Extended address
} AMD_PCI_ADDR;

/// Port Information Structure
typedef struct {
  AMD_PCI_ADDR    EndPointBDF;          ///< Bus/Device/Function of Root Port in PCI_ADDR format
  BOOLEAN         IsCxl2;
  UINT8           Cxl11Count;
} AMD_CXL_PORT_INFO;

typedef struct {
  EFI_HANDLE                    Handle;
  UINTN                         Uid;
  UINTN                         GlobalInterruptStart;
  VOID                          *Configuration; // Never free this buffer
  AMD_PCI_ROOT_BRIDGE_OBJECT    *Object;        // Never free this object
  UINTN                         RootPortCount;
  AMD_PCI_ROOT_PORT_OBJECT      *RootPort[PCIE_MAX_ROOTPORT]; // Never free this object
  UINTN                         CxlCount;
  AMD_CXL_PORT_INFO             CxlPortInfo;
  UINTN                         PxmDomain;   // Proximity domain
  UINTN                         EcrcSupport; // PCIe core ECRC Support
} AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE;

//
// Any fields of SMBIOS type0 need to be added in this structure.
// And get the information from BoardPkg side if fields were override.
//
typedef struct {
  CHAR8    VendorStr[SMBIOS_STRING_MAX_LENGTH];
  UINTN    VendorStrLen;
  CHAR8    VersionStr[SMBIOS_STRING_MAX_LENGTH];
  UINTN    VerStrLen;
  CHAR8    DateStr[SMBIOS_STRING_MAX_LENGTH];
  UINTN    DateStrLen;
} AMD_SMBIOS_TYPE0_BIOS_INFO;

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
  );

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
  );

/**
  Get the ECRC support setting from PCIe core topology for a root bridge.

  @param[in]      Segment      PCI segment number.
  @param[in]      Bus          PCI bus number.
  @param[out]     EcrcSupport  PCIe core ECRC support value.

  @retval EFI_SUCCESS             Successfully retrieved ECRC support.
  @retval EFI_INVALID_PARAMETER   Incorrect parameters provided.
  @retval EFI_NOT_FOUND           Matching PCIe core was not found in topology.
  @retval EFI_UNSUPPORTED         Platform does not support this function.
  @retval Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetPcieEcrcSupport (
  IN  UINTN  Segment,
  IN  UINTN  Bus,
  OUT UINTN  *EcrcSupport
  );

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
  );

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
  );

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
  );

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
  );

/**
  Get the BIOS information for SMBIOS type 0.

  @param[out]  SmBiosType0Info         The structure to hold BIOS information for SMBIOS type0.

  @retval EFI_UNSUPPORTED         Platform do not support this function.
**/
EFI_STATUS
EFIAPI
GetBoardBiosInfo (
  OUT AMD_SMBIOS_TYPE0_BIOS_INFO  *SmBiosType0Info
  );

/**
  Update the platform specific Configuration Manager information.
  PlatformRepo needs to be typecasted to the platform specific Configuration Manager information.
  e.g EDKII_PLATFORM_REPOSITORY_INFO *PlatRepo = (EDKII_PLATFORM_REPOSITORY_INFO *)PlatformRepo;

  @param[in, out]  PlatformRepo   The platform specific Configuration Manager information.
  @retval EFI_SUCCESS             Successfully updated the configuration information.
  @retval EFI_UNSUPPORTED         Platform do not support this function.
  @retval Other value             Returns other EFI_STATUS in case of failure.
**/
EFI_STATUS
EFIAPI
UpdatePlatformCmInfo (
  IN OUT VOID  *PlatformRepo
  );
