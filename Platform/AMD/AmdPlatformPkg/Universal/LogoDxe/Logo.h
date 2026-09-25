/** @file
  LogoDxe header file

  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

///
/// Logo display attributes structure
///
typedef struct {
  EFI_IMAGE_ID                             ImageId;   ///< Image ID
  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE    Attribute; ///< Logo display location
  INTN                                     OffsetX;   ///< Logo display X coordination
  INTN                                     OffsetY;   ///< Logo display Y coordination
} LOGO_ENTRY;
