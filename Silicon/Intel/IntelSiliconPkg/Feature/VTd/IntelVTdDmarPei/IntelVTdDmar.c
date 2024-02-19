/** @file

  Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>

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
#include "IntelVTdDmarPei.h"

VOID
SetGlobalCommandRegisterBits (
  IN UINTN     VtdUnitBaseAddress,
  IN UINT32    BitMask
  );

VOID
ClearGlobalCommandRegisterBits (
  IN UINTN     VtdUnitBaseAddress,
  IN UINT32    BitMask
  );

/**
  Flush VTD page table and context table memory.

  This action is to make sure the IOMMU engine can get final data in memory.

  @param[in]  VTdUnitInfo       The VTd engine unit information.
  @param[in]  Base              The base address of memory to be flushed.
  @param[in]  Size              The size of memory in bytes to be flushed.
**/
VOID
FlushPageTableMemory (
  IN VTD_UNIT_INFO              *VTdUnitInfo,
  IN UINTN                      Base,
  IN UINTN                      Size
  )
{
  if (VTdUnitInfo->ECapReg.Bits.C == 0) {
    WriteBackDataCacheRange ((VOID *) Base, Size);
  }
}

/**
  Flush VTd engine write buffer.

  @param[in]  VtdUnitBaseAddress        The base address of the VTd engine.
**/
VOID
FlushWriteBuffer (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  UINT32                        Reg32;
  VTD_CAP_REG                   CapReg;

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);

  if (CapReg.Bits.RWBF != 0) {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
    Reg32 = (Reg32 & 0x96FFFFFF);       // Reset the one-shot bits
    MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Reg32 | B_GMCD_REG_WBF);
    do {
      Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
    } while ((Reg32 & B_GSTS_REG_WBF) != 0);
  }
}

/**
  Perpare cache invalidation interface.

  @param[in]  VTdUnitInfo       The VTd engine unit information.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_UNSUPPORTED       Invalidation method is not supported.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
PerpareCacheInvalidationInterface (
  IN VTD_UNIT_INFO *VTdUnitInfo
  )
{
  UINT32         Reg32;
  VTD_ECAP_REG   ECapReg;
  VTD_IQA_REG    IqaReg;
  UINTN          VtdUnitBaseAddress;

  VtdUnitBaseAddress = VTdUnitInfo->VtdUnitBaseAddress;

  if (VTdUnitInfo->VerReg.Bits.Major <= 5) {
    VTdUnitInfo->EnableQueuedInvalidation = 0;
    DEBUG ((DEBUG_INFO, "Use Register-based Invalidation Interface for engine [0x%x]\n", VtdUnitBaseAddress));
    return EFI_SUCCESS;
  }

  ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
  if (ECapReg.Bits.QI == 0) {
    DEBUG ((DEBUG_ERROR, "Hardware does not support queued invalidations interface for engine [0x%x]\n", VtdUnitBaseAddress));
    return EFI_UNSUPPORTED;
  }

  VTdUnitInfo->EnableQueuedInvalidation = 1;
  DEBUG ((DEBUG_INFO, "Use Queued Invalidation Interface for engine [0x%x]\n", VtdUnitBaseAddress));

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  if ((Reg32 & B_GSTS_REG_QIES) != 0) {
    DEBUG ((DEBUG_INFO,"Queued Invalidation Interface was enabled.\n"));
    ClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_QIE);
    MmioWrite64 (VtdUnitBaseAddress + R_IQA_REG, 0);
  }

  //
  // Initialize the Invalidation Queue Tail Register to zero.
  //
  MmioWrite64 (VtdUnitBaseAddress + R_IQT_REG, 0);

  //
  // Setup the IQ address, size and descriptor width through the Invalidation Queue Address Register
  //
  if (VTdUnitInfo->QiDescBuffer == NULL) {
    VTdUnitInfo->QiDescBufferSize = (sizeof (QI_256_DESC) * ((UINTN) 1 << (VTD_INVALIDATION_QUEUE_SIZE + 7)));
    VTdUnitInfo->QiDescBuffer = AllocatePages (EFI_SIZE_TO_PAGES (VTdUnitInfo->QiDescBufferSize));
    if (VTdUnitInfo->QiDescBuffer == NULL) {
      DEBUG ((DEBUG_ERROR,"Could not Alloc Invalidation Queue Buffer.\n"));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  DEBUG ((DEBUG_INFO, "Invalidation Queue Buffer Size : %d\n", VTdUnitInfo->QiDescBufferSize));
  //
  // 4KB Aligned address
  //
  IqaReg.Uint64 = (UINT64) (UINTN) VTdUnitInfo->QiDescBuffer;
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
  SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_QIE);

  return EFI_SUCCESS;
}

/**
  Disable queued invalidation interface.

  @param[in]  VTdUnitInfo       The VTd engine unit information.
**/
VOID
DisableQueuedInvalidationInterface (
  IN VTD_UNIT_INFO *VTdUnitInfo
  )
{
  if (VTdUnitInfo->EnableQueuedInvalidation != 0) {
    DEBUG ((DEBUG_INFO, "Disable Queued Invalidation Interface.\n"));
    ClearGlobalCommandRegisterBits (VTdUnitInfo->VtdUnitBaseAddress, B_GMCD_REG_QIE);

    if (VTdUnitInfo->QiDescBuffer != NULL) {
      FreePages(VTdUnitInfo->QiDescBuffer, EFI_SIZE_TO_PAGES (VTdUnitInfo->QiDescBufferSize));
      VTdUnitInfo->QiDescBuffer = NULL;
      VTdUnitInfo->QiDescBufferSize = 0;
    }

    VTdUnitInfo->EnableQueuedInvalidation = 0;
  }
}

/**
  Check Queued Invalidation Fault.

  @param[in]  VTdUnitInfo       The VTd engine unit information.

  @retval EFI_SUCCESS           The operation was successful.
  @retval RETURN_DEVICE_ERROR   A fault is detected.
**/
EFI_STATUS
QueuedInvalidationCheckFault (
  IN VTD_UNIT_INFO *VTdUnitInfo
  )
{
  UINT32            FaultReg;
  VTD_IQERCD_REG    IqercdReg;

  FaultReg = MmioRead32 (VTdUnitInfo->VtdUnitBaseAddress + R_FSTS_REG);
  if (FaultReg & (B_FSTS_REG_IQE | B_FSTS_REG_ITE | B_FSTS_REG_ICE)) {
    IqercdReg.Uint64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + R_IQERCD_REG);

    DEBUG((DEBUG_ERROR, "VTD 0x%x Detect Queue Invalidation Error [0x%08x] - IQERCD [0x%016lx]\n", VTdUnitInfo->VtdUnitBaseAddress, FaultReg, IqercdReg.Uint64));

    MmioWrite32 (VTdUnitInfo->VtdUnitBaseAddress + R_FSTS_REG, FaultReg);
    return RETURN_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Submit the queued invalidation descriptor to the remapping
   hardware unit and wait for its completion.

  @param[in]  VTdUnitInfo       The VTd engine unit information.
  @param[in]  Desc              The invalidate descriptor

  @retval EFI_SUCCESS           The operation was successful.
  @retval RETURN_DEVICE_ERROR   A fault is detected.
  @retval EFI_INVALID_PARAMETER Parameter is invalid.
**/
EFI_STATUS
SubmitQueuedInvalidationDescriptor (
  IN VTD_UNIT_INFO     *VTdUnitInfo,
  IN QI_256_DESC       *Desc
  )
{
  EFI_STATUS     Status;
  UINTN          VtdUnitBaseAddress;
  UINTN          QueueSize;
  UINTN          QueueTail;
  UINTN          QueueHead;
  QI_DESC        *Qi128Desc;
  QI_256_DESC    *Qi256Desc;
  VTD_IQA_REG    IqaReg;
  VTD_IQT_REG    IqtReg;
  VTD_IQH_REG    IqhReg;
  UINT64         IQBassAddress;

  if (Desc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VtdUnitBaseAddress = VTdUnitInfo->VtdUnitBaseAddress;
  IqaReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQA_REG);
  //
  // Get IQA_REG.IQA (Invalidation Queue Base Address)
  //
  IQBassAddress = RShiftU64 (IqaReg.Uint64, 12);
  if (IQBassAddress == 0) {
    DEBUG ((DEBUG_ERROR,"Invalidation Queue Buffer not ready [0x%lx]\n", IqaReg.Uint64));
    return EFI_NOT_READY;
  }
  IqtReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQT_REG);

  //
  // Check IQA_REG.DW (Descriptor Width)
  //
  if ((IqaReg.Uint64 & BIT11) == 0) {
    //
    // 128-bit descriptor
    //
    QueueSize = (UINTN) (1 << (IqaReg.Bits.QS + 8));
    Qi128Desc = (QI_DESC *) (UINTN) LShiftU64 (IQBassAddress, VTD_PAGE_SHIFT);
    //
    // Get IQT_REG.QT for 128-bit descriptors
    //
    QueueTail = (UINTN) (RShiftU64 (IqtReg.Uint64, 4) & 0x7FFF);
    Qi128Desc += QueueTail;
    Qi128Desc->Low = Desc->Uint64[0];
    Qi128Desc->High = Desc->Uint64[1];
    FlushPageTableMemory (VTdUnitInfo, (UINTN) Qi128Desc, sizeof(QI_DESC));
    QueueTail = (QueueTail + 1) % QueueSize;

    DEBUG ((DEBUG_VERBOSE, "[0x%x] Submit QI Descriptor 0x%x [0x%016lx, 0x%016lx]\n",
            VtdUnitBaseAddress,
            QueueTail,
            Desc->Uint64[0],
            Desc->Uint64[1]));

    IqtReg.Uint64 &= ~(0x7FFF << 4);
    IqtReg.Uint64 |= LShiftU64 (QueueTail, 4);
  } else {
    //
    // 256-bit descriptor
    //
    QueueSize = (UINTN) (1 << (IqaReg.Bits.QS + 7));
    Qi256Desc = (QI_256_DESC *) (UINTN) LShiftU64 (IQBassAddress, VTD_PAGE_SHIFT);
    //
    // Get IQT_REG.QT for 256-bit descriptors
    //
    QueueTail = (UINTN) (RShiftU64 (IqtReg.Uint64, 5) & 0x3FFF);
    Qi256Desc += QueueTail;
    Qi256Desc->Uint64[0] = Desc->Uint64[0];
    Qi256Desc->Uint64[1] = Desc->Uint64[1];
    Qi256Desc->Uint64[2] = Desc->Uint64[2];
    Qi256Desc->Uint64[3] = Desc->Uint64[3];
    FlushPageTableMemory (VTdUnitInfo, (UINTN) Qi256Desc, sizeof(QI_256_DESC));
    QueueTail = (QueueTail + 1) % QueueSize;

    DEBUG ((DEBUG_VERBOSE, "[0x%x] Submit QI Descriptor 0x%x [0x%016lx, 0x%016lx, 0x%016lx, 0x%016lx]\n",
            VtdUnitBaseAddress,
            QueueTail,
            Desc->Uint64[0],
            Desc->Uint64[1],
            Desc->Uint64[2],
            Desc->Uint64[3]));

    IqtReg.Uint64 &= ~(0x3FFF << 5);
    IqtReg.Uint64 |= LShiftU64 (QueueTail, 5);
  }

  //
  // Update the HW tail register indicating the presence of new descriptors.
  //
  MmioWrite64 (VtdUnitBaseAddress + R_IQT_REG, IqtReg.Uint64);

  Status = EFI_SUCCESS;
  do {
    Status = QueuedInvalidationCheckFault(VTdUnitInfo);
    if (Status != EFI_SUCCESS) {
      DEBUG((DEBUG_ERROR,"Detect Queued Invalidation Fault.\n"));
      break;
    }

    IqhReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQH_REG);
    //
    // Check IQA_REG.DW (Descriptor Width) and get IQH_REG.QH
    //
    if ((IqaReg.Uint64 & BIT11) == 0) {
      QueueHead = (UINTN) (RShiftU64 (IqhReg.Uint64, 4) & 0x7FFF);
    } else {
      QueueHead = (UINTN) (RShiftU64 (IqhReg.Uint64, 5) & 0x3FFF);
    }
  } while (QueueTail != QueueHead);

  DEBUG((DEBUG_INFO,"SubmitQueuedInvalidationDescriptor end\n"));
  return Status;
}

/**
  Invalidate VTd context cache.

  @param[in]  VTdUnitInfo       The VTd engine unit information.
**/
EFI_STATUS
InvalidateContextCache (
  IN VTD_UNIT_INFO              *VTdUnitInfo
  )
{
  UINT64                        Reg64;
  QI_256_DESC                   QiDesc;

  if (VTdUnitInfo->EnableQueuedInvalidation == 0) {
    //
    // Register-based Invalidation
    //
    Reg64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + R_CCMD_REG);
    if ((Reg64 & B_CCMD_REG_ICC) != 0) {
      DEBUG ((DEBUG_ERROR,"ERROR: InvalidateContextCache: B_CCMD_REG_ICC is set for VTD(%x)\n", VTdUnitInfo->VtdUnitBaseAddress));
      return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
    Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_GLOBAL);
    MmioWrite64 (VTdUnitInfo->VtdUnitBaseAddress + R_CCMD_REG, Reg64);

    do {
      Reg64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + R_CCMD_REG);
    } while ((Reg64 & B_CCMD_REG_ICC) != 0);
  } else {
    //
    // Queued Invalidation
    //
    QiDesc.Uint64[0] = QI_CC_FM(0) | QI_CC_SID(0) | QI_CC_DID(0) | QI_CC_GRAN(1) | QI_CC_TYPE;
    QiDesc.Uint64[1] = 0;
    QiDesc.Uint64[2] = 0;
    QiDesc.Uint64[3] = 0;

    return SubmitQueuedInvalidationDescriptor(VTdUnitInfo, &QiDesc);
  }

  return EFI_SUCCESS;
}

