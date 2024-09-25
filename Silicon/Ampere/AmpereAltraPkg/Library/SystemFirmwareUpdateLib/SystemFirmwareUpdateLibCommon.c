/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FirmwareUpdateLib.h>

#include "SystemFirmwareUpdateLibCommon.h"

#define MM_FWU_PAYLOAD_LENGTH        5
#define MM_FWU_ENABLE_UPDATE_REPORT  1

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
  )
{
  EFI_MM_COMMUNICATE_FWU_RESPONSE  MmFirmwareUpdateRes;
  EFI_STATUS                       Status;
  UINT64                           MmData[MM_FWU_PAYLOAD_LENGTH];

  if (  ((Resume == FALSE) && ((Data == NULL) || (DataLen == 0)))
     || (UpdateStatus == NULL) || (UpdateFWProcess == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (MmData, sizeof (MmData));
  MmData[0] = ImageId;
  MmData[1] = Resume ? 0 : DataLen;
  MmData[2] = Resume ? 0 : (UINT64)Data;   // Physical address
  MmData[3] = MM_FWU_ENABLE_UPDATE_REPORT; // MM yield for progress reporting
  if (ImageId == FWU_IMG_ID_SINGLE_ATFUEFI) {
    MmData[3] |= SubId;
  }

  Status = FirmwareUpdateMmCommunicate (
             MmData,
             sizeof (MmData),
             &MmFirmwareUpdateRes,
             sizeof (MmFirmwareUpdateRes)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Return data in the first double word of payload
  switch (MmFirmwareUpdateRes.Status) {
    case FWU_MM_RES_SUCCESS:
      *UpdateStatus = FWU_RET_SUCCESS;
      break;

    case FWU_MM_RES_IN_PROGRESS:
      *UpdateStatus = FWU_RET_IN_PROGRESS;
      break;

    case FWU_MM_RES_IO_ERROR:
      *UpdateStatus = FWU_RET_ERR_IO_ERROR;
      break;

    case FWU_MM_RES_OUT_OF_RESOURCES:
      *UpdateStatus = FWU_RET_ERR_OUT_OF_RESOURCES;
      break;

    case FWU_MM_RES_SECURITY_VIOLATION:
      *UpdateStatus = FWU_RET_ERR_SECURITY_VIOLATION;
      break;

    case FWU_MM_RES_FAIL:
      *UpdateStatus = FWU_RET_ERR_GENERIC;
      break;

    default:
      *UpdateStatus = FWU_RET_SUCCESS;
      break;
  }

  if (*UpdateStatus == FWU_RET_IN_PROGRESS) {
    *UpdateFWProcess = MmFirmwareUpdateRes.Progress;
  }

  return EFI_SUCCESS;
}
