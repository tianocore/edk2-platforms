/** @file

  Copyright (c) 2017 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

#define VTD_CAP_REG_NFR_MAX (256)

UINTN                            mVtdUnitNumber = 0;
VTD_UNIT_INFORMATION             *mVtdUnitInformation = NULL;
VTD_REGESTER_INFO                *mVtdRegsInfoBuffer = NULL;

BOOLEAN  mVtdEnabled;

/**
  Flush VTD page table and context table memory.

  This action is to make sure the IOMMU engine can get final data in memory.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Base              The base address of memory to be flushed.
  @param[in]  Size              The size of memory in bytes to be flushed.
**/
VOID
FlushPageTableMemory (
  IN UINTN  VtdIndex,
  IN UINTN  Base,
  IN UINTN  Size
  )
{
  if (mVtdUnitInformation[VtdIndex].ECapReg.Bits.C == 0) {
    WriteBackDataCacheRange ((VOID *)Base, Size);
  }
}

/**
  Perpare cache invalidation interface.

  @param[in]  VtdIndex          The index used to identify a VTd engine.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_UNSUPPORTED       Invalidation method is not supported.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
PerpareCacheInvalidationInterface (
  IN UINTN  VtdIndex
  )
{
  UINT32                Reg32;
  VTD_IQA_REG           IqaReg;
  VTD_UNIT_INFORMATION  *VtdUnitInfo;
  UINTN                 VtdUnitBaseAddress;

  VtdUnitInfo = &mVtdUnitInformation[VtdIndex];
  VtdUnitBaseAddress = VtdUnitInfo->VtdUnitBaseAddress;

  if (VtdUnitInfo->VerReg.Bits.Major <= 5) {
    VtdUnitInfo->EnableQueuedInvalidation = 0;
    DEBUG ((DEBUG_INFO, "Use Register-based Invalidation Interface for engine [%d]\n", VtdIndex));
    return EFI_SUCCESS;
  }

  if (VtdUnitInfo->ECapReg.Bits.QI == 0) {
    DEBUG ((DEBUG_ERROR, "Hardware does not support queued invalidations interface for engine [%d]\n", VtdIndex));
    return EFI_UNSUPPORTED;
  }

  VtdUnitInfo->EnableQueuedInvalidation = 1;
  DEBUG ((DEBUG_INFO, "Use Queued Invalidation Interface for engine [%d]\n", VtdIndex));

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  if ((Reg32 & B_GSTS_REG_QIES) != 0) {
    DEBUG ((DEBUG_ERROR,"Queued Invalidation Interface was enabled.\n"));

    VtdLibDisableQueuedInvalidationInterface (VtdUnitBaseAddress);
  }

  //
  // Initialize the Invalidation Queue Tail Register to zero.
  //
  MmioWrite64 (VtdUnitBaseAddress + R_IQT_REG, 0);

  //
  // Setup the IQ address, size and descriptor width through the Invalidation Queue Address Register
  //
  if (VtdUnitInfo->QiDescBuffer == NULL) {
    VtdUnitInfo->QiDescBufferSize = (sizeof (QI_256_DESC) * ((UINTN) 1 << (VTD_INVALIDATION_QUEUE_SIZE + 7)));
    VtdUnitInfo->QiDescBuffer = AllocatePages (EFI_SIZE_TO_PAGES (VtdUnitInfo->QiDescBufferSize));
    if (VtdUnitInfo->QiDescBuffer == NULL) {
      DEBUG ((DEBUG_ERROR,"Could not Alloc Invalidation Queue Buffer.\n"));
      VTdLogAddEvent (VTDLOG_DXE_QUEUED_INVALIDATION, VTD_LOG_QI_ERROR_OUT_OF_RESOURCES, VtdUnitBaseAddress);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  DEBUG ((DEBUG_INFO, "Invalidation Queue Buffer Size : %d\n", VtdUnitInfo->QiDescBufferSize));
  //
  // 4KB Aligned address
  //
  IqaReg.Uint64 = (UINT64) (UINTN) VtdUnitInfo->QiDescBuffer;
  IqaReg.Bits.DW = VTD_QUEUED_INVALIDATION_DESCRIPTOR_WIDTH;
  IqaReg.Bits.QS = VTD_INVALIDATION_QUEUE_SIZE;
  MmioWrite64 (VtdUnitBaseAddress + R_IQA_REG, IqaReg.Uint64);
  IqaReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQA_REG);
  DEBUG ((DEBUG_INFO, "IQA_REG = 0x%lx, IQH_REG = 0x%lx\n", IqaReg.Uint64, MmioRead64 (VtdUnitBaseAddress + R_IQH_REG)));

  //
  // Enable the queued invalidation interface through the Global Command Register.
  // When enabled, hardware sets the QIES field in the Global Status Register.
  //
  DEBUG ((DEBUG_INFO, "Enable Queued Invalidation Interface.\n"));
  VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_QIE);

  VTdLogAddEvent (VTDLOG_DXE_QUEUED_INVALIDATION, VTD_LOG_QI_ENABLE, VtdUnitBaseAddress);

  return EFI_SUCCESS;
}

/**
  Submit the queued invalidation descriptor to the remapping
   hardware unit and wait for its completion.

  @param[in] VtdUnitBaseAddress The base address of the VTd engine.
  @param[in]  Desc              The invalidate descriptor

  @retval EFI_SUCCESS           The operation was successful.
  @retval RETURN_DEVICE_ERROR   A fault is detected.
  @retval EFI_INVALID_PARAMETER Parameter is invalid.
**/
EFI_STATUS
SubmitQueuedInvalidationDescriptor (
  IN UINTN             VtdUnitBaseAddress,
  IN QI_256_DESC       *Desc
  )
{
  EFI_STATUS                   Status;
  VTD_REGESTER_QI_INFO         RegisterQi;

  Status = VtdLibSubmitQueuedInvalidationDescriptor (VtdUnitBaseAddress, Desc, FALSE);
  if (Status == EFI_DEVICE_ERROR) {
    RegisterQi.BaseAddress = VtdUnitBaseAddress;
    RegisterQi.FstsReg     = MmioRead32 (VtdUnitBaseAddress + R_FSTS_REG);;
    RegisterQi.IqercdReg   = MmioRead64 (VtdUnitBaseAddress + R_IQERCD_REG);
    VTdLogAddDataEvent (VTDLOG_PEI_REGISTER, VTDLOG_REGISTER_QI, &RegisterQi, sizeof (VTD_REGESTER_QI_INFO));

    MmioWrite32 (VtdUnitBaseAddress + R_FSTS_REG, RegisterQi.FstsReg & (B_FSTS_REG_IQE | B_FSTS_REG_ITE | B_FSTS_REG_ICE));
  }

  return Status;
}

