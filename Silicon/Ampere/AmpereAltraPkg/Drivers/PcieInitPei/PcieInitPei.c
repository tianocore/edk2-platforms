/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/RootComplexConfigHii.h>
#include <Guid/RootComplexInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BoardPcieLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/Ac01PcieLib.h>
#include <Library/PcieHotPlugLib.h>
#include <Library/PeiServicesLib.h>
#include <Platform/Ac01.h>
#include <Ppi/ReadOnlyVariable2.h>

#include "RootComplexNVParam.h"

#define TCU_OFFSET         0
#define SNPSRAM_OFFSET     0x9000
#define HB_CSR_OFFSET      0x01000000
#define PCIE0_CSR_OFFSET   0x01010000
#define SERDES_CSR_OFFSET  0x01200000
#define MMCONFIG_OFFSET    0x10000000

#define PCIE_CSR_SIZE  0x10000

STATIC AC01_ROOT_COMPLEX  mRootComplexList[AC01_PCIE_MAX_ROOT_COMPLEX];
STATIC UINT64             mCsrBase[AC01_PCIE_MAX_ROOT_COMPLEX]      = { AC01_PCIE_CSR_BASE_LIST };
STATIC UINT64             mMmio32Base[AC01_PCIE_MAX_ROOT_COMPLEX]   = { AC01_PCIE_MMIO32_BASE_LIST };
STATIC UINT64             mMmio32Base1P[AC01_PCIE_MAX_ROOT_COMPLEX] = { AC01_PCIE_MMIO32_BASE_1P_LIST };
STATIC UINT64             mMmio32Size[AC01_PCIE_MAX_ROOT_COMPLEX]   = { AC01_PCIE_MMIO32_SIZE_LIST };
STATIC UINT64             mMmio32Size1P[AC01_PCIE_MAX_ROOT_COMPLEX] = { AC01_PCIE_MMIO32_SIZE_1P_LIST };
STATIC UINT64             mMmioBase[AC01_PCIE_MAX_ROOT_COMPLEX]     = { AC01_PCIE_MMIO_BASE_LIST };
STATIC UINT64             mMmioSize[AC01_PCIE_MAX_ROOT_COMPLEX]     = { AC01_PCIE_MMIO_SIZE_LIST };

AC01_ROOT_COMPLEX_TYPE
GetRootComplexType (
  UINT8  RootComplexId
  )
{
  if (IsAc01Processor ()) {
    return (RootComplexId < MaxRootComplexA) ? RootComplexTypeA : RootComplexTypeB;
  }

  return RootComplexTypeA;
}

VOID
ConfigureRootComplex (
  BOOLEAN                            IsConfigFound,
  ROOT_COMPLEX_CONFIG_VARSTORE_DATA  RootComplexConfig
  )
{
  UINT8              RCIndex;
  UINT8              PcieIndex;
  AC01_ROOT_COMPLEX  *RootComplex;

  for (RCIndex = 0; RCIndex < AC01_PCIE_MAX_ROOT_COMPLEX; RCIndex++) {
    RootComplex                    = &mRootComplexList[RCIndex];
    RootComplex->Active            = IsConfigFound ? RootComplexConfig.RCStatus[RCIndex] : TRUE;
    RootComplex->DevMapLow         = IsConfigFound ? RootComplexConfig.RCBifurcationLow[RCIndex] : 0;
    RootComplex->DevMapHigh        = IsConfigFound ? RootComplexConfig.RCBifurcationHigh[RCIndex] : 0;
    RootComplex->Socket            = RCIndex / AC01_PCIE_MAX_RCS_PER_SOCKET;
    RootComplex->ID                = RCIndex % AC01_PCIE_MAX_RCS_PER_SOCKET;
    RootComplex->CsrBase           = mCsrBase[RCIndex];
    RootComplex->TcuBase           = RootComplex->CsrBase + TCU_OFFSET;
    RootComplex->HostBridgeBase    = RootComplex->CsrBase + HB_CSR_OFFSET;
    RootComplex->SerdesBase        = RootComplex->CsrBase + SERDES_CSR_OFFSET;
    RootComplex->MmcfgBase         = RootComplex->CsrBase + MMCONFIG_OFFSET;
    RootComplex->MmioBase          = mMmioBase[RCIndex];
    RootComplex->MmioSize          = mMmioSize[RCIndex];
    RootComplex->Mmio32Base        = mMmio32Base[RCIndex];
    RootComplex->Mmio32Size        = mMmio32Size[RCIndex];
    RootComplex->Type              = GetRootComplexType (RootComplex->ID);
    RootComplex->MaxPcieController = (RootComplex->Type == RootComplexTypeB)
                                     ? MaxPcieControllerOfRootComplexB : MaxPcieControllerOfRootComplexA;
    RootComplex->Logical = BoardPcieGetSegmentNumber (RootComplex);

    for (PcieIndex = 0; PcieIndex < RootComplex->MaxPcieController; PcieIndex++) {
      RootComplex->Pcie[PcieIndex].ID          = PcieIndex;
      RootComplex->Pcie[PcieIndex].CsrBase     = RootComplex->CsrBase + PCIE0_CSR_OFFSET + PcieIndex * PCIE_CSR_SIZE;
      RootComplex->Pcie[PcieIndex].SnpsRamBase = RootComplex->Pcie[PcieIndex].CsrBase + SNPSRAM_OFFSET;
      RootComplex->Pcie[PcieIndex].DevNum      = PcieIndex + 1;
    }

    ParseRootComplexNVParamData (RootComplex);

    DEBUG ((
      DEBUG_INFO,
      " + S%d - RootComplex%a%d, MMCfgBase:0x%lx, MmioBase:0x%lx, Mmio32Base:0x%lx, Enabled:%a\n",
      RootComplex->Socket,
      (RootComplex->Type == RootComplexTypeA) ? "A" : "B",
      RootComplex->ID,
      RootComplex->MmcfgBase,
      RootComplex->MmioBase,
      RootComplex->Mmio32Base,
      (RootComplex->Active) ? "Y" : "N"
      ));

    DEBUG ((DEBUG_INFO, " +   DevMapLo/Hi: 0x%x/0x%x\n", RootComplex->DevMapLow, RootComplex->DevMapHigh));
    for (PcieIndex = 0; PcieIndex < RootComplex->MaxPcieController; PcieIndex++) {
      DEBUG ((
        DEBUG_INFO,
        " +     PCIE%d:0x%lx - Enabled:%a - DevNum:0x%x\n",
        PcieIndex,
        RootComplex->Pcie[PcieIndex].CsrBase,
        (RootComplex->Pcie[PcieIndex].Active) ? "Y" : "N",
        RootComplex->Pcie[PcieIndex].DevNum
        ));
    }
  }
}

