/** @file

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent<BR>

**/

#ifndef BOOT_OPTION_CONFIG_H_
#define BOOT_OPTION_CONFIG_H_

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootManagerLib.h>
#include <IndustryStandard/Ipmi.h>
#include <Library/IpmiBaseLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define UEFI_HARD_DRIVE_NAME  L"UEFI Hard Drive"

/**
  Find the Lan-On-Motherboard device path.

  @param[out]   LomDevicePath         DevicePath of the LOM device. NULL if the LOM
                                      is not found
  @retval       EFI NOT_FOUND         LOM device path is not found
  @retval       EFI_SUCCESS           LOM device path found
**/
EFI_STATUS
EFIAPI
GetLomDevicePath (
  OUT EFI_DEVICE_PATH_PROTOCOL  **LomDevicePath
  );

#endif