/**
  Invalidate VTd context cache.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
**/
EFI_STATUS
InvalidateContextCache (
  IN UINTN  VtdIndex
  )
{
  UINT64         Reg64;
  QI_256_DESC    QiDesc;

  if (mVtdUnitInformation[VtdIndex].EnableQueuedInvalidation == 0) {
    //
    // Register-based Invalidation
    //
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
    if ((Reg64 & B_CCMD_REG_ICC) != 0) {
      DEBUG ((DEBUG_ERROR,"ERROR: InvalidateContextCache: B_CCMD_REG_ICC is set for VTD(%d)\n",VtdIndex));
      return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
    Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_GLOBAL);
    MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG, Reg64);

    do {
      Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
    } while ((Reg64 & B_CCMD_REG_ICC) != 0);
  } else {
    //
    // Queued Invalidation
    //
    QiDesc.Uint64[0] = QI_CC_FM(0) | QI_CC_SID(0) | QI_CC_DID(0) | QI_CC_GRAN(1) | QI_CC_TYPE;
    QiDesc.Uint64[1] = 0;
    QiDesc.Uint64[2] = 0;
    QiDesc.Uint64[3] = 0;

    return SubmitQueuedInvalidationDescriptor(mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress, &QiDesc);
  }
  return EFI_SUCCESS;
}

