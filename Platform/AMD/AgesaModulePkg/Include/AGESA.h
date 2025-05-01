/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AGESA_H_
#define AGESA_H_

#include  "AMD.h"
#include  "SocLogicalId.h"

#define DESCRIPTOR_TERMINATE_LIST   0x80000000ull
#define DESCRIPTOR_IGNORE           0x40000000ull
#define DESCRIPTOR_INITIALIZE_LIST  0x20000000ull

/// PCIe link initialization
typedef enum {
  EndpointDetect = 0,                                     ///< Detect endpoint presence
  EndpointNotPresent                                      ///< Endpoint not present (or connected). Used in case there is alternative way to determine
                                                          ///< if device present on board or in slot. For example GPIO can be used to determine device presence.
} PCIE_ENDPOINT_STATUS;

/// PCIe port misc extended controls
typedef struct  {
  IN      UINT8    LinkComplianceMode : 1;                  ///< Force port into compliance mode (device will not be trained, port output compliance pattern)
  IN      UINT8    LinkSafeMode       : 2;                  ///< Safe mode PCIe capability. (Parameter may limit PCIe speed requested through PCIe_PORT_DATA::LinkSpeedCapability)
                                                            ///<   0 - port can advertise maximum supported capability
                                                            ///<   1 - port limit advertized capability and speed to PCIe Gen1
  IN      UINT8    SbLink             : 1;                  ///< PCIe link type
                                                            ///<  0 - General purpose port
                                                            ///<  1 - Port connected to SB
  IN      UINT8    ClkPmSupport       : 1;                  ///< Clock Power Management Support
                                                            ///<   0 - Clock Power Management not configured
                                                            ///<   1 - Clock Power Management configured according to PCIe device capability
  IN      UINT8    CsLink             : 1;                  ///< PCIe link type
                                                            ///<   0 - General purpose port
                                                            ///<   1 - Port connected to chipset
  IN      UINT8    Reserved0          : 2;                  ///< Unused space
} PCIE_PORT_MISC_CONTROL;

/// The IO APIC Interrupt Mapping Info
typedef struct {
  IN      UINT8    GroupMap;                              ///< Group mapping for slot or endpoint device (connected to PCIE port) interrupts .
                                                          ///<   - IGNORE THIS STRUCTURE AND USE RECOMMENDED SETTINGS
                                                          ///<   - mapped to Grp 0 (Interrupts 0..3   of IO APIC redirection table)
                                                          ///<   - mapped to Grp 1 (Interrupts 4..7   of IO APIC redirection table)
                                                          ///<   - mapped to Grp 7 (Interrupts 28..31 of IO APIC redirection table)
  IN      UINT8    Swizzle;                               ///< Swizzle interrupt in the Group.
                                                          ///<   - ABCD
                                                          ///<   - BCDA
                                                          ///<   - CDAB
                                                          ///<   - DABC
  IN      UINT8    BridgeInt;                             ///< IOAPIC redirection table entry for PCIE bridge interrupt
                                                          ///<   - Entry 0  of IO APIC redirection table
                                                          ///<   - Entry 1  of IO APIC redirection table
                                                          ///<   - Entry 31 of IO APIC redirection table
} APIC_DEVICE_INFO;

/// GEN3 RxAdaptMode Configuration Structure
typedef struct {
  IN      BOOLEAN    InitOffsetCancellation;                 ///< Initial Offset Cancellation Enable
  IN      UINT8      DFEControl;                             ///< DFE Control
  IN      UINT8      LEQControl;                             ///< LEQ Control
  IN      BOOLEAN    DynamicOffsetCalibration;               ///< Dynamic Offset Calibration Enable
  IN      BOOLEAN    FOMCalculation;                         ///< FOM Calculation Enable
  IN      BOOLEAN    PIOffsetCalibration;                    ///< PI Offset Calibratino Enable
} RX_ADAPT_MODE;

