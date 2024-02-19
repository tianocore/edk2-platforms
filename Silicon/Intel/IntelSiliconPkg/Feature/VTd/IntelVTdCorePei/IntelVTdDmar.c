/** @file

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeiServicesLib.h>
#include <IndustryStandard/Vtd.h>
#include <Ppi/VtdInfo.h>
#include <Ppi/VtdNullRootEntryTable.h>
#include <Ppi/IoMmu.h>
#include <Guid/VtdLogDataHob.h>
#include "IntelVTdCorePei.h"

#define VTD_CAP_REG_NFR_MAX (256)

/**
  Flush VTD page table and context table memory.

  This action is to make sure the IOMMU engine can get final data in memory.

  @param[in]  VtdUnitInfo       The VTd engine unit information.
  @param[in]  Base              The base address of memory to be flushed.
  @param[in]  Size              The size of memory in bytes to be flushed.
**/
VOID
FlushPageTableMemory (
  IN VTD_UNIT_INFO              *VtdUnitInfo,
  IN UINTN                      Base,
  IN UINTN                      Size
  )
{
  if (VtdUnitInfo->ECapReg.Bits.C == 0) {
    WriteBackDataCacheRange ((VOID *) Base, Size);
  }
}

/**
  Perpare cache invalidation interface.

  @param[in]  VtdUnitInfo       The VTd engine unit information.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_UNSUPPORTED       Invalidation method is not supported.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
PerpareCacheInvalidationInterface (
  IN VTD_UNIT_INFO *VtdUnitInfo
  )
{
  UINT32         Reg32;
  VTD_ECAP_REG   ECapReg;
  VTD_IQA_REG    IqaReg;
  UINTN          VtdUnitBaseAddress;

  VtdUnitBaseAddress = VtdUnitInfo->VtdUnitBaseAddress;

  if (VtdUnitInfo->VerReg.Bits.Major <= 5) {
    VtdUnitInfo->EnableQueuedInvalidation = 0;
    DEBUG ((DEBUG_INFO, "Use Register-based Invalidation Interface for engine [0x%x]\n", VtdUnitBaseAddress));
    return EFI_SUCCESS;
  }

  ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
  if (ECapReg.Bits.QI == 0) {
    DEBUG ((DEBUG_ERROR, "Hardware does not support queued invalidations interface for engine [0x%x]\n", VtdUnitBaseAddress));
    return EFI_UNSUPPORTED;
  }

  VtdUnitInfo->EnableQueuedInvalidation = 1;
  DEBUG ((DEBUG_INFO, "Use Queued Invalidation Interface for engine [0x%x]\n", VtdUnitBaseAddress));

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  if ((Reg32 & B_GSTS_REG_QIES) != 0) {
    DEBUG ((DEBUG_INFO,"Queued Invalidation Interface was enabled.\n"));

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
      VTdLogAddEvent (VTDLOG_PEI_QUEUED_INVALIDATION, VTD_LOG_QI_ERROR_OUT_OF_RESOURCES, VtdUnitBaseAddress);
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

  VTdLogAddEvent (VTDLOG_PEI_QUEUED_INVALIDATION, VTD_LOG_QI_ENABLE, VtdUnitBaseAddress);

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
  VTD_REGESTER_QI_INFO  RegisterQi;

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

  @param[in]  VtdUnitInfo       The VTd engine unit information.
**/
EFI_STATUS
InvalidateContextCache (
  IN VTD_UNIT_INFO              *VtdUnitInfo
  )
{
  UINT64                        Reg64;
  QI_256_DESC                   QiDesc;

  if (VtdUnitInfo->EnableQueuedInvalidation == 0) {
    //
    // Register-based Invalidation
    //
    Reg64 = MmioRead64 (VtdUnitInfo->VtdUnitBaseAddress + R_CCMD_REG);
    if ((Reg64 & B_CCMD_REG_ICC) != 0) {
      DEBUG ((DEBUG_ERROR,"ERROR: InvalidateContextCache: B_CCMD_REG_ICC is set for VTD(%x)\n", VtdUnitInfo->VtdUnitBaseAddress));
      return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
    Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_GLOBAL);
    MmioWrite64 (VtdUnitInfo->VtdUnitBaseAddress + R_CCMD_REG, Reg64);

    do {
      Reg64 = MmioRead64 (VtdUnitInfo->VtdUnitBaseAddress + R_CCMD_REG);
    } while ((Reg64 & B_CCMD_REG_ICC) != 0);
  } else {
    //
    // Queued Invalidation
    //
    QiDesc.Uint64[0] = QI_CC_FM(0) | QI_CC_SID(0) | QI_CC_DID(0) | QI_CC_GRAN(1) | QI_CC_TYPE;
    QiDesc.Uint64[1] = 0;
    QiDesc.Uint64[2] = 0;
    QiDesc.Uint64[3] = 0;

    return SubmitQueuedInvalidationDescriptor(VtdUnitInfo->VtdUnitBaseAddress, &QiDesc);
  }

  return EFI_SUCCESS;
}

