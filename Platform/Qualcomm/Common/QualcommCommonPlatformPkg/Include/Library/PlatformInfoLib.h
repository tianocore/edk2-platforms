/** @file
  Public interface include file for accessing the PlatformInfo driver.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - MTP  - Modem Test Platform
    - QRD  - Qualcomm Reference Design
    - CDT  - Configuration Data Table
    - Kvps - Key-Value PairS
**/

#pragma once

#include <Base.h>
#include <PlatformInfoDefs.h>

/**
  Returns the platform type.

  @return  Platform type if successful, or
           PLATFORM_INFO_TYPE_UNKNOWN if called before PlatformInfo is
           initialized.
**/
PlatformInfoPlatformType
EFIAPI
PlatformInfoGetPlatformType (
  VOID
  );

/**
  Returns the platform subtype.

  @return  Platform subtype as read from HW, or
           0 if called before PlatformInfo is initialized.
**/
UINT32
EFIAPI
PlatformInfoGetPlatformSubtype (
  VOID
  );

/**
  Returns the platform version.

  @return  Platform version as read from HW, or
           0 if called before PlatformInfo is initialized.
**/
UINT32
EFIAPI
PlatformInfoGetPlatformVersion (
  VOID
  );

/**
  Returns whether the platform is a Fusion variant.

  @return  TRUE if the platform is a Fusion variant, FALSE otherwise or if
           called before PlatformInfo is initialized.
**/
BOOLEAN
EFIAPI
PlatformInfoIsFusion (
  VOID
  );

/**
  Returns a key value stored in the CDT.

  @param[in]   Key    Key to get the value for.
  @param[out]  Value  Pointer to hold the key's value.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Key or Value is invalid.
  @return  EFI_NOT_FOUND if Key was not found.
**/
EFI_STATUS
EFIAPI
PlatformInfoGetKeyValue (
  IN  PlatformInfoKeyType  Key,
  OUT UINT32               *Value
  );

/**
  Returns the OEM variant ID.

  OEM variants are OEM-specific customizations of a specific platform
  type, subtype, and version.

  Variant ID 0 => Qualcomm platform without any modifications.
  OEM variants can go from 1 to 2^32-1, with CDT v6.

  OEM variants serve a different purpose from the OEM platform type:
    OEM variants identify modifications to one of the platform configurations
    that Qualcomm provides, e.g. MTP subtype 1 v1.0, OEM Variant 1.

    The OEM platform type identifies platform types that aren't provided by
    Qualcomm (i.e. not MTP, QRD, etc.)

  With the exception of Variant 0, OEM variants are specific to each OEM
  and are not tracked by Qualcomm.

  Added with CDT Format v5.

  @return  OEM variant ID, or
           0 if called before PlatformInfo is initialized, or if the CDT
           format predates v5.
**/
UINT32
EFIAPI
PlatformInfoGetOemVariant (
  VOID
  );

/**
  Returns information on the current platform.

  @param[out]  Info  Pointer to a caller-allocated buffer to hold the
                      platform information.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Info is NULL.
**/
EFI_STATUS
EFIAPI
PlatformInfoGetPlatformInfo (
  OUT PlatformInfoPlatformInfoType  *Info
  );

/**
  Initialize the PlatformInfo driver.

  If platform information can't be read (e.g., in a pre-silicon standalone
  build), each field is set to its documented UNKNOWN value instead of
  returning an error.

  @return  EFI_SUCCESS always.
**/
EFI_STATUS
EFIAPI
PlatformInfoInit (
  VOID
  );
