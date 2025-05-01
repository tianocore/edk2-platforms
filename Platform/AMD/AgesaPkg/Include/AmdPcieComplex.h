/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_PCIE_COMPLEX_H_
#define AMD_PCIE_COMPLEX_H_

//
// GNB PCIe configuration info
//
#define DESCRIPTOR_TERMINATE_LIST  0x80000000ull
#define DESCRIPTOR_IGNORE          0x40000000ull

//
// Port parameter count
//
#define PCIE_PORT_PARAMETER_COUNT  64

///
/// PCIe link initialization
/// DXIO endpoint status
///
typedef enum {
  DxioEndpointDetect = 0,                                    ///< Detect endpoint presence
  DxioEndpointNotPresent                                     ///< Endpoint not present (or connected). Used in case there is alternative way to determine
                                                             ///< if device present on board or in slot. For example GPIO can be used to determine device presence.
} DXIO_ENDPOINT_STATUS;

#pragma pack(push,1)
typedef struct {
  UINT16    ParamType;                           ///< This identifies a specific port parameter to set.
  UINT16    ParamValue;                          ///< Specifies the value to be assigned to port parameter.
} PORT_PARAM;

typedef struct {
  PORT_PARAM    PhyParam[PCIE_PORT_PARAMETER_COUNT];               ///< PHY port parameter
} PORT_PARAMS;

///
///  Ancillary data struct with table size and address
///
typedef struct {
  IN       UINT32    Count;                          ///< Total count in this Ancillary data table
  IN       UINT32    Ovrd;                           ///< Ancillary data table address point to ANC_DATA_PARAM[]
} ANC_DATA;

/**
 *  @brief Port parameter override enumerator.
 *  @details This enumerator provides a parmeter type for platform topology override values.
 *  If unused all ports will default to platform configurations.
 */
typedef enum {
  PP_SLOT_NUM,                 /**< (__UINT16__) Specify a SLOT NUMBER value.
                                *  @li Valid Values: __0-0xFFFF__
                                */
} DXIO_PORT_PARAM_TYPE;

typedef struct {
  UINT16    ParamType;                 ///< This identifies a specific PHY parameter
  UINT16    ParamValue;                ///< This specifies the value to be assigned to indicated PHY parameter
} DXIO_PHY_PARAM;

typedef struct {
  DXIO_PHY_PARAM    PhyParam[44];          ///< physical parameter
} PHY_DATA;

///
/// PCIe specific data structures
/// PCIe port misc extended controls
///
typedef struct  {
  UINT8    LinkComplianceMode : 1;                      ///< Force port into compliance mode (device will not be trained, port output compliance pattern)
  UINT8    LinkSafeMode       : 1;                      ///< Safe mode PCIe capability. (Parameter may limit PCIe speed requested through DXIO_PORT_DATA::LinkSpeedCapability)
                                                        ///<   0 - port can advertize muximum supported capability
                                                        ///<   1 - port limit advertized capability and speed to PCIe Gen1
  UINT8    SbLink             : 1;                      ///< PCIe link type
                                                        ///<   0 - General purpose port
                                                        ///<   1 - Port connected to SB
  UINT8    ClkPmSupport       : 1;                      ///< Clock Power Management Support
                                                        ///<   0 - Clock Power Management not configured
                                                        ///<   1 - Clock Power Management configured according to PCIe device capability
  UINT8    ChannelType        : 3;                      ///< Channel Type
                                                        ///<   0 - Channel Type Not Specified
                                                        ///<    - Channel Type Short Trace
                                                        ///<   2 - Channel Type Long Trace
  UINT8    TurnOffUnusedLanes : 1;                      ///< Turn Off Unused Lanes
                                                        ///<   0 - Turn on
                                                        ///<   1 - Turn off
} DXIO_PORT_MISC_CONTROL;