/**
  Invalidate VTd IOTLB.

  @param[in]  VtdUnitInfo       The VTd engine unit information.
**/
EFI_STATUS
InvalidateIOTLB (
  IN VTD_UNIT_INFO              *VtdUnitInfo
  )
{
  UINT64                        Reg64;
  VTD_ECAP_REG                  ECapReg;
  VTD_CAP_REG                   CapReg;
  QI_256_DESC                   QiDesc;

  if (VtdUnitInfo->EnableQueuedInvalidation == 0) {
    //
    // Register-based Invalidation
    //
    ECapReg.Uint64 = MmioRead64 (VtdUnitInfo->VtdUnitBaseAddress + R_ECAP_REG);

    Reg64 = MmioRead64 (VtdUnitInfo->VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
     if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
       DEBUG ((DEBUG_ERROR, "ERROR: InvalidateIOTLB: B_IOTLB_REG_IVT is set for VTD(%x)\n", VtdUnitInfo->VtdUnitBaseAddress));
       return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
    Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_GLOBAL);
    MmioWrite64 (VtdUnitInfo->VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

    do {
      Reg64 = MmioRead64 (VtdUnitInfo->VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    } while ((Reg64 & B_IOTLB_REG_IVT) != 0);
  } else {
    //
    // Queued Invalidation
    //
    CapReg.Uint64 = MmioRead64 (VtdUnitInfo->VtdUnitBaseAddress + R_CAP_REG);
    QiDesc.Uint64[0] = QI_IOTLB_DID(0) | (CapReg.Bits.DRD ? QI_IOTLB_DR(1) : QI_IOTLB_DR(0)) | (CapReg.Bits.DWD ? QI_IOTLB_DW(1) : QI_IOTLB_DW(0)) | QI_IOTLB_GRAN(1) | QI_IOTLB_TYPE;
    QiDesc.Uint64[1] = QI_IOTLB_ADDR(0) | QI_IOTLB_IH(0) | QI_IOTLB_AM(0);
    QiDesc.Uint64[2] = 0;
    QiDesc.Uint64[3] = 0;

    return SubmitQueuedInvalidationDescriptor(VtdUnitInfo->VtdUnitBaseAddress, &QiDesc);
  }

  return EFI_SUCCESS;
}

/**
  Enable DMAR translation in pre-mem phase.

  @param[in]  VtdUnitBaseAddress  The base address of the VTd engine.
  @param[in]  RtaddrRegValue      The value of RTADDR_REG.

  @retval EFI_SUCCESS             DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR        DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmarPreMem (
  IN UINTN                        VtdUnitBaseAddress,
  IN UINT64                       RtaddrRegValue
  )
{
  UINT32                          Reg32;

  DEBUG ((DEBUG_INFO, ">>>>>>EnableDmarPreMem() for engine [%x] \n", VtdUnitBaseAddress));

  DEBUG ((DEBUG_INFO, "RTADDR_REG : 0x%016lx \n", RtaddrRegValue));
  MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, RtaddrRegValue);

  DEBUG ((DEBUG_INFO, "EnableDmarPreMem: waiting for RTPS bit to be set... \n"));
  VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  DEBUG ((DEBUG_INFO, "EnableDmarPreMem: R_GSTS_REG = 0x%x \n", Reg32));

  //
  // Write Buffer Flush
  //
  VtdLibFlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Enable VTd
  //
  VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  DEBUG ((DEBUG_INFO, "VTD () enabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Enable DMAR translation.

  @param[in]  VtdUnitInfo       The VTd engine unit information.
  @param[in]  RootEntryTable    The address of the VTd RootEntryTable.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmar (
  IN VTD_UNIT_INFO              *VtdUnitInfo,
  IN UINTN                      RootEntryTable
  )
{
  UINTN                         VtdUnitBaseAddress;
  BOOLEAN                       TEWasEnabled;

  VtdUnitBaseAddress = VtdUnitInfo->VtdUnitBaseAddress;

  DEBUG ((DEBUG_INFO, ">>>>>>EnableDmar() for engine [%x] \n", VtdUnitBaseAddress));

  //
  // Check TE was enabled or not.
  //
  TEWasEnabled = ((MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG) & B_GSTS_REG_TE) == B_GSTS_REG_TE);

  if (TEWasEnabled && (VtdUnitInfo->ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
    //
    // For implementations reporting Enhanced SRTP Support (ESRTPS) field as
    // Clear in the Capability register, software must not modify this field while
    // DMA remapping is active (TES=1 in Global Status register).
    //
    if (VtdUnitInfo->CapReg.Bits.ESRTPS == 0) {
      VtdLibClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);
    }

    //
    // Enable ADM
    //
    MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64) (RootEntryTable | V_RTADDR_REG_TTM_ADM));

    DEBUG ((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

    DEBUG ((DEBUG_INFO, "Enable Abort DMA Mode...\n"));
    VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  } else {
    DEBUG ((DEBUG_INFO, "RootEntryTable 0x%x \n", RootEntryTable));
    MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64) RootEntryTable);

    DEBUG ((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);
  }

  //
  // Write Buffer Flush before invalidation
  //
  VtdLibFlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Invalidate the context cache
  //
  InvalidateContextCache (VtdUnitInfo);

  //
  // Invalidate the IOTLB cache
  //
  InvalidateIOTLB (VtdUnitInfo);

  if (TEWasEnabled && (VtdUnitInfo->ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
    if (VtdUnitInfo->CapReg.Bits.ESRTPS == 0) {
      VtdLibClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);
    }

    DEBUG ((DEBUG_INFO, "RootEntryTable 0x%x \n", RootEntryTable));
    MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64) RootEntryTable);

    DEBUG ((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);
  }

  //
  // Enable VTd
  //
  VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  DEBUG ((DEBUG_INFO, "VTD () enabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Enable VTd translation table protection for block DMA

  @param[in] VtdUnitBaseAddress The base address of the VTd engine.

  @retval EFI_SUCCESS         DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR    DMAR translation is not enabled.
**/
EFI_STATUS
EnableVTdTranslationProtectionBlockDma (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  EFI_STATUS                            Status;
  VTD_ECAP_REG                          ECapReg;
  EDKII_VTD_NULL_ROOT_ENTRY_TABLE_PPI   *RootEntryTable;
  UINT8                                 Mode;

  DEBUG ((DEBUG_INFO, "EnableVTdTranslationProtectionBlockDma - 0x%08x\n", VtdUnitBaseAddress));

  DEBUG ((DEBUG_INFO, "PcdVTdSupportAbortDmaMode : %d\n", PcdGetBool (PcdVTdSupportAbortDmaMode)));

  ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
  DEBUG ((DEBUG_INFO, "ECapReg.ADMS : %d\n", ECapReg.Bits.ADMS));

  if ((ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
    Mode = VTD_LOG_PEI_PRE_MEM_ADM;
    //
    // Use Abort DMA Mode
    //
    DEBUG ((DEBUG_INFO, "Enable abort DMA mode.\n"));
    Status = EnableDmarPreMem (VtdUnitBaseAddress, V_RTADDR_REG_TTM_ADM);
  } else {
    //
    // Use Null Root Entry Table
    //
    Status = PeiServicesLocatePpi (
               &gEdkiiVTdNullRootEntryTableGuid,
               0,
               NULL,
               (VOID **)&RootEntryTable
               );
    if (EFI_ERROR (Status)) {
      Mode = VTD_LOG_PEI_PRE_MEM_DISABLE;
      DEBUG ((DEBUG_ERROR, "Locate Null Root Entry Table Ppi Failed : %r\n", Status));
      ASSERT (FALSE);
    } else {
      Mode = VTD_LOG_PEI_PRE_MEM_TE;
      DEBUG ((DEBUG_INFO, "Block All DMA by TE.\n"));
      Status = EnableDmarPreMem (VtdUnitBaseAddress, (UINT64) (*RootEntryTable));
    }
  }

  VTdLogAddPreMemoryEvent (VtdUnitBaseAddress, Mode, EFI_ERROR (Status) ? 0 : 1);

  return Status;
}

/**
  Enable VTd translation table protection.

  @param[in]  VTdInfo           The VTd engine context information.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableVTdTranslationProtection (
  IN VTD_INFO                   *VTdInfo
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  VTD_UNIT_INFO                 *VtdUnitInfo;

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    VtdUnitInfo = &VTdInfo->VtdUnitInfo[Index];
    if (VtdUnitInfo->Done) {
      DEBUG ((DEBUG_INFO, "EnableVtdDmar (%d) was enabled\n", Index));
      continue;
    }

    if (VtdUnitInfo->ExtRootEntryTable != 0) {
      DEBUG ((DEBUG_INFO, "EnableVtdDmar (%d) ExtRootEntryTable 0x%x\n", Index, VtdUnitInfo->ExtRootEntryTable));
      Status = EnableDmar (VtdUnitInfo, VtdUnitInfo->ExtRootEntryTable | BIT11);
    } else {
      DEBUG ((DEBUG_INFO, "EnableVtdDmar (%d) RootEntryTable 0x%x\n", Index, VtdUnitInfo->RootEntryTable));
      Status = EnableDmar (VtdUnitInfo, VtdUnitInfo->RootEntryTable);
    }

    VTdLogAddEvent (VTDLOG_PEI_POST_MEM_ENABLE_DMA_PROTECT, VTdInfo->VtdUnitInfo[Index].VtdUnitBaseAddress, Status);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EnableVtdDmar (%d) Failed !\n", Index));
      return Status;
    }
    VtdUnitInfo->Done = TRUE;
  }
  return EFI_SUCCESS;
}

/**
  Disable VTd translation table protection.

  @param[in]  VTdInfo           The VTd engine context information.
**/
VOID
DisableVTdTranslationProtection (
  IN VTD_INFO                   *VTdInfo
  )
{
  UINTN                         Index;
  VTD_UNIT_INFO                 *VtdUnitInfo;

  if (VTdInfo == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "DisableVTdTranslationProtection - %d Vtd Engine\n", VTdInfo->VTdEngineCount));

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    VtdUnitInfo = &VTdInfo->VtdUnitInfo[Index];

    VtdLibDisableDmar (VtdUnitInfo->VtdUnitBaseAddress);
    VTdLogAddEvent (VTDLOG_PEI_POST_MEM_DISABLE_DMA_PROTECT, VtdUnitInfo->VtdUnitBaseAddress, 0);

    if (VtdUnitInfo->EnableQueuedInvalidation != 0) {
      //
      // Disable queued invalidation interface.
      //
      VtdLibDisableQueuedInvalidationInterface (VtdUnitInfo->VtdUnitBaseAddress);

      if (VtdUnitInfo->QiDescBuffer != NULL) {
        FreePages(VtdUnitInfo->QiDescBuffer, EFI_SIZE_TO_PAGES (VtdUnitInfo->QiDescBufferSize));
        VtdUnitInfo->QiDescBuffer = NULL;
        VtdUnitInfo->QiDescBufferSize = 0;
      }

      VtdUnitInfo->EnableQueuedInvalidation = 0;
      VTdLogAddEvent (VTDLOG_PEI_QUEUED_INVALIDATION, VTD_LOG_QI_DISABLE, VtdUnitInfo->VtdUnitBaseAddress);
    }
  }

  return;
}

/**
  Check if VTd engine use 5 level paging.

  @param[in]  HostAddressWidth  Host Address Width.
  @param[in]  VtdUnitInfo       The VTd engine unit information.
  @param[out] Is5LevelPaging    Use 5 level paging or not

  @retval EFI_SUCCESS           Success
  @retval EFI_UNSUPPORTED       Feature is not support

**/
EFI_STATUS
VtdCheckUsing5LevelPaging (
  IN  UINT8                            HostAddressWidth,
  IN  VTD_CAP_REG                      CapReg,
  OUT BOOLEAN                          *Is5LevelPaging
  )
{
  DEBUG ((DEBUG_INFO, "  CapReg SAGAW bits : 0x%02x\n", CapReg.Bits.SAGAW));

  *Is5LevelPaging = FALSE;
  if ((CapReg.Bits.SAGAW & BIT3) != 0) {
    *Is5LevelPaging = TRUE;
    if ((HostAddressWidth <= 48) &&
         ((CapReg.Bits.SAGAW & BIT2) != 0)) {
      *Is5LevelPaging = FALSE;
    } else {
      return EFI_UNSUPPORTED;
    }
  }
  if ((CapReg.Bits.SAGAW & (BIT3 | BIT2)) == 0) {
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_INFO, "  Using %d Level Paging\n", *Is5LevelPaging ? 5 : 4));
  return EFI_SUCCESS;
}


