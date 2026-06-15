/** @file
  Public data types for the Qualcomm Shared Memory (SMEM) subsystem.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - ADSP   - Audio Digital Signal Processor
    - AARM   - Application Processor ARM (Current boot core - akin to BSP in PI spec)
    - BAM    - Bus Access Manager
    - CDSP   - Compute Digital Signal Processor
    - CPR    - Core Power Reduction
    - DAL    - Device Abstraction Layer
    - DCP    - Data Compression Processor
    - DSP    - Digital Signal Processor
    - EFS    - Embedded File System
    - GLINK  - Generic Link
    - GPDSP  - General Purpose DSP
    - HLOS   - High Level Operating System
    - IPA    - IP Accelerator
    - IPC    - Inter-Processor Communication
    - LA     - Linux Android
    - LC     - Linux Client
    - LPASS  - Low Power Audio Sub-System
    - MSS    - Modem Sub-System
    - MPROC  - Multi-processor
    - NPU    - Neural Processing Unit
    - NSP    - Network Sub-Processor
    - OOB    - Out of Band
    - OSSRRC - ASN.1 RRC decode/encode working memory between the Modem (MPSS) and another processor
    - QECP   - Qualcomm Enhanced Compute Processor
    - RPM    - Resource Power Manager
    - SEFS   - Secure Embedded File System
    - SMEM   - Shared Memory
    - SMD    - Shared Memory Driver
    - SMP2P  - Shared Memory Point to Point
    - SMSM   - Shared Memory State Machine
    - SOCCP  - System on Chip Compute Processor
    - SPSS   - Secure Processor Sub-System
    - SSC    - Sensor Sub-System Compute
    - SSR    - Sub-System Restart
    - TEE    - Trusted Execution Environment
    - TME    - Trusted Memory Expansion
    - TZ     - TrustZone
    - VM     - Virtual Machine
    - WCN    - Wireless Connectivity Network
    - WPSS   - Wireless Processor Sub-System
    - XBL    - eXtensible Boot Loader
**/

#pragma once

#include <Base.h>

/**
  The most significant two bytes of SMEM_VERSION_ID are the SMEM major version
  and the least significant two bytes are the SMEM minor version. The major
  version must be updated whenever an incompatible change is introduced.

  The minor version may track API changes and deprecations that do not affect
  remote processors, including changes to the SMEM_MEM_TYPE enum when
  dependencies have already been satisfied on the relevant processors.

  Minor version inconsistencies between processors will not prevent SMEM from
  booting, but major version inconsistencies will.
**/

#define SMEM_VERSION_ID  0x000C0000

#define SMEM_MAJOR_VERSION_MASK  0xFFFF0000
#define SMEM_MINOR_VERSION_MASK  0x0000FFFF
#define SMEM_FULL_VERSION_MASK   0xFFFFFFFF

#define SMEM_NUM_SMD_CHANNELS     64
#define SMEM_NUM_SMP2P_EDGES       8
#define SMEM_NUM_VERSION_ENTRIES  24