///
/// The IO APIC Interrupt Mapping Info
///
typedef struct {
  UINT8    GroupMap;                                ///< Group mapping for slot or endpoint device (connected to PCIE port) interrupts .
                                                    ///<   0 - IGNORE THIS STRUCTURE AND USE RECOMMENDED SETTINGS
                                                    ///<   1 - mapped to Grp 0 (Interrupts 0..3   of IO APIC redirection table)
                                                    ///<   2 - mapped to Grp 1 (Interrupts 4..7   of IO APIC redirection table)
                                                    ///<       ...
                                                    ///<   8  - mapped to Grp 7 (Interrupts 28..31 of IO APIC redirection table)
  UINT8    Swizzle;                                 ///< Swizzle interrupt in the Group.
                                                    ///<   0 - ABCD
                                                    ///<   1 - BCDA
                                                    ///<   2 - CDAB
                                                    ///<   3 - DABC
  UINT8    BridgeInt;                               ///< IOAPIC redirection table entry for PCIE bridge interrupt
                                                    ///<   0 - Entry 0  of IO APIC redirection table
                                                    ///<   1 - Entry 1  of IO APIC redirection table
                                                    ///<   ...
                                                    ///<   31 - Entry 31 of IO APIC redirection table
} DXIO_APIC_DEVICE_INFO;
///
/// PCIe port configuration data
///
typedef struct  {
  UINT8                     PortPresent         : 1;  ///< Enable PCIe port for initialization.
  UINT8                     Reserved1           : 2;  ///< Reserved
  UINT8                     DeviceNumber        : 5;  ///< PCI Device number for port.
                                                      ///<   0 - Native port device number
                                                      ///<   N - Port device number (See available configurations in BKDG
  UINT8                     FunctionNumber      : 3;  ///< Reserved for future use
  UINT8                     LinkSpeedCapability : 3;  ///< PCIe link speed/
                                                      ///<   0 - Maximum supported by silicon
                                                      ///<   1 - Gen1
                                                      ///<   2 - Gen2
                                                      ///<   3 - Gen3
                                                      ///<   4 - Gen4
                                                      ///<   5 - Gen5
  UINT8                     AutoSpdChng         : 2;  ///< Upstread Auto Speed Change Allowed/
                                                      ///<   0 - Use default implementation (Disabled for Gen1, Enabled for Gen2/3)
                                                      ///<   1 - Always Disabled
                                                      ///<   2 - Always Enabled
                                                      ///<   3 - Reserved
  UINT8                     EqPreset            : 4;  ///< Gen3 Equalization Preset */
  UINT8                     LinkAspm            : 2;  ///< ASPM control. (see AgesaPcieLinkAspm for additional option to control ASPM)
                                                      ///<   0 - Disabled
                                                      ///<   1 - L0s only
                                                      ///<   2 - L1 only
                                                      ///<   3 - L0s and L1
  UINT8                     LinkAspmL1_1        : 1;  ///< ASPM control. (see AgesaPcieLinkAspm for additional option to control ASPM)
                                                      ///<   0 - Disabled
                                                      ///<   1 - Enabled
  UINT8                     LinkAspmL1_2        : 1;  ///< ASPM control. (see AgesaPcieLinkAspm for additional option to control ASPM)
                                                      ///<   0 - Disabled
                                                      ///<   1 - Enabled
  UINT8                     ClkReq              : 4;  ///< ASPM Reserved Field
                                                      ///<   0 - NONE
                                                      ///<   1 - CLKREQ0 signal
                                                      ///<   2 - CLKREQ1 signal
                                                      ///<   3 - CLKREQ2 signal
                                                      ///<   4 - CLKREQ3 signal
                                                      ///<   5 - CLKREQG signal
  UINT8                     LinkHotplug         : 4;  ///< Hotplug control.
                                                      ///<   0 - Disabled
                                                      ///<   1 - Basic
                                                      ///<   2 - Server
                                                      ///<   3 - Enhanced
  UINT8                     SlotPowerLimit;           ///< PCIe slot power limit.
  UINT8                     SlotPowerLimitScale : 2;  ///< PCIe slot power limit Scale.
                                                      ///<   00b = 1.0x
                                                      ///<   01b = 0.1x
                                                      ///<   10b = 0.01x
                                                      ///<   11b = 0.001x
  UINT8                     IsMasterPLL         : 1;  ///< IsMasterPLL
  UINT8                     Gen4Features        : 5;  ///< Unused bits
                                                      ///<   BIT0(DLF_Exchange) 1 - Disable, 0 - Enable
                                                      ///<   IT1(DLF_Capability) 1 - Disable, 0 - Enable
  UINT16                    SlotNum             : 13; ///< PHYSICAL_SLOT_NUM
  UINT16                    CsLink              : 3;  ///< Reserved
  DXIO_PORT_MISC_CONTROL    MiscControls;             ///< Misc extended controls
  DXIO_APIC_DEVICE_INFO     ApicDeviceInfo;           ///< IOAPIC device programming info
  DXIO_ENDPOINT_STATUS      EndpointStatus;           ///< PCIe endpoint (device connected to PCIe port) status
  UINT8                     EsmSpeedBump;             ///< Speed bump for ESM
  UINT8                     EsmControl          : 1;  ///< Enhanced speed mode control
  UINT8                     CcixControl         : 1;  ///< Ccix/Cxl control
  UINT8                     TxVetting           : 1;  ///< Tx Vetting
  UINT8                     RxVetting           : 1;  ///< Rx Vetting
  UINT8                     InvertPolarity      : 1;  ///< Invert RX Polarity
  UINT8                     InvertPolarity2     : 1;  ///< Invert TX Polarity
  UINT8                     NtbHotplug          : 1;  ///< NTB Hotplug flag
                                                      ///<   0b = Disabled
                                                      ///<   1b = Enabled
  UINT8                     Reserved2           : 1;  ///< Reserved
  UINT8                     SetGen3FixedPreset  : 1;  ///< Gen3 Fixed Preset Set
  UINT8                     SetGen4FixedPreset  : 1;  ///< Gen4 Fixed Preset Set
  UINT8                     SetGen5FixedPreset  : 1;  ///< Gen5 Fixed Preset Set
  UINT8                     Reserved3           : 5;  ///< Reserved
  UINT8                     Gen3FixedPreset     : 4;  ///< Gen3 Fixed Preset
  UINT8                     Gen4FixedPreset     : 4;  ///< Gen4 Fixed Preset
  UINT8                     Gen5FixedPreset     : 4;  ///< Gen5 Fixed Preset
  UINT8                     Reserved4           : 4;  ///< Reserved
  UINT16                    PsppPolicyDC;             ///< Pspp DC control
  UINT16                    PsppPolicyAC;             ///< PSPP AC control
  UINT8                     PsppDeviceType;           ///< Pspp Device Type
  UINT8                     DisGen3EQPhase      : 1;  ///< Gen3 Bypass phase2/3 EQ
  UINT8                     DisGen4EQPhase      : 1;  ///< Gen4 Bypass phase2/3 EQ
  UINT8                     TXDeEmphasisOride   : 1;  ///< Override Gen2 DXIO deemphasis default
  UINT8                     TXDeEmphasis        : 2;  ///< Gen2 DXIO deemphasis setting
  UINT8                     Reserved5           : 3;  ///< Reserved
  struct {
    UINT16    DsTxPreset        : 4;                  ///< Gen3 Downstream Tx Preset
    UINT16    DsRxPresetHint    : 3;                  ///< Gen3 Downstream Rx Preset Hint
    UINT16    UsTxPreset        : 4;                  ///< Gen3 Upstream Tx Preset
    UINT16    UsRxPresetHint    : 3;                  ///< Gen3 Upstream Rx Preset Hint
    UINT16    Reserved1         : 2;                  ///< Unused bits
    UINT8     SetDsTxPreset     : 1;                  ///< Gen3 Set Downstream Tx Preset
    UINT8     SetDsRxPresetHint : 1;                  ///< Gen3 Set Downstream Rx Preset Hint
    UINT8     SetUsTxPreset     : 1;                  ///< Gen3 Set Upstream Tx Preset
    UINT8     SetUsRxPresetHint : 1;                  ///< Gen3 Set Upstream Rx Preset Hint
    UINT8     Reserved2         : 4;                  ///< Unused bits
  } LaneEqualizationCntl;                             ///< Lane equalization control structure used for Gen3 values
  struct {
    UINT8    DsTxPreset    : 4;                       ///< Gen4 Downstream Tx Preset
    UINT8    UsTxPreset    : 4;                       ///< Gen4 Upstream Tx Preset
    UINT8    SetDsTxPreset : 1;                       ///< Gen4 Set Downstream Tx Preset
    UINT8    SetUsTxPreset : 1;                       ///< Gen4 Set Upstream Tx Preset
    UINT8    Reserved1     : 6;                       ///< Unused bits
  } Gen4LaneEqualizationCntl;                                ///< Lane equalization control structure used for Gen4 values
  struct {
    UINT8    DsTxPreset    : 4;                       ///< Gen5 Downstream Tx Preset
    UINT8    UsTxPreset    : 4;                       ///< Gen5 Upstream Tx Preset
    UINT8    SetDsTxPreset : 1;                       ///< Gen5 Set Downstream Tx Preset
    UINT8    SetUsTxPreset : 1;                       ///< Gen5 Set Upstream Tx Preset
    UINT8    Reserved1     : 6;                       ///< Unused bits
  } Gen5LaneEqualizationCntl;                         ///< Lane equalization control structure used for Gen5 values
  struct {
    UINT32    PresetMask8Gt     : 10;                 ///< Preset Mask 8GT.
    UINT32    PresetMask16Gt    : 10;                 ///< Preset Mask 16GT.
    UINT32    PresetMask32Gt    : 10;                 ///< Preset Mask 32GT.
    UINT32    Reserved1         : 2;                  ///< Unused bits
    UINT8     SetPresetMask8Gt  : 1;                  ///< Preset Mask 8GT Set
    UINT8     SetPresetMask16Gt : 1;                  ///< Preset Mask 16GT Set
    UINT8     SetPresetMask32Gt : 1;                  ///< Preset Mask 32GT Set
    UINT8     Reserved2         : 5;                  ///< Unused bits
  } PresetMaskCntl;                                 ///< Preset Mask control structure used for Gen3/Gen4/Gen5 values
  UINT8     TargetLinkSpeed      : 3;               ///< Target Link Speed
  UINT8     BypassGen3EQ         : 1;               ///< Bypass Gen3 equalization
  UINT8     BypassGen4EQ         : 1;               ///< Bypass Gen4 equalization
  UINT8     SrisSkipInterval     : 3;               ///< Controls SRIS SKP generation interval
  UINT8     SrisEnableMode       : 4;               ///< 0:Disable 1:Enable 0xF:Auto
  UINT8     SrisAutoDetectMode   : 4;               ///< Controls SRIS Autodetect mode 0:Disable 1:Enable 0xF:Auto
  UINT8     LowerSkpOsGenSup;                       ///< Controls LOWER_SKP_OS_GEN_SUPPORT
  UINT8     LowerSkpOsRcvSup;                       ///< Controls LOWER_SKP_OS_RCV_SUPPORT
  UINT8     SrisSkpIntervalSel   : 2;               ///< Controls SRIS SKIP Interval Selection Mode
  UINT8     SrisAutodetectFactor : 2;               ///< Controls the multiplier for SKP ordered set interval when generated based on elasticity buffer pointer slip feedback from PCS
  UINT8     IsBmcLocation        : 1;               ///< IsBmcLocation
  UINT8     SetEsmControl        : 1;               ///< Set ESM Control
  UINT8     SetEsmSpeedBump      : 1;               ///< Set Speed bump for ESM
  UINT8     Reserved6            : 1;               ///< Unused bits
  UINT8     I2CMuxInfo           : 6;               ///< Legacy I2c switch
  UINT8     AlwaysExpose         : 1;               ///< Always expose unused PCIE port
  UINT8     Reserved7            : 1;               ///< Unused bits
  UINT16    NpemEnable           : 12;              ///< Controls NPEM Enable
  UINT16    Reserved8            : 4;               ///< Unused bits
  UINT16    NpemCapability       : 12;              ///< Controls NPEM Capability
  UINT8     SwingMode            : 3;               ///< PCIe Swing Mode
  UINT16    Reserved9            : 1;               ///< Unused bits
  UINT16    MpioAncDataIdx;                         ///< Reserved for internal use only
  UINT8     Reserved10;                             ///< Reserved bits
} DXIO_PORT_DATA;