/**
  Prepare VTD configuration.

  @param[in]  VTdInfo           The VTd engine context information.

  @retval EFI_SUCCESS           Prepare Vtd config success
**/
EFI_STATUS
PrepareVtdConfig (
  IN VTD_INFO                   *VTdInfo
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  VTD_UNIT_INFO                 *VtdUnitInfo;
  UINTN                         VtdUnitBaseAddress;

  if (VTdInfo->RegsInfoBuffer == NULL) {
    VTdInfo->RegsInfoBuffer = AllocateZeroPages (EFI_SIZE_TO_PAGES (sizeof (VTD_REGESTER_THIN_INFO) + sizeof (VTD_UINT128) * VTD_CAP_REG_NFR_MAX));
    ASSERT (VTdInfo->RegsInfoBuffer != NULL);
  }

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    VtdUnitInfo = &VTdInfo->VtdUnitInfo[Index];
    if (VtdUnitInfo->Done) {
      continue;
    }
    VtdUnitBaseAddress = VtdUnitInfo->VtdUnitBaseAddress;
    DEBUG ((DEBUG_INFO, "VTd Engine: 0x%08X\n", VtdUnitBaseAddress));

    VtdUnitInfo->VerReg.Uint32 = MmioRead32 (VtdUnitBaseAddress + R_VER_REG);
    VtdUnitInfo->CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
    VtdUnitInfo->ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
    DEBUG ((DEBUG_INFO, "  VER_REG  : 0x%08X\n", VtdUnitInfo->VerReg.Uint32));
    DEBUG ((DEBUG_INFO, "  CAP_REG  : 0x%016lX\n", VtdUnitInfo->CapReg.Uint64));
    DEBUG ((DEBUG_INFO, "  ECAP_REG : 0x%016lX\n", VtdUnitInfo->ECapReg.Uint64));

    Status = VtdCheckUsing5LevelPaging (VTdInfo->HostAddressWidth, VtdUnitInfo->CapReg, &(VtdUnitInfo->Is5LevelPaging));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "!!!! Page-table type 0x%X is not supported!!!!\n", VtdUnitInfo->CapReg.Bits.SAGAW));
      return Status;
    }

    Status = PerpareCacheInvalidationInterface(&VTdInfo->VtdUnitInfo[Index]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Dump VTd registers if there is error.
**/
VOID
DumpVtdIfError (
  VOID
  )
{
  VTD_INFO                       *VTdInfo;
  UINTN                          Num;
  UINTN                          VtdUnitBaseAddress;
  UINT16                         Index;
  VTD_REGESTER_THIN_INFO         *VtdRegInfo;
  VTD_FRCD_REG                   FrcdReg;
  VTD_CAP_REG                    CapReg;
  UINT32                         FstsReg32;
  UINT32                         FectlReg32;
  BOOLEAN                        HasError;

  VTdInfo = GetVTdInfoHob ();
  if (VTdInfo == NULL) {
    return;
  }

  VtdRegInfo = VTdInfo->RegsInfoBuffer;
  if (VtdRegInfo == NULL) {
    return;
  }

  for (Num = 0; Num < VTdInfo->VTdEngineCount; Num++) {
    HasError = FALSE;
    VtdUnitBaseAddress = VTdInfo->VtdUnitInfo[Num].VtdUnitBaseAddress;
    FstsReg32 = MmioRead32 (VtdUnitBaseAddress + R_FSTS_REG);
    if (FstsReg32 != 0) {
      HasError = TRUE;
    }
    FectlReg32 = MmioRead32 (VtdUnitBaseAddress + R_FECTL_REG);
    if ((FectlReg32 & BIT30) != 0) {
      HasError = TRUE;
    }

    CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
    for (Index = 0; Index < (UINT16) CapReg.Bits.NFR + 1; Index++) {
      FrcdReg.Uint64[0] = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG));
      FrcdReg.Uint64[1] = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
      if (FrcdReg.Bits.F != 0) {
        HasError = TRUE;
        break;
      }
    }

    if (HasError) {
      DEBUG ((DEBUG_INFO, "\n#### ERROR ####\n"));

      VtdRegInfo->BaseAddress = VtdUnitBaseAddress;
      VtdRegInfo->GstsReg     = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
      VtdRegInfo->RtaddrReg   = MmioRead64 (VtdUnitBaseAddress + R_RTADDR_REG);;
      VtdRegInfo->FstsReg     = FstsReg32;
      VtdRegInfo->FectlReg    = FectlReg32;
      VtdRegInfo->IqercdReg   = MmioRead64 (VtdUnitBaseAddress + R_IQERCD_REG);

      CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
      for (Index = 0; Index < (UINT16) CapReg.Bits.NFR + 1; Index++) {
        VtdRegInfo->FrcdReg[Index].Uint64Lo = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG));
        VtdRegInfo->FrcdReg[Index].Uint64Hi = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
      }
      VtdRegInfo->FrcdRegNum = Index;

      DEBUG ((DEBUG_INFO, "\n#### ERROR ####\n"));

      VtdLibDumpVtdRegsThin (NULL, NULL, VtdRegInfo);

      DEBUG ((DEBUG_INFO, "#### ERROR ####\n\n"));

      VTdLogAddDataEvent (VTDLOG_PEI_REGISTER, VTDLOG_REGISTER_THIN, VtdRegInfo, sizeof (VTD_REGESTER_THIN_INFO) + sizeof (VTD_UINT128) * (VtdRegInfo->FrcdRegNum - 1));

      //
      // Clear
      //
      for (Index = 0; Index < (UINT16) CapReg.Bits.NFR + 1; Index++) {
        FrcdReg.Uint64[1] = MmioRead64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
        if (FrcdReg.Bits.F != 0) {
          //
          // Software writes the value read from this field (F) to Clear it.
          //
          MmioWrite64 (VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)), FrcdReg.Uint64[1]);
        }
      }
      MmioWrite32 (VtdUnitBaseAddress + R_FSTS_REG, MmioRead32 (VtdUnitBaseAddress + R_FSTS_REG));
    }
  }
}