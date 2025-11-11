/** @file
  LcdHwLib implementation for ARM Mali Dxx display controller family

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdHwLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "RegisterMap.h"

/// Milliseconds to block for device acknowledge operations
#define DEVICE_TIMEOUT_MS  100

/// The base MMIO address for the device
#define DEVICE_BASE \
    ((EFI_PHYSICAL_ADDRESS)(FixedPcdGet64 (PcdArmMaliDxxBase)))

/**
  Writes a device register.

  Writes the register specified by Register with the value specified by Value.

  If Register is unaligned or out of range, then ASSERT().

  @param[in]  Register The device register to write.
  @param[in]  Value    The value to write to the device register.

**/
STATIC
VOID
DeviceWrite (
  IN UINT32  Register,
  IN UINT32  Value
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  MmioWrite32 (DEVICE_BASE + Register, Value);
}

/**
  Reads a device register.

  Reads the register specified by Register.

  If Register is unaligned or out of range, then ASSERT().

  @param[in] Register The device register to read.

  @return The value read.

**/
STATIC
UINT32
DeviceRead (
  IN UINT32  Register
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  return MmioRead32 (DEVICE_BASE + Register);
}

/**
  Reads a device register, performs a bitwise AND, and writes the result
  back to the register.

  Reads the device register specified by Register, performs a bitwise AND
  between the read result and the value specified by AndData, and writes the
  result to the device register specified by Register.

  If Register is unaligned or out of range, then ASSERT().

  @param[in]  Register The device register to modify.
  @param[in]  AndData  The value to AND with the read value from the device
                       register.

**/
STATIC
VOID
DeviceAnd (
  IN UINT32  Register,
  IN UINT32  AndData
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  MmioAnd32 (DEVICE_BASE + Register, AndData);
}

/**
  Reads a device register, performs a bitwise OR, and writes the result
  back to the register.

  Reads the device register specified by Register, performs a bitwise OR
  between the read result and the value specified by OrData, and writes the
  result to the device register specified by Register.

  If Register is unaligned or out of range, then ASSERT().

  @param[in]  Register The device register to modify.
  @param[in]  OrData   The value to OR with the read value from the device
                       register.

**/
STATIC
VOID
DeviceOr (
  IN UINT32  Register,
  IN UINT32  OrData
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  MmioOr32 (DEVICE_BASE + Register, OrData);
}

/**
  Retrieve mode information from the platform library.

  @param[in]  ModeNumber          Display mode number.
  @param[out] Horizontal          Pointer to horizontal timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)
  @param[out] Vertical            Pointer to vertical timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)
  @param[out] PixelFormat         The physical format of a pixel.
**/
STATIC
EFI_STATUS
GetModeDetailFromPlatform (
  IN CONST UINT32                ModeNumber,
  OUT CONST SCAN_TIMINGS         **Horizontal,
  OUT CONST SCAN_TIMINGS         **Vertical,
  OUT EFI_GRAPHICS_PIXEL_FORMAT  *PixelFormat
  )
{
  EFI_STATUS                            Status;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  ModeInfo;

  Status = LcdPlatformGetTimings (
             ModeNumber,
             (SCAN_TIMINGS **)Horizontal,
             (SCAN_TIMINGS **)Vertical
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*Horizontal == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (*Vertical == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = LcdPlatformQueryMode (ModeNumber, &ModeInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ModeInfo.HorizontalResolution != (*Horizontal)->Resolution) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (ModeInfo.VerticalResolution != (*Vertical)->Resolution) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *PixelFormat = ModeInfo.PixelFormat;

  return EFI_SUCCESS;
}

/**
  Check for presence of display.

  @retval EFI_SUCCESS    Platform implements display.
  @retval EFI_NOT_FOUND  Display not found on the platform.

**/
EFI_STATUS
LcdIdentify (
  VOID
  )
{
  UINT32  ArchId;
  UINT32  ProductId;

  if (DEVICE_BASE == 0) {
    return EFI_NOT_FOUND;
  }

  ArchId = FIELD_GET (DeviceRead (GCU_GLB_ARCH_ID), ARCH_ID);
  if (ArchId != ARCH) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:MaliDxx@0x%p]: unexpected architecture value: 0x%x\n",
      gEfiCallerBaseName,
      DEVICE_BASE,
      ArchId
      ));
    return EFI_NOT_FOUND;
  }

  // while this driver should work with all cores from this architecture we only
  // enable it for those it have been validated on
  ProductId = FIELD_GET (DeviceRead (GCU_GLB_CORE_ID), PRODUCT_ID);
  switch (ProductId) {
    // we treat the D71 as it would have been a D32 (D71 is a strict superset
    // of D32)
    case CORE_D71:
    case CORE_D32:
      DEBUG ((
        DEBUG_INFO,
        "[%a:MaliDxx@0x%p]: D%x core detected.\n",
        gEfiCallerBaseName,
        DEVICE_BASE,
        ProductId
        ));

      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "[%a:MaliDxx@0x%p]: unsupported core: 0x%x\n",
        gEfiCallerBaseName,
        DEVICE_BASE,
        ProductId
        ));
      return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Converts milliseconds into number of ticks of the performance counter.

  @param[in] Milliseconds  Milliseconds to convert into ticks.

  @retval Milliseconds expressed as number of ticks.

