/** @file
  This file implements the parts of the ChipInfo driver specific to UEFI.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Qfprom - Qualcomm Fuse Programmable Read-Only Memory
    - Smem   - Shared Memory
**/

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ChipInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <ChipPlatformInfoSmem.h>
#include "ChipInfoImage.h"
#include "ChipInfoLocal.h"

/**
  Internal structure to hold chip info.

  During XBL Core, ChipInfoDxe init pulls data from the SMEM region
  and stores it in here.

  The query APIs read data from this structure.
**/
ChipInfoCtxType  mChipInfoCtx;

/**
  Populates mChipInfoCtx with the chip information read from Smem.

  @param[in]  Smem  Pointer to the SoCInfo structure to read from.
**/
STATIC
VOID
ChipInfoPopulateCtxFromSmem (
  IN PlatformInfoSmemType  *Smem
  )
{
  UINT32                    *CpuClusters;
  UINT32                    *DisabledFeaturesArray;
  UINT32                    *CpuCoresArray;
  UINT32                    ClusterArraySize;
  PlatformInfoPartInfoType  *PartInfo;
  UINT32                    Size;
  UINTN                     ChipIdStrLen;

  mChipInfoCtx.RawPartNum     = Smem->RawChipId;
  mChipInfoCtx.ChipInfoId     = (ChipInfoIdType)Smem->ChipId;
  mChipInfoCtx.RevNumber      = Smem->RawChipVersion;
  mChipInfoCtx.Version        = Smem->ChipVersion;
  mChipInfoCtx.FamilyId       = (ChipInfoFamilyType)Smem->ChipFamily;
  mChipInfoCtx.FamilyDeviceId = Smem->RawDeviceNumber;
  mChipInfoCtx.RawFamilyId    = Smem->RawDeviceFamily;
  mChipInfoCtx.FoundryId      = (ChipInfoFoundryIdType)Smem->FoundryId;
  mChipInfoCtx.SerialNum      = Smem->ChipSerial;
  mChipInfoCtx.QfpromChipId   = Smem->QfpromChipId;
  mChipInfoCtx.ModemSupported = Smem->ModemSupported;

  //
  // Smem->ChipIdStr is not guaranteed to be NUL-terminated if it is fully
  // packed, so bound the length lookup to the SMEM field size instead of
  // using AsciiStrCpyS directly on it.
  //
  ChipIdStrLen = AsciiStrnLenS (Smem->ChipIdStr, PLATFORM_INFO_SMEM_MAX_CHIP_ID_LENGTH);
  if (ChipIdStrLen >= CHIPINFO_MAX_ID_LENGTH) {
    ChipIdStrLen = CHIPINFO_MAX_ID_LENGTH - 1;
  }

  CopyMem (mChipInfoCtx.ChipIdString, Smem->ChipIdStr, ChipIdStrLen);
  mChipInfoCtx.ChipIdString[ChipIdStrLen] = '\0';

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 14)) {
    mChipInfoCtx.NumClusters = Smem->NumClusters;

    // Avoid crashes from DALSYS_Malloc on some images if size == 0
    if (mChipInfoCtx.NumClusters > 0) {
      CpuClusters      = (UINT32 *)((UINT8 *)Smem + Smem->ClusterArrayOffset);
      ClusterArraySize = mChipInfoCtx.NumClusters * sizeof (UINT32);

      mChipInfoCtx.CpuClusters = AllocatePages (EFI_SIZE_TO_PAGES (ClusterArraySize));
      if (mChipInfoCtx.CpuClusters != NULL) {
        CopyMem (mChipInfoCtx.CpuClusters, CpuClusters, ClusterArraySize);
      }
    }

    DisabledFeaturesArray = (UINT32 *)((UINT8 *)Smem + Smem->DisabledFeaturesArrayOffset);
    CopyMem (
      mChipInfoCtx.DisabledFeatures,
      DisabledFeaturesArray,
      MIN (Smem->NumParts, (UINT32)CHIPINFO_NUM_PARTS) * sizeof (UINT32)
      );
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 16)) {
    mChipInfoCtx.FeatureCode = Smem->FeatureCode;
    mChipInfoCtx.PCode       = Smem->PCode;
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 19)) {
    mChipInfoCtx.NumFunctionalClusters = Smem->NumFunctionalClusters;
    mChipInfoCtx.BootCluster           = Smem->BootCluster;
    mChipInfoCtx.BootCore              = Smem->BootCore;
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 20)) {
    mChipInfoCtx.RawPackageType = Smem->RawPackageType;
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 21)) {
    DisabledFeaturesArray = (UINT32 *)((UINT8 *)Smem + Smem->PartialFeaturesArrayOffset);
    CopyMem (
      mChipInfoCtx.PartialFeatures,
      DisabledFeaturesArray,
      MIN (Smem->NumParts, (UINT32)CHIPINFO_NUM_PARTS) * sizeof (UINT32)
      );
  } else {
    CopyMem (
      mChipInfoCtx.PartialFeatures,
      mChipInfoCtx.DisabledFeatures,
      CHIPINFO_NUM_PARTS * sizeof (UINT32)
      );
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 22)) {
    CpuCoresArray = (UINT32 *)((UINT8 *)Smem + Smem->CpuCoresArrayOffset);
    // CpuCoresArray contains no. of cores per cluster for all clusters
    ClusterArraySize        = mChipInfoCtx.NumClusters * sizeof (UINT32);
    mChipInfoCtx.NumCores = AllocatePages (EFI_SIZE_TO_PAGES (ClusterArraySize));
    if (mChipInfoCtx.NumCores != NULL) {
      CopyMem (mChipInfoCtx.NumCores, CpuCoresArray, ClusterArraySize);
    }
  }

  if (Smem->Format >= PLATFORM_INFO_VERSION (0, 23)) {
    Size                    = Smem->NumPartInstances * sizeof (PlatformInfoPartInfoType);
    mChipInfoCtx.PartInfo = AllocatePages (EFI_SIZE_TO_PAGES (Size));
    if (mChipInfoCtx.PartInfo != NULL) {
      PartInfo = (PlatformInfoPartInfoType *)((UINT8 *)Smem + Smem->PartInstancesOffset);
      CopyMem (mChipInfoCtx.PartInfo, PartInfo, Size);
      mChipInfoCtx.PartInfoLen = Smem->NumPartInstances;
    }
  }
}

/**
  Initialize the ChipInfo driver.

  Any missing or unknown information will be indicated as specified
  in the individual API documentation.

  @return  EFI_SUCCESS always.
**/
EFI_STATUS
EFIAPI
ChipInfoInit (
  VOID
  )
{
  PlatformInfoSmemType  *Smem;

  if (mChipInfoCtx.InitComplete) {
    return EFI_SUCCESS;
  }

  // Get the address of the SoCInfo structure using the
  // image-specific API
  Smem = ChipInfoGetSocInfo ();

  if (Smem == NULL) {
    // This will only happen in pre-sil on a standalone build.
    // Return SUCCESS using the UNKNOWN values for each field,
    // and let the information be set manually. However, mark the driver as
    // initialized, to record that this has happened.
    AsciiStrCpyS (mChipInfoCtx.ChipIdString, CHIPINFO_MAX_ID_LENGTH, "UNKNOWN");
    mChipInfoCtx.InitComplete = TRUE;
    return EFI_SUCCESS;
  }

  ChipInfoPopulateCtxFromSmem (Smem);

  ChipInfoUnmapSmem ();
  mChipInfoCtx.InitComplete = TRUE;
  return EFI_SUCCESS;
}
