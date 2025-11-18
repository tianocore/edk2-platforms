/** @file
  NXP TDA19988 HDMI transmitter UEFI protocol

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MORELLO_PROTOCOL_TDA19988_H_
#define MORELLO_PROTOCOL_TDA19988_H_

#include <Uefi.h>

typedef struct _TDA19988_PROTOCOL TDA19988_PROTOCOL;

/// TDA19988 CEC core I2C address index
#define TDA19988_CEC_INDEX  0

/// TDA19988 HDMI core I2C address index
#define TDA19988_HDMI_INDEX  1

typedef struct {
  UINT32     Active;
  UINT32     FrontPorch;
  UINT32     Sync;
  UINT32     BackPorch;
  BOOLEAN    SyncPolarityIsNegative;
} TDA19988_SCAN_TIMINGS;

/// This describe a mode for the TDA19988 driver
typedef struct {
  //
  // Pixel clock speed  in kHz.
  //
  UINT32                   PixelFreq;
  TDA19988_SCAN_TIMINGS    Horizontal;
  TDA19988_SCAN_TIMINGS    Vertical;
} TDA19988_MODEINFO;

/**
  Check if a sink is present.

  Check if a sink is present by checking the hot-plug detection level.

  @param[in,out] This         Pointer to an TDA19988_PROTOCOL structure.
  @param[out]    SinkPresent  Write TRUE if sink is detected, otherwise FALSE.

  @retval EFI_SUCCESS           *SinkPresent updated and valid.
  @retval EFI_INVALID_PARAMETER This refers to a TDA19988_PROTOCOL not created
                                by this driver.
  @retval EFI_INVALID_PARAMETER SinkPresent is NULL.
  @retval *                     Other errors are possible.

**/
typedef
EFI_STATUS
(EFIAPI *TDA19988_GET_SINK_PRESENT)(
  IN OUT TDA19988_PROTOCOL *This,
  OUT    BOOLEAN           *SinkPresent
  );

/**
  Configure the specified mode.

  @param[in,out] This  Pointer to an TDA19988_PROTOCOL structure.
  @param[in]     Mode  Describe the mode to configure.

  @retval EFI_SUCCESS           Mode configured.
  @retval EFI_INVALID_PARAMETER This refers to a TDA19988_PROTOCOL not created
                                by this driver.
  @retval EFI_INVALID_PARAMETER Mode is NULL.
  @retval *                     Other errors are possible.

**/
typedef
EFI_STATUS
(EFIAPI *TDA19988_SET_MODE)(
  IN OUT TDA19988_PROTOCOL *This,
  IN     CONST TDA19988_MODEINFO *Mode
  );

/**
  Retrieve the EDID.

  Retrieve the EDID and copy it into an allocated buffer of type
  EfiBootServicesData.
  On success the EdidData is update to point to the allocated buffer and
  EdidSize is updated with the size of the size of the EDID.

  @param[in,out] This     Pointer to an TDA19988_PROTOCOL structure.
  @param[out]    EdidData Updated with pointer to an allocated buffer containing
                          the EDID.
  @param[out]    EdidSize Size of the EDID that EdidData is pointing to.

  @retval EFI_SUCCESS           Mode configured.
  @retval EFI_INVALID_PARAMETER This refers to a TDA19988_PROTOCOL not created
                                by this driver.
  @retval EFI_INVALID_PARAMETER EdidData is NULL.
  @retval EFI_INVALID_PARAMETER EdidSize is NULL.
  @retval *                     Other errors are possible.

**/
typedef
EFI_STATUS
(EFIAPI *TDA19988_GET_EDID)(
  IN OUT TDA19988_PROTOCOL  *This,
  OUT    VOID              **EdidData,
  OUT    UINTN              *EdidSize
  );

/// Provides a basic interface to control the NXP TDA19988 HDMI transmitter
typedef struct _TDA19988_PROTOCOL {
  //
  // Check if a sink is present.
  //
  TDA19988_GET_SINK_PRESENT    GetSinkPresent;
  //
  // Set the TDA19988 device into a specified mode.
  //
  TDA19988_SET_MODE            SetMode;
  //
  // Retrieve the EDID.
  //
  TDA19988_GET_EDID            GetEdid;
} TDA19988_PROTOCOL;

#endif // MORELLO_PROTOCOL_TDA19988_H_
