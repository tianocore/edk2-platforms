/** @file
  AMD Platform SoC Library.
  Provides interface to Get/Set platform specific data.

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_PLATFORM_SOC_LIB_H_
#define AMD_PLATFORM_SOC_LIB_H_

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
  UINTN                         PxmDomain;  // Proximity domain
} AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE;

/**
  Get the platform specific IOAPIC information.

  NOTE: Caller will need to free structure once finished.

  @param  IoApicInfo  The IOAPIC information
  @param  IoApicCount Number of IOAPIC present

  @retval EFI_SUCCESS             Successfully retrieve the IOAPIC information.
          EFI_INVALID_PARAMETERS  Incorrect parameters provided.
          EFI_UNSUPPORTED         Platform do not support this function.
          Other value             Returns other EFI_STATUS in case of failure.

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

  @param  RootBridge              The root bridge information
  @param  RootBridgeCount         Number of root bridges present

  @retval EFI_SUCCESS             Successfully retrieve the root bridge information.
          EFI_INVALID_PARAMETERS  Incorrect parameters provided.
          EFI_UNSUPPORTED         Platform do not support this function.
          Other value             Returns other EFI_STATUS in case of failure.

**/
EFI_STATUS
EFIAPI
GetPcieInfo (
  IN OUT AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  **RootBridge,
  IN OUT UINTN                                *RootBridgeCount
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

#endif