///
/// EtherNet specific data structures
///
typedef struct  {
  UINT32    PortNum  : 8;                        ///< Port Number
  UINT32    PlatConf : 4;                        ///< Platform Config
                                                 ///<   0 = Reserved
                                                 ///<   1 = 10G/1G BackPlane
                                                 ///<   2 = 2.5G BackPlane
                                                 ///<   3= Soldered down 1000Base-T
                                                 ///<   4 = Soldered down 1000Base-X
                                                 ///<   5 = Soldered down NBase-T
                                                 ///<   6 = Soldered down 10GBase-T
                                                 ///<   7 = Soldered down 10GBase-r
                                                 ///<   8 = SFP+ Connector
  UINT32    Reserved1 : 4;                       ///< Unused 12-15
  UINT32    MdioId    : 5;                       ///< MDIO ID when MDIO Side band is used
  UINT32    Reserved2 : 2;                       ///< Unused 21-22
  UINT32    SuppSpeed : 4;                       ///< Supported Speeds by Platform
                                                 ///<   1 = 100M Supported
                                                 ///<   2 = 1G Supported
                                                 ///<   4 = 2.5G Supported
                                                 ///<   8 = 10G Supported
  UINT32    Reserved3 : 1;                       ///< Unused 27
  UINT32    ConnType  : 3;                       ///< Supported Speeds by Platform
                                                 ///<   0 = Port not Used
                                                 ///<   1 = SFP+ Connection I2C interface
                                                 ///<   2 = MDIO PHY
                                                 ///<   4 = Backplane Connection
  UINT32    Reserved4 : 1;                       ///< Unused 31
} ETH_PORT_PROPERTY0;