VOID
BuildRootComplexData (
  VOID
  )
{
  BOOLEAN                            IsConfigFound;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI    *VariablePpi;
  EFI_STATUS                         Status;
  ROOT_COMPLEX_CONFIG_VARSTORE_DATA  RootComplexConfig;
  UINTN                              DataSize;

  IsConfigFound = FALSE;
  ZeroMem ((VOID *)&RootComplexConfig, sizeof (ROOT_COMPLEX_CONFIG_VARSTORE_DATA));

  //
  // Get the Root Complex config from NVRAM
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&VariablePpi
             );
  if (!EFI_ERROR (Status)) {
    DataSize = sizeof (RootComplexConfig);
    Status   = VariablePpi->GetVariable (
                              VariablePpi,
                              ROOT_COMPLEX_CONFIG_VARSTORE_NAME,
                              &gRootComplexConfigFormSetGuid,
                              NULL,
                              &DataSize,
                              &RootComplexConfig
                              );
    if (!EFI_ERROR (Status)) {
      IsConfigFound = TRUE;
    }
  }

  ZeroMem (&mRootComplexList, sizeof (AC01_ROOT_COMPLEX) * AC01_PCIE_MAX_ROOT_COMPLEX);

  //
  // Adjust Root Complex MMIO32 base address in 1P or 2P configuration
  //
  if (!IsSlaveSocketAvailable ()) {
    CopyMem ((VOID *)&mMmio32Base, (VOID *)&mMmio32Base1P, sizeof (mMmio32Base1P));
    CopyMem ((VOID *)&mMmio32Size, (VOID *)&mMmio32Size1P, sizeof (mMmio32Size1P));
  }

  //
  // All necessary information is available, config Root complex accordingly
  //
  ConfigureRootComplex (IsConfigFound, RootComplexConfig);
}

EFI_STATUS
EFIAPI
PcieInitEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  AC01_ROOT_COMPLEX  *RootComplex;
  EFI_STATUS         Status;
  UINT8              Index;

  BuildRootComplexData ();

  //
  // Initialize Root Complex and underneath controllers
  //
  for (Index = 0; Index < AC01_PCIE_MAX_ROOT_COMPLEX; Index++) {
    RootComplex = &mRootComplexList[Index];
    if (!RootComplex->Active) {
      continue;
    }

    DEBUG ((DEBUG_INIT, "Initializing S%d-RC%d...", RootComplex->Socket, RootComplex->ID));
    Status = Ac01PcieCoreSetupRC (RootComplex, FALSE, 0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed\n"));
      RootComplex->Active = FALSE;
      continue;
    } else {
      DEBUG ((DEBUG_INIT, "Done + DevMapLow/High: %d/%d\n", RootComplex->DevMapLow, RootComplex->DevMapHigh));
    }
  }

  Ac01PcieCorePostSetupRC (mRootComplexList);

  PcieHotPlugStart ();

  //
  // Build Root Complex info Hob
  //
  BuildGuidDataHob (
    &gRootComplexInfoHobGuid,
    (VOID *)&mRootComplexList,
    sizeof (mRootComplexList)
    );

  return EFI_SUCCESS;
}