/** Types of memory that can be requested via SmemAlloc.

  SmemVersionFirst and SmemVersionLast are the first and last
  boundaries for external version checking via the SmemVersionSet routine.

  SmemVersionLast need not be the last item in the enum.
**/
typedef enum {
  SmemMemFirst                           =   0,
  SmemReservedProcComm                   =   0,  /* SmemMemFirst */
  SmemFirstFixedBuffer                   =   0,  /* SmemReservedProcComm */
  SmemHeapInfo                           =   1,
  SmemAllocationTable                    =   2,
  SmemVersionInfo                        =   3,
  SmemHwResetDetect                      =   4,
  SmemReservedAarmWarmBoot               =   5,
  SmemDiagErrMessage                     =   6,
  SmemSpinlockArray                      =   7,
  SmemMemoryBarrierLocation              =   8,
  SmemLastFixedBuffer                    =   8,  /* SmemMemoryBarrierLocation */
  SmemAarmPartitionTable                 =   9,
  SmemAarmBadBlockTable                  =  10,
  SmemErrCrashLogAdsp                    =  11,
  SmemBootBoardInfo                      =  12,
  SmemChannelAllocTbl                    =  13,
  SmemSmdBaseId                          =  14,
  SmemSmemLogIdx                         = SmemSmdBaseId + SMEM_NUM_SMD_CHANNELS,
  SmemSmemLogEvents                      =  79,
  SmemReservedSmemStaticLogIdx           =  80,
  SmemXblLoaderCoreInfo                  =  80,  /* SmemReservedSmemStaticLogIdx */
  SmemReservedSmemStaticLogEvents        =  81,
  SmemChargerBatteryInfo                 =  81,  /* SmemReservedSmemStaticLogEvents */
  SmemReservedSmemSlowClockSync          =  82,
  SmemWlanConfig                         =  82,  /* SmemReservedSmemSlowClockSync */
  SmemReservedSmemSlowClockValue         =  83,
  SmemMePlayback                         =  83,  /* SmemReservedSmemSlowClockValue */
  SmemReservedBioLedBuf                  =  84,
  SmemSmsmSharedState                    =  85,
  SmemReservedSmsmIntInfo                =  86,
  SmemReservedSmsmSleepDelay             =  87,
  SmemReservedSmsmLimitSleep             =  88,
  SmemReservedSleepPowerCollapseDisabled =  89,
  SmemReservedKeypadKeysPressed          =  90,
  SmemReservedKeypadStateUpdated         =  91,
  SmemReservedKeypadStateIdx             =  92,
  SmemReservedGpioInt                    =  93,
  SmemIdSmp2pBaseCdsp                    =  94,
  SmemReservedSmdProfiles                = SmemIdSmp2pBaseCdsp + SMEM_NUM_SMP2P_EDGES,
  SmemReservedTsscBusy                   = 103,
  SmemReservedHsSuspendFilterInfo        = 104,
  SmemReservedBattInfo                   = 105,
  SmemReservedAppsBootMode               = 106,
  SmemVersionFirst                       = 107,
  SmemVersionSmd                         = 107,  /* SmemVersionFirst */
  SmemVersionSmdBridge                   = 108,
  SmemVersionSmsm                        = 109,
  SmemVersionSmdNwayLoop                 = 110,
  SmemVersionLast                        = SmemVersionFirst + SMEM_NUM_VERSION_ENTRIES,
  SmemReservedOssRrcasn1Buf1             = 132,
  SmemReservedOssRrcasn1Buf2             = 133,
  SmemIdVendor0                          = 134,
  SmemIdVendor1                          = 135,
  SmemIdVendor2                          = 136,
  SmemHwSwBuildId                        = 137,
  SmemReservedSmdBlockPortBaseId         = 138,
  SmemReservedSmdBlockPortProc0Heap      = SmemReservedSmdBlockPortBaseId + SMEM_NUM_SMD_CHANNELS,
  SmemReservedSmdBlockPortProc1Heap      = SmemReservedSmdBlockPortProc0Heap + SMEM_NUM_SMD_CHANNELS,
  SmemI2cMutex                           = SmemReservedSmdBlockPortProc1Heap + SMEM_NUM_SMD_CHANNELS,
  SmemSclkConversion                     = 331,
  SmemReservedSmdSmsmIntrMux             = 332,
  SmemSmsmCpuIntrMask                    = 333,
  SmemReservedAppsDemSlaveData           = 334,
  SmemReservedQdsp6DemSlaveData          = 335,
  SmemVsenseData                         = 336,
  SmemReservedClkregimSources            = 337,
  SmemSmdFifoBaseId                      = 338,
  SmemUsableRamPartitionTable            = SmemSmdFifoBaseId + SMEM_NUM_SMD_CHANNELS,
  SmemPowerOnStatusInfo                  = 403,
  SmemDalArea                            = 404,
  SmemSmemLogPowerIdx                    = 405,
  SmemSmemLogPowerWrap                   = 406,
  SmemSmemLogPowerEvents                 = 407,
  SmemErrCrashLog                        = 408,
  SmemErrF3TraceLog                      = 409,
  SmemSmdBridgeAllocTable                = 410,
  SmemSmdFeatureSet                      = 411,
  SmemReservedSdImgUpgradeStatus         = 412,
  SmemSefsInfo                           = 413,
  SmemReservedResetLog                   = 414,
  SmemReservedResetLogSymbols            = 415,
  SmemModemSwBuildId                     = 416,
  SmemSmemLogMprocWrap                   = 417,
  SmemReservedBootInfoForApps            = 418,
  SmemSmsmSizeInfo                       = 419,
  SmemSmdLoopbackRegister                = 420,
  SmemSsrReasonMss0                      = 421,
  SmemSsrReasonWcnss0                    = 422,
  SmemSsrReasonLpass0                    = 423,
  SmemSsrReasonDsps0                     = 424,
  SmemSsrReasonVcodec0                   = 425,
  SmemVoice                              = 426,
  SmemIdSmp2pBaseApps                    = 427,
  SmemIdSmp2pBaseModem                   = SmemIdSmp2pBaseApps + SMEM_NUM_SMP2P_EDGES,
  SmemIdSmp2pBaseAdsp                    = SmemIdSmp2pBaseModem + SMEM_NUM_SMP2P_EDGES,
  SmemIdSmp2pBaseWcn                     = SmemIdSmp2pBaseAdsp + SMEM_NUM_SMP2P_EDGES,
  SmemIdSmp2pBaseRpm                     = SmemIdSmp2pBaseWcn + SMEM_NUM_SMP2P_EDGES,
  SmemFlashDeviceInfo                    = SmemIdSmp2pBaseRpm + SMEM_NUM_SMP2P_EDGES,
  SmemBamPipeMemory                      = 468,
  SmemImageVersionTable                  = 469,
  SmemLcDebugger                         = 470,
  SmemFlashNandDevInfo                   = 471,
  SmemA2BamDescriptorFifo                = 472,
  SmemCprConfig                          = 473,
  SmemClockInfo                          = 474,
  SmemIpcInfo                            = 475,
  SmemRfEepromData                       = 476,
  SmemCoexMdmWcn                         = 477,
  SmemGlinkNativeXportDescriptor         = 478,
  SmemGlinkNativeXportFifo0              = 479,
  SmemGlinkNativeXportFifo1              = 480,
  SmemIdSmp2pBaseSsc                     = 481,
  SmemIdSmp2pBaseTz                      = SmemIdSmp2pBaseSsc + SMEM_NUM_SMP2P_EDGES,
  SmemIpaFilterTable                     = SmemIdSmp2pBaseTz + SMEM_NUM_SMP2P_EDGES,
  SmemMemLast                            = 497,  /* SmemIpaFilterTable */
  SmemInvalid                            = 498
} SMEM_MEM_TYPE;

