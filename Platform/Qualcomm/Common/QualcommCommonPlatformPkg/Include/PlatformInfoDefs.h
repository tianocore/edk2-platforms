/** @file
  Public definitions for the PlatformInfo driver.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Id  - Identifier
    - Oem - Original Equipment Manufacturer
**/

#pragma once

#include <Base.h>
#include <ChipPlatformInfoSmem.h>

#define PLATFORM_INFO_GET_MAJOR_VERSION(Version)  (((Version) >> 16) & 0xF)
#define PLATFORM_INFO_GET_MINOR_VERSION(Version)  ((Version) & 0xF)

/**
  Keys to get data out of the CDT.
**/
typedef enum {
  PLATFORM_INFO_KEY_UNKNOWN     = 0x00,
  PLATFORM_INFO_KEY_DDR_FREQ    = 0x01,
  PLATFORM_INFO_KEY_GFX_FREQ    = 0x02,
  PLATFORM_INFO_KEY_CAMERA_FREQ = 0x03,
  PLATFORM_INFO_KEY_FUSION      = 0x04,
  PLATFORM_INFO_KEY_CUST        = 0x05,
  PLATFORM_INFO_KEY_NAND_SCRUB  = 0x06,
  PLATFORM_INFO_KEY_SLT         = 0x07,
  PLATFORM_INFO_KEY_PMIC        = 0x08,
  PLATFORM_INFO_KEY_POWER_GRID  = 0x09,

  PLATFORM_INFO_NUM_KEYS,

  PLATFORM_INFO_KEY_32BITS = 0x7FFFFFFF
} PlatformInfoKeyType;

/**
  Stores the target platform, the platform version, and the
  platform subtype.
**/
typedef struct {
  PlatformInfoPlatformType    PlatformType;  /**< Type of the current target. */
  UINT32                      Version;       /**< Version of the platform in use. */
  UINT32                      Subtype;       /**< Sub-type of the platform. */
  BOOLEAN                     Fusion;        /**< TRUE if Fusion; FALSE otherwise. */
  UINT32                      OemVariantId;  /**< OEM variant ID. */
} PlatformInfoPlatformInfoType;