/**
  Invalidate VTd IOTLB.

  @param[in]  VTdUnitInfo       The VTd engine unit information.
**/
EFI_STATUS
InvalidateIOTLB (
  IN VTD_UNIT_INFO              *VTdUnitInfo
  )
{
  UINT64                        Reg64;
  VTD_ECAP_REG                  ECapReg;
  VTD_CAP_REG                   CapReg;
  QI_256_DESC                   QiDesc;

  if (VTdUnitInfo->EnableQueuedInvalidation == 0) {
    //
    // Register-based Invalidation
    //
    ECapReg.Uint64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + R_ECAP_REG);

    Reg64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
     if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
       DEBUG ((DEBUG_ERROR, "ERROR: InvalidateIOTLB: B_IOTLB_REG_IVT is set for VTD(%x)\n", VTdUnitInfo->VtdUnitBaseAddress));
       return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
    Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_GLOBAL);
    MmioWrite64 (VTdUnitInfo->VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

    do {
      Reg64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    } while ((Reg64 & B_IOTLB_REG_IVT) != 0);
  } else {
    //
    // Queued Invalidation
    //
    CapReg.Uint64 = MmioRead64 (VTdUnitInfo->VtdUnitBaseAddress + R_CAP_REG);
    QiDesc.Uint64[0] = QI_IOTLB_DID(0) | (CapReg.Bits.DRD ? QI_IOTLB_DR(1) : QI_IOTLB_DR(0)) | (CapReg.Bits.DWD ? QI_IOTLB_DW(1) : QI_IOTLB_DW(0)) | QI_IOTLB_GRAN(1) | QI_IOTLB_TYPE;
    QiDesc.Uint64[1] = QI_IOTLB_ADDR(0) | QI_IOTLB_IH(0) | QI_IOTLB_AM(0);
    QiDesc.Uint64[2] = 0;
    QiDesc.Uint64[3] = 0;

    return SubmitQueuedInvalidationDescriptor(VTdUnitInfo, &QiDesc);
  }

  return EFI_SUCCESS;
}

/**
  Clear Global Command Register Bits

  @param[in] VtdUnitBaseAddress The base address of the VTd engine.
  @param[in] BitMask            Bit mask.
**/
VOID
ClearGlobalCommandRegisterBits (
  IN UINTN     VtdUnitBaseAddress,
  IN UINT32    BitMask
  )
{
  UINT32    Reg32;
  UINT32    Status;
  UINT32    Command;

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  Status = (Reg32 & 0x96FFFFFF);       // Reset the one-shot bits
  Command = (Status & (~BitMask));
  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Command);

  DEBUG((DEBUG_INFO, "Clear GCMD_REG bits 0x%x.\n", BitMask));

  //
  // Poll on Status bit of Global status register to become zero
  //
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while ((Reg32 & BitMask) == BitMask);
  DEBUG ((DEBUG_INFO, "GSTS_REG : 0x%08x \n", Reg32));
}