**/
STATIC
UINT64
MilliSecondsToTicks (
  IN UINTN  Milliseconds
  )
{
  CONST UINT64  NanoSecondsPerTick = GetTimeInNanoSecond (1);

  return (Milliseconds * 1000000) / NanoSecondsPerTick;
}

/**
  Poll Register, apply Mask and compare with Value until it matches or
  TimeoutMilliseconds have passed.

  @param[in] Register             The register to read.
  @param[in] Mask                 Value to mask content of Register with.
  @param[in] Value                Compare masked register content to this.
  @param[in] TimeoutMilliseconds  Max time to block.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT
**/
STATIC
EFI_STATUS
BlockUntilDeviceRegisterValueOrTimeout (
  IN UINT32  Register,
  IN UINT32  Mask,
  IN UINT32  Value,
  IN UINTN   TimeoutMilliseconds
  )
{
  UINT64  TickOut;

  TickOut = GetPerformanceCounter () + MilliSecondsToTicks (TimeoutMilliseconds);
  while ((DeviceRead (Register) & Mask) != Value) {
    if (GetPerformanceCounter () > TickOut) {
      return EFI_TIMEOUT;
    }
  }

  return EFI_SUCCESS;
}

/**
  Commit DPU configuration.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT
**/
STATIC
EFI_STATUS
GcuCommit (
  VOID
  )
{
  EFI_STATUS  Status;

  DeviceOr (GCU_GCU_CONFIG_VALID0, FIELD (CVAL));
  Status = BlockUntilDeviceRegisterValueOrTimeout (
             GCU_GCU_CONFIG_VALID0,
             FIELD_MASK (CVAL),
             0,
             DEVICE_TIMEOUT_MS
             );

  if (EFI_ERROR (Status)) {
    DeviceAnd (GCU_GCU_CONFIG_VALID0, ~FIELD(CVAL));
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Disable all sub-system and then reset the device.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT
  @retval EFI_DEVICE_ERROR  Unexpected device state.
**/
STATIC
EFI_STATUS
DeviceReset (
  VOID
  )
{
  EFI_STATUS  Status;

  // The sub-system disable is mainly to avoid some situations that may prevent
  // a full SRST so errors during the first phase, while unexpected, is
  // not necessary a critical failure.
  // This function will only fail if the SRST fails.

  // make sure no changes are in progress
  DeviceWrite (GCU_GCU_CONFIG_VALID0, 0);

  // switch to GCU_MODE_INACTIVE as soon as possible
  DeviceWrite (GCU_GCU_CONTROL, 0);

  switch ( FIELD_GET (DeviceRead (GCU_GCU_STATUS), MODE)) {
    case GCU_MODE_INACTIVE:
      // the sub-systems is already enough disabled
      goto Reset;
    case GCU_MODE_DO0_ACTIVE:
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "[%a:MaliDxx@0x%p]: GCU.GCU_STATUS.MODE == %u is not expected, try SRST anyway!\n",
        gEfiCallerBaseName,
        DEVICE_BASE
        ));
      ASSERT (0);
      goto Reset;
  }

  DeviceWrite (LPU0_LAYER0_LR_CONTROL, 0);
  DeviceWrite (CU0_CU_INPUT0_CONTROL, 0);
  DeviceWrite (DOU0_BS_BS_CONTROL, 0);

  Status = GcuCommit ();
  if (EFI_ERROR (Status)) {
    goto Reset;
  }

  Status = BlockUntilDeviceRegisterValueOrTimeout (
             GCU_GCU_STATUS,
             FIELD_MASK (MODE),
             GCU_MODE_INACTIVE,
             DEVICE_TIMEOUT_MS
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:MaliDxx@0x%p]: set GCU.GCU_CONTROL.MODE == INACTIVE time-out!\n",
      gEfiCallerBaseName,
      DEVICE_BASE
      ));
  }

