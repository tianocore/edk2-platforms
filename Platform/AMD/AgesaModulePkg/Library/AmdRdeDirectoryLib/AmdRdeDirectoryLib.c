/** @file
  Null stub implementations of AmdRdeDirectoryLib for standalone
  AmdPlatformPkg build.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <AmdRdeDirectory.h>

EFI_STATUS EFIAPI
AmdRdeDirectoryGetImage (
  IN      UINT16  ImageType,
  IN      UINT64  *InMediaOffset OPTIONAL,
  IN OUT  UINT32  *Imagesize,
  OUT     VOID    *Buffer
  )
{
  return EFI_UNSUPPORTED;
}
