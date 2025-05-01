/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

/**
 * @brief Bios send these commands to PSP to grant dTPM status and event log
 *
 * @param[out]      DesiredConfig       dTPM configuration requested
 * @param[out]      ConfigStatus        0 - success. non-zero failure.
 * @param[in,out]   LogDataSize         Size of LogData buffer
 * @param[out]      LogData             Point to allocated event log buffer
 *
 * @retval EFI_STATUS                   0: Success, NonZero Error
 */
EFI_STATUS
PspMboxGetDTPMData (
  OUT UINT32     *DesiredConfig,
  OUT UINT32     *ConfigStatus,
  IN OUT UINT32  *LogDataSize,
  OUT VOID       *LogData
  )
{
  return EFI_SUCCESS;
}
