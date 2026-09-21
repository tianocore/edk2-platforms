/** @file

  Get SoC specific UART information.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <FchRegistersCommon.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include "AcpiSsdtSerialLib.h"

///
/// Table of FCH UART configurations
///
STATIC CONST UART_CONFIG_ENTRY  mUartConfigTable[] = {
  { UART0, FCH_RT_DEVICE_ENABLE_MAP_UART0_BIT, FCH_MMIO_BASE_UART0, "UART0" },
  { UART1, FCH_RT_DEVICE_ENABLE_MAP_UART1_BIT, FCH_MMIO_BASE_UART1, "UART1" },
  { UART2, FCH_RT_DEVICE_ENABLE_MAP_UART2_BIT, FCH_MMIO_BASE_UART2, "UART2" }
};

#define FCH_UART_COUNT  (sizeof (mUartConfigTable) / sizeof (mUartConfigTable[0]))

/**
  Get the access mode string for debug output.

  @param[in]  Mode  The UART access mode.

  @retval  String representation of the access mode.
**/
STATIC
CONST CHAR8 *
GetUartModeString (
  IN  UART_ACCESS_MODE  Mode
  )
{
  switch (Mode) {
    case UartModeMmio:
      return "MMIO";
    case UartModeLegacyIo:
      return "Legacy IO";
    default:
      return "Disabled";
  }
}

///
/// Legacy IO range mapping table
///
typedef struct {
  UINT16    IoBase;     ///< Legacy IO base address
  UINT8     RangeIndex; ///< Range index (0-3)
} LEGACY_IO_RANGE_ENTRY;

STATIC CONST LEGACY_IO_RANGE_ENTRY  mLegacyIoRanges[] = {
  { 0x2E8, 0 },  // Range 0
  { 0x2F8, 1 },  // Range 1
  { 0x3E8, 2 },  // Range 2
  { 0x3F8, 3 }   // Range 3
};

#define LEGACY_IO_RANGE_COUNT  (sizeof (mLegacyIoRanges) / sizeof (mLegacyIoRanges[0]))

/**
  Get legacy I/O range for UART if enabled in AL2AHB register.

  @param[in]  UartIoEnableReg    The AL2AHB UART I/O enable register value.
  @param[in]  UartIndex          The UART index (0, 1, or 2).

  @retval     Legacy I/O base address if enabled, 0 otherwise.
**/
STATIC
UINT16
GetUartLegacyIoRange (
  IN  AL2AHB_LEGACY_UART_IO_ENABLE_REGISTER  UartIoEnableReg,
  IN  UINT8                                  UartIndex
  )
{
  UINT32  EnableBits;
  UINT32  UartAssignment;
  UINT8   Index;

  //
  // Extract enable bits (bits 0-3) and assignment fields
  //
  EnableBits = UartIoEnableReg.Value & 0xF;

  for (Index = 0; Index < LEGACY_IO_RANGE_COUNT; Index++) {
    //
    // Check if this range is enabled
    //
    if ((EnableBits & (1 << Index)) == 0) {
      continue;
    }

    //
    // Get the UART assignment for this range (2 bits per range, starting at bit 8)
    //
    UartAssignment = (UartIoEnableReg.Value >> (8 + (Index * 2))) & 0x3;

    if (UartAssignment == UartIndex) {
      return mLegacyIoRanges[Index].IoBase;
    }
  }

  return 0;
}

/**
  Determine the access mode for a specific UART.

  @param[in]  UartIndex           The UART index (0, 1, or 2).
  @param[in]  FchDeviceEnableMap  The FchRTDeviceEnableMap value.
  @param[in]  UartIoEnableReg     The AL2AHB UART I/O enable register value.
  @param[out] BaseAddress         Pointer to store the base address.

  @retval  UART_ACCESS_MODE indicating how the UART is configured.
**/
STATIC
UART_ACCESS_MODE
GetUartAccessMode (
  IN  UINT8                                  UartIndex,
  IN  UINT32                                 FchDeviceEnableMap,
  IN  AL2AHB_LEGACY_UART_IO_ENABLE_REGISTER  UartIoEnableReg,
  OUT UINT64                                 *BaseAddress
  )
{
  UINT32  EnableBit;
  UINT16  LegacyIoBase;

  if (BaseAddress == NULL) {
    return UartModeDisabled;
  }

  *BaseAddress = 0;

  //
  // Get the enable bit for this UART from the config table
  //
  if (UartIndex >= FCH_UART_COUNT) {
    return UartModeDisabled;
  }

  EnableBit = mUartConfigTable[UartIndex].EnableBit;

  //
  // Check if UART is enabled in FchRTDeviceEnableMap
  //
  if ((FchDeviceEnableMap & EnableBit) == 0) {
    return UartModeDisabled;
  }

  //
  // UART is enabled - check if it's configured for Legacy IO or MMIO
  //
  LegacyIoBase = GetUartLegacyIoRange (UartIoEnableReg, UartIndex);
  if (LegacyIoBase != 0) {
    *BaseAddress = LegacyIoBase;
    return UartModeLegacyIo;
  }

  //
  // Not Legacy IO, use MMIO
  //
  *BaseAddress = mUartConfigTable[UartIndex].MmioBase;
  return UartModeMmio;
}