/// PCIe port configuration data
typedef struct  {
  IN       UINT8                     PortPresent;            ///< Enable PCIe port for initialization.
  IN       UINT8                     ChannelType;            ///< Channel type.
                                                             ///<   0 - "lowLoss",
                                                             ///<   1 - "highLoss",
                                                             ///<   2 - "mob0db",
                                                             ///<   3 - "mob3db",
                                                             ///<   4 - "extnd6db"
                                                             ///<   5 - "extnd8db"
  IN       UINT8                     DeviceNumber;           ///< PCI Device number for port.
                                                             ///<   0 - Native port device number
                                                             ///<   N - Port device number (See available configurations in BKDG
  IN       UINT8                     FunctionNumber;         ///< Reserved for future use
  IN       UINT8                     LinkSpeedCapability;    ///< PCIe link speed/
                                                             ///<   0 - Maximum supported by silicon
                                                             ///<   1 - Gen1
                                                             ///<   2 - Gen2
                                                             ///<   3 - Gen3
  IN       UINT8                     LinkAspm;               ///< ASPM control. (see AgesaPcieLinkAspm for additional option to control ASPM)
                                                             ///<   0 - Disabled
                                                             ///<   1 - L0s only
                                                             ///<   2 - L1 only
                                                             ///<   3 - L0s and L1
  IN       UINT8                     LinkHotplug;            ///< Hotplug control.
                                                             ///<   0 - Disabled
                                                             ///<   1 - Basic
                                                             ///<   2 - Server
                                                             ///<   3 - Enhanced
  IN       UINT8                     ResetId;                ///<  Arbitrary number greater than 0 assigned by platform firmware for GPIO
                                                             ///<  identification which control reset for given port.
                                                             ///<  Each port with unique GPIO should have unique ResetId assigned.
                                                             ///<  All ports use same GPIO to control reset should have same ResetId assigned.
                                                             ///<  see AgesaPcieSlotResetContol.
  IN       UINT16                    SlotNum;                ///< Physical Slot Number
  IN       PCIE_PORT_MISC_CONTROL    MiscControls;           ///< Misc extended controls
  IN       APIC_DEVICE_INFO          ApicDeviceInfo;         ///< IOAPIC device programming info
  IN       PCIE_ENDPOINT_STATUS      EndpointStatus;         ///< PCIe endpoint (device connected to PCIe port) status
  IN       RX_ADAPT_MODE             RxAdaptMode;            ///< Gen3 RxAdaptMode configuration
} PCIE_PORT_DATA;

/// PCIe Complex descriptor
typedef struct {
  IN       UINT32    Flags;                               ///< Descriptor flags
                                                          ///<   Bit31 - last descriptor in topology
  IN       UINT32    SocketId;                            ///< Socket Id
  IN       VOID      *Reserved;                           ///< Reserved for future use
} PCIE_COMPLEX_DESCRIPTOR;

/// VBIOS image info
typedef struct {
  IN      AMD_CONFIG_PARAMS    StdHeader;                 ///< Standard configuration header
  OUT     VOID                 *ImagePtr;                 ///< Pointer to VBIOS image
  IN      PCI_ADDR             GfxPciAddress;             ///< PCI address of integrated graphics controller
  IN      UINT32               Flags;                     ///< BIT[0] - special repost requred
} GFX_VBIOS_IMAGE_INFO;

//
// CPU MSR Register definitions
//
#define SYS_CFG  0xC0010010ul     ///< Refer to AMD64 Architecture Programming manual.
#define TOP_MEM  0xC001001Aul     ///< Refer to AMD64 Architecture Programming manual.
#define HWCR     0xC0010015ul     ///< Refer to AMD64 Architecture Programming manual.

///
/// VDDP_VDDR Voltage Info for Low Power DIMM
///
typedef struct _VDDP_VDDR_VOLTAGE {
  IN BOOLEAN    IsValid; ///< Indicates if daata is valid
  IN UINT8      Voltage; ///< VDDP VDDR Voltage Value
} VDDP_VDDR_VOLTAGE;

// CPU Build Configuration structures and definitions

#define AMD_AP_MTRR_FIX64k_00000  0x00000250ul    ///< Refer to AMD64 Architecture Programming manual.
#define AMD_AP_MTRR_FIX16k_80000  0x00000258ul    ///< Refer to AMD64 Architecture Programming manual.
#define AMD_AP_MTRR_FIX16k_A0000  0x00000259ul    ///< Refer to AMD64 Architecture Programming manual.
#define AMD_AP_MTRR_FIX4k_C0000   0x00000268ul    ///< Refer to AMD64 Architecture Programming manual.
#define AMD_AP_MTRR_FIX4k_C8000   0x00000269ul    ///< Refer to AMD64 Architecture Programming manual.
#define AMD_AP_MTRR_FIX4k_D0000   0x0000026Aul    ///< Refer to AMD64 Architecture Programming manual
#define AMD_AP_MTRR_FIX4k_D8000   0x0000026Bul    ///< Refer to AMD64 Architecture Programming manual
#define AMD_AP_MTRR_FIX4k_E0000   0x0000026Cul    ///< Refer to AMD64 Architecture Programming manual
#define AMD_AP_MTRR_FIX4k_E8000   0x0000026Dul    ///< Refer to AMD64 Architecture Programming manual
#define AMD_AP_MTRR_FIX4k_F0000   0x0000026Eul    ///< Refer to AMD64 Architecture Programming manual
#define AMD_AP_MTRR_FIX4k_F8000   0x0000026Ful    ///< Refer to AMD64 Architecture Programming manual

#endif // AGESA_H_