typedef struct  {
  UINT32    MdioReset        : 2;                ///< MDIO Reset Type
                                                 ///<   0 = None
                                                 ///<   1 = I2C GPIO
                                                 ///<   2 = Integrated GPIO
                                                 ///<   3 = Reserved
  UINT32    Reserved1        : 2;                ///< Unused 2-3
  UINT32    MdioGpioResetNum : 2;                ///< Integrated GPIO number for reset
  UINT32    Reserved2        : 2;                ///< Unused 6-7
  UINT32    SfpGpioAdd       : 3;                ///< Lower I2C address of GPIO Expander PCA9535
  UINT32    Reserved3        : 1;                ///< Unused 11
  UINT32    TxFault          : 4;                ///< TX FAULT
  UINT32    Rs               : 4;                ///< RS Signal
  UINT32    ModAbs           : 4;                ///< MOD_ABS signal
  UINT32    RxLoss           : 4;                ///< Rx_LOS signal
  UINT32    SfpGpioMask      : 4;                ///< SFP+ sideband signals that are not supported by platform
} ETH_PORT_PROPERTY3;

typedef struct  {
  UINT32    SfpMux            : 3;               ///< Lower address of Mux PCA 9545
  UINT32    Reserved1         : 1;               ///< Unused 3
  UINT32    SfpBusSeg         : 3;               ///< SFP BUS Segment. Downstream channels of PCA9545
  UINT32    Reserved2         : 1;               ///< Unused 7
  UINT32    SfpMuxUpAdd       : 5;               ///< Upper address of Mux PCA 9545
  UINT32    Reserved3         : 3;               ///< Unused 13-15
  UINT32    RedriverAddress   : 7;               ///< Address of ReDriver
  UINT32    RedriverInterface : 1;               ///< ReDriver Interface Descriptor
  UINT32    RedriverLane      : 3;               ///< ReDriver Lane number
  UINT32    Reserved4         : 1;               ///< Unused 27
  UINT32    RedriverModel     : 3;               ///< ReDriver Model
  UINT32    RedriverPresent   : 1;               ///< Redriver Present
} ETH_PORT_PROPERTY4;

typedef struct  {
  UINT32    TxEqPre   : 6;                       ///< TX EQ PRE
  UINT32    Reserved1 : 2;                       ///< Unused 7-6
  UINT32    TxEqMain  : 6;                       ///< TX EQ MAIN
  UINT32    Reserved2 : 2;                       ///< Unused 15-14
  UINT32    TxEqPost  : 6;                       ///< TX EQ POST
  UINT32    Reserved3 : 10;                      ///< Unused 31-23
} ETH_PORT_TXEQ;
/// Ethernet PCIe port configuration data
///
typedef struct  {
  ETH_PORT_PROPERTY0    EthPortProp0;            ///< XGBE_PORT_PROPERTY_0
  ETH_PORT_PROPERTY3    EthPortProp3;            ///< XGBE_PORT_PROPERTY_3
  ETH_PORT_PROPERTY4    EthPortProp4;            ///< XGBE_PORT_PROPERTY_4
  UINT32                PadMux0;                 ///< PadMux0 Setting (8 bits)
  UINT32                PadMux1;                 ///< PadMux1 Setting (8 bits)
  UINT32                MacAddressLo;            ///< Lower 32 bits of MAC Address
  UINT32                MacAddressHi;            ///< Upper 32 bits of MAC Address
  ETH_PORT_TXEQ         EthPortTxEq;             ///< TX EQ Settings
} ETHERNET_PORT_DATA;

