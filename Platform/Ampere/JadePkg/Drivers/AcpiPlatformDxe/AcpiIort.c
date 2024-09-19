/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AcpiHeader.h>
#include <Guid/RootComplexConfigHii.h>
#include <Guid/RootComplexInfoHob.h>
#include <IndustryStandard/Acpi30.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <Library/AcpiLib.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Platform/Ac01.h>
#include <Protocol/AcpiTable.h>

#define __AC01_ID_MAPPING(In, Num, Out, Ref, Flags)    \
  {                                                    \
    In,                                                \
    Num,                                               \
    Out,                                               \
    OFFSET_OF (AC01_IO_REMAPPING_STRUCTURE, Ref),      \
    Flags                                              \
  }

#define TCU_TO_SMMU_OFFSET    0x2000
#define PAGE1_TO_PMCG_OFFSET  0x10000

STATIC AC01_ROOT_COMPLEX  *mRootComplexList;

STATIC UINT32  mTbuPmuIrqArray[]  = { AC01_SMMU_TBU_PMU_IRQS_LIST };
STATIC UINT32  mTcuPmuIrqArray[]  = { AC01_SMMU_TCU_PMU_IRQS_LIST };
STATIC UINT64  mRcaTbuPmuOffset[] = { AC01_RCA_TBU_PMU_OFFSET_LIST };
STATIC UINT64  mRcbTbuPmuOffset[] = { AC01_RCB_TBU_PMU_OFFSET_LIST };

#pragma pack(1)

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_NODE    Node;
  UINT64                            Base;
  UINT32                            Flags;
  UINT32                            Reserved;
  UINT64                            VatosAddress;
  UINT32                            Model;
  UINT32                            Event;
  UINT32                            Pri;
  UINT32                            Gerr;
  UINT32                            Sync;
  UINT32                            ProximityDomain;
  UINT32                            DeviceIdMapping;
} EFI_ACPI_6_2_IO_REMAPPING_SMMU3_NODE;

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE    Node;
  UINT32                                ItsIdentifier;
} AC01_ITS_NODE;

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_RC_NODE     Node;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE    RcIdMapping;
} AC01_RC_NODE;

typedef struct {
  EFI_ACPI_6_2_IO_REMAPPING_SMMU3_NODE    Node;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE      InterruptMsiMapping;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE      InterruptMsiMappingSingle;
} AC01_SMMU_NODE;

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_TABLE    Iort;
  AC01_ITS_NODE                      ItsNode[2];
  AC01_RC_NODE                       RcNode[2];
  AC01_SMMU_NODE                     SmmuNode[2];
} AC01_IO_REMAPPING_STRUCTURE;

#pragma pack()

EFI_ACPI_6_0_IO_REMAPPING_TABLE  mIortHeader = {
  .Header     = __ACPI_HEADER (
                  EFI_ACPI_6_0_IO_REMAPPING_TABLE_SIGNATURE,
                  AC01_IO_REMAPPING_STRUCTURE,
                  EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00
                  ),
  .NumNodes   = 0, // To be filled
  .NodeOffset = sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE),
  0
};

AC01_ITS_NODE  mItsNodeTemplate = {
  .Node                = {
    {
      EFI_ACPI_IORT_TYPE_ITS_GROUP,
      sizeof (EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE) + 4,
      0x0,
      0x0,
      0x0,
      0x0,
    },
    .NumItsIdentifiers = 1,
  },
  .ItsIdentifier       = 1,
};

AC01_RC_NODE  mRcNodeTemplate = {
  {
    {
      EFI_ACPI_IORT_TYPE_ROOT_COMPLEX,
      sizeof (AC01_RC_NODE),
      0x1,
      0x0,
      0x1,
      OFFSET_OF (AC01_RC_NODE, RcIdMapping),
    },
    EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA,
    0x0,
    0x0,
    EFI_ACPI_IORT_MEM_ACCESS_FLAGS_CPM |
    EFI_ACPI_IORT_MEM_ACCESS_FLAGS_DACS,
    EFI_ACPI_IORT_ROOT_COMPLEX_ATS_SUPPORTED,
    .PciSegmentNumber  = 0,
    .MemoryAddressSize = 64,
  },
  __AC01_ID_MAPPING (0x0, 0xffff, 0x0, SmmuNode, 0),
};

AC01_SMMU_NODE  mSmmuNodeTemplate = {
  {
    {
      EFI_ACPI_IORT_TYPE_SMMUv3,
      sizeof (AC01_SMMU_NODE),
      0x2,  // Revision
      0x0,
      0x2,  // Mapping Count
      OFFSET_OF (AC01_SMMU_NODE, InterruptMsiMapping),
    },
    .Base = 0,
    EFI_ACPI_IORT_SMMUv3_FLAG_COHAC_OVERRIDE | EFI_ACPI_IORT_SMMUv3_FLAG_PROXIMITY_DOMAIN,
    0,
    0,
    0,
    0,
    0,
    0x0,
    0x0,
    0,     // Proximity domain - need fill in
    .DeviceIdMapping = 1,
  },
  __AC01_ID_MAPPING (0x0, 0xffff, 0, SmmuNode, 0),
  __AC01_ID_MAPPING (0x0, 0x1, 0, SmmuNode, 1),
};

EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE  mPmcgNodeTemplate = {
  {
    EFI_ACPI_IORT_TYPE_PMCG,
    sizeof (EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE),
    0x1,
    0x0,
    0x0,
    0x0,
  },
  0,  // Page 0 Base. Need to be filled
  0,  // GSIV. Need to be filled
  0,  // Node reference. Need to be filled
  0,  // Page 1 Base. Need to be filled
};

STATIC
VOID
ConstructIort (
  VOID    *IortBuffer,
  UINT32  RcCount,
  UINT32  SmmuPmuAgentCount,
  UINT32  HeaderCount,
  INT32   *EnabledRCs
  )
{
  AC01_ROOT_COMPLEX  *RootComplex;
  UINT32             Idx, Idx1;
  UINT32             ItsOffset[AC01_PCIE_MAX_ROOT_COMPLEX];
  UINT32             SmmuNodeOffset[AC01_PCIE_MAX_ROOT_COMPLEX];
  UINT64             *TbuPmuOffset;
  UINTN              MaxTbuPmu;
  VOID               *IortIter, *SmmuIter, *PmcgIter;

  IortIter                  = IortBuffer;
  mIortHeader.Header.Length = HeaderCount;
  mIortHeader.NumNodes      = (3 * RcCount) + SmmuPmuAgentCount,
  CopyMem (IortIter, &mIortHeader, sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE));

  IortIter += sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE);
  for (Idx = 0; Idx < RcCount; Idx++) {
    ItsOffset[Idx]                 = IortIter - IortBuffer;
    mItsNodeTemplate.ItsIdentifier = EnabledRCs[Idx];
    CopyMem (IortIter, &mItsNodeTemplate, sizeof (AC01_ITS_NODE));
    IortIter += sizeof (AC01_ITS_NODE);
  }

  SmmuIter = IortIter + RcCount * sizeof (AC01_RC_NODE);
  PmcgIter = SmmuIter + RcCount * sizeof (AC01_SMMU_NODE);
  for (Idx = 0; Idx < RcCount; Idx++) {
    SmmuNodeOffset[Idx]                                         = SmmuIter - IortBuffer;
    RootComplex                                                 = &mRootComplexList[EnabledRCs[Idx]];
    mSmmuNodeTemplate.Node.Base                                 = RootComplex->TcuBase;
    mSmmuNodeTemplate.InterruptMsiMapping.OutputBase            = EnabledRCs[Idx] << 16;
    mSmmuNodeTemplate.InterruptMsiMapping.OutputReference       = ItsOffset[Idx];
    mSmmuNodeTemplate.InterruptMsiMappingSingle.OutputBase      = EnabledRCs[Idx] << 16;
    mSmmuNodeTemplate.InterruptMsiMappingSingle.OutputReference = ItsOffset[Idx];
    /* All RCs on master be assigned to node 0, while remote RCs will be assigned to first remote node */
    mSmmuNodeTemplate.Node.ProximityDomain = 0;
    if ((RootComplex->TcuBase & SLAVE_SOCKET_BASE_ADDRESS_OFFSET) != 0) {
      // RootComplex on remote socket
      switch (CpuGetSubNumaMode ()) {
        case SUBNUMA_MODE_MONOLITHIC:
          mSmmuNodeTemplate.Node.ProximityDomain += MONOLITIC_NUM_OF_REGION;
          break;
        case SUBNUMA_MODE_HEMISPHERE:
          mSmmuNodeTemplate.Node.ProximityDomain += HEMISPHERE_NUM_OF_REGION;
          break;
        case SUBNUMA_MODE_QUADRANT:
          mSmmuNodeTemplate.Node.ProximityDomain += QUADRANT_NUM_OF_REGION;
          break;
      }
    }

    CopyMem (SmmuIter, &mSmmuNodeTemplate, sizeof (AC01_SMMU_NODE));
    SmmuIter += sizeof (AC01_SMMU_NODE);

    if (SmmuPmuAgentCount == 0) {
      continue;
    }

    //
    // Add TBU PMCG nodes
    //
    if (RootComplex->Type == RootComplexTypeA) {
      MaxTbuPmu    = AC01_RCA_MAX_TBU_PMU;
      TbuPmuOffset = mRcaTbuPmuOffset;
    } else {
      MaxTbuPmu    = AC01_RCB_MAX_TBU_PMU;
      TbuPmuOffset = mRcbTbuPmuOffset;
    }

    for (Idx1 = 0; Idx1 < MaxTbuPmu; Idx1++) {
      mPmcgNodeTemplate.Base                  = RootComplex->TcuBase + TCU_TO_SMMU_OFFSET + TbuPmuOffset[Idx1];
      mPmcgNodeTemplate.Page1Base             = mPmcgNodeTemplate.Base + PAGE1_TO_PMCG_OFFSET;
      mPmcgNodeTemplate.NodeReference         = SmmuNodeOffset[Idx];
      mPmcgNodeTemplate.OverflowInterruptGsiv = mTbuPmuIrqArray[EnabledRCs[Idx]] + Idx1;
      CopyMem (PmcgIter, &mPmcgNodeTemplate, sizeof (mPmcgNodeTemplate));
      PmcgIter += sizeof (mPmcgNodeTemplate);
    }

    //
    // Add TCU PMCG node
    //
    mPmcgNodeTemplate.Base                  = RootComplex->TcuBase + TCU_TO_SMMU_OFFSET;
    mPmcgNodeTemplate.Page1Base             = mPmcgNodeTemplate.Base + PAGE1_TO_PMCG_OFFSET;
    mPmcgNodeTemplate.NodeReference         = SmmuNodeOffset[Idx];
    mPmcgNodeTemplate.OverflowInterruptGsiv = mTcuPmuIrqArray[EnabledRCs[Idx]];
    CopyMem (PmcgIter, &mPmcgNodeTemplate, sizeof (mPmcgNodeTemplate));
    PmcgIter += sizeof (mPmcgNodeTemplate);
  }

  for (Idx = 0; Idx < RcCount; Idx++) {
    mRcNodeTemplate.Node.PciSegmentNumber       = mRootComplexList[EnabledRCs[Idx]].Logical;
    mRcNodeTemplate.RcIdMapping.OutputReference = SmmuNodeOffset[Idx];
    CopyMem (IortIter, &mRcNodeTemplate, sizeof (AC01_RC_NODE));
    IortIter += sizeof (AC01_RC_NODE);
  }
}

