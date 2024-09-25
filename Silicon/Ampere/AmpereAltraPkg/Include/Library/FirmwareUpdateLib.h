/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FIRMWARE_UPDATE_LIB_
#define FIRMWARE_UPDATE_LIB_

#define FIRMWARE_UPDATE_MAX_SIZE  SIZE_32MB

#define  FWU_IMG_ID_SCP             1
#define  FWU_IMG_ID_ATFUEFI         2
#define  FWU_IMG_ID_CFGUEFI         3
#define  FWU_IMG_ID_UEFI            4
#define  FWU_IMG_ID_SINGLE_ATFUEFI  5

#define  FWU_IMG_SUBID_SINGLE_ATFUEFI_FULL_FLASH     2
#define  FWU_IMG_SUBID_SINGLE_ATFUEFI_CLEAR_SETTING  4
#define  FWU_IMG_SUBID_SINGLE_ATFUEFI_FW             6

#define  FWU_STATE_NOT_STARTED        0
#define  FWU_STATE_UPLOADING_STARTED  1

#define  FWU_RET_SUCCESS                 0
#define  FWU_RET_IN_PROGRESS             1
#define  FWU_RET_ERR_SECURITY_VIOLATION  2
#define  FWU_RET_ERR_OUT_OF_RESOURCES    3
#define  FWU_RET_ERR_IO_ERROR            4
#define  FWU_RET_ERR_GENERIC             5

/**
  Perform firmware update

  @param[in] ImageId                Image ID to update
  @param[in] SubId                  Subfunction to update for FWU_IMG_ID_SINGLE_ATFUEFI
                                      0x02 – Full Image update
                                      0x04 – Update settings only
                                      0x06 – Update FW image only
  @param[in] Resume                 Perform update or get update status
                                      TRUE: perform update.
                                      FALSE: get update status.
  @param[in] Data                   Pointer to an UINT8 data buffer.
  @param[in] DataLen                Data length
  @param[out] UpdateStatus          Pointer to an UINT64 to the update status.
  @param[out] UpdateFWProcess       Pointer to an UINT64 to the update process.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_ACCESS_DENIED       Permission not allowed.
  @retval EFI_DEVICE_ERROR        Service is unavailable.
  @retval EFI_INVALID_PARAMETER   Return status is invalid.
**/
EFI_STATUS
FWUpdateProcess (
  IN UINTN    ImageId,
  IN UINTN    SubId,
  IN BOOLEAN  Resume,
  IN UINT8    *Data,
  IN UINTN    DataLen,
  OUT UINT64  *UpdateStatus,
  OUT UINT64  *UpdateFWProcess
  );

#endif /* FIRMWARE_UPDATE_LIB_ */