Reset:
  DeviceWrite (GCU_GCU_CONTROL, FIELD (SRST));
  Status = BlockUntilDeviceRegisterValueOrTimeout (
             GCU_GCU_CONTROL,
             FIELD_MASK (SRST),
             0,
             DEVICE_TIMEOUT_MS
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:MaliDxx@0x%p]: GCU reset time-out!\n",
      gEfiCallerBaseName,
      DEVICE_BASE
      ));
    DeviceAnd (GCU_GCU_CONTROL, ~FIELD(SRST));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Initialize display.

  @param[in] FrameBufferBase  Address of the frame buffer.

  @retval EFI_SUCCESS           Display initialization success.
  @retval EFI_INVALID_PARAMETER Reserved bit(s) set in FrameBufferBase.
  @retval *                     Display initialization failure.

**/
EFI_STATUS
LcdInitialize (
  IN EFI_PHYSICAL_ADDRESS  FrameBufferBase
  )
{
  EFI_STATUS    Status;
  CONST UINT32  PtrLow          = FrameBufferBase;
  CONST UINT32  BitFieldPtrLow  = FIELD_SET (PTR_LOW, PtrLow);
  CONST UINT32  PtrHigh         = FrameBufferBase >> 32;
  CONST UINT32  BitFieldPtrHigh = FIELD_SET (PTR_HIGH, PtrHigh);

  if (  (FIELD_GET (BitFieldPtrLow, PTR_LOW) != PtrLow)
     || (FIELD_GET (BitFieldPtrHigh, PTR_HIGH) != PtrHigh))
  {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:MaliDxx@0x%p]: reserved bits set in frame-buffer address 0x%x\n",
      gEfiCallerBaseName,
      DEVICE_BASE,
      FrameBufferBase
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = DeviceReset ();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  // setup the data-flow FrameBufferBase -> LOU0_L0 (Rich Layer) -> CU0 -> DOU0
  DeviceWrite (LPU0_LAYER0_LR_P0_PTR_LOW, BitFieldPtrLow);
  DeviceWrite (LPU0_LAYER0_LR_P0_PTR_HIGH, BitFieldPtrHigh);
  DeviceWrite (
    CU0_CU_INPUT_ID0,
    // Layer Processing Unit 0 Layer 0 as input to Composition Unit 0
    DeviceRead (LPU0_LAYER0_OUTPUT_ID0)
    );
  DeviceWrite (
    DOU0_IPS_IPS_INPUT_ID0,
    // Composition Unit block 0 as input to the Display Output Unit 0
    DeviceRead (CU0_OUTPUT_ID0)
    );

  return EFI_SUCCESS;
}