/**
  Invalidate VTd IOTLB.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
**/
EFI_STATUS
InvalidateIOTLB (
  IN UINTN  VtdIndex
  )
{
  UINT64         Reg64;
  QI_256_DESC    QiDesc;

  if (mVtdUnitInformation[VtdIndex].EnableQueuedInvalidation == 0) {
    //
    // Register-based Invalidation
    //
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
      DEBUG ((DEBUG_ERROR,"ERROR: InvalidateIOTLB: B_IOTLB_REG_IVT is set for VTD(%d)\n", VtdIndex));
      return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
    Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_GLOBAL);
    MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

    do {
      Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    } while ((Reg64 & B_IOTLB_REG_IVT) != 0);
  } else {
    //
    // Queued Invalidation
    //
    QiDesc.Uint64[0] = QI_IOTLB_DID(0) | QI_IOTLB_DR(CAP_READ_DRAIN(mVtdUnitInformation[VtdIndex].CapReg.Uint64)) | QI_IOTLB_DW(CAP_WRITE_DRAIN(mVtdUnitInformation[VtdIndex].CapReg.Uint64)) | QI_IOTLB_GRAN(1) | QI_IOTLB_TYPE;
    QiDesc.Uint64[1] = QI_IOTLB_ADDR(0) | QI_IOTLB_IH(0) | QI_IOTLB_AM(0);
    QiDesc.Uint64[2] = 0;
    QiDesc.Uint64[3] = 0;

    return SubmitQueuedInvalidationDescriptor(mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress, &QiDesc);
  }

  return EFI_SUCCESS;
}

/**
  Invalid VTd global IOTLB.

  @param[in]  VtdIndex              The index of VTd engine.

  @retval EFI_SUCCESS           VTd global IOTLB is invalidated.
  @retval EFI_DEVICE_ERROR      VTd global IOTLB is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBGlobal (
  IN UINTN  VtdIndex
  )
{
  if (!mVtdEnabled) {
    return EFI_SUCCESS;
  }

  DEBUG((DEBUG_VERBOSE, "InvalidateVtdIOTLBGlobal(%d)\n", VtdIndex));

  //
  // Write Buffer Flush before invalidation
  //
  VtdLibFlushWriteBuffer (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress);

  //
  // Invalidate the context cache
  //
  if (mVtdUnitInformation[VtdIndex].HasDirtyContext) {
    InvalidateContextCache (VtdIndex);
  }

  //
  // Invalidate the IOTLB cache
  //
  if (mVtdUnitInformation[VtdIndex].HasDirtyContext || mVtdUnitInformation[VtdIndex].HasDirtyPages) {
    InvalidateIOTLB (VtdIndex);
  }

  return EFI_SUCCESS;
}

/**
  Prepare VTD configuration.
**/
VOID
PrepareVtdConfig (
  VOID
  )
{
  UINTN         Index;
  UINTN         DomainNumber;
  EFI_STATUS    Status;

  if (mVtdRegsInfoBuffer == NULL) {
    mVtdRegsInfoBuffer = AllocateZeroPool (sizeof (VTD_REGESTER_INFO) + sizeof (VTD_UINT128) * VTD_CAP_REG_NFR_MAX);
    ASSERT (mVtdRegsInfoBuffer != NULL);
  }

  //
  // Dump VTd error before DXE phase
  //
  DumpVtdIfError ();

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    DEBUG ((DEBUG_INFO, "Dump VTd Capability (%d)\n", Index));
    mVtdUnitInformation[Index].VerReg.Uint32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_VER_REG);
    DumpVtdVerRegs (&mVtdUnitInformation[Index].VerReg);
    mVtdUnitInformation[Index].CapReg.Uint64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CAP_REG);
    DumpVtdCapRegs (&mVtdUnitInformation[Index].CapReg);
    mVtdUnitInformation[Index].ECapReg.Uint64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_ECAP_REG);
    DumpVtdECapRegs (&mVtdUnitInformation[Index].ECapReg);

    if ((mVtdUnitInformation[Index].CapReg.Bits.SLLPS & BIT0) == 0) {
      DEBUG((DEBUG_WARN, "!!!! 2MB super page is not supported on VTD %d !!!!\n", Index));
    }
    if ((mVtdUnitInformation[Index].CapReg.Bits.SAGAW & BIT3) != 0) {
      DEBUG((DEBUG_INFO, "Support 5-level page-table on VTD %d\n", Index));
    }
    if ((mVtdUnitInformation[Index].CapReg.Bits.SAGAW & BIT2) != 0) {
      DEBUG((DEBUG_INFO, "Support 4-level page-table on VTD %d\n", Index));
    }
    if ((mVtdUnitInformation[Index].CapReg.Bits.SAGAW & (BIT3 | BIT2)) == 0) {
      DEBUG((DEBUG_ERROR, "!!!! Page-table type 0x%X is not supported on VTD %d !!!!\n", Index, mVtdUnitInformation[Index].CapReg.Bits.SAGAW));
      return ;
    }

    DomainNumber = (UINTN)1 << (UINT8)((UINTN)mVtdUnitInformation[Index].CapReg.Bits.ND * 2 + 4);
    if (mVtdUnitInformation[Index].PciDeviceInfo->PciDeviceDataNumber >= DomainNumber) {
      DEBUG((DEBUG_ERROR, "!!!! Pci device Number(0x%x) >= DomainNumber(0x%x) !!!!\n", mVtdUnitInformation[Index].PciDeviceInfo->PciDeviceDataNumber, DomainNumber));
      return ;
    }

    Status = PerpareCacheInvalidationInterface(Index);
    if (EFI_ERROR (Status)) {
      ASSERT(FALSE);
      return;
    }
  }
  return ;
}

