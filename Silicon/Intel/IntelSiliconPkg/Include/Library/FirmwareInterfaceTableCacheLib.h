/** @file
  Library to cache the FIT (Firmware Interface Table) into a GUIDed HOB.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FIRMWARE_INTERFACE_TABLE_CACHE_LIB_H_
#define _FIRMWARE_INTERFACE_TABLE_CACHE_LIB_H_

#include <Guid/FitEntryHob.h>

/**
  Cache the FIT table entries into a GUIDed HOB so that post-memory
  consumers can retrieve FIT records without accessing the top-of-flash
  region directly.

  @retval EFI_SUCCESS           FIT entries cached successfully.
  @retval EFI_NOT_FOUND         FIT pointer is invalid or signature not found.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the HOB.
**/
EFI_STATUS
EFIAPI
BuildFitEntryHob (
  VOID
  );

#endif // _FIRMWARE_INTERFACE_TABLE_CACHE_LIB_H_
