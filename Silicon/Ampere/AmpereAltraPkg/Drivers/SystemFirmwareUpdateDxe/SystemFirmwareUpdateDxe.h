/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SYSTEM_FIRMWARE_UPDATE_DXE_H_
#define SYSTEM_FIRMWARE_UPDATE_DXE_H_

#define  FWU_IMG_STR_SCP                           "SCP"
#define  FWU_IMG_STR_ATFUEFI                       "ATFUEFI"
#define  FWU_IMG_STR_CFGUEFI                       "CFGUEFI"
#define  FWU_IMG_STR_UEFI                          "UEFI"
#define  FWU_IMG_STR_SINGLE_ATFUEFI_FULL_FLASH     "SINGLE_IMG_FULL_FLASH"
#define  FWU_IMG_STR_SINGLE_ATFUEFI_CLEAR_SETTING  "SINGLE_IMG_FW_ONLY"
#define  FWU_IMG_STR_SINGLE_ATFUEFI_FW             "SINGLE_IMG_CLEAR_SETTING"
#define  FWU_IMG_STR_SINGLE_ATFUEFI_UNKNOWN        "SINGLE_IMG_UNKNOWN"
#define  FWU_IMG_STR_UNKNOWN                       "UNKNOWN"
#define  FWU_IMG_STR_NULL                          "NULL"

#define  FWU_STATUS_STR_NOTSTART            "NOTSTART"
#define  FWU_STATUS_STR_SUCCESS             "SUCCESS"
#define  FWU_STATUS_STR_IN_PROCESS          "IN_PROCESS"
#define  FWU_STATUS_STR_IO_ERROR            "IO_ERROR"
#define  FWU_STATUS_STR_OUT_OF_RESOURCE     "OUT_OF_RESOURCE"
#define  FWU_STATUS_STR_SECURITY_VIOLATION  "SECURITY_VIOLATION"
#define  FWU_STATUS_STR_FAILED              "FAILED"
#define  FWU_STATUS_STR_UNKNOWN             "UNKNOWN"

#define FWU_VARIABLE_SETUP_LOAD_OFFSET                 L"UpgradeSetUploadOffset"
#define FWU_VARIABLE_CONTINUE_UPLOAD                   L"UpgradeContinueUpload"
#define FWU_VARIABLE_SCP_REQUEST                       L"UpgradeSCPRequest"
#define FWU_VARIABLE_ATFUEFI_REQUEST                   L"UpgradeATFUEFIRequest"
#define FWU_VARIABLE_CFGUEFI_REQUEST                   L"UpgradeCFGUEFIRequest"
#define FWU_VARIABLE_UEFI_REQUEST                      L"UpgradeUEFIRequest"
#define FWU_VARIABLE_SINGLE_IMG_FULL_FLASH_REQUEST     L"UpgradeSingleImageFullFlashRequest"
#define FWU_VARIABLE_SINGLE_IMG_FWONLY_REQUEST         L"UpgradeSingleImageFWOnlyRequest"
#define FWU_VARIABLE_SINGLE_IMG_CLEAR_SETTING_REQUEST  L"UpgradeSingleImageClearSettingRequest"

#endif // SYSTEM_FIRMWARE_UPDATE_DXE_H_