/**
  Get SoC specific UART information.

  This function scans FCH UARTs (UART0-2) to determine:
  1. Which UARTs are enabled (via FchRTDeviceEnableMap)
  2. Whether each enabled UART uses MMIO or Legacy IO mode

  @param [out]  SerialPortInfo   Pointer to the serial port information structure.
  @param [out]  SerialPortCount  Number of serial ports found.

  @retval EFI_SUCCESS           The information was retrieved successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
EFIAPI
GetSocUartInfo (
  OUT SERIAL_PORT_CONFIG  **SerialPortInfo,
  OUT UINT8               *SerialPortCount
  )
{
  AL2AHB_LEGACY_UART_IO_ENABLE_REGISTER  UartIoEnableReg;
  SERIAL_PORT_CONFIG                     *UartInfo;
  UINT32                                 FchDeviceEnableMap;
  UINT8                                  EnabledCount;
  UINT8                                  MmioCount;
  UINT8                                  LegacyIoCount;
  UINT8                                  PortIndex;
  UINT8                                  UartIndex;
  UART_ACCESS_MODE                       Mode;
  UINT64                                 BaseAddress;

  if ((SerialPortInfo == NULL) || (SerialPortCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *SerialPortInfo  = NULL;
  *SerialPortCount = 0;

  UartInfo = AllocateZeroPool (sizeof (SERIAL_PORT_CONFIG) * MAX_UART_PORTS);
  if (UartInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read configuration registers
  //
  FchDeviceEnableMap    = PcdGet32 (FchRTDeviceEnableMap);
  UartIoEnableReg.Value = MmioRead32 (FCH_AL2AHBx20_LEGACY_UART_IO_ENABLE);

  DEBUG ((
    DEBUG_INFO,
    "SSDT-SERIAL: FchRTDeviceEnableMap=0x%08X, AL2AHB_LegacyUartIoEnable=0x%08X\n",
    FchDeviceEnableMap,
    UartIoEnableReg.Value
    ));

  //
  // Initialize counters
  //
  EnabledCount  = 0;
  MmioCount     = 0;
  LegacyIoCount = 0;
  PortIndex     = 0;

  //
  // Check FCH UARTs (UART0, UART1, UART2) using table-driven approach
  //
  for (UartIndex = 0; UartIndex < FCH_UART_COUNT; UartIndex++) {
    Mode = GetUartAccessMode (
             mUartConfigTable[UartIndex].UartIndex,
             FchDeviceEnableMap,
             UartIoEnableReg,
             &BaseAddress
             );

    if (Mode == UartModeDisabled) {
      DEBUG ((
        DEBUG_INFO,
        "SSDT-SERIAL:   [%a] DISABLED (EnableBit=0x%08X not set)\n",
        mUartConfigTable[UartIndex].Name,
        mUartConfigTable[UartIndex].EnableBit
        ));
      continue;
    }

    //
    // UART is enabled - add to the list
    //
    DEBUG ((
      DEBUG_INFO,
      "SSDT-SERIAL:   [%a] ENABLED - Mode: %a, Address: 0x%lX\n",
      mUartConfigTable[UartIndex].Name,
      GetUartModeString (Mode),
      BaseAddress
      ));

    UartInfo[PortIndex].BaseAddress = BaseAddress;
    UartInfo[PortIndex].Interrupt   = GET_UART_INTR_NUM (mUartConfigTable[UartIndex].UartIndex);
    UartInfo[PortIndex].BaudRate    = PcdGet32 (PcdAmdIdsUartBaudRate);
    PortIndex++;
    EnabledCount++;

    if (Mode == UartModeMmio) {
      MmioCount++;
    } else if (Mode == UartModeLegacyIo) {
      LegacyIoCount++;
    }
  }

  //
  // Print detailed port list
  //
  if (EnabledCount > 0) {
    DEBUG ((DEBUG_INFO, "SSDT-SERIAL: Port List:\n"));
    for (UartIndex = 0; UartIndex < PortIndex; UartIndex++) {
      DEBUG ((
        DEBUG_INFO,
        "SSDT-SERIAL:   Port[%d]: Address=0x%lX, IRQ=%d, Baud=%lu\n",
        UartIndex,
        UartInfo[UartIndex].BaseAddress,
        UartInfo[UartIndex].Interrupt,
        UartInfo[UartIndex].BaudRate
        ));
    }
  }

  *SerialPortInfo  = UartInfo;
  *SerialPortCount = PortIndex;

  return EFI_SUCCESS;
}