/**
  Set Global Command Register Bits

  @param[in] VtdUnitBaseAddress The base address of the VTd engine.
  @param[in] BitMask            Bit mask.
**/
VOID
SetGlobalCommandRegisterBits (
  IN UINTN     VtdUnitBaseAddress,
  IN UINT32    BitMask
  )
{
  UINT32    Reg32;
  UINT32    Status;
  UINT32    Command;

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  Status = (Reg32 & 0x96FFFFFF);       // Reset the one-shot bits
  Command = (Status | BitMask);
  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Command);

  DEBUG((DEBUG_INFO, "Set GCMD_REG bits 0x%x.\n", BitMask));

  //
  // Poll on Status bit of Global status register to become not zero
  //
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while ((Reg32 & BitMask) == 0);
  DEBUG ((DEBUG_INFO, "GSTS_REG : 0x%08x \n", Reg32));
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
  SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  DEBUG ((DEBUG_INFO, "EnableDmarPreMem: R_GSTS_REG = 0x%x \n", Reg32));

  //
  // Write Buffer Flush
  //
  FlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Enable VTd
  //
  SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  DEBUG ((DEBUG_INFO, "VTD () enabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Enable DMAR translation.

  @param[in]  VTdUnitInfo       The VTd engine unit information.
  @param[in]  RootEntryTable    The address of the VTd RootEntryTable.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmar (
  IN VTD_UNIT_INFO              *VTdUnitInfo,
  IN UINTN                      RootEntryTable
  )
{
  UINTN                         VtdUnitBaseAddress;
  BOOLEAN                       TEWasEnabled;

  VtdUnitBaseAddress = VTdUnitInfo->VtdUnitBaseAddress;

  DEBUG ((DEBUG_INFO, ">>>>>>EnableDmar() for engine [%x] \n", VtdUnitBaseAddress));

  //
  // Check TE was enabled or not.
  //
  TEWasEnabled = ((MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG) & B_GSTS_REG_TE) == B_GSTS_REG_TE);

  if (TEWasEnabled && (VTdUnitInfo->ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
    //
    // For implementations reporting Enhanced SRTP Support (ESRTPS) field as
    // Clear in the Capability register, software must not modify this field while
    // DMA remapping is active (TES=1 in Global Status register).
    //
    if (VTdUnitInfo->CapReg.Bits.ESRTPS == 0) {
      ClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);
    }

    //
    // Enable ADM
    //
    MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64) (RootEntryTable | V_RTADDR_REG_TTM_ADM));

    DEBUG ((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

    DEBUG((DEBUG_INFO, "Enable Abort DMA Mode...\n"));
    SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  } else {
    DEBUG ((DEBUG_INFO, "RootEntryTable 0x%x \n", RootEntryTable));
    MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64) RootEntryTable);

    DEBUG ((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);
  }

  //
  // Write Buffer Flush before invalidation
  //
  FlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Invalidate the context cache
  //
  InvalidateContextCache (VTdUnitInfo);

  //
  // Invalidate the IOTLB cache
  //
  InvalidateIOTLB (VTdUnitInfo);

  if (TEWasEnabled && (VTdUnitInfo->ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
    if (VTdUnitInfo->CapReg.Bits.ESRTPS == 0) {
      ClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);
    }

    DEBUG ((DEBUG_INFO, "RootEntryTable 0x%x \n", RootEntryTable));
    MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64) RootEntryTable);

    DEBUG ((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);
  }

  //
  // Enable VTd
  //
  SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  DEBUG ((DEBUG_INFO, "VTD () enabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Disable DMAR translation.

  @param[in] VtdUnitBaseAddress         The base address of the VTd engine.

  @retval EFI_SUCCESS           DMAR translation is disabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not disabled.
**/
EFI_STATUS
DisableDmar (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  UINT32                        Reg32;

  DEBUG ((DEBUG_INFO, ">>>>>>DisableDmar() for engine [%x] \n", VtdUnitBaseAddress));

  //
  // Write Buffer Flush before invalidation
  //
  FlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Disable Dmar
  //
  //
  // Set TE (Translation Enable: BIT31) of Global command register to zero
  //
  ClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  //
  // Set SRTP (Set Root Table Pointer: BIT30) of Global command register in order to update the root table pointerDisable VTd
  //
  SetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  DEBUG((DEBUG_INFO, "DisableDmar: GSTS_REG - 0x%08x\n", Reg32));

  MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, 0);

  DEBUG ((DEBUG_INFO,"VTD () Disabled!<<<<<<\n"));

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

  DEBUG ((DEBUG_INFO, "EnableVTdTranslationProtectionBlockDma - 0x%08x\n", VtdUnitBaseAddress));

  DEBUG ((DEBUG_INFO, "PcdVTdSupportAbortDmaMode : %d\n", PcdGetBool (PcdVTdSupportAbortDmaMode)));

  ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
  DEBUG ((DEBUG_INFO, "ECapReg.ADMS : %d\n", ECapReg.Bits.ADMS));

  if ((ECapReg.Bits.ADMS == 1) && PcdGetBool (PcdVTdSupportAbortDmaMode)) {
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
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "Locate Null Root Entry Table Ppi Failed : %r\n", Status));
      ASSERT (FALSE);
      return EFI_DEVICE_ERROR;
    }
    DEBUG ((DEBUG_INFO, "Block All DMA by TE.\n"));
    Status = EnableDmarPreMem (VtdUnitBaseAddress, (UINT64) (*RootEntryTable));
  }

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

  if (VTdInfo == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "DisableVTdTranslationProtection - %d Vtd Engine\n", VTdInfo->VTdEngineCount));

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    DisableDmar (VTdInfo->VtdUnitInfo[Index].VtdUnitBaseAddress);

    DisableQueuedInvalidationInterface(&VTdInfo->VtdUnitInfo[Index]);
  }

  return;
}

