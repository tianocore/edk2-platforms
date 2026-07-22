/** @file
  Shared SMEM ABI definitions for chip and platform information.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Pmic      - Power Management Integrated Circuit
    - Qfprom    - Qualcomm Fuse Programmable Read-Only Memory
    - Qultivate - Qualcomm Cultivate, the part binning/SKU fuse system
    - Smem      - Qualcomm Shared Memory
**/

#pragma once

#include <Base.h>

/**
  Generate a platform version number.

  This macro generates the platform version number from the specified
  major and minor numbers. For example, version 1.2 is represented by
  @code PLATFORM_INFO_VERSION (1, 2) @endcode
**/
#define PLATFORM_INFO_VERSION(Major, Minor)  (((Major) << 16) | (Minor))

/**
  Target type of the device on which the platform is running.
**/
typedef enum {
  PLATFORM_INFO_TYPE_UNKNOWN     = 0x00,
  PLATFORM_INFO_TYPE_SURF        = 0x01,
  PLATFORM_INFO_TYPE_CDP         = PLATFORM_INFO_TYPE_SURF, /**< Alias for SURF. */
  PLATFORM_INFO_TYPE_FFA         = 0x02,
  PLATFORM_INFO_TYPE_FLUID       = 0x03,
  PLATFORM_INFO_TYPE_FUSION      = 0x04,
  PLATFORM_INFO_TYPE_OEM         = 0x05,
  PLATFORM_INFO_TYPE_QT          = 0x06,
  PLATFORM_INFO_TYPE_MTP         = 0x08,
  PLATFORM_INFO_TYPE_MTP_MDM     = PLATFORM_INFO_TYPE_MTP, /**< MTP variant for MDM chips. */
  PLATFORM_INFO_TYPE_MTP_MSM     = PLATFORM_INFO_TYPE_MTP, /**< MTP variant for MSM chips. */
  PLATFORM_INFO_TYPE_LIQUID      = 0x09,
  PLATFORM_INFO_TYPE_DRAGONBOARD = 0x0A,
  PLATFORM_INFO_TYPE_QRD         = 0x0B,
  PLATFORM_INFO_TYPE_EVB         = 0x0C,
  PLATFORM_INFO_TYPE_HRD         = 0x0D,
  PLATFORM_INFO_TYPE_DTV         = 0x0E,
  PLATFORM_INFO_TYPE_RUMI        = 0x0F,
  PLATFORM_INFO_TYPE_VIRTIO      = 0x10,
  PLATFORM_INFO_TYPE_GOBI        = 0x11,
  PLATFORM_INFO_TYPE_CBH         = 0x12,
  PLATFORM_INFO_TYPE_BTS         = 0x13,
  PLATFORM_INFO_TYPE_XPM         = 0x14,
  PLATFORM_INFO_TYPE_RCM         = 0x15,
  PLATFORM_INFO_TYPE_DMA         = 0x16,
  PLATFORM_INFO_TYPE_STP         = 0x17,
  PLATFORM_INFO_TYPE_SBC         = 0x18,
  PLATFORM_INFO_TYPE_ADP         = 0x19,
  PLATFORM_INFO_TYPE_CHI         = 0x1A,
  PLATFORM_INFO_TYPE_SDP         = 0x1B,
  PLATFORM_INFO_TYPE_RRP         = 0x1C,
  PLATFORM_INFO_TYPE_CLS         = 0x1D,
  PLATFORM_INFO_TYPE_TTP         = 0x1E,
  PLATFORM_INFO_TYPE_HDK         = 0x1F,
  PLATFORM_INFO_TYPE_IOT         = 0x20,
  PLATFORM_INFO_TYPE_ATP         = 0x21,
  PLATFORM_INFO_TYPE_IDP         = 0x22,
  PLATFORM_INFO_TYPE_AEDK        = 0x23,
  PLATFORM_INFO_TYPE_WDP         = 0x24,
  PLATFORM_INFO_TYPE_QAM         = 0x25,
  PLATFORM_INFO_TYPE_QXR         = 0x26,
  PLATFORM_INFO_TYPE_X100        = 0x27, /**< PCIe X100 card. */
  PLATFORM_INFO_TYPE_CRD         = 0x28, /**< Compute Reference Device. */
  PLATFORM_INFO_TYPE_QQVP        = 0x29, /**< Qualcomm QEMU Virtual Platform. */
  PLATFORM_INFO_TYPE_DCP         = 0x2A, /**< Data-Center Platform. */
  PLATFORM_INFO_TYPE_QCB         = 0x2B, /**< Qualcomm Compute Board. */
  PLATFORM_INFO_TYPE_QAR         = 0x2C, /**< Qualcomm Augmented Reality. */
  PLATFORM_INFO_TYPE_WRD         = 0x2D, /**< Wearable Reference Design. */
  PLATFORM_INFO_TYPE_COME        = 0x2E, /**< COM Express. */
  PLATFORM_INFO_TYPE_EVK         = 0x2F, /**< Evaluation Kit. */
  PLATFORM_INFO_TYPE_TDP         = 0x30, /**< Telematics Development Platform. */
  PLATFORM_INFO_TYPE_OMTP        = 0x31,
  PLATFORM_INFO_TYPE_ITPS        = 0x32, /**< IOT Test Platform, Socketed. */
  PLATFORM_INFO_TYPE_ITP         = 0x33, /**< IOT Test Platform, Open Platform. */
  PLATFORM_INFO_TYPE_EITP        = 0x34, /**< Enclosed IOT Test Platform. */

  PLATFORM_INFO_NUM_TYPES, /**< Number of known targets (including unknown). */
  PLATFORM_INFO_TYPE_32BITS = 0x7FFFFFFF
} PlatformInfoPlatformType;

