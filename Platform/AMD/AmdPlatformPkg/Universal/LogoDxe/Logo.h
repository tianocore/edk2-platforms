/** @file
  LogoDxe header file

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_LOGO_H_
#define AMD_LOGO_H_

///
/// Logo display attributes structure
///
typedef struct {
  EFI_IMAGE_ID                             ImageId;   ///< Image ID
  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE    Attribute; ///< Logo display location
  INTN                                     OffsetX;   ///< Logo display X coordination
  INTN                                     OffsetY;   ///< Logo display Y coordination
} LOGO_ENTRY;

#endif //AMD_LOGO_H_
