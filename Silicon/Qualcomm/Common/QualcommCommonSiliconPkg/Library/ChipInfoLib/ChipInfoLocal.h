/** @file
  Internal function and data structure declarations for ChipInfo
  shared across all images.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Qfprom    - Qualcomm Fuse Programmable Read-Only Memory
    - Qultivate - Qualcomm Cultivate, the part binning/SKU fuse system
**/

#pragma once

#include <Base.h>
#include <ChipInfoDefs.h>
#include <Library/ChipInfoLib.h>
#include <ChipPlatformInfoSmem.h>

/**
  ChipInfoType

  Container for all chip info
**/
typedef struct {
  BOOLEAN                     InitComplete;                          ///< Driver has been initialized
  UINT32                      RawPartNum;                            ///< JTAG ID/Device Number
  ChipInfoIdType              ChipInfoId;                            ///< Sequential ID, unique across all chips
  ChipInfoVersionType         Version;                               ///< Major and Minor versions
  UINT32                      RevNumber;                             ///< HW revision (e.g. 0=v1.0, 1=v1.1, 2=v2.0)
  ChipInfoFamilyType          FamilyId;                              ///< Internal family (distinct from arch family)
  UINT32                      FamilyDeviceId;                        ///< Device ID, unique within this family
  UINT32                      RawFamilyId;                           ///< Architectural family number
  ChipInfoFoundryIdType       FoundryId;                             ///< Foundry in which the chip was manufactured
  ChipInfoSerialNumType       SerialNum;                             ///< Unique (within chip family) serial number
  ChipInfoQfpromChipIdType    QfpromChipId;                          ///< Chip ID read from QFPROM fuses
  ChipInfoModemType           ModemSupported;                        ///< Whether modem hardware is supported
  CHAR8                       ChipIdString[CHIPINFO_MAX_ID_LENGTH];  ///< The chip's name as a string, e.g. "MSM8998"
  UINT32                      NumClusters;                           ///< Number of CPU clusters
  UINT32                      *CpuClusters;                          ///< Bitmask of disabled CPU cores, per cluster
  UINT32                      DisabledFeatures[CHIPINFO_NUM_PARTS];  ///< Fuse data for supported parts (cam/video/etc)
  ChipInfoFeatureCodeType     FeatureCode;                           ///< Feature Code enum for this device
  ChipInfoPCodeType           PCode;                                 ///< PCode enum for this device
  UINT32                      NumFunctionalClusters;                 ///< Number of clusters with >=1 functional core
  UINT32                      BootCluster;                           ///< zero-indexed boot cluster
  UINT32                      BootCore;                              ///< zero-indexed boot core
  UINT32                      RawPackageType;                        ///< Raw package type
  UINT32                      PartialFeatures[CHIPINFO_NUM_PARTS];   ///< Qultivate Fuse value for each part
  UINT32                      *NumCores;                             ///< Number of CPU cores, per cluster
  PlatformInfoPartInfoType    *PartInfo;                             ///< Per-instance Qultivate info, all plan parts
  UINT32                      PartInfoLen;                           ///< Number of entries in PartInfo
} ChipInfoCtxType;

extern ChipInfoCtxType  mChipInfoCtx;
