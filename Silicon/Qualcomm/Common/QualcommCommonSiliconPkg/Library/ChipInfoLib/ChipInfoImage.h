/** @file
  Internal ChipInfo header with common APIs to access image-specific
  functionality, such as reading SMEM and copying strings.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Smem - Shared Memory
**/

#pragma once

#include <Base.h>
#include <ChipPlatformInfoSmem.h>

/**
  Common way to access the socinfo SMEM region,
  since not all images have an SMEM driver for accessing socinfo.

  @return  A pointer to the socinfo structure if successful,
           NULL if not successful
**/
PlatformInfoSmemType *
EFIAPI
ChipInfoGetSocInfo (
  VOID
  );

/**
  Unmap the SMEM region once it's no longer needed.
**/
VOID
EFIAPI
ChipInfoUnmapSmem (
  VOID
  );

/**
  Common way to copy strings on all images, since different images
  use different versions of a safe string copy. This function is a
  wrapper around the image's safe string copy, and doesn't
  implement anything extra other than some minor input validation.

  @param[in,out]  Dest     The destination buffer
  @param[in]      Src      The NULL-terminated source buffer
  @param[in]      DestLen  The size of the destination buffer.
                            At most DestLen-1 characters will
                            be copied into Dest, and the buffer
                            will be NULL-terminated.

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
  );