/**
  Disable PMR in all VTd engine.
**/
VOID
DisablePmr (
  VOID
  )
{
  UINTN         Index;
  EFI_STATUS    Status;

  DEBUG ((DEBUG_INFO,"DisablePmr\n"));

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    Status = VtdLibDisablePmr (mVtdUnitInformation[Index].VtdUnitBaseAddress);
    VTdLogAddEvent (VTDLOG_DXE_DISABLE_PMR, mVtdUnitInformation[Index].VtdUnitBaseAddress, Status);
  }

  return ;
}

/**
  Update Root Table Address Register

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  EnableADM         TRUE - Enable ADM in TTM bits
**/
VOID
UpdateRootTableAddressRegister (
  IN UINTN   VtdIndex,
  IN BOOLEAN EnableADM
  )
{
  UINT64 Reg64;

  if (mVtdUnitInformation[VtdIndex].ExtRootEntryTable != NULL) {
    DEBUG((DEBUG_INFO, "ExtRootEntryTable 0x%x \n", mVtdUnitInformation[VtdIndex].ExtRootEntryTable));
    Reg64 = (UINT64)(UINTN)mVtdUnitInformation[VtdIndex].ExtRootEntryTable | (EnableADM ? V_RTADDR_REG_TTM_ADM : BIT11);
  } else {
    DEBUG((DEBUG_INFO, "RootEntryTable 0x%x \n", mVtdUnitInformation[VtdIndex].RootEntryTable));
    Reg64 = (UINT64)(UINTN)mVtdUnitInformation[VtdIndex].RootEntryTable | (EnableADM ? V_RTADDR_REG_TTM_ADM : 0);
  }
  MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_RTADDR_REG, Reg64);
}

