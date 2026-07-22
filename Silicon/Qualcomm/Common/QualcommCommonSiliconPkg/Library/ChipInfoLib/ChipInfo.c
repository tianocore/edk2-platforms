/** @file
  This file implements the Chip Info APIs.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Qfprom - Qualcomm Fuse Programmable Read-Only Memory
**/

#include <Uefi.h>
#include <Base.h>
#include <ChipInfoDefs.h>
#include "ChipInfoImage.h"
#include "ChipInfoLocal.h"

/**
  Returns the version of the chip as a ChipInfoVersionType.

  @return  Chip version if successful,
           CHIPINFO_VERSION_UNKNOWN if called before ChipInfo is initialized.
**/
ChipInfoVersionType
EFIAPI
ChipInfoGetChipVersion (
  VOID
  )
{
  return mChipInfoCtx.Version;
}

/**
  Returns the version of the chip as read from fuses.

  This raw version is a HW revision counter, strictly increasing with
  each new HW version (e.g. v1.0 = 0, v1.1 = 1, v2.0 = 3, ...). It
  predates ChipInfoVersionType, and is now only used to detect patch
  revisions, i.e., the 'z' in vX.Y.Z, which aren't reflected in the
  value returned by ChipInfoGetChipVersion. Use ChipInfoGetChipVersion
  for everything else.

  @return  Chip version as read from HW, or
           CHIPINFO_RAW_VERSION_UNKNOWN if called before ChipInfo is initialized.
**/
UINT32
EFIAPI
ChipInfoGetRawChipVersion (
  VOID
  )
{
  return mChipInfoCtx.RevNumber;
}

/**
  Returns the chip ID as a ChipInfoIdType.

  @return  Chip ID associated with the part number read from HW, or
           CHIPINFO_ID_UNKNOWN if no associated chip ID could be found,
           or if called before ChipInfo is initialized.
**/
ChipInfoIdType
EFIAPI
ChipInfoGetChipId (
  VOID
  )
{
  return mChipInfoCtx.ChipInfoId;
}

/**
  Returns the chip ID as read from HW (i.e. the JTAG ID).

  Rarely needed, except as a workaround until ChipInfoIdType is updated
  with support for new chips. Use ChipInfoGetChipId instead.

  @return  Chip ID as read from HW, or
           CHIPINFO_RAW_ID_UNKNOWN if called before ChipInfo is initialized.
**/
UINT32
EFIAPI
ChipInfoGetRawChipId (
  VOID
  )
{
  return mChipInfoCtx.RawPartNum;
}