/** A list of hosts supported in SMEM */
typedef enum {
  SmemApps       =  0,                  /**< Apps Processor */
  SmemModem      =  1,                  /**< Modem processor */
  SmemAdsp       =  2,                  /**< ADSP processor */
  SmemSsc        =  3,                  /**< Sensor processor */
  SmemWcn        =  4,                  /**< WCN processor */
  SmemCdsp       =  5,                  /**< CDSP processor */
  SmemRpm        =  6,                  /**< RPM processor */
  SmemTz         =  7,                  /**< TZ processor */
  SmemSpss       =  8,                  /**< Secure processor */
  SmemHyp        =  9,                  /**< Hypervisor */
  SmemNpu        =  10,                 /**< NPU processor */
  SmemSpssSp     =  11,                 /**< SPSS Host that shares partition with TZ */
  SmemNsp1       =  12,                 /**< NSP1 processor */
  SmemWpss       =  13,                 /**< WPSS processor */
  SmemTme        =  14,                 /**< TME processor */
  SmemAppsVmLa   =  15,                 /**< VM LA Host of Apps processor */
  SmemExtPm      =  16,                 /**< External PM */
  SmemGpdsp0     =  17,                 /**< General Purpose DSP 0 */
  SmemGpdsp1     =  18,                 /**< General Purpose DSP 1 */
  SmemSoccp      =  19,                 /**< SOCCP processor */
  SmemOobTee     =  20,                 /**< OOB Secure processor */
  SmemOobNs      =  21,                 /**< OOB Non-Secure processor */
  SmemDcp        =  22,                 /**< DCP processor */
  SmemWm         =  23,                 /**< WM processor */
  SmemAm         =  24,                 /**< AM processor */
  SmemQecp       =  25,                 /**< QECP processor */
  SmemNumHosts   =  26,                 /**< Max number of host in target */
  SmemQ6           = SmemAdsp,          /**< Kept for legacy purposes. New code should use SmemAdsp. */
  SmemRiva         = SmemWcn,           /**< Kept for legacy purposes. New code should use SmemWcn. */
  SmemDsps         = SmemSsc,           /**< Kept for legacy purposes. New code should use SmemSsc. */
  SmemSpssNonsp    = SmemSpss,          /**< SPSS Host that shares partition with HLOS */
  SmemNsp          = SmemCdsp,          /**< Alias for SmemCdsp. Kept for legacy purposes. */
  SmemCmddb        = 0xFFFD,            /**< Reserved partition for command DB use case */
  SmemCommonHost   = 0xFFFE,            /**< Common host */
  SmemInvalidHost  = 0xFFFF,            /**< Invalid processor */
} SMEM_HOST_TYPE;

/** Hardware reset detection magic value structure */
typedef struct {
  UINT32    Magic1;  ///< Most-significant word of the predefined magic value.
  UINT32    Magic2;  ///< Least-significant word of the predefined magic value.
} SMEM_HW_RESET_ID_TYPE;

/** Parameters structure for SmemAllocEx and SmemGetAddrEx */
typedef struct {
  SMEM_HOST_TYPE    RemoteHost;  ///< Remote endpoint with which this SMEM item will be shared.
                                 ///< Determines which partition to allocate from. If set to
                                 ///< SmemInvalidHost, the allocation is from the default
                                 ///< unprotected partition.
  SMEM_MEM_TYPE     SmemType;    ///< Identifier for the allocation. Allowed types are in SMEM_MEM_TYPE.
  UINT32            Size;        ///< Size of the allocation in bytes. Allocated size may be larger
                                 ///< than requested and is rounded up to 8 bytes.
  VOID              *Buffer;     ///< Pointer to the allocated buffer in shared memory.
  UINT32            Flags;       ///< Allocation flags. See SMEM_ALLOC_FLAG for options.
} SMEM_ALLOC_PARAMS_TYPE;
