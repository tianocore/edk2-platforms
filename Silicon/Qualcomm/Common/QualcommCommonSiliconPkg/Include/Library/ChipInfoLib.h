/** @file
  This is the interface specification for the chip driver/service.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Architecture - In the context of this library, a hardware design
                     lineage distinct from both the Arm CPU architecture
                     (e.g. ARMv8.2) and the SoC itself: multiple unrelated
                     SoCs can share the same architecture family, and a
                     single SoC's CPUs implement their own, independent
                     Arm CPU architecture version. The architecture family
                     and device number together identify which
                     architectural generation a chip's non-CPU hardware
                     belongs to, and bear no relation to
                     ChipInfoFamilyType, which instead groups revisions
                     and variants of one specific chip.
    - Qfprom       - Qualcomm Fuse Programmable Read-Only Memory
    - SKU          - Stock Keeping Unit
    - SOC          - System On Chip
**/

#pragma once

#include <Base.h>
#include <ChipInfoDefs.h>

/**
  Returns the version of the chip as a ChipInfoVersionType.

  @return  Chip version if successful,
           CHIPINFO_VERSION_UNKNOWN if called before ChipInfo is initialized.
**/
ChipInfoVersionType
EFIAPI
ChipInfoGetChipVersion (
  VOID
  );

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
  );

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
  );

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
  );

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
  );

/**
  Returns the chip's family as a ChipInfoFamilyType.

  The chip family is used to group all variants of an SOC.

  @return  Chip family if successful, or
           CHIPINFO_FAMILY_UNKNOWN if no associated chip family can be found,
           or if called before ChipInfo is initialized.
**/
ChipInfoFamilyType
EFIAPI
ChipInfoGetChipFamily (
  VOID
  );

/**
  Returns whether or not the chip supports a modem.

  @return  Non-zero if modems are supported for this chip, or
           CHIPINFO_MODEM_UNKNOWN if modems are not supported or if called
           before ChipInfo is initialized.
**/
ChipInfoModemType
EFIAPI
ChipInfoGetModemSupport (
  VOID
  );

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
  );

/**
  Returns the foundry ID for the chip as a ChipInfoFoundryIdType.

  @return  Foundry ID if successful, or
           CHIPINFO_FOUNDRY_ID_UNKNOWN if called before ChipInfo is initialized.
**/
ChipInfoFoundryIdType
EFIAPI
ChipInfoGetFoundryId (
  VOID
  );

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
  );

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
  );

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
  );

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
**/
EFI_STATUS
EFIAPI
ChipInfoGetDisabledCpus (
  IN  UINT32  CpuCluster,
  OUT UINT32  *Mask
  );

/**
  Initialize the ChipInfo driver.

  If chip information can't be read (e.g., in a pre-silicon standalone
  build), each field is set to its documented UNKNOWN value instead of
  returning an error.

  @return  EFI_SUCCESS always.
**/
EFI_STATUS
EFIAPI
ChipInfoInit (
  VOID
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Returns the raw package type as read from HW.

  @return  Raw package type as read from HW.
**/
UINT32
EFIAPI
ChipInfoGetRawPackageType (
  VOID
  );

/**
  Returns the number of CPU cores in the specified cluster.

  @param[in]   CpuCluster  CPU cluster to query.
  @param[out]  Cores       Pointer to store the number of cores.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Cores is NULL.
  @return  EFI_NOT_READY if called before ChipInfoInit.
  @return  EFI_INVALID_PARAMETER if CpuCluster is out of range.
**/
EFI_STATUS
EFIAPI
ChipInfoGetNumCpuCores (
  IN  UINT32  CpuCluster,
  OUT UINT32  *Cores
  );