///
/// High level data structures for passing topology from platform to AGESA
///
typedef struct {
  UINT8    EngineType;                           ///< Engine type
                                                 ///<   0 -  Ignore engine configuration
                                                 ///<   1 -  PCIe port
  UINT8    HotPluggable : 1;                     ///< HotPluggable
                                                 ///<   0 - Link is NOT Hot-Switchable
                                                 ///<   1 - Link IS Hot-Switchable
  UINT8    Reserved1    : 7;                     ///< Unused field, leave as 0
  UINT8    StartLane;                            ///< Start Lane ID (in reversed configuration StartLane > EndLane)
                                                 ///< Refer to lane descriptions and supported configurations in BKDG
  UINT8    EndLane;                              ///< End lane ID (in reversed configuration StartLane > EndLane)
                                                 ///< Refer to lane descriptions and supported configurations in BKDG
  UINT8    GpioGroupId;                          ///< Unique identifier for the GPIO or GPIO group associated with
                                                 ///< this engine.  GPIOs are used for hotplug notification and link
                                                 ///< type (e.g SATA Express or PCIe)
  UINT8    DxioStartLane;                        ///< Internal coding of start lane
  UINT8    DxioEndLane;                          ///< Internal coding of end lane
  UINT8    SearchDepth;                          ///< SearchDepth only uses 1 bit - always initialize to 0 will be updated dynamically
} DXIO_ENGINE_DATA;

