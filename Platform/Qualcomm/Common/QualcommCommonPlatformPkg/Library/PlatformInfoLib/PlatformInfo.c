/** @file

  Qualcomm Platform Information Library.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Base.h>
#include "PlatformInfoInternal.h"

/**
  Driver context.
**/
PlatformInfoDrvCtx  mPlatformInfoDrvCtx;

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
  )
{
  PlatformInfoDrvCtx  *DrvCtx;

  DrvCtx = PlatformInfoGetDrvCtx ();

  return DrvCtx->PlatformInfo.PlatformType;
}

/**
  Returns the platform subtype.

  @return  Platform subtype as read from HW, or
           0 if called before PlatformInfo is initialized.
**/
UINT32
EFIAPI
PlatformInfoGetPlatformSubtype (
  VOID
  )
{
  PlatformInfoDrvCtx  *DrvCtx;

  DrvCtx = PlatformInfoGetDrvCtx ();

  return DrvCtx->PlatformInfo.Subtype;
}

/**
  Returns the platform version.

  @return  Platform version as read from HW, or
           0 if called before PlatformInfo is initialized.
**/
UINT32
EFIAPI
PlatformInfoGetPlatformVersion (
  VOID
  )
{
  PlatformInfoDrvCtx  *DrvCtx;

  DrvCtx = PlatformInfoGetDrvCtx ();

  return DrvCtx->PlatformInfo.Version;
}

/**
  Returns whether the platform is a Fusion variant.

  @return  TRUE if the platform is a Fusion variant, FALSE otherwise or if
           called before PlatformInfo is initialized.
**/
BOOLEAN
EFIAPI
PlatformInfoIsFusion (
  VOID
  )
{
  PlatformInfoDrvCtx  *DrvCtx;

  DrvCtx = PlatformInfoGetDrvCtx ();

  return DrvCtx->PlatformInfo.Fusion;
}

/**
  Retrieves the platform information.

  @param[out] Info  Pointer to the platform info structure to populate.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Info is NULL.
**/
EFI_STATUS
EFIAPI
PlatformInfoGetPlatformInfo (
  OUT PlatformInfoPlatformInfoType  *Info
  )
{
  PlatformInfoDrvCtx  *DrvCtx;

  DrvCtx = PlatformInfoGetDrvCtx ();

  if (Info != NULL) {
    *Info = DrvCtx->PlatformInfo;
    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Retrieves the value associated with a platform key.

  @param[in]  Key    The platform info key to look up.
  @param[out] Value  Pointer to store the retrieved value.

  @return  EFI_SUCCESS if successful.
  @return  EFI_INVALID_PARAMETER if Key or Value is invalid.
  @return  EFI_NOT_FOUND if Key was not found.
**/
EFI_STATUS
EFIAPI
PlatformInfoGetKeyValue (
  IN  PlatformInfoKeyType  Key,
  OUT UINT32               *Value
  )
{
  PlatformInfoDrvCtx  *DrvCtx;
  UINT32               Index;

  DrvCtx = PlatformInfoGetDrvCtx ();

  if ((Key >= PLATFORM_INFO_NUM_KEYS) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < DrvCtx->NumKvps; Index++) {
    if (Key == DrvCtx->Kvps[Index].Key) {
      *Value = DrvCtx->Kvps[Index].Value;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Returns the OEM variant ID.

  @return  OEM variant ID, or
           0 if called before PlatformInfo is initialized, or if the SMEM
           format predates PLATFORM_INFO_VERSION (0, 17).
**/
UINT32
EFIAPI
PlatformInfoGetOemVariant (
  VOID
  )
{
  PlatformInfoDrvCtx  *DrvCtx;

  DrvCtx = PlatformInfoGetDrvCtx ();

  return DrvCtx->PlatformInfo.OemVariantId;
}

/**
  Returns a pointer to the driver context, initializing it if necessary.

  @return  Pointer to the driver context.
**/
PlatformInfoDrvCtx *
EFIAPI
PlatformInfoGetDrvCtx (
  VOID
  )
{
  STATIC BOOLEAN  Initializing = FALSE;

  if (!mPlatformInfoDrvCtx.Initialized && !Initializing) {
    Initializing = TRUE;
    PlatformInfoInit ();
    Initializing = FALSE;
  }

  return &mPlatformInfoDrvCtx;
}
