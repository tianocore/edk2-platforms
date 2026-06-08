/** @file
  GUID and data structure for the FIT Entry HOB.

  The FIT Entry HOB caches FIT (Firmware Interface Table) entries
  during early pre-memory init so that post-memory consumers do not
  need to access the top-of-flash region directly.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FIT_ENTRY_HOB_H_
#define _FIT_ENTRY_HOB_H_

#include <IndustryStandard/FirmwareInterfaceTable.h>

#define FIT_ENTRY_HOB_GUID \
  { 0x3a7c2b1d, 0x8f4e, 0x4a6b, { 0x91, 0xd5, 0xe2, 0x7c, 0x0b, 0x5f, 0x3a, 0x18 } }

#pragma pack(1)

typedef struct {
  UINT32                          EntryCount;
  FIRMWARE_INTERFACE_TABLE_ENTRY  Entry[];
} FIT_ENTRY_HOB_DATA;

#pragma pack()

extern EFI_GUID gFitEntryHobGuid;

#endif // _FIT_ENTRY_HOB_H_
