/** @file
*
*  Copyright (c) Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SBSAQEMUPLATFORM_VERSION_H
#define SBSAQEMUPLATFORM_VERSION_H

/*
 * Compare PlatformVersion
 *
 */

#define PLATFORM_VERSION_LESS_THAN(Major, Minor)  (    \
  (                                                    \
    ( PcdGet32 (PcdPlatformVersionMajor) < Major)   || \
    (                                                  \
      ( PcdGet32 (PcdPlatformVersionMajor) == Major) && \
      ( PcdGet32 (PcdPlatformVersionMinor) < Minor)    \
    )                                                  \
  )                                                    \
)
#endif
