/** @file
  Image-specific implementations of external functions used by
  ChipInfo which are accessed using different APIs on each image

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Smem - Shared Memory
**/

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/SmemLib.h>
#include "ChipInfoImage.h"
#include <ChipPlatformInfoSmem.h>

/**
  Get the SoC information from SMEM.

  Common way to access the socinfo SMEM region, since not all images
  have an SMEM driver for accessing socinfo.

  @retval  Non-NULL  Pointer to the PlatformInfoSmemType structure in SMEM.
  @retval  NULL      SMEM address is unavailable or the region size is zero.

**/
PlatformInfoSmemType *
EFIAPI
ChipInfoGetSocInfo (
  VOID
  )
{
  PlatformInfoSmemType  *Smem;
  UINT32                Size;

  Smem = (PlatformInfoSmemType *)SmemGetAddr (SmemHwSwBuildId, &Size);
  if ((Smem == NULL) || (Size == 0)) {
    return NULL;
  }

  return Smem;
}

/**
  Unmap the SMEM region once it is no longer needed.

  Releases any mapping obtained by ChipInfoGetSocInfo. On this image,
  no unmapping is required.

**/
VOID
EFIAPI
ChipInfoUnmapSmem (
  VOID
  )
{
  // Nothing to unmap
  return;
}

/**
  Copy a string on this image, using the UEFI safe string copy.

  @param[in,out]  Dest     The destination buffer
  @param[in]      Src      The NULL-terminated source buffer
  @param[in]      DestLen  The size of the destination buffer.

  @return  A pointer to the (possibly truncated) destination buffer,
             if successful
           NULL if one of the inputs is NULL or 0

**/
CHAR8 *
EFIAPI
ChipInfoStrcpy (
  IN OUT CHAR8        *Dest,
  IN     CONST CHAR8  *Src,
  IN     UINT32       DestLen
  )
{
  RETURN_STATUS  Status;

  if ((Dest == NULL) || (Src == NULL) || (DestLen == 0)) {
    return NULL;
  }

  Status = AsciiStrCpyS (Dest, DestLen, Src);
  if (RETURN_ERROR (Status)) {
    return NULL;
  }

  return Dest;
}
