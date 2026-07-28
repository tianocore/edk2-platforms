/** @file
  Library for detecting and accessing the Simics UEFI PCI config device.

  Provides shared helpers that probe PCI config space for the virtual
  Simics device and perform subsequent reads/writes through the
  PCI Root Bridge I/O protocol.

  Copyright (c) 2026, Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SIMICS_UEFI_DEVICE_LIB_H_
#define SIMICS_UEFI_DEVICE_LIB_H_

/**
  Check for the presence of the Simics UEFI PCI config device.

  Reads PCDs for bus/device/function, assembles the PCI config address,
  locates the PCI Root Bridge I/O protocol, and probes the vendor /
  subsystem IDs to confirm the expected Simics device is present.

  @retval EFI_SUCCESS          The Simics UEFI device was found.
  @retval EFI_NOT_FOUND        The device is not present or IDs do not match.
  @retval other                LocateProtocol or PCI read failed.
**/
EFI_STATUS
EFIAPI
SimicsUefiDeviceCheck (
  VOID
  );

/**
  Read from PCI config space of the Simics UEFI device.

  Locates the device and reads at the given offset.  Callers that
  have already obtained the protocol / address via SimicsUefiDeviceCheck
  may use the protocol pointer directly instead.

  @param[in]  Offset  Byte offset within PCI config space.
  @param[in]  Width   Access width in bytes (1, 2, 4, or 8).
  @param[out] Buffer  Destination for the data read.

  @retval EFI_SUCCESS            Read completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is not 1, 2, 4, or 8.
  @retval other                  Device not found or read failed.
**/
EFI_STATUS
EFIAPI
SimicsUefiDeviceRead (
  IN  UINT64  Offset,
  IN  UINT8   Width,
  OUT VOID    *Buffer
  );

/**
  Write to PCI config space of the Simics UEFI device.

  @param[in] Offset  Byte offset within PCI config space.
  @param[in] Width   Access width in bytes (1, 2, 4, or 8).
  @param[in] Buffer  Source data to write.

  @retval EFI_SUCCESS            Write completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is not 1, 2, 4, or 8.
  @retval other                  Device not found or write failed.
**/
EFI_STATUS
EFIAPI
SimicsUefiDeviceWrite (
  IN UINT64  Offset,
  IN UINT8   Width,
  IN VOID    *Buffer
  );

#endif // SIMICS_UEFI_DEVICE_LIB_H_
