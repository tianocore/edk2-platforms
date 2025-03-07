/** @file
    Implements OtaCapsuleUpdate.h
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OTA_CAPSULE_UPDATE_H_
#define OTA_CAPSULE_UPDATE_H_

#pragma pack (1)
//
// OTA for BIOS Capsule update
//
typedef struct {
  UINT8    UpdateFlag;      // 0: No update required; 1: update required
  UINT8    UpdateSlot;      // 0: Slot A; 1: Slot B
} OTA_CAPSULE_UPDATE;

#pragma pack ()

//
// Variable Name for OTA BIOS Update
//
#define OTA_CAPSULE_VAR_NAME  L"OtaCapsuleVar"

//
// Variable Name for ISH update. (The variable name is ignored in ISH update function)
//
#define ISH_VAR_NAME  L"IshVar"

#define MAX_SLOT_NUM  2
#define SLOT_A        0
#define SLOT_B        1

#define EFS_LOCATION  0x20000

#endif
