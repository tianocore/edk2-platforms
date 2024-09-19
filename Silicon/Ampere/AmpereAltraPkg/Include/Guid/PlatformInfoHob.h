/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_INFO_HOB_H_
#define PLATFORM_INFO_HOB_H_

#include <IndustryStandard/Tpm20.h>
#include <Platform/Ac01.h>

#define PLATFORM_INFO_HOB_GUID \
  { 0x7f73e372, 0x7183, 0x4022, { 0xb3, 0x76, 0x78, 0x30, 0x32, 0x6d, 0x79, 0xb4 } }

extern EFI_GUID  gPlatformInfoHobGuid;

//
// DIMM type
//
#define UDIMM    0x00
#define RDIMM    0x01
#define SODIMM   0x02
#define RSODIMM  0x03
#define LRDIMM   0x04
#define NVRDIMM  0x05

//
// DIMM status
//
#define DIMM_NOT_INSTALLED             0x00
#define DIMM_INSTALLED_OPERATIONAL     0x01    // installed and operational
#define DIMM_INSTALLED_NONOPERATIONAL  0x02    // installed and non-operational
#define DIMM_INSTALLED_FAILED          0x03    // installed and failed

typedef struct {
  UINT32    NumRegion;
  UINT64    TotalSize;
  UINT64    Base[PLATFORM_DRAM_INFO_MAX_REGION];
  UINT64    Size[PLATFORM_DRAM_INFO_MAX_REGION];
  UINT64    Node[PLATFORM_DRAM_INFO_MAX_REGION];
  UINT64    Socket[PLATFORM_DRAM_INFO_MAX_REGION];
  UINT32    MaxSpeed;
  UINT32    McuMask[PLATFORM_CPU_MAX_SOCKET];
  UINT32    NvdRegion[PLATFORM_DRAM_INFO_MAX_REGION];
  UINT32    NvdimmMode[PLATFORM_CPU_MAX_SOCKET];
} PLATFORM_DRAM_INFO;

typedef struct {
  CHAR8     PartNumber[32];
  UINT64    DimmSize;
  UINT16    DimmMfcId;
  UINT16    Reserved;
  UINT8     DimmNrRank;
  UINT8     DimmType;
  UINT8     DimmStatus;
  UINT8     DimmDevType;
} PLATFORM_DIMM_INFO;

typedef struct {
  UINT8    Data[512];
} PLATFORM_DIMM_SPD_DATA;

typedef struct {
  PLATFORM_DIMM_INFO        Info;
  PLATFORM_DIMM_SPD_DATA    SpdData;
  UINT32                    NodeId;
} PLATFORM_DIMM;

typedef struct {
  UINT32           BoardDimmSlots;
  PLATFORM_DIMM    Dimm[PLATFORM_DIMM_INFO_MAX_SLOT];
} PLATFORM_DIMM_LIST;

typedef struct {
  UINT32    EnableMask[4];
} PLATFORM_CLUSTER_EN;

//
// Algorithm ID defined in pre-UEFI firmware
//
typedef enum {
  PlatformAlgorithmSha1 = 1,
  PlatformAlgorithmSha256,
  PlatformAlgorithmMax,
} PLATFORM_ALGORITHM_ID;

//
// Platform digest data definition
//
typedef union {
  unsigned char    Sha1[SHA1_DIGEST_SIZE];
  unsigned char    Sha256[SHA256_DIGEST_SIZE];
} PLATFORM_TPM_DIGEST;

#define MAX_VIRTUAL_PCR_INDEX  0x0002

#pragma pack(1)
typedef struct {
  PLATFORM_ALGORITHM_ID    AlgorithmId;
  struct {
    PLATFORM_TPM_DIGEST    Hash;
  } VPcr[MAX_VIRTUAL_PCR_INDEX]; // vPCR 0 or 1
} PLATFORM_VPCR_HASH_INFO;

typedef struct {
  UINT8     InterfaceType;              // If I/F is CRB then CRB parameters are expected
  UINT64    InterfaceParametersAddress; // Physical address of interface, by Value */
  UINT64    InterfaceParametersLength;
  UINT32    SupportedAlgorithmsBitMask;
  UINT64    EventLogAddress;
  UINT64    EventLogLength;
  UINT8     Reserved[3];
} PLATFORM_TPM2_CONFIG_DATA;

typedef struct {
  UINT32    CurrentRequest;
  UINT32    LastRequest;
  UINT32    LastRequestStatus;
} PLATFORM_TPM2_PPI_REQUEST;

typedef struct {
  UINT64                       AddressOfControlArea;
  UINT64                       ControlAreaLength;
  UINT8                        InterruptMode;
  UINT8                        Reserved[3];
  UINT32                       InterruptNumber;         // Should have a value of zero polling
  UINT32                       SmcFunctionId;           // SMC Function ID
  UINT64                       PpiRequestNotifyAddress; // Doorbell/Interrupt Address
  PLATFORM_TPM2_PPI_REQUEST    *PpiRequest;             // PPI Request
} PLATFORM_TPM2_CRB_INTERFACE_PARAMETERS;

typedef struct {
  PLATFORM_TPM2_CONFIG_DATA                 Tpm2ConfigData;
  PLATFORM_TPM2_CRB_INTERFACE_PARAMETERS    Tpm2CrbInterfaceParams;
  PLATFORM_VPCR_HASH_INFO                   Tpm2VPcrHashInfo;
} PLATFORM_TPM2_INFO;
#pragma pack()

typedef struct {
  UINT8                  MajorNumber;
  UINT8                  MinorNumber;
  UINT64                 PcpClk;
  UINT64                 CpuClk;
  UINT64                 SocClk;
  UINT64                 AhbClk;
  UINT64                 SysClk;
  UINT8                  CpuInfo[128];
  UINT8                  CpuVer[32];
  UINT8                  SmPmProVer[32];
  UINT8                  SmPmProBuild[32];
  PLATFORM_DRAM_INFO     DramInfo;
  PLATFORM_DIMM_LIST     DimmList;
  PLATFORM_CLUSTER_EN    ClusterEn[2];
  UINT32                 FailSafeStatus;
  UINT32                 RcDisableMask[2];
  UINT8                  ResetStatus;
  UINT16                 CoreVoltage[2];
  UINT16                 SocVoltage[2];
  UINT16                 Dimm1Voltage[2];
  UINT16                 Dimm2Voltage[2];

  /* Chip information */
  UINT32                 ScuProductId[2];
  UINT8                  MaxNumOfCore[2];
  UINT8                  Warranty[2];
  UINT8                  SubNumaMode[2];
  UINT8                  AvsEnable[2];
  UINT32                 AvsVoltageMV[2];
  UINT8                  TurboCapability[2];
  UINT32                 TurboFrequency[2];

  UINT8                  SkuMaxTurbo[2];
  UINT8                  SkuMaxCore[2];
  UINT32                 AHBCId[2];

  /* TPM2 Info */
  PLATFORM_TPM2_INFO     Tpm2Info;

  /* 2P link info for RCA0/RCA1 */
  UINT8                  Link2PSpeed[2];
  UINT8                  Link2PWidth[2];

  UINT32                 Ecid[2][4];
  UINT8                  SkuMaxLink2pSpeed;
  UINT8                  VdmCapability[2];
} PLATFORM_INFO_HOB;

#endif /* PLATFORM_INFO_HOB_H_ */
