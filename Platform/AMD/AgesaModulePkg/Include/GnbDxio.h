/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GNBDXIO_H_
#define GNBDXIO_H_

#pragma pack (push, 1)

#include <AGESA.h>
#include <Gnb.h>

#define MAX_NUMBER_OF_COMPLEXES  16

#define DESCRIPTOR_TERMINATE_GNB       0x40000000ull
#define DESCRIPTOR_TERMINATE_TOPOLOGY  0x20000000ull
#define DESCRIPTOR_ALLOCATED           0x10000000ull
#define DESCRIPTOR_PLATFORM            0x08000000ull
#define DESCRIPTOR_COMPLEX             0x04000000ull
#define DESCRIPTOR_SILICON             0x02000000ull
#define DESCRIPTOR_PCIE_WRAPPER        0x01000000ull
#define DESCRIPTOR_PCIE_ENGINE         0x00800000ull
#define DESCRIPTOR_CXL_ENGINE          0x00200000ull

#define SILICON_CXL_CAPABLE  0x00008000ull

#define DESCRIPTOR_ALL_WRAPPERS  (DESCRIPTOR_PCIE_WRAPPER)
#define DESCRIPTOR_ALL_ENGINES   (DESCRIPTOR_PCIE_ENGINE | DESCRIPTOR_CXL_ENGINE)

#define DESCRIPTOR_ALL_TYPES  (DESCRIPTOR_ALL_WRAPPERS | DESCRIPTOR_ALL_ENGINES | DESCRIPTOR_SILICON | DESCRIPTOR_PLATFORM)
#define PcieLibGetNextDescriptor(Descriptor)  ((Descriptor == NULL) ? NULL : ((Descriptor->Header.DescriptorFlags & DESCRIPTOR_TERMINATE_LIST) != 0) ? NULL : (Descriptor + 1))

#define SILICON_CXL_CAPABLE  0x00008000ull

typedef UINT16 PCIE_ENGINE_INIT_STATUS;

///
/// Engine Configuration
///
typedef struct {
  IN       UINT8     EngineType;                          ///< Engine type
                                                          ///<   0 -  Ignore engine configuration
                                                          ///<   1 -  PCIe port
  IN       UINT16    StartLane;                           ///< Start Lane ID (in reversed configuration StartLane > EndLane)
                                                          ///< Refer to lane descriptions and supported configurations in BKDG
  IN       UINT16    EndLane;                             ///< End lane ID (in reversed configuration StartLane > EndLane)
                                                          ///< Refer to lane descriptions and supported configurations in BKDG
} PCIE_ENGINE;

///
/// PCIe port misc extended controls
///
typedef struct  {
  IN      UINT8    SbLink       : 1;                        ///< PCIe link type
                                                            ///<   0 - General purpose port
                                                            ///<   1 - Port connected to SB
  IN      UINT8    ClkPmSupport : 1;                        ///< Clock Power Management Support
                                                            ///<   0 - Clock Power Management not configured
                                                            ///<   1 - Clock Power Management configured according to PCIe device capability
  IN      UINT8    CsLink       : 1;                        ///< PCIe link type
                                                            ///<   0 - General purpose port
                                                            ///<   1 - Port connected to chipset
  IN      UINT8    Reserved0    : 5;                        ///< Unused space
} PORT_MISC_CONTROL;

///
/// PCIe port configuration data
///
typedef struct  {
  IN       UINT8                PortPresent;                 ///< Enable PCIe port for initialization.
  IN       UINT8                FunctionNumber      : 3;     ///< Reserved for future use
  IN       UINT8                DeviceNumber        : 5;     ///< PCI Device number for port.
                                                             ///<   0 - Native port device number
                                                             ///<   N - Port device number (See available configurations in BKDG
  IN       UINT8                LinkSpeedCapability : 4;     ///< PCIe link speed/
                                                             ///<   0 - Maximum supported by silicon
                                                             ///<   1 - Gen1
                                                             ///<   2 - Gen2
                                                             ///<   3 - Gen3
  IN       UINT8                LinkAspm            : 4;     ///< ASPM control. (see AgesaPcieLinkAspm for additional option to control ASPM)
                                                             ///<   0 - Disabled
                                                             ///<   1 - L0s only
                                                             ///<   2 - L1 only
                                                             ///<   3 - L0s and L1
  IN       UINT8                LinkHotplug;                 ///< Hotplug control.
                                                             ///<   0 - Disabled
                                                             ///<   1 - Basic
                                                             ///<   2 - Server
                                                             ///<   3 - Enhanced
  IN       UINT16               SlotNum;                     ///< Physical Slot Number
  IN       PORT_MISC_CONTROL    MiscControls;                ///< Misc extended controls
  IN       UINT8                Reserved1;                   ///< Reserved for future use
} PORT_DATA;