/**
  Prepare VTD cache invalidation configuration.

  @param[in]  VTdInfo           The VTd engine context information.

  @retval EFI_SUCCESS           Prepare Vtd config success
**/
EFI_STATUS
PrepareVtdCacheInvalidationConfig (
  IN VTD_INFO                   *VTdInfo
  )
{
  UINTN                         Index;
  EFI_STATUS                    Status;

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    Status = PerpareCacheInvalidationInterface(&VTdInfo->VtdUnitInfo[Index]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
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
  IN  VTD_UNIT_INFO                    *VtdUnitInfo,
  OUT BOOLEAN                          *Is5LevelPaging
  )
{
  DEBUG((DEBUG_INFO, "  CapReg SAGAW bits : 0x%02x\n", VtdUnitInfo->CapReg.Bits.SAGAW));

  *Is5LevelPaging = FALSE;
  if ((VtdUnitInfo->CapReg.Bits.SAGAW & BIT3) != 0) {
    *Is5LevelPaging = TRUE;
    if ((HostAddressWidth <= 48) &&
         ((VtdUnitInfo->CapReg.Bits.SAGAW & BIT2) != 0)) {
      *Is5LevelPaging = FALSE;
    } else {
      return EFI_UNSUPPORTED;
    }
  }
  if ((VtdUnitInfo->CapReg.Bits.SAGAW & (BIT3 | BIT2)) == 0) {
    return EFI_UNSUPPORTED;
  }
  DEBUG((DEBUG_INFO, "  Using %d Level Paging\n", *Is5LevelPaging ? 5 : 4));
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

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    VtdUnitInfo = &VTdInfo->VtdUnitInfo[Index];
    if (VtdUnitInfo->Done) {
      continue;
    }
    VtdUnitBaseAddress = VtdUnitInfo->VtdUnitBaseAddress;
    DEBUG((DEBUG_INFO, "VTd Engine: 0x%08X\n", VtdUnitBaseAddress));

    VtdUnitInfo->VerReg.Uint32 = MmioRead32 (VtdUnitBaseAddress + R_VER_REG);
    VtdUnitInfo->CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
    VtdUnitInfo->ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);
    DEBUG((DEBUG_INFO, "  VerReg  : 0x%08X\n", VtdUnitInfo->VerReg.Uint32));
    DEBUG((DEBUG_INFO, "  CapReg  : 0x%016lX\n", VtdUnitInfo->CapReg.Uint64));
    DEBUG((DEBUG_INFO, "  ECapReg : 0x%016lX\n", VtdUnitInfo->ECapReg.Uint64));

    Status = VtdCheckUsing5LevelPaging (VTdInfo->HostAddressWidth, VtdUnitInfo, &(VtdUnitInfo->Is5LevelPaging));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "!!!! Page-table type 0x%X is not supported!!!!\n", VtdUnitInfo->CapReg.Bits.SAGAW));
      return Status;
    }

    Status = PerpareCacheInvalidationInterface(VtdUnitInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
