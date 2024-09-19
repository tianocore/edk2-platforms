/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_MANAGER_HII_GUID_H_
#define PLATFORM_MANAGER_HII_GUID_H_

#define PLATFORM_MANAGER_FORMSET_GUID  \
  { \
  0x83ABD546, 0x7AD9, 0x4DE7, { 0xBD, 0x52, 0x12, 0x23, 0xF6, 0xE8, 0xFD, 0x4B } \
  }

#define PLATFORM_MANAGER_ENTRY_EVENT_GUID  \
  { \
  0x28A4731E, 0x14A9, 0x488A, { 0xA8, 0x19, 0xFF, 0x27, 0x80, 0x6E, 0xDB, 0x0E } \
  }

#define PLATFORM_MANAGER_EXIT_EVENT_GUID  \
  { \
  0xE8887242, 0x4EFF, 0x4323, { 0x81, 0xF4, 0xC9, 0x5F, 0xD5, 0x8D, 0x80, 0xD5 } \
  }

extern EFI_GUID  gPlatformManagerFormsetGuid;
extern EFI_GUID  gPlatformManagerEntryEventGuid;
extern EFI_GUID  gPlatformManagerExitEventGuid;

#endif