/**
  Enable DMAR translation.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmar (
  VOID
  )
{
  UINTN     Index;
  UINTN     VtdUnitBaseAddress;
  BOOLEAN   TEWasEnabled;

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    VtdUnitBaseAddress = mVtdUnitInformation[Index].VtdUnitBaseAddress;
    DEBUG((DEBUG_INFO, ">>>>>>EnableDmar() for engine [%d] BAR [0x%x]\n", Index, VtdUnitBaseAddress));

    //
    // Check TE was enabled or not.
    //
    TEWasEnabled = ((MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG) & B_GSTS_REG_TE) == B_GSTS_REG_TE);

    if (TEWasEnabled && (mVtdUnitInformation[Index].ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
      //
      // For implementations reporting Enhanced SRTP Support (ESRTPS) field as
      // Clear in the Capability register, software must not modify this field while
      // DMA remapping is active (TES=1 in Global Status register).
      //
      if (mVtdUnitInformation[Index].CapReg.Bits.ESRTPS == 0) {
        VtdLibClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);
      }

      //
      // Enable ADM
      //
      UpdateRootTableAddressRegister (Index, TRUE);

      DEBUG((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
      VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

      DEBUG((DEBUG_INFO, "Enable Abort DMA Mode...\n"));
      VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

    } else {
      UpdateRootTableAddressRegister (Index, FALSE);

      DEBUG((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
      VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);
    }

    //
    // Write Buffer Flush before invalidation
    //
    VtdLibFlushWriteBuffer (VtdUnitBaseAddress);

    //
    // Invalidate the context cache
    //
    InvalidateContextCache (Index);

    //
    // Invalidate the IOTLB cache
    //
    InvalidateIOTLB (Index);

    if (TEWasEnabled && (mVtdUnitInformation[Index].ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
      if (mVtdUnitInformation[Index].CapReg.Bits.ESRTPS == 0) {
        VtdLibClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);
      }

      UpdateRootTableAddressRegister (Index, FALSE);

      DEBUG((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
      VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);
    }

    //
    // Enable VTd
    //
    DEBUG ((DEBUG_INFO, "EnableDmar: Waiting B_GSTS_REG_TE ...\n"));
    VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

    DEBUG ((DEBUG_INFO,"VTD (%d) enabled!<<<<<<\n",Index));

    VTdLogAddEvent (VTDLOG_DXE_ENABLE_DMAR, mVtdUnitInformation[Index].VtdUnitBaseAddress, 0);
  }

  //
  // Need disable PMR, since we already setup translation table.
  //
  DisablePmr ();

  mVtdEnabled = TRUE;

  return EFI_SUCCESS;
}

/**
  Disable DMAR translation.

  @retval EFI_SUCCESS           DMAR translation is disabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not disabled.
**/
EFI_STATUS
DisableDmar (
  VOID
  )
{
  UINTN                   Index;
  UINTN                   SubIndex;
  VTD_UNIT_INFORMATION    *VtdUnitInfo;

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    VtdUnitInfo = &mVtdUnitInformation[Index];

    VtdLibDisableDmar (VtdUnitInfo->VtdUnitBaseAddress);
    VTdLogAddEvent (VTDLOG_DXE_DISABLE_DMAR, VtdUnitInfo->VtdUnitBaseAddress, 0);

    if (VtdUnitInfo->EnableQueuedInvalidation != 0) {
      //
      // Disable queued invalidation interface.
      //
      VtdLibDisableQueuedInvalidationInterface (VtdUnitInfo->VtdUnitBaseAddress);
      VTdLogAddEvent (VTDLOG_DXE_QUEUED_INVALIDATION, VTD_LOG_QI_DISABLE, VtdUnitInfo->VtdUnitBaseAddress);

      //
      // Free descriptor queue memory
      //
      if (VtdUnitInfo->QiDescBuffer != NULL) {
        FreePages(VtdUnitInfo->QiDescBuffer, EFI_SIZE_TO_PAGES (VtdUnitInfo->QiDescBufferSize));
        VtdUnitInfo->QiDescBuffer = NULL;
        VtdUnitInfo->QiDescBufferSize = 0;
      }

      VtdUnitInfo->EnableQueuedInvalidation = 0;
    }
  }

  mVtdEnabled = FALSE;

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    VtdUnitInfo = &mVtdUnitInformation[Index];
    DEBUG((DEBUG_INFO, "engine [%d] access\n", Index));
    for (SubIndex = 0; SubIndex < VtdUnitInfo->PciDeviceInfo->PciDeviceDataNumber; SubIndex++) {
      DEBUG ((DEBUG_INFO, "  PCI S%04X B%02x D%02x F%02x - %d\n",
        VtdUnitInfo->Segment,
        VtdUnitInfo->PciDeviceInfo->PciDeviceData[Index].PciSourceId.Bits.Bus,
        VtdUnitInfo->PciDeviceInfo->PciDeviceData[Index].PciSourceId.Bits.Device,
        VtdUnitInfo->PciDeviceInfo->PciDeviceData[Index].PciSourceId.Bits.Function,
        VtdUnitInfo->PciDeviceInfo->PciDeviceData[Index].AccessCount
        ));
    }
  }

  return EFI_SUCCESS;
}

