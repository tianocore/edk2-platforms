/** @file
  SoC Morello GOP platform implementation details

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/EdidActive.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/Tda19988.h>

typedef struct {
  UINT32          OscFreq;
  SCAN_TIMINGS    Horizontal;
  SCAN_TIMINGS    Vertical;
} DISPLAY_MODE;

STATIC DISPLAY_MODE  mDisplayMode = {
  // 1080p (1920 x 1080 @ 60) CEA-861
  148500000,
  { 1920,   44, 148, 88 },
  { 1080,   5,  36,  4  },
};

STATIC EFI_EDID_DISCOVERED_PROTOCOL  mEdidDiscovered;
STATIC EFI_EDID_ACTIVE_PROTOCOL      mEdidActive;

/**
  Retrieve the TDA 19988 protocol marked for the Morello GOP.

  This function should always return a valid pointer because this DXE Depex
  depends on the relevant GUID.

  @return NULL  Unable to retrieve the protocol.
  @return !NULL Valid TDA 19988 protocol.
**/
STATIC
TDA19988_PROTOCOL *
LocateGopTda19988 (
  VOID
  )
{
  EFI_STATUS         Status;
  TDA19988_PROTOCOL  *Tda19988;

  Status = gBS->LocateProtocol (
                  &gArmMorelloPlatformGopTda19988Guid,
                  NULL,
                  (VOID **)&Tda19988
                  );

  // this should never fail because we DEPEX on it
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:%a]: No ArmMorelloPlatformGopTda19988?! -- %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    ASSERT (0);
    Tda19988 = NULL;
  }

  return Tda19988;
}

/** Platform related initialization function.

  @param[in] Handle              Handle to the GOP.

  @retval EFI_SUCCESS            Platform library initialized successfully.
  @retval *                      Other errors are possible.
**/
EFI_STATUS
LcdPlatformInitializeDisplay (
  IN  EFI_HANDLE  Handle
  )
{
  VOID        *EdidData;
  UINTN       EdidSize;
  BOOLEAN     IsSinkPresent;
  EFI_STATUS  Status;

  TDA19988_PROTOCOL *CONST  Tda19988 = LocateGopTda19988 ();

  if (Tda19988 == NULL) {
    return EFI_NOT_READY;
  }

  Status = Tda19988->GetSinkPresent (Tda19988, &IsSinkPresent);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IsSinkPresent) {
    Status = Tda19988->GetEdid (Tda19988, &EdidData, &EdidSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    mEdidDiscovered.Edid       = EdidData;
    mEdidDiscovered.SizeOfEdid = EdidSize;

    mEdidActive.Edid       = EdidData;
    mEdidActive.SizeOfEdid = EdidSize;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiEdidDiscoveredProtocolGuid,
                    &mEdidDiscovered,
                    &gEfiEdidActiveProtocolGuid,
                    &mEdidActive,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/** Return total number of modes supported.

  Note: Valid mode numbers are 0 to MaxMode - 1
  See Section 12.9 of the UEFI Specification 2.7

  @retval UINT32             Mode Number.
**/
UINT32
LcdPlatformGetMaxMode (
  VOID
  )
{
  return 1;
}

/** Set the requested display mode.

  @param[in] ModeNumber            Mode Number.

  @retval  EFI_SUCCESS             Mode set successfully.
  @retval  EFI_INVALID_PARAMETER   Requested mode not found.
  @retval  EFI_NOT_READY           HDMI transceiver not ready.
  @retval  *                       Other errors are possible.

**/
EFI_STATUS
LcdPlatformSetMode (
  IN  UINT32  ModeNumber
  )
{
  if (ModeNumber != 0) {
    return EFI_INVALID_PARAMETER;
  }

  TDA19988_PROTOCOL *CONST  Tda19988 = LocateGopTda19988 ();

  if (Tda19988 == NULL) {
    return EFI_NOT_READY;
  }

  CONST TDA19988_MODEINFO  Mode = {
    mDisplayMode.OscFreq/1000,
    {
      mDisplayMode.Horizontal.Resolution,
      mDisplayMode.Horizontal.FrontPorch,
      mDisplayMode.Horizontal.Sync,
      mDisplayMode.Horizontal.BackPorch,
      TRUE,
    },
    {
      mDisplayMode.Vertical.Resolution,
      mDisplayMode.Vertical.FrontPorch,
      mDisplayMode.Vertical.Sync,
      mDisplayMode.Vertical.BackPorch,
      FALSE,
    }
  };

  return Tda19988->SetMode (Tda19988, &Mode);
}

/** Return information for the requested mode number.

  @param[in]  ModeNumber         Mode Number.
  @param[out] Info               Pointer for returned mode information
                                 (on success).

  @retval EFI_SUCCESS             Mode information for the requested mode
                                  returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformQueryMode (
  IN  UINT32                                ModeNumber,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  if (ModeNumber != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Info->Version              = 0;
  Info->HorizontalResolution = mDisplayMode.Horizontal.Resolution;
  Info->PixelsPerScanLine    = mDisplayMode.Horizontal.Resolution;
  Info->VerticalResolution   = mDisplayMode.Vertical.Resolution;
  Info->PixelFormat          = PixelBlueGreenRedReserved8BitPerColor;

  return EFI_SUCCESS;
}

/** Return display timing information for the requested mode number.

  @param[in]  ModeNumber          Mode Number.
  @param[out] Horizontal          Pointer to horizontal timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)
  @param[out] Vertical            Pointer to vertical timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)


  @retval EFI_SUCCESS             Display timing information for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetTimings (
  IN  UINT32        ModeNumber,
  OUT SCAN_TIMINGS  **Horizontal,
  OUT SCAN_TIMINGS  **Vertical
  )
{
  if (ModeNumber != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *Horizontal = &mDisplayMode.Horizontal;
  *Vertical   = &mDisplayMode.Vertical;
  return EFI_SUCCESS;
}

/** Return bits per pixel information for a mode number.

  @param[in]  ModeNumber          Mode Number.
  @param[out] Bpp                 Pointer to value bits per pixel information.

  @retval EFI_SUCCESS             Bit per pixel information for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetBpp (
  IN  UINT32   ModeNumber,
  OUT LCD_BPP  *Bpp
  )
{
  if (ModeNumber != 0) {
    return EFI_INVALID_PARAMETER;
  }

  // LcdBitsPerPixel_24 means 32-bits pixel in LcdGraphicsOutputBlt
  *Bpp = LcdBitsPerPixel_24;
  return EFI_SUCCESS;
}