/**
  Set requested mode of the display.

  @param[in]  ModeNumber             Display mode number.

  @retval EFI_SUCCESS                Display mode set successful.
  @retval EFI_DEVICE_ERROR           Display mode not found/supported.
**/
EFI_STATUS
LcdSetMode (
  IN CONST UINT32  ModeNumber
  )
{
  EFI_STATUS                 Status;
  EFI_GRAPHICS_PIXEL_FORMAT  GopPixelFormat;
  CONST SCAN_TIMINGS         *Horizontal;
  CONST SCAN_TIMINGS         *Vertical;
  UINT32                     DevicePixelFormat;

  Status = GetModeDetailFromPlatform (
             ModeNumber,
             &Horizontal,
             &Vertical,
             &GopPixelFormat
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (GopPixelFormat) {
    case PixelRedGreenBlueReserved8BitPerColor:
      DevicePixelFormat = FORMAT_XBGR_8888;
      break;
    case PixelBlueGreenRedReserved8BitPerColor:
      DevicePixelFormat = FORMAT_XRGB_8888;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  // the memory stride is the total visible pixels on a line * 4 (4 == all
  // format we support are 4 bytes per pixel)
  CONST UINT32  Stride         = Horizontal->Resolution * sizeof (UINT32);
  CONST UINT32  BitFieldStride = FIELD_SET (STRIDE, Stride);

  if (FIELD_GET (BitFieldStride, STRIDE) != Stride) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DeviceWrite (
    LPU0_LAYER0_LR_IN_SIZE,
    FIELD_SET (VSIZE, Vertical->Resolution) |
    FIELD_SET (HSIZE, Horizontal->Resolution)
    );

  DeviceWrite (LPU0_LAYER0_LR_FORMAT, DevicePixelFormat);
  DeviceWrite (LPU0_LAYER0_LR_P0_STRIDE, BitFieldStride);

  // no scaling
  DeviceWrite (
    CU0_CU_SIZE,
    FIELD_SET (VSIZE, Vertical->Resolution) |
    FIELD_SET (HSIZE, Horizontal->Resolution)
    );
  DeviceWrite (
    CU0_CU_INPUT0_SIZE,
    FIELD_SET (VSIZE, Vertical->Resolution) |
    FIELD_SET (HSIZE, Horizontal->Resolution)
    );
  DeviceWrite (
    DOU0_IPS_IPS_SIZE,
    FIELD_SET (VSIZE, Vertical->Resolution) |
    FIELD_SET (HSIZE, Horizontal->Resolution)
    );
  DeviceWrite (DOU0_IPS_IPS_DEPTH, OUT_DEPTH_8);

  DeviceWrite (
    DOU0_BS_BS_ACTIVESIZE,
    FIELD_SET (VACTIVE, Vertical->Resolution) |
    FIELD_SET (HACTIVE, Horizontal->Resolution)
    );

  DeviceWrite (
    DOU0_BS_BS_HINTERVALS,
    FIELD_SET (HBACKPORCH, Horizontal->BackPorch) |
    FIELD_SET (HFRONTPORCH, Horizontal->FrontPorch)
    );

  DeviceWrite (
    DOU0_BS_BS_VINTERVALS,
    FIELD_SET (VBACKPORCH, Vertical->BackPorch) |
    FIELD_SET (VFRONTPORCH, Vertical->FrontPorch)
    );

  DeviceWrite (
    DOU0_BS_BS_SYNC,
    FIELD_SET (VSP, 0) | // 0 == negative vsync polarization
    FIELD_SET (VSYNCWIDTH, Vertical->Sync) |
    FIELD_SET (HSP, 0) | // 0 == negative hsync polarization
    FIELD_SET (HSYNCWIDTH, Horizontal->Sync)
    );

  DeviceWrite (
    LPU0_LAYER0_LR_CONTROL,
    // cacheable and bufferable, but do not allocate (data-sheet default)
    ARCACHE_AXIC_BUF_CACHE |
    // enable LPU0 rich (0) layer
    FIELD (EN)
    );
  DeviceWrite (
    CU0_CU_INPUT0_CONTROL,
    // global alpha value (data-sheet default is 0xff)
    FIELD_SET (LALPHA, 0xFF) |
    // disable pixel alpha
    FIELD (PAD) |
    // enable Composition Unit 0
    FIELD (EN)
    );
  DeviceWrite (
    DOU0_BS_BS_CONTROL,
    // video mode (normal stream of frames)
    FIELD (VM) |
    // enable display output unit 0, display backend
    FIELD (EN)
    );

  DeviceWrite (GCU_GCU_CONTROL, GCU_MODE_DO0_ACTIVE);
  Status = BlockUntilDeviceRegisterValueOrTimeout (
             GCU_GCU_STATUS,
             FIELD_MASK (MODE),
             GCU_MODE_DO0_ACTIVE,
             DEVICE_TIMEOUT_MS
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a:MaliDxx@0x%p]: GCU_GCU_CONTROL.MODE == DO0_ACTIVE time-out!\n",
      gEfiCallerBaseName,
      DEVICE_BASE
      ));
    return EFI_TIMEOUT;
  }

  Status = GcuCommit ();
  if (EFI_ERROR (Status)) {
    goto error;
  }

  /*
    Older versions of the Linux ARM Mali Dxx (komeda) driver assume that all
    bits in the GCU.GCU_CONTROL are zero during its probe.

    This can be done without anything happening with the hardware because the
    state change DO0_ACTIVE -> INACTIVE require all of LPU0, CU0, and DOU0 to
    be inactive.

    The only other valid bits are SRST so it is safe to 0 the whole register.
  */
  DeviceWrite (GCU_GCU_CONTROL, 0);
  return EFI_SUCCESS;

error:
  {
    EFI_STATUS  CleanupStatus;
    CleanupStatus = DeviceReset ();
    ASSERT_EFI_ERROR (CleanupStatus);
  }
  return Status;
}

/**
   This function de-initializes the display.
**/
VOID
LcdShutdown (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = DeviceReset ();
  ASSERT_EFI_ERROR (Status);
}