/**
  Returns the name of the chip (e.g. MSM8998, MDM9665).

  @param[out]  ChipIdStr  Pointer to a buffer to hold the chip's name. If
                           called before ChipInfo is initialized, or if the
                           chip ID string is unknown, this function copies
                           "UNKNOWN" into this buffer.
  @param[in]   MaxLength  Length of the string to copy. Max supported size
                           is CHIPINFO_MAX_ID_LENGTH.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if ChipIdStr is NULL, MaxLength is 0, or
           MaxLength is too small to hold the chip's name and its NULL
           terminator.
**/
EFI_STATUS
EFIAPI
ChipInfoGetChipIdString (
  OUT CHAR8   *ChipIdStr,
  IN  UINT32  MaxLength
  )
{
  UINT32  Len;

  if (MaxLength < CHIPINFO_MAX_ID_LENGTH) {
    Len = MaxLength;
  } else {
    Len = CHIPINFO_MAX_ID_LENGTH;
  }

  if (ChipInfoStrcpy (ChipIdStr, mChipInfoCtx.ChipIdString, Len) == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Returns the chip's family as a ChipInfoFamilyType.

  This has no connection to the architectural family - it's used to group
  all revisions of a specific chip.

  @return  Chip family if successful, or
           CHIPINFO_FAMILY_UNKNOWN if no associated chip family can be found,
           or if called before ChipInfo is initialized.
**/
ChipInfoFamilyType
EFIAPI
ChipInfoGetChipFamily (
  VOID
  )
{
  return mChipInfoCtx.FamilyId;
}

/**
  Returns whether or not the chip supports a modem.

  The value returned should be treated as indicating the presence of a modem:
  0 = modem not supported, nonzero = modem supported.

  @return  Non-zero if modems are supported for this chip, or
           CHIPINFO_MODEM_UNKNOWN if modems are not supported or if called
           before ChipInfo is initialized.
**/
ChipInfoModemType
EFIAPI
ChipInfoGetModemSupport (
  VOID
  )
{
  return mChipInfoCtx.ModemSupported;
}

/**
  Returns the chip's serial number.

  Serial numbers are only unique within a given commercial product family.
  Use ChipInfoGetQfpromChipId to find the commercial product family.
  Combining the two values gives a globally unique ID for this device.

  @return  Serial number as read from HW, or
           CHIPINFO_SERIAL_NUM_UNKNOWN if called before ChipInfo is initialized.
**/
ChipInfoSerialNumType
EFIAPI
ChipInfoGetSerialNumber (
  VOID
  )
{
  return mChipInfoCtx.SerialNum;
}

/**
  Returns the foundry ID for the chip as a ChipInfoFoundryIdType.

  @return  Foundry ID if successful, or
           CHIPINFO_FOUNDRY_ID_UNKNOWN if called before ChipInfo is initialized.
**/
ChipInfoFoundryIdType
EFIAPI
ChipInfoGetFoundryId (
  VOID
  )
{
  return mChipInfoCtx.FoundryId;
}

/**
  Returns the chip's architecture family number as read from HW.

  @return  Architectural family as read from HW, or
           CHIPINFO_RAW_DEVICE_FAMILY_UNKNOWN if called before ChipInfo is
           initialized.
**/
UINT32
EFIAPI
ChipInfoGetRawDeviceFamily (
  VOID
  )
{
  return mChipInfoCtx.RawFamilyId;
}

/**
  Returns the chip's device ID within its architectural family.

  Functionally, this is a generation counter.

  @return  Device number as read from HW, or
           CHIPINFO_RAW_DEVICE_NUMBER_UNKNOWN if called before ChipInfo is
           initialized.
**/
UINT32
EFIAPI
ChipInfoGetRawDeviceNumber (
  VOID
  )
{
  return mChipInfoCtx.FamilyDeviceId;
}

/**
  Returns the chip's QFPROM chip ID.

  Can be used along with the serial number to uniquely identify the chip.

  @return  Chip ID as read from QFPROM fuses if successful, or
           CHIPINFO_QFPROM_CHIP_ID_UNKNOWN if called before ChipInfo is
           initialized.
**/
ChipInfoQfpromChipIdType
EFIAPI
ChipInfoGetQfpromChipId (
  VOID
  )
{
  return mChipInfoCtx.QfpromChipId;
}

/**
  Retrieves a mask of CPUs in the specified cluster marked as disabled in
  PTE fuses. For a cluster with fewer than 32 CPUs, the bits corresponding
  to CPUs that don't exist read as enabled.

  @param[in]   CpuCluster  CPU cluster to check.
  @param[out]  Mask        Pointer to hold the mask of disabled CPUs in the
                            selected cluster.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if CpuCluster exceeds the
           number of clusters present.
  @return  EFI_DEVICE_ERROR for other general errors.
  @return  EFI_UNSUPPORTED if the per-cluster disabled-CPU masks are not
           available on this SMEM format.
**/
EFI_STATUS
EFIAPI
ChipInfoGetDisabledCpus (
  IN  UINT32  CpuCluster,
  OUT UINT32  *Mask
  )
{
  if (Mask == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (mChipInfoCtx.NumClusters == 0) {
    //
    // If this target doesn't have a binning plan, we don't know how many
    // CPU clusters there are for this target, and ChipInfo.NumClusters
    // will be left at its default value of 0. This is the only case where
    // NumClusters will be 0.
    //
    // When this happens, return success with Mask = "enabled/unknown"
    // since there's always at least 1 core that's working (the one
    // that's running this function).
    //
    *Mask = 0;
    return EFI_SUCCESS;
  }

  if (CpuCluster >= mChipInfoCtx.NumClusters) {
    return EFI_INVALID_PARAMETER;
  }

  if (mChipInfoCtx.CpuClusters == NULL) {
    return EFI_UNSUPPORTED;
  }

  *Mask = mChipInfoCtx.CpuClusters[CpuCluster];
  return EFI_SUCCESS;
}

/**
  Returns SKU and P-Code information for the current SoC.

  If the SKU_ID and/or P_CODE fuses are not blown, this function returns
  the corresponding CHIPINFO_*_UNKNOWN value.

  @param[out]  Info  Pointer to a caller-allocated buffer to hold the SKU
                      information.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Info is NULL.
  @return  EFI_UNSUPPORTED if this target does not support
           SKU information.
**/
EFI_STATUS
EFIAPI
ChipInfoGetSku (
  OUT ChipInfoSkuType  *Info
  )
{
  if (Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Info->FeatureCode = mChipInfoCtx.FeatureCode;
  Info->PCode       = mChipInfoCtx.PCode;

  if ((mChipInfoCtx.FeatureCode == CHIPINFO_FEATURE_CODE_UNKNOWN) &&
      (mChipInfoCtx.PCode == CHIPINFO_P_CODE_UNKNOWN))
  {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Returns the number of functional clusters.

  This is the total number of functional clusters based on partial
  binning and SKUing. A cluster is deemed functional if it has at least
  one functional core. There will always be at least 1 functional
  cluster: the one that's running this code.

  @param[out]  NumClusters  Buffer to store the number of functional clusters.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if NumClusters is NULL.
  @return  EFI_NOT_READY if called before ChipInfoInit.
**/
EFI_STATUS
EFIAPI
ChipInfoGetNumFunctionalClusters (
  OUT UINT32  *NumClusters
  )
{
  if (NumClusters == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mChipInfoCtx.InitComplete) {
    return EFI_NOT_READY;
  }

  *NumClusters = mChipInfoCtx.NumFunctionalClusters;
  return EFI_SUCCESS;
}

/**
  Returns the boot cluster and core indices.

  The format is identical to the corresponding MPIDR_EL1 AFFx field.

  @param[out]  Cluster  Buffer to store the boot cluster index.
  @param[out]  Core     Buffer to store the boot core index.

  @return  EFI_SUCCESS if both pointers were filled correctly.
  @return  EFI_INVALID_PARAMETER if either pointer is NULL.
  @return  EFI_NOT_READY if called before ChipInfoInit.
**/
EFI_STATUS
EFIAPI
ChipInfoGetBootClusterAndCore (
  OUT UINT32  *Cluster,
  OUT UINT32  *Core
  )
{
  if ((Cluster == NULL) || (Core == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mChipInfoCtx.InitComplete) {
    return EFI_NOT_READY;
  }

  *Cluster = mChipInfoCtx.BootCluster;
  *Core    = mChipInfoCtx.BootCore;
  return EFI_SUCCESS;
}

/**
  Returns the HW part-level disabled feature details from fuses, read
  from the QTV/PTE region.

  @param[in]   Part  The ChipInfoPartType being queried.
  @param[in]   Idx   Hardware instance of the selected part.
  @param[out]  Mask  Used to store the part-specific details.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Mask is NULL or Part is unknown.
  @return  EFI_INVALID_PARAMETER if Part is invalid.
  @return  EFI_NOT_READY if called before ChipInfoInit.
**/
EFI_STATUS
EFIAPI
ChipInfoGetDisabledFeatures (
  IN  ChipInfoPartType  Part,
  IN  UINT32            Idx,
  OUT UINT32            *Mask
  )
{
  UINT32  Index;

  PlatformInfoPartInfoType  *PartInfo;

  if ((Mask == NULL) || (Part == CHIPINFO_PART_UNKNOWN)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Part >= CHIPINFO_NUM_PARTS) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mChipInfoCtx.InitComplete) {
    return EFI_NOT_READY;
  }

  if (Idx == 0) {
    *Mask = mChipInfoCtx.PartialFeatures[Part];
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < mChipInfoCtx.PartInfoLen; Index++) {
    PartInfo = &mChipInfoCtx.PartInfo[Index];
    if ((PartInfo->Part == Part) && (PartInfo->Instance == Idx)) {
      *Mask = PartInfo->DisabledFeatures;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Checks if the specified part is completely disabled.

  FALSE indicates the part is either fully or partially enabled.
  TRUE indicates the part is completely disabled.
  Use ChipInfoGetDisabledFeatures to obtain partial disable information.

  @param[in]   Part  The ChipInfoPartType being queried.
  @param[in]   Idx   Hardware instance of the selected part.
  @param[out]  Mask  Pointer to a BOOLEAN to store the disabled status.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Mask is NULL or Part is unknown.
  @return  EFI_INVALID_PARAMETER if Part is invalid.
  @return  EFI_NOT_READY if called before ChipInfoInit.
**/
EFI_STATUS
EFIAPI
ChipInfoIsPartDisabled (
  IN  ChipInfoPartType  Part,
  IN  UINT32            Idx,
  OUT BOOLEAN           *Mask
  )
{
  UINT32  Index;

  PlatformInfoPartInfoType  *PartInfo;

  if ((Mask == NULL) || (Part == CHIPINFO_PART_UNKNOWN)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Part >= CHIPINFO_NUM_PARTS) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mChipInfoCtx.InitComplete) {
    return EFI_NOT_READY;
  }

  if (Idx == 0) {
    *Mask = ((mChipInfoCtx.DisabledFeatures[Part] & 0x1) ? TRUE : FALSE);
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < mChipInfoCtx.PartInfoLen; Index++) {
    PartInfo = &mChipInfoCtx.PartInfo[Index];
    if ((PartInfo->Part == Part) && (PartInfo->Instance == Idx)) {
      *Mask = PartInfo->Disabled;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Returns the raw package type as read from HW.

  @return  Raw package type as read from HW.
**/
UINT32
EFIAPI
ChipInfoGetRawPackageType (
  VOID
  )
{
  return mChipInfoCtx.RawPackageType;
}

/**
  Returns the number of CPU cores in the specified cluster.

  @param[in]   CpuCluster  CPU cluster to query.
  @param[out]  Cores       Pointer to store the number of cores.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Cores is NULL.
  @return  EFI_NOT_READY if called before ChipInfoInit.
  @return  EFI_INVALID_PARAMETER if CpuCluster is out of range.
  @return  EFI_UNSUPPORTED if the per-cluster core counts are not available
           on this SMEM format.
**/
EFI_STATUS
EFIAPI
ChipInfoGetNumCpuCores (
  IN  UINT32  CpuCluster,
  OUT UINT32  *Cores
  )
{
  if (Cores == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mChipInfoCtx.InitComplete) {
    return EFI_NOT_READY;
  }

  if (CpuCluster >= mChipInfoCtx.NumClusters) {
    return EFI_INVALID_PARAMETER;
  }

  if (mChipInfoCtx.NumCores == NULL) {
    return EFI_UNSUPPORTED;
  }

  *Cores = mChipInfoCtx.NumCores[CpuCluster];

  return EFI_SUCCESS;
}