///
/// PCIe port descriptor
///
typedef struct {
  UINT32                Flags;                   ///< Descriptor flags
                                                 ///<   Bit31 - last descriptor in complex
  DXIO_ENGINE_DATA      EngineData;              ///< Engine data
  DXIO_PORT_DATA        Port;                    ///< PCIe port specific configuration info
  ETHERNET_PORT_DATA    EtherNet;                ///< Ancillary data for EtherNet
  PHY_DATA              Phy;                     ///< Ancillary data for PHY programming customization
  PORT_PARAMS           PortParams;              ///< Extensible port parameter list for simplified topology structure
  ANC_DATA              AncData;                 ///< Ancillary data override
} DXIO_PORT_DESCRIPTOR;

#pragma pack(pop)

///
///
/// PCIe Complex descriptor
///
typedef struct {
  UINT32                  Flags;                 ///< Descriptor flags
                                                 ///<   Bit31 - last descriptor in topology
  UINT32                  SocketId;              ///< Socket Id
  DXIO_PORT_DESCRIPTOR    *PciePortList;         ///< Pointer to array of PCIe port descriptors or NULL (Last element of array must be terminated with DESCRIPTOR_TERMINATE_LIST).
  VOID                    *Reserved2;            ///< Reserved for future use
  UINT8                   BmcLinkLocation;       ///< Identifies the socket/die location of a BMC link (Used by AGESA, input not required)
  UINT8                   BmcLinkLaneNum;        ///< Identifies the socket/die location of a BMC Lane number
  UINT8                   Reserved3[2];          ///< Reserved for future
} DXIO_COMPLEX_DESCRIPTOR;

///
/// Engine descriptor type
///
typedef enum {
  DxioUnusedEngine = 0,                                   ///< Unused descriptor Excluded from configuration
  DxioPcieEngine   = 1,                                   ///< PCIe port
  DxioUSBEngine    = 2,                                   ///< USB port
                                                          ///< __Deprecated__
  DxioSATAEngine     = 3,                                 ///< SATA
  DxioUSB_OVER_PCIE  = 4,                                 ///< USB4 PCIe (internal use only)
  DxioUBMHFCEngine   = 5,                                 ///< New for Genoa UBM HFC Connector for auto-discovery
  DxioOCP3Engine     = 6,                                 ///< New for Genoa OCP3 Bifurcatable Connector
  DxioUdot3Engine    = 7,                                 ///< New for Genoa U.3 Multiprotocol Connector
  DxioDPEngine       = 8,                                 ///< Digital Display __For APU display connector list__
  DxioEthernetEngine = 0x10,                              ///< Ethernet (GBe, XGBe)
                                                          ///< __Deprecated__
  DxioGOPEngine = 0x20,                                   ///< GOP
                                                          ///< __Deprecated__
  DxioNTBDevice = 0x60,                                   ///< For NBIF NTB Enable (internal use only)
  DxioHDaudioEngine,                                      ///< For NBIF HDaudtio Enable (internal use only)
  DxioACPEngine,                                          ///< For NBIF ACP Enable (internal use only)
  DxioMP2Engine,                                          ///< For NBIF MP2 Enable (internal use only)
  DxioMaxPcieEngine                                       ///< Max engine type for boundary check.
} DXIO_ENGINE_TYPE;

///
/// PCIe link speeds
///
typedef enum  {
  DxioGenMaxSupported,                                    ///< Maximum supported
  DxioGen1 = 1,                                           ///< Gen1
  DxioGen2,                                               ///< Gen2
  DxioGen3,                                               ///< Gen3
  DxioGen4,                                               ///< Gen4
  DxioGen5,                                               ///< Gen5
  MaxDxioGen                                              ///< Max Gen for boundary check
} DXIO_LINK_SPEEDS;

/**
 * @brief  PCIe link initialization
 * @details NOTE: UBM HFC hotplug types are identified during auto-discovery
 */
typedef enum {
  DxioPortDisabled,                                           ///< Disable
  DxioPortEnabled                                             ///< Enable
} DXIO_PORT_ENABLE;

#endif // AMD_PCIE_COMPLEX_H_
