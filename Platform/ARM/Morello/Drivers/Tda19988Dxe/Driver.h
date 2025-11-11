/** @file
  NXP TDA19988 HDMI transmitter UEFI Driver

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MORELLO_TDA19988DXE_DRIVER_H_
#define MORELLO_TDA19988DXE_DRIVER_H_

#include <Protocol/I2cIo.h>
#include <Protocol/Tda19988.h>

/// Device context struct for NXP TDA19988 HDMI transmitter
typedef struct {
  UINT64                 Signature;
  /// The last register page set on the HDMI core
  UINT8                  HdmiCurrentPage;

  /// Consumed protocols
  EFI_I2C_IO_PROTOCOL    *I2cIo;

  /// Produced protocols
  TDA19988_PROTOCOL      Tda19988;
} TDA19988_CONTEXT;

/**
  Check if a sink is present by checking the hot-plug detection level.

  @param[in,out] Dev          Device context.
  @param[out]    SinkPresent  Write TRUE if sink is detected, otherwise FALSE.

  @retval EFI_SUCCESS           *SinkPresent updated and valid.
  @retval *                     Other errors are possible.

**/
EFI_STATUS
DriverGetSinkPresent (
  IN OUT TDA19988_CONTEXT  *Dev,
  OUT    BOOLEAN           *SinkPresent
  );

/**
  Configure the specified mode.

  @param[in,out] Dev   Device context.
  @param[in]     Mode  Describe the mode to configure.

  @retval EFI_SUCCESS           Mode configured.
  @retval *                     Other errors are possible.

**/
EFI_STATUS
DriverSetMode (
  IN OUT TDA19988_CONTEXT         *Dev,
  IN     CONST TDA19988_MODEINFO  *Mode
  );

/**
  Retrieve the EDID.

  Retrieve the EDID and copy it into an allocated buffer of type
  EfiBootServicesData.
  On success the EdidData is updated to point to the allocated buffer and
  EdidSize is updated with the size of the EDID.

  @param[in,out] Dev      Device context.
  @param[out]    EdidData Updated with pointer to an allocated buffer containing
                          the EDID.
  @param[out]    EdidSize Size of the EDID that EdidData is pointing to.

  @retval EFI_SUCCESS           *EdidData and *EdidSize updated and valid.
  @retval *                     Other errors are possible.

**/
EFI_STATUS
DriverGetEdid (
  IN OUT TDA19988_CONTEXT  *Dev,
  OUT    VOID              **EdidData,
  OUT    UINTN             *EdidSize
  );

/**
  Initialize a NXP TDA19988 hardware and connect it to the device context
  instance Dev.

  @param[in,out] Dev       Device context.

  @retval EFI_SUCCESS      The device was started.
  @retval EFI_UNSUPPORTED  The hardware is not compatible with this driver.
  @retval *                Other errors are possible.

**/
EFI_STATUS
DriverStart (
  IN OUT TDA19988_CONTEXT  *Dev
  );

/**
  Disconnect the hardware from the driver instance.

  @param[in,out] Dev  Device context.
**/
VOID
DriverStop (
  IN OUT TDA19988_CONTEXT  *Dev
  );

#endif // MORELLO_TDA19988DXE_DRIVER_H_