/**
  Format of the PlatformInfoSmemType structure. Minor revision ticks
  are backwards compatible.
**/
#define PLATFORM_INFO_SMEM_FORMAT  PLATFORM_INFO_VERSION (0, 23)

/**
  Length of the build ID buffer in PlatformInfoSmemType.
**/
#define PLATFORM_INFO_SMEM_BUILD_ID_LENGTH  32

/**
  Length of the chip ID buffer in PlatformInfoSmemType.
**/
#define PLATFORM_INFO_SMEM_MAX_CHIP_ID_LENGTH  32

/**
  Maximum number of PMIC devices in PlatformInfoSmemType.
**/
#define PLATFORM_INFO_SMEM_MAX_PMIC_DEVICES  3

/**
  SMEM structure for PMIC information.
**/
typedef struct {
  UINT32    PmicModel;    /**< PMIC device model type, for Badger matches the revision id subtype */
  UINT32    PmicVersion;  /**< PMIC version, same format as Platform version */
} PlatformInfoSmemPmicType;

/**
  Qultivate information for a part instance.
**/
typedef struct {
  UINT16    Part;
  UINT8     Instance;
  UINT8     Disabled;
  UINT32    DisabledFeatures;
} PlatformInfoPartInfoType;

/**
  Structure for the shared memory location which is used to store
  platform, chip, build, and pmic information.
**/
typedef struct {
  UINT32                      Format;                                           /**< Format of the structure. */
  UINT32                      ChipId;                                           /**< Chip ID. */
  UINT32                      ChipVersion;                                      /**< Chip version. */
  CHAR8                       BuildId[PLATFORM_INFO_SMEM_BUILD_ID_LENGTH];      /**< Build ID. */
  UINT32                      RawChipId;                                        /**< Raw chip ID. */
  UINT32                      RawChipVersion;                                   /**< Raw chip version. */
  PlatformInfoPlatformType    PlatformType;                                     /**< Platform type. */
  UINT32                      PlatformVersion;                                  /**< Platform version. */
  UINT32                      Fusion;                                           /**< TRUE if Fusion; FALSE otherwise. */
  UINT32                      PlatformSubtype;                                  /**< Platform subtype. */
  PlatformInfoSmemPmicType    Deprecated[PLATFORM_INFO_SMEM_MAX_PMIC_DEVICES];  /**< Don't use. May not contain the full list; use the array at PmicArrayOffset instead. */
  UINT32                      FoundryId;                                        /**< Chip foundry ID. */
  UINT32                      ChipSerial;                                       /**< Chip serial number. */
  UINT32                      NumPmics;                                         /**< Number of PMICs in array. */
  UINT32                      PmicArrayOffset;                                  /**< Offset from base of structure to array of PlatformInfoSmemPmicType. */
  UINT32                      ChipFamily;                                       /**< Chip family. */
  UINT32                      RawDeviceFamily;                                  /**< Raw device family. */
  UINT32                      RawDeviceNumber;                                  /**< Raw device number. */
  UINT32                      QfpromChipId;                                     /**< QFPROM chip ID. */
  CHAR8                       ChipIdStr[PLATFORM_INFO_SMEM_MAX_CHIP_ID_LENGTH]; /**< Chip name. */
  UINT32                      NumClusters;                                      /**< Number of clusters used by ChipInfoGetDisabledCpus. */
  UINT32                      ClusterArrayOffset;                               /**< Offset from base of structure to UINT32 array of disabled CPU clusters, used by ChipInfoGetDisabledCpus. */
  UINT32                      NumParts;                                         /**< Number of parts supported by ChipInfoGetDisabledFeatures. */
  UINT32                      DisabledFeaturesArrayOffset;                      /**< Offset from base of structure to UINT32 array of disabled parts. */
  UINT32                      ModemSupported;                                   /**< 0 if not supported, nonzero if supported. */
  UINT32                      FeatureCode;                                      /**< Feature code enum for this device. */
  UINT32                      PCode;                                            /**< P-Code for this device. */
  UINT32                      PartNameMapOffset;                                /**< Offset from base of structure to the part name strings for the current chip. */
  UINT32                      NumPartNameMappings;                              /**< Number of part name strings. */
  UINT32                      OemVariantId;                                     /**< OEM variant ID. */
  UINT32                      NumKvps;                                          /**< Number of KVPs. */
  UINT32                      KvpsOffset;                                       /**< Offset of the KVPs. */
  UINT32                      NumFunctionalClusters;                            /**< Number of clusters with at least 1 enabled core. */
  UINT32                      BootCluster;                                      /**< Boot cluster index. */
  UINT32                      BootCore;                                         /**< Boot core index. */
  UINT32                      RawPackageType;                                   /**< Raw package type. */
  UINT32                      PartialFeaturesArrayOffset;                       /**< Offset from base of structure to an array of UINT32s indexed by ChipInfoPartType. Each bit corresponds to a component of that part, with 0 = enabled/unknown and 1 = disabled. */
  UINT32                      CpuCoresArrayOffset;                              /**< Offset from base of structure to UINT32 array of number of CPU cores per cluster, used by ChipInfoGetNumCpuCores. */
  UINT32                      PartInstancesOffset;                              /**< Offset from socinfo base, to an array of PlatformInfoPartInfoType structures. NOT indexed by ChipInfoPartType; iterate and look for a matching .Part and .Instance. */
  UINT32                      NumPartInstances;                                 /**< Length of the array at PartInstancesOffset. */
} PlatformInfoSmemType;