typedef struct {
  UINT8    LinkSpeed;
  UINT8    MaxPayloadSupport;
  UINT8    AspmCapability;
  UINT8    PciPmL1_1;
  UINT8    PciPmL1_2;
  UINT8    AspmL1_1;
  UINT8    AspmL1_2;
  UINT8    EsmSupport;
  UINT8    LtrSupport;
  UINT8    SurpriseDownErrorReport;
  UINT8    TenBitTagSupport;
  UINT8    AriForwarding;
  UINT8    AcsSupport;
  UINT8    AcsSourceValidation;
  UINT8    AcsTranslationBlocking;
  UINT8    AcsP2pRequestRedirect;
  UINT8    AcsP2pCompletionRedirect;
  UINT8    AcsUpstreamForwarding;
  UINT8    AcsP2pEgressControl;
  UINT8    AcsDirectTranslatedP2p;
  UINT8    LaneMargining;
  UINT8    DataLinkFeature;
  UINT8    DownstreamPortContainment;
  UINT8    AdvancedErrorReporting;
  UINT8    ECRCSupport;
  UINT8    MulticastEnable;
  UINT8    NativePCIeEnclosureManagement;
  UINT8    Capability1Address;
  UINT8    Capability1Data;
  UINT8    Capability2Address;
  UINT8    Capability2Data;
  UINT8    Capability3Address;
  UINT8    Capability3Data;
  UINT8    Capability4Address;
  UINT8    Capability4Data;
} PORT_CAPABILITIES;

///
/// PCIe PORT_FEATURES
///
typedef struct {
  UINT8    LinkSpeedControl;
  UINT8    MaxPayloadSizeControl;
  UINT8    ESMControl;
  UINT8    LTRControl;
  UINT8    DataLinkFeatureExchangeControl;
  UINT8    TenBitTagControl;
  UINT8    ARIControl;
  UINT8    ACSControl;
  UINT8    RxLaneMarginingControl;
  UINT8    DynLanesPwrState;
  UINT8    L1PowerDown;
  UINT8    L11PowerDown;
  UINT8    L12PowerDown;
  UINT8    AutoSpdChngEn;
  UINT8    TurnOffUnusedLanes;
} PORT_FEATURES;

typedef struct {
  UINT8    SpcGen1 : 1;                                 ///< SPC Mode 2P5GT
  UINT8    SpcGen2 : 1;                                 ///< SPC Mode 5GT
  UINT8    SpcGen3 : 2;                                 ///< SPC Mode 8GT
  UINT8    SpcGen4 : 2;                                 ///< SPC Mode 16GT
  UINT8    SpcGen5 : 2;                                 ///< SPC Mode 32GT
} SPC_MODE;

typedef struct {
  UINT32    DsTxPreset      : 4;                        ///< Gen3 Downstream Tx Preset
  UINT32    DsRxPresetHint  : 3;                        ///< Gen3 Downstream Rx Preset Hint
  UINT32    UsTxPreset      : 4;                        ///< Gen3 Upstream Tx Preset
  UINT32    UsRxPresetHint  : 3;                        ///< Gen3 Upstream Rx Preset Hint
  UINT32    LcPresetMask8Gt : 10;                       ///< Gen3 Preset Mask
  UINT32    LcFapeEnable8GT : 1;                        ///< Gen3 FapeEnable
  UINT32    UNUSED2         : 7;                        ///< Currently unassigned - for alignment
} GEN3_LANE_CNTL;

typedef struct {
  UINT32    DsTxPreset       : 4;                        ///< Gen4 Downstream Tx Preset
  UINT32    UsTxPreset       : 4;                        ///< Gen4 Upstream Tx Preset
  UINT32    LcPresetMask16Gt : 10;                       ///< Gen4 Preset Mask
  UINT32    LcFapeEnable16GT : 1;                        ///< Gen4 FapeEnable
  UINT32    UNUSED3          : 13;                       ///< Currently unassigned - for alignment
} GEN4_LANE_CNTL;

