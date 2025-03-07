/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

/**
 * @brief Amd Apcb V3 Pei Driver Entry
 *
 * @param[in]     FileHandle      File Handie
 * @param[in]     PeiServices     Pei Services
 *
 *  @retval EFI_SUCCESS           Set APCB value successfully
 *          Non-EFI_SUCCESS       Function Error
 *
 **/
EFI_STATUS
EFIAPI
AmdApcbV3PeiDriverEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  return EFI_SUCCESS;
}