/**
  Dump VTd version registers.

  @param[in]  VerReg            The version register.
**/
VOID
DumpVtdVerRegs (
  IN VTD_VER_REG                *VerReg
  )
{
  DEBUG ((DEBUG_INFO, "   VerReg - 0x%x\n", VerReg->Uint32));
  DEBUG ((DEBUG_INFO, "    Major - 0x%x\n", VerReg->Bits.Major));
  DEBUG ((DEBUG_INFO, "    Minor - 0x%x\n", VerReg->Bits.Minor));
}

/**
  Dump VTd capability registers.

  @param[in]  CapReg              The capability register.
**/
VOID
DumpVtdCapRegs (
  IN VTD_CAP_REG *CapReg
  )
{
  DEBUG((DEBUG_INFO, "  CapReg   - 0x%lx\n", CapReg->Uint64));
  DEBUG((DEBUG_INFO, "    ND     - 0x%x\n", CapReg->Bits.ND));
  DEBUG((DEBUG_INFO, "    AFL    - 0x%x\n", CapReg->Bits.AFL));
  DEBUG((DEBUG_INFO, "    RWBF   - 0x%x\n", CapReg->Bits.RWBF));
  DEBUG((DEBUG_INFO, "    PLMR   - 0x%x\n", CapReg->Bits.PLMR));
  DEBUG((DEBUG_INFO, "    PHMR   - 0x%x\n", CapReg->Bits.PHMR));
  DEBUG((DEBUG_INFO, "    CM     - 0x%x\n", CapReg->Bits.CM));
  DEBUG((DEBUG_INFO, "    SAGAW  - 0x%x\n", CapReg->Bits.SAGAW));
  DEBUG((DEBUG_INFO, "    MGAW   - 0x%x\n", CapReg->Bits.MGAW));
  DEBUG((DEBUG_INFO, "    ZLR    - 0x%x\n", CapReg->Bits.ZLR));
  DEBUG((DEBUG_INFO, "    FRO    - 0x%x\n", CapReg->Bits.FRO));
  DEBUG((DEBUG_INFO, "    SLLPS  - 0x%x\n", CapReg->Bits.SLLPS));
  DEBUG((DEBUG_INFO, "    PSI    - 0x%x\n", CapReg->Bits.PSI));
  DEBUG((DEBUG_INFO, "    NFR    - 0x%x\n", CapReg->Bits.NFR));
  DEBUG((DEBUG_INFO, "    MAMV   - 0x%x\n", CapReg->Bits.MAMV));
  DEBUG((DEBUG_INFO, "    DWD    - 0x%x\n", CapReg->Bits.DWD));
  DEBUG((DEBUG_INFO, "    DRD    - 0x%x\n", CapReg->Bits.DRD));
  DEBUG((DEBUG_INFO, "    FL1GP  - 0x%x\n", CapReg->Bits.FL1GP));
  DEBUG((DEBUG_INFO, "    PI     - 0x%x\n", CapReg->Bits.PI));
}