typedef struct {
  UINT32    DsTxPreset          : 4;                    ///< Gen5 Downstream Tx Preset
  UINT32    UsTxPreset          : 4;                    ///< Gen5 Upstream Tx Preset
  UINT32    LcPresetMask32Gt    : 10;                   ///< Gen5 Preset Mask
  UINT32    LcFapeEnable32GT    : 1;                    ///< Gen5 FapeEnable
  UINT32    PrecodeRequest      : 1;                    ///< Precoding Request
  UINT32    AdvertiseEqToHiRate : 1;                    ///< Advertise EQ To High Rate Support
  UINT32    UNUSED4             : 11;                   ///< Currently unassigned - for alignment
} GEN5_LANE_CNTL;

typedef struct {
  UINT32    LcFapeReqPostCursor0 : 5;                   ///< PostCursor0
  UINT32    LcFapeReqPreCursor0  : 4;                   ///< PreCursor0
  UINT32    LcFapeReqPostCursor1 : 5;                   ///< PostCursor1
  UINT32    LcFapeReqPreCursor1  : 4;                   ///< PreCursor1
  UINT32    LcFapeReqPostCursor2 : 5;                   ///< PostCursor2
  UINT32    LcFapeReqPreCursor2  : 4;                   ///< PreCursor2
  UINT32    UNUSED6              : 5;                   ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_0;

typedef struct {
  UINT32    LcFapeReqPostCursor3 : 5;                   ///< PostCursor3
  UINT32    LcFapeReqPreCursor3  : 4;                   ///< PreCursor3
  UINT32    LcFapeReqPostCursor4 : 5;                   ///< PostCursor4
  UINT32    LcFapeReqPreCursor4  : 4;                   ///< PreCursor4
  UINT32    LcFapeReqPostCursor5 : 5;                   ///< PostCursor5
  UINT32    LcFapeReqPreCursor5  : 4;                   ///< PreCursor5
  UINT32    UNUSED7              : 5;                   ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_1;

typedef struct {
  UINT32    LcFapeReqPostCursor6 : 5;                   ///< PostCursor6
  UINT32    LcFapeReqPreCursor6  : 4;                   ///< PreCursor6
  UINT32    LcFapeReqPostCursor7 : 5;                   ///< PostCursor7
  UINT32    LcFapeReqPreCursor7  : 4;                   ///< PreCursor7
  UINT32    LcFapeReqPostCursor8 : 5;                   ///< PostCursor8
  UINT32    LcFapeReqPreCursor8  : 4;                   ///< PreCursor8
  UINT32    UNUSED8              : 5;                   ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_2;

typedef struct {
  UINT32    LcFapeReqPostCursor9  : 5;                  ///< PostCursor9
  UINT32    LcFapeReqPreCursor9   : 4;                  ///< PreCursor9
  UINT32    LcFapeReqPostCursor10 : 5;                  ///< PostCursor10
  UINT32    LcFapeReqPreCursor10  : 4;                  ///< PreCursor10
  UINT32    LcFapeReqPostCursor11 : 5;                  ///< PostCursor11
  UINT32    LcFapeReqPreCursor11  : 4;                  ///< PreCursor11
  UINT32    UNUSED9               : 5;                  ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_3;

typedef struct {
  UINT32    LcFapeReqPostCursor12 : 5;                  ///< PostCursor12
  UINT32    LcFapeReqPreCursor12  : 4;                  ///< PreCursor12
  UINT32    LcFapeReqPostCursor13 : 5;                  ///< PostCursor13
  UINT32    LcFapeReqPreCursor13  : 4;                  ///< PreCursor13
  UINT32    LcFapeReqPostCursor14 : 5;                  ///< PostCursor14
  UINT32    LcFapeReqPreCursor14  : 4;                  ///< PreCursor14
  UINT32    UNUSED10              : 5;                  ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_4;

typedef struct {
  UINT32    LcFapeReqPostCursor15 : 5;                  ///< PostCursor15
  UINT32    LcFapeReqPreCursor15  : 4;                  ///< PreCursor15
  UINT32    LcFapeReqPostCursor16 : 5;                  ///< PostCursor16
  UINT32    LcFapeReqPreCursor16  : 4;                  ///< PreCursor16
  UINT32    LcFapeReqPostCursor17 : 5;                  ///< PostCursor17
  UINT32    LcFapeReqPreCursor17  : 4;                  ///< PreCursor17
  UINT32    UNUSED11              : 5;                  ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_5;

typedef struct {
  UINT32    LcFapeReqPostCursor18 : 5;                  ///< PostCursor18
  UINT32    LcFapeReqPreCursor18  : 4;                  ///< PreCursor18
  UINT32    LcFapeReqPostCursor19 : 5;                  ///< PostCursor19
  UINT32    LcFapeReqPreCursor19  : 4;                  ///< PreCursor19
  UINT32    LcFapeReqPostCursor20 : 5;                  ///< PostCursor20
  UINT32    LcFapeReqPreCursor20  : 4;                  ///< PreCursor20
  UINT32    UNUSED12              : 5;                  ///< Currently unassigned - for alignment
} LC_FAPE_GROUP_6;

///
/// PCIe port configuration info
///
typedef struct {
  PORT_DATA            PortData;                        ///< Port data
  UINT8                StartCoreLane;                   ///< Start Core Lane
  UINT8                EndCoreLane;                     ///< End Core lane
  UINT8                NativeDevNumber : 5;             ///< Native PCI device number of the port
  UINT8                NativeFunNumber : 3;             ///< Native PCI function number of the port
  UINT8                CoreId          : 4;             ///< PCIe core ID
  UINT8                PortId          : 4;             ///< Port ID on wrapper
  PCI_ADDR             Address;                         ///< PCI address of the port
  UINT8                PcieBridgeId    : 7;             ///< IOC PCIe bridge ID
  UINT8                IsBmcLocation   : 1;             ///< Port Location of BMC
  UINT8                LogicalBridgeId;                 ///< Logical Bridge ID
  UINT8                SlotPowerLimit;                  ///< Slot Power Limit
  UINT8                MaxPayloadSize;                  ///< Max_Payload_Size

  UINT8                TXDeEmphasis    : 4;             ///< TX De-emphasis
  UINT8                TXMargin        : 3;             ///< TX Margin
  UINT8                UNUSED1         : 1;             ///< Currently unassigned - for alignment

  PORT_CAPABILITIES    PortCapabilities;                ///< Port Capabilities CBS

  SPC_MODE             SpcMode;

  GEN3_LANE_CNTL       LaneEqualizationCntl;
  GEN4_LANE_CNTL       Gen4LaneEqualizationCntl;
  GEN5_LANE_CNTL       Gen5LaneEqualizationCntl;

  UINT8                LowerSkpOsGenSup;                ///< Controls LOWER_SKP_OS_GEN_SUPPORT
  UINT8                LowerSkpOsRcvSup;                ///< Controls LOWER_SKP_OS_RCV_SUPPORT
  UINT8                SrisSkipInterval     : 3;        ///< Controls SRIS SKP generation interval
  UINT8                SrisSkpIntervalSel   : 2;        ///< Controls SRIS SKIP Interval Selection Mode
  UINT8                SrisAutodetectFactor : 2;        ///< Controls the multiplier for SKP ordered set interval when generated based on elasticity buffer pointer slip feedback from PCS
  UINT8                UNUSED4              : 1;        ///< Currently unassigned - for alignment
  UINT8                SRIS_SRNS            : 1;        ///< SRIS SRNS
  UINT8                SRIS_LowerSKPSupport : 1;        ///< SRIS Lower SKP Support
  UINT8                CcixControl          : 1;        ///< Bit to enable/disable ESM
  UINT8                CxlControl           : 1;        ///< Bit to enable CXL Capability
  UINT8                AlwaysExpose         : 1;        ///< Always expose unused PCIE port
  UINT8                SlotPowerLimitScale  : 2;        ///< Slot Power Limit Scale
  UINT8                UNUSED5              : 1;        ///< Currently unassigned - for alignment

  UINT8                RxMarginPersistence  : 1;        ///< Bit to enable/disable Rx Margin persistence mode
  UINT8                SetGen3FixedPreset   : 1;        ///< Gen3 Fixed Preset Set
  UINT8                SetGen4FixedPreset   : 1;        ///< Gen4 Fixed Preset Set
  UINT8                SetGen5FixedPreset   : 1;        ///< Gen5 Fixed Preset Set
  UINT8                TxVetting            : 1;        ///< Gen4 Tx Vetting
  UINT8                RxVetting            : 1;        ///< Gen4 Rx Vetting
  UINT8                TxVettingGen5        : 1;        ///< Gen5 Tx Vetting
  UINT8                RxVettingGen5        : 1;        ///< Gen5 Rx Vetting

  UINT8                IsMasterPLL          : 1;        ///< IsMasterPLL
  UINT8                TargetLinkSpeed      : 3;        ///< Target Link Speed
  UINT8                DlfCapDisable        : 1;        ///< DLF Capability 1:Disable 0:Enable
  UINT8                DlfExchangeDisable   : 1;        ///< DLF Exchange 1:Disable 0:Enable
  UINT8                InvertPolarity       : 1;        ///< Invert RX Polarity
  UINT8                InvertPolarity2      : 1;        ///< Invert TX Polarity

  UINT8                EqSearchMode         : 2;        ///< Equalization Search Mode
  UINT8                BypassGen3EQ         : 1;        ///< BypassGen3EQ
  UINT8                DisGen3EQPhase       : 1;        ///< Disable Gen3 EQ Phase2/3
  UINT8                Gen3FixedPreset      : 4;        ///< Gen3 Fixed Preset value

  UINT8                EqSearchModeGen4     : 2;        ///< Equalization Search Mode for Gen4
  UINT8                BypassGen4EQ         : 1;        ///< Gen4 Bypass phase3 EQ
  UINT8                DisGen4EQPhase       : 1;        ///< Gen4 Bypass phase2/3 EQ
  UINT8                Gen4FixedPreset      : 4;        ///< Gen4 Fixed Preset value
  UINT8                EqSearchModeGen5     : 2;        ///< Equalization Search Mode for Gen5
  UINT8                BypassGen5EQ         : 1;        ///< Gen5 Bypass phase3 EQ
  UINT8                DisGen5EQPhase       : 1;        ///< Gen5 Bypass phase2/3 EQ
  UINT8                Gen5FixedPreset      : 4;        ///< Gen5 Fixed Preset value

  UINT16               PsppPolicyDC;                    ///< Pspp Policy DC
  UINT16               PsppPolicyAC;                    ///< Pspp Policy AC
  UINT8                PsppDeviceType;                  ///< Pspp Device Type

  LC_FAPE_GROUP_0      LcFapeSettingsGroup0;
  LC_FAPE_GROUP_1      LcFapeSettingsGroup1;
  LC_FAPE_GROUP_2      LcFapeSettingsGroup2;
  LC_FAPE_GROUP_3      LcFapeSettingsGroup3;
  LC_FAPE_GROUP_4      LcFapeSettingsGroup4;
  LC_FAPE_GROUP_5      LcFapeSettingsGroup5;
  LC_FAPE_GROUP_6      LcFapeSettingsGroup6;

  UINT8                ForceSteering      : 1;          ///< Steering is forced
  UINT8                EsmUsTxPreset      : 4;          ///< ESM Upstream Tx Preset
  UINT8                UNUSED13           : 3;          ///< Currently unassigned - for alignment

  // Used by DXE
  PORT_FEATURES        PortFeatures;                    ///< Port Features CBS
  UINT8                EsmSpeedBump;                    ///< Speed bump for ESM
  UINT8                I2CMuxInfo;                      ///< First I2c Mux on Bus
  UINT8                SrisEnableMode     : 4;          ///< 0:Disable 1:SRIS 2:SRNS 3:SRNS in SRIS 0xF:Auto
  UINT8                SrisAutoDetectMode : 4;          ///< Controls SRIS Autodetect mode 0:Disable 1:Enable 0xF:Auto
  UINT8                ClkReq             : 4;          ///< ClkReq:[0:3]
  UINT8                EqPreset           : 4;          ///< EqPreset:[4:7]
  UINT8                LinkAspmL1_1       : 1;          ///< Enable PM L1 SS L1.1
  UINT8                LinkAspmL1_2       : 1;          ///< Enable PM L1 SS L1.2
  UINT8                EsmControl         : 1;          ///< Bit to enable/disable ESM
  UINT8                EsmDsTxPreset      : 4;          ///< ESM Downstream Tx Preset
  UINT8                ClkReqFilterEn     : 1;          ///< Controls filtering of CLKREQb signal in LC in order to avoid false L1 substate entries/exits.
} PCIE_PORT_CONFIG;

///
/// CXL port configuration info
///
typedef struct {
  PORT_DATA    PortData;                                ///< Port data
  UINT8        StartCoreLane;                           ///< Start Core Lane
  UINT8        EndCoreLane;                             ///< End Core lane
  UINT8        NativeDevNumber   : 5;                   ///< Native PCI device number of the port
  UINT8        NativeFunNumber   : 3;                   ///< Native PCI function number of the port
  UINT8        CoreId            : 4;                   ///< PCIe core ID
  UINT8        PortId            : 4;                   ///< Port ID on wrapper
  PCI_ADDR     Address;                                 ///< PCI address of the port
  UINT8        PcieBridgeId      : 7;                   ///< IOC PCIe bridge ID
  UINT8        UNUSED0           : 1;                   ///< Currently unassigned - for alignment
  UINT8        LogicalBridgeId;                         ///< Logical Bridge ID
  UINT8        SlotPowerLimit;                          ///< Slot Power Limit
  UINT8        MaxPayloadSize;                          ///< Max_Payload_Size

  UINT8        CxlIndex;
  UINT8        CxlDeviceType     : 2;                   ///< Type of CXL device connected
  UINT8        CxlVersion        : 2;                   ///< Version of CXL device connected (1=CXL1.1, 2=CXL2.0)
  UINT8        IsCxlScanned      : 1;                   ///< Indicates if the CXL device has been scanned
  UINT8        ReportToMpioinDxe : 1;                   ///< Indicates if the CXL info needs to be reported to MPIO in DXE
  UINT8        UNUSED1           : 2;                   ///< Currently unassigned - for alignment

  UINT32       UsRcrb;                                  ///< Upstream Port RCRB address
  UINT32       DsRcrb;                                  ///< Downstream Port RCRB address
  UINT32       UsMemBar0;                               ///< Upstream port MEMBAR0
  UINT32       DsMemBar0;                               ///< Downstream port MEMBAR0
  UINT32       Mmio32Base;
  UINT32       Mmio32Size;
  UINT64       Mmio64Base;
  UINT64       Mmio64Size;
  UINT32       Mmio32Gran;
} PCIE_CXL_CONFIG;

///
/// Descriptor header
///
typedef struct {
  UINT32    DescriptorFlags;                            ///< Descriptor flags
  UINT16    Parent;                                     ///< Offset of parent descriptor
  UINT16    Peer;                                       ///< Offset of the peer descriptor
  UINT16    Child;                                      ///< Offset of the list of child descriptors
} PCIE_DESCRIPTOR_HEADER;

///
/// Engine configuration data
///
typedef struct {
  PCIE_DESCRIPTOR_HEADER     Header;                    ///< Descriptor header
  PCIE_ENGINE                EngineData;                ///< Engine Data
  PCIE_ENGINE_INIT_STATUS    InitStatus;                ///< Initialization Status
  UINT8                      Scratch;                   ///< Scratch pad
  union {
    PCIE_PORT_CONFIG    Port;                           ///< PCIe port configuration data
    PCIE_CXL_CONFIG     Cxl;                            ///< CXL Configuration data
  } Type;
} PCIE_ENGINE_CONFIG;

///
/// Wrapper configuration data
///
typedef struct {
  PCIE_DESCRIPTOR_HEADER    Header;                     ///< Descriptor Header
  UINT8                     WrapId;                     ///< Wrapper ID
  UINT8                     CcixCoreConfig;             ///< Ccix CORE Configuration
  UINT8                     StartPhyLane;               ///< Start PHY Lane
  UINT8                     EndPhyLane;                 ///< End PHY Lane
  UINT8                     StartDxioLane;              ///< Start Dxio Lane (Translated)
  UINT8                     EndDxioLane;                ///< End Dxio Lane (Translated)
  struct {
    UINT8    PowerOffUnusedLanes     : 1;               ///< Power Off unused lanes
    UINT8    PowerOffUnusedPlls      : 1;               ///< Power Off unused Plls
    UINT8    ClkGating               : 1;               ///< TXCLK gating
    UINT8    LclkGating              : 1;               ///< LCLK gating
    UINT8    TxclkGatingPllPowerDown : 1;               ///< TXCLK clock gating PLL power down
    UINT8    PllOffInL1              : 1;               ///< PLL off in L1
    UINT8    AccessEncoding          : 1;               ///< Reg access encoding
    UINT8    CoreReversed            : 1;               ///< Indicates lanes are reversed in package connection
  } Features;
  UINT8     MasterPll;                                  ///< Bitmap of master PLL
  UINT32    AcsSupport                    : 1;          ///< Acs Support
  UINT32    LtrSupport                    : 1;          ///< LTR Support
  UINT32    AriForwarding                 : 1;          ///< ARI Forwarding
  UINT32    LaneMargining                 : 1;          ///< Lane Margining
  UINT32    NativePCIeEnclosureManagement : 1;          ///< NPEM
  UINT32    DownstreamPortContainment     : 1;          ///< Downstream port containment
  UINT32    AdvancedErrorReporting        : 1;          ///< Advacned Error Reporting
  UINT32    ECRCSupport                   : 2;          ///< ECRC Capability
  UINT32    Reserved                      : 23;         ///< Reserved bits
} PCIE_WRAPPER_CONFIG;

///
/// Silicon configuration data
///
typedef struct  {
  PCIE_DESCRIPTOR_HEADER    Header;                     ///< Descriptor Header
  UINT8                     SocketId;                   ///< Socket ID
  UINT8                     DieNumber;                  ///< Module ID
  UINT8                     RBIndex;                    ///< Physical Root Bridge
  UINT8                     InstanceId;                 ///< Logical Instance Identifier
  PCI_ADDR                  Address;                    ///< PCI address of GNB host bridge
  UINT16                    StartLane;                  ///< Start Lane of this node
  UINT16                    EndLane;                    ///< End Lane of this node
  UINT8                     BusNumberLimit;             ///< Last Bus Number assigned to this node
  UINT8                     SbPresent   : 1;            ///< Set to 1 if FCH connected to this NBIO
  UINT8                     SmuPresent  : 1;            ///< Set to 1 if SMU connected to this NBIO
  UINT8                     MP_Instance : 6;            ///< MP Instance
  UINT8                     LogicalRBIndex;             ///< Logical Root Bridge
  UINT8                     NumEngineDesc;              ///< Total number of lane bifurcation descriptors
} PCIE_SILICON_CONFIG;

typedef PCIE_SILICON_CONFIG GNB_HANDLE;

///
/// Complex configuration data
///
typedef struct {
  PCIE_DESCRIPTOR_HEADER    Header;                     ///< Descriptor Header
  UINT8                     NodeId;                     ///< Processor Node ID
  UINT8                     Reserved;                   ///< For alignment
} PCIE_COMPLEX_CONFIG;

///
/// PCIe platform configuration info
///
typedef struct {
  PCIE_DESCRIPTOR_HEADER    Header;                               ///< Descriptor Header
  PVOID                     Reserved1;                            ///< Reserved
  UINT32                    Reserved2;                            ///< Reserved
  UINT32                    PhyConfigData;                        ///< Phy Configuration Data
  UINT32                    Reserved3;                            ///< Reserved
  UINT32                    Reserved4;                            ///< Reserved
  UINT32                    PsppTuningParams;                     ///< Tuning parameters for PSPP
  UINT32                    PsppTuningParams2;                    ///< Tuning parameters 2 for PSPP
  UINT8                     Reserved5;                            ///< Reserved
  UINT8                     PsppPolicy;                           ///< PSPP policy
  UINT8                     Reserved6;                            ///< Reserved
  UINT8                     RootBridgesPerSocket;                 ///< Number of root bridges per socket
  PCIE_COMPLEX_CONFIG       ComplexList[MAX_NUMBER_OF_COMPLEXES]; ///< Complex
} PCIE_PLATFORM_CONFIG;

///
/// PCIe Engine Description
///
typedef struct {
  UINT32         Flags;                                 ///< Descriptor flags
                                                        ///<   Bit31 - last descriptor on wrapper
                                                        ///<   Bit30 - Descriptor allocated for PCIe port
  PCIE_ENGINE    EngineData;                            ///< Engine Data
} PCIE_ENGINE_DESCRIPTOR;
#pragma pack (pop)

#endif // GNBDXIO_H_
