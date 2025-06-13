/** @file
  FVP Morello GOP platform implementation details

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/LcdPlatformLib.h>

#define STD_DISPLAY_MODE_INIT(mode) \
  { \
    mode##_OSC_FREQUENCY, \
    { \
      mode##_H_RES_PIXELS, \
      mode##_H_SYNC, \
      mode##_H_BACK_PORCH, \
      mode##_H_FRONT_PORCH \
    }, \
    { \
      mode##_V_RES_PIXELS, \
      mode##_V_SYNC, \
      mode##_V_BACK_PORCH, \
      mode##_V_FRONT_PORCH \
    }, \
  }

typedef struct {
  UINT32          OscFreq;
  SCAN_TIMINGS    Horizontal;
  SCAN_TIMINGS    Vertical;
} DISPLAY_MODE;

STATIC DISPLAY_MODE  mDisplayMode = STD_DISPLAY_MODE_INIT (VGA);

/** Platform related initialization function.

  @param[in] Handle              Handle to the LCD device instance.

  @retval EFI_SUCCESS            Platform library initialized successfully.
**/
EFI_STATUS
LcdPlatformInitializeDisplay (
  IN  EFI_HANDLE  Handle
  )
{
  return EFI_SUCCESS;
}

/** Return total number of modes supported.

  Note: Valid mode numbers are 0 to LcdPlatformGetMaxMode() - 1
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
**/
EFI_STATUS
LcdPlatformSetMode (
  IN  UINT32  ModeNumber
  )
{
  if (ModeNumber != 0) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
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
