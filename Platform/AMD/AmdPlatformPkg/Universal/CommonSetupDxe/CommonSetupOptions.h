/** @file
  The header file for the setup VFR.

  Copyright (C) 2019 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef COMMON_SETUP_OPTIONS_H_
#define COMMON_SETUP_OPTIONS_H_

// {E863E18A-AC7B-4BFE-AA49-62AA9B6048C6}
#define COMMON_SETUP_GUID \
  { 0xE863E18A, 0xAC7B, 0x4BFE, {0xAA, 0x49, 0x62, 0xAA, 0x9B, 0x60, 0x48, 0xC6} }

// {0e2ca972-9bc5-4f0e-9445-03bf67479d8c}
#define AMD_CRB_SETUP_GUID \
  { 0x0e2ca972, 0x9bc5, 0x4f0e, {0x94, 0x45, 0x3, 0xbf, 0x67, 0x47, 0x9d, 0x8c} }

// {87482891-0141-4705-86C8-A0F4908E8315}
#define COMMON_SETUP_FORMSET_GUID \
  { 0x87482891, 0x0141, 0x4705, {0x86, 0xC8, 0xA0, 0xF4, 0x90, 0x8E, 0x83, 0x15} }

#define COMMON_SETUP_VARIABLE_NAME   L"CommonSetup"
#define AMD_CRB_SETUP_VARIABLE_NAME  L"AMD_CRB_SETUP"

#define COMMON_SETUP_MENU_FORM_ID    50
#define COMMON_SETUP_MENU_CLASS      1
#define COMMON_SETUP_MENU_SUB_CLASS  0

#define BIOS_PASSWORD_MENU_FORM_ID  51
#define AMD_CRB_SETUP_FORM_ID       52

#define COMMON_SETUP_ALLOCATED_SIZE  ((UINT8) (OFFSET_OF (COMMON_SETUP_OPTIONS, ReservedBuffer)))
#define COMMON_SETUP_MAXIMAL_SIZE    (128)

extern EFI_GUID  gAmdPlatformSetupOptionsGuid;
extern EFI_GUID  gAmdCrbSetupGuid;

// Structure for Variables to be used in common setup menu.
#pragma pack (push, 1)

typedef struct {
  UINT8    AllocatedSize;
  UINT8    Reserved2;                           ///<
  UINT8    Reserved1;                           ///<
  UINT8    BiosPasswordEnable;                  ///<  0: Disable
                                                ///<  1: Enable
  UINT8    BiosPasswordRequest;                 ///<  0: Not requested
                                                ///<  1: Requested
  UINT8    UsbBiosSupport;                      ///<  0: Disable
                                                ///<  1: Enable
  UINT8    PcdPcieResizableBarSupport;          ///<  gEfiMdeModulePkgTokenSpaceGuid.PcdPcieResizableBarSupport
  UINT8    NetworkStack;                        ///<  0: Disable
                                                ///<  1: Enable
  UINT8    KeyboardHotkeyDetect;                ///<  0: Disable
                                                ///<  1: Enable
  UINT8    SecureBootControl;                   ///<  0: Disable
                                                ///<  1: Enable
  UINT8    ApplyDefaultBootOrder;               ///<  0: Disable
                                                ///<  1: Enable
  UINT8    NetworkBootFirst;                    ///<  0: Disable
                                                ///<  1: Enable
  UINT8    NetworkBootSelection;                ///<  0: PXE boot
                                                ///<  1: HTTP boot
  UINT8    ReservedBuffer[115];                 ///<  Must be the last member in this structure.
} COMMON_SETUP_OPTIONS;

//
// Structure for Variables to be used for CRB
//
typedef struct {
  UINT8    BypassFtpmPrompt;                       // Bypass fTpm Prompt
  UINT8    ReservedBuffer[0xF];                    // Must be the last member in this structure.
} AMD_CRB_SETUP_OPTION;

#pragma pack (pop)

#endif // COMMON_SETUP_OPTIONS_H_