/**
  Dump VTd extended capability registers.

  @param[in]  ECapReg              The extended capability register.
**/
VOID
DumpVtdECapRegs (
  IN VTD_ECAP_REG *ECapReg
  )
{
  DEBUG((DEBUG_INFO, "  ECapReg  - 0x%lx\n", ECapReg->Uint64));
  DEBUG((DEBUG_INFO, "    C      - 0x%x\n", ECapReg->Bits.C));
  DEBUG((DEBUG_INFO, "    QI     - 0x%x\n", ECapReg->Bits.QI));
  DEBUG((DEBUG_INFO, "    DT     - 0x%x\n", ECapReg->Bits.DT));
  DEBUG((DEBUG_INFO, "    IR     - 0x%x\n", ECapReg->Bits.IR));
  DEBUG((DEBUG_INFO, "    EIM    - 0x%x\n", ECapReg->Bits.EIM));
  DEBUG((DEBUG_INFO, "    PT     - 0x%x\n", ECapReg->Bits.PT));
  DEBUG((DEBUG_INFO, "    SC     - 0x%x\n", ECapReg->Bits.SC));
  DEBUG((DEBUG_INFO, "    IRO    - 0x%x\n", ECapReg->Bits.IRO));
  DEBUG((DEBUG_INFO, "    MHMV   - 0x%x\n", ECapReg->Bits.MHMV));
  DEBUG((DEBUG_INFO, "    MTS    - 0x%x\n", ECapReg->Bits.MTS));
  DEBUG((DEBUG_INFO, "    NEST   - 0x%x\n", ECapReg->Bits.NEST));
  DEBUG((DEBUG_INFO, "    PASID  - 0x%x\n", ECapReg->Bits.PASID));
  DEBUG((DEBUG_INFO, "    PRS    - 0x%x\n", ECapReg->Bits.PRS));
  DEBUG((DEBUG_INFO, "    ERS    - 0x%x\n", ECapReg->Bits.ERS));
  DEBUG((DEBUG_INFO, "    SRS    - 0x%x\n", ECapReg->Bits.SRS));
  DEBUG((DEBUG_INFO, "    NWFS   - 0x%x\n", ECapReg->Bits.NWFS));
  DEBUG((DEBUG_INFO, "    EAFS   - 0x%x\n", ECapReg->Bits.EAFS));
  DEBUG((DEBUG_INFO, "    PSS    - 0x%x\n", ECapReg->Bits.PSS));
  DEBUG((DEBUG_INFO, "    SMTS   - 0x%x\n", ECapReg->Bits.SMTS));
  DEBUG((DEBUG_INFO, "    ADMS   - 0x%x\n", ECapReg->Bits.ADMS));
  DEBUG((DEBUG_INFO, "    PDS    - 0x%x\n", ECapReg->Bits.PDS));
}

