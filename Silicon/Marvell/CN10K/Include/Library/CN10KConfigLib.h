/** @file

  Header file for CN10K Configuration Helper Library.

  Copyright (c) 2016, Cavium Inc. All rights reserved.<BR>
  Copyright (c) 2026, Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

CONST VOID *
CN10KGetConfigProperty (
  IN  CONST CHAR8  *Name,
  OUT INT32        *Length
  );

EFI_STATUS
CN10KGetConfigPropertyInt (
  IN  CONST CHAR8  *Name,
  OUT UINT64       *Value,
  IN  UINTN        Base
  );

EFI_STATUS
CN10KGetConfigPropertyAscii (
  IN  CONST CHAR8  *Name,
  OUT CHAR8        *Value,
  IN  UINTN        Length
  );

VOID *
CN10KGetFdt (
  VOID
  );
