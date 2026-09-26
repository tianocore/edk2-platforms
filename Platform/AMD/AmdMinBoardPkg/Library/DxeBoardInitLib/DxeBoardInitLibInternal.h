/** @file

  Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#define VGA_MEM_BASE  0xA0000
#define VGA_MEM_SIZE  0x20000

/**
  Report an I/O range to GCD and allocate it if needed.

  @param[in]  ImageHandle  Handle of the DXE image reserving the range.
  @param[in]  BaseAddress  Base address of the I/O range.
  @param[in]  Length       Length of the I/O range.
  @param[in]  RangeName    Debug name of the I/O range.

  @retval EFI_SUCCESS            The I/O range is reserved in GCD.
  @retval EFI_INVALID_PARAMETER  RangeName is NULL, BaseAddress is 0, or Length is 0.
  @retval Others                 Error returned by GCD services.
**/
EFI_STATUS
EFIAPI
ReportIoRangeToGcd (
  IN EFI_HANDLE   ImageHandle,
  IN UINT64       BaseAddress,
  IN UINT64       Length,
  IN CONST CHAR8  *RangeName
  );

/**
  Reserve Legacy VGA IO space.

  @retval  EFI_SUCCESS  MMIO at Legacy VGA region has been allocated.
  @retval  !EFI_SUCCESS Error allocating the legacy VGA region.

**/
EFI_STATUS
EFIAPI
ReserveLegacyVgaIoSpace (
  VOID
  );

/**
  Report the FADT I/O resources to GCD and allocate them if required.

  @param[in]  ImageHandle  Handle of the DXE image reserving the ranges.

  @retval EFI_SUCCESS      The FADT I/O resources are reserved in GCD.
  @retval Others           Error reserving one or more ranges.
**/
EFI_STATUS
EFIAPI
ReserveFadtIoResources (
  IN EFI_HANDLE  ImageHandle
  );
