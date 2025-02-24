/** @file
  Platform Library for Process Video OpRom.

  Copyright (C) 2022 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PROCESS_VIDEO_OPROM_LIB_H_
#define PROCESS_VIDEO_OPROM_LIB_H_

#define NVDA_VID    0x10DE
#define AMD_DP_VID  0x1002

/**
  Execute registered handlers based on input AuthenticationOperation until
  one returns an error and that error is returned.

  If none of the handlers return an error, then EFI_SUCCESS is returned.
  The handlers those satisfy AuthenticationOperation will only be executed.
  The handlers are executed in same order to their registered order.

  @param[in]  AuthenticationOperation
                           The operation type specifies which handlers will be executed.
  @param[in]  AuthenticationStatus
                           The authentication status for the input file.
  @param[in]  File         This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]  FileBuffer   A pointer to the buffer with the UEFI file image
  @param[in]  FileSize     The size of File buffer.
  @param[in]  BootPolicy   A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval EFI_INVALID_PARAMETER   File and FileBuffer are both NULL.

**/
EFI_STATUS
EFIAPI
ExecuteSecurity2Handlers (
  IN  UINT32                          AuthenticationOperation,
  IN  UINT32                          AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File  OPTIONAL,
  IN  VOID                            *FileBuffer,
  IN  UINTN                           FileSize,
  IN  BOOLEAN                         BootPolicy
  );

#endif // PROCESS_VIDEO_OPROM_LIB_H_