EFI_STATUS
EFIAPI
AcpiInstallIort (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL            *AcpiTableProtocol;
  EFI_STATUS                         Status;
  INT32                              EnabledRCs[AC01_PCIE_MAX_ROOT_COMPLEX];
  ROOT_COMPLEX_CONFIG_VARSTORE_DATA  VarStoreConfig;
  UINT32                             RcCount, SmmuPmuAgentCount, TotalCount;
  UINT8                              Idx;
  UINTN                              BufferSize;
  UINTN                              TableKey;
  VOID                               *Hob;
  VOID                               *IortBuffer;

  Hob = GetFirstGuidHob (&gRootComplexInfoHobGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  mRootComplexList = (AC01_ROOT_COMPLEX *)GET_GUID_HOB_DATA (Hob);

  for (Idx = 0, RcCount = 0; Idx < AC01_PCIE_MAX_ROOT_COMPLEX; Idx++) {
    if (mRootComplexList[Idx].Active) {
      EnabledRCs[RcCount++] = Idx;
    }
  }

  EnabledRCs[RcCount] = -1;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IORT: Unable to locate ACPI table entry\n"));
    return Status;
  }

  SmmuPmuAgentCount = 0;

  //
  // Check SMMU setting
  //
  BufferSize = sizeof (VarStoreConfig);
  Status     = gRT->GetVariable (
                      ROOT_COMPLEX_CONFIG_VARSTORE_NAME,
                      &gRootComplexConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      &VarStoreConfig
                      );
  if (!EFI_ERROR (Status) && VarStoreConfig.SmmuPmu) {
    for (Idx = 0; Idx < RcCount; Idx++) {
      if (mRootComplexList[EnabledRCs[Idx]].Type == RootComplexTypeA) {
        SmmuPmuAgentCount += AC01_RCA_MAX_TBU_PMU;
      } else {
        SmmuPmuAgentCount += AC01_RCB_MAX_TBU_PMU;
      }

      // Plus 1 TCU
      SmmuPmuAgentCount += 1;
    }
  }

  TotalCount = sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE) +
               RcCount * (sizeof (AC01_ITS_NODE) + sizeof (AC01_RC_NODE) + sizeof (AC01_SMMU_NODE)) +
               SmmuPmuAgentCount * sizeof (EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE);

  IortBuffer = AllocateZeroPool (TotalCount);
  if (IortBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ConstructIort (IortBuffer, RcCount, SmmuPmuAgentCount, TotalCount, EnabledRCs);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                IortBuffer,
                                TotalCount,
                                &TableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "IORT: Unable to install IORT table entry\n"));
  }

  FreePool (IortBuffer);
  return Status;
}
