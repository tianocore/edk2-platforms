/** @file
  Internal header file for the PlatformInfo driver.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Kvps - Key-Value PairS
**/

#pragma once

#include <Base.h>
#include <Library/PlatformInfoLib.h>

typedef struct {
  UINT32    Key;
  UINT32    Value;
} PlatformInfoKvpsType;

/**
  PlatformInfo driver context.
**/
typedef struct {
  BOOLEAN                         Initialized;
  PlatformInfoPlatformInfoType    PlatformInfo;
  UINT32                          NumKvps;
  PlatformInfoKvpsType            *Kvps;
} PlatformInfoDrvCtx;

/**
  Returns a pointer to the driver context, initializing it if necessary.

  @return  Pointer to the driver context.
**/
PlatformInfoDrvCtx *
EFIAPI
PlatformInfoGetDrvCtx (
  VOID
  );