/**
  Dump VTd registers.

  @param[in]  VtdUnitBaseAddress    The base address of the VTd engine.
**/
VOID
DumpVtdRegs (
  IN UINTN  VtdUnitBaseAddress
  )
{
  VTD_REGESTER_INFO                *VtdRegInfo;
  VTD_ECAP_REG                     ECapReg;
  VTD_CAP_REG                      CapReg;

  if (mVtdRegsInfoBuffer == NULL) {
    return;
  }

  VtdRegInfo              = mVtdRegsInfoBuffer;
  VtdRegInfo->BaseAddress = VtdUnitBaseAddress;
  VtdRegInfo->VerReg      = MmioRead32 (VtdUnitBaseAddress + R_VER_REG);
  VtdRegInfo->CapReg      = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
  VtdRegInfo->EcapReg     = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
  VtdRegInfo->GstsReg     = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  VtdRegInfo->RtaddrReg   = MmioRead64 (VtdUnitBaseAddress + R_RTADDR_REG);
  VtdRegInfo->CcmdReg     = MmioRead64 (VtdUnitBaseAddress + R_CCMD_REG);
  VtdRegInfo->FstsReg     = MmioRead32 (VtdUnitBaseAddress + R_FSTS_REG);
  VtdRegInfo->FectlReg    = MmioRead32 (VtdUnitBaseAddress + R_FECTL_REG);
  VtdRegInfo->FedataReg   = MmioRead32 (VtdUnitBaseAddress + R_FEDATA_REG);
  VtdRegInfo->FeaddrReg   = MmioRead32 (VtdUnitBaseAddress + R_FEADDR_REG);
  VtdRegInfo->FeuaddrReg  = MmioRead32 (VtdUnitBaseAddress + R_FEUADDR_REG);
  VtdRegInfo->IqercdReg   = MmioRead64 (VtdUnitBaseAddress + R_IQERCD_REG);

  CapReg.Uint64 = VtdRegInfo->CapReg;
  for (VtdRegInfo->FrcdRegNum = 0; VtdRegInfo->FrcdRegNum < (UINT16) CapReg.Bits.NFR + 1; VtdRegInfo->FrcdRegNum++) {
    VtdRegInfo->FrcdReg[VtdRegInfo->FrcdRegNum].Uint64Lo = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (VtdRegInfo->FrcdRegNum * 16) + R_FRCD_REG));
    VtdRegInfo->FrcdReg[VtdRegInfo->FrcdRegNum].Uint64Hi = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (VtdRegInfo->FrcdRegNum * 16) + R_FRCD_REG + sizeof(UINT64)));
  }

  ECapReg.Uint64 = VtdRegInfo->EcapReg;
  VtdRegInfo->IvaReg = MmioRead64 (VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IVA_REG);
  VtdRegInfo->IotlbReg = MmioRead64 (VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);

  DEBUG((DEBUG_INFO, "#### DumpVtdRegs(0x%016lx) Begin ####\n", VtdUnitBaseAddress));

  VtdLibDumpVtdRegsAll (NULL, NULL, VtdRegInfo);

  DEBUG((DEBUG_INFO, "#### DumpVtdRegs(0x%016lx) End ####\n", VtdUnitBaseAddress));

  VTdLogAddDataEvent (VTDLOG_DXE_REGISTER, VTDLOG_REGISTER_ALL, (VOID *) VtdRegInfo, sizeof (VTD_REGESTER_INFO) + sizeof (VTD_UINT128) * (VtdRegInfo->FrcdRegNum - 1));
}

/**
  Dump VTd registers for all VTd engine.
**/
VOID
DumpVtdRegsAll (
  VOID
  )
{
  UINTN       VtdIndex;

  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    DumpVtdRegs (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress);
  }
}

/**
  Dump VTd registers if there is error.
**/
VOID
DumpVtdIfError (
  VOID
  )
{
  UINTN         Num;
  UINTN         Index;
  VTD_FRCD_REG  FrcdReg;
  VTD_CAP_REG   CapReg;
  UINT32        Reg32;
  BOOLEAN       HasError;

  for (Num = 0; Num < mVtdUnitNumber; Num++) {
    HasError = FALSE;
    Reg32 = MmioRead32 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_FSTS_REG);
    if (Reg32 != 0) {
      HasError = TRUE;
    }
    Reg32 = MmioRead32 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_FECTL_REG);
    if ((Reg32 & BIT30) != 0) {
      HasError = TRUE;
    }

    CapReg.Uint64 = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_CAP_REG);
    for (Index = 0; Index < (UINTN)CapReg.Bits.NFR + 1; Index++) {
      FrcdReg.Uint64[0] = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG));
      FrcdReg.Uint64[1] = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
      if (FrcdReg.Bits.F != 0) {
        HasError = TRUE;
      }
    }

    if (HasError) {
      REPORT_STATUS_CODE (EFI_ERROR_CODE, PcdGet32 (PcdErrorCodeVTdError));
      DEBUG((DEBUG_INFO, "\n#### ERROR ####\n"));
      DumpVtdRegs (mVtdUnitInformation[Num].VtdUnitBaseAddress);
      DEBUG((DEBUG_INFO, "#### ERROR ####\n\n"));
      //
      // Clear
      //
      for (Index = 0; Index < (UINTN)CapReg.Bits.NFR + 1; Index++) {
        FrcdReg.Uint64[1] = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
        if (FrcdReg.Bits.F != 0) {
          //
          // Software writes the value read from this field (F) to Clear it.
          //
          MmioWrite64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)), FrcdReg.Uint64[1]);
        }
      }
      MmioWrite32 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_FSTS_REG, MmioRead32 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_FSTS_REG));
    }
  }
}
