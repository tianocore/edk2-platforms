/** @file

  ACPI SSDT Serial table header file.

  Copyright (c) 2019 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#pragma once

#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/AcpiHelperLib.h>

///
/// Maximum number of UART ports
///
#define MAX_UART_PORTS  4

///
/// Minimum UART address length
///
#define MIN_UART_ADDRESS_LENGTH  0x1000U

///
/// UART Assignment Values
///
#define UART0  0x0
#define UART1  0x1
#define UART2  0x2

///
/// UART mapping as per FchRTDeviceEnableMap PCD from AGESA
///
#define FCH_RT_DEVICE_ENABLE_MAP_UART0_BIT  BIT11
#define FCH_RT_DEVICE_ENABLE_MAP_UART1_BIT  BIT12
#define FCH_RT_DEVICE_ENABLE_MAP_UART2_BIT  BIT16

///
/// UART MMIO as per PPR vol.7
///
///
#define FCH_MMIO_BASE_UART0  0xFEDC9000
#define FCH_MMIO_BASE_UART1  0xFEDCA000
#define FCH_MMIO_BASE_UART2  0xFEDCE000

///
/// Get FCH MMIO base for given UART index
///
#define GET_FCH_MMIO_BASE(Index) \
  ((Index) == UART0 ? FCH_MMIO_BASE_UART0 : \
  ((Index) == UART1 ? FCH_MMIO_BASE_UART1 : \
  ((Index) == UART2 ? FCH_MMIO_BASE_UART2 : 0)))

#define GET_FCH_LEGACY_BASE(Encoding) \
  ((Encoding) == 1 ? 0x2E8 : \
  ((Encoding) == 2 ? 0x2F8 : \
  ((Encoding) == 3 ? 0x3E8 : \
  ((Encoding) == 4 ? 0x3F8 : 0))))

#define GET_UART_INTR_NUM(Index) \
  ((Index) == UART0 ? PcdGet8 (PcdFchUart0Irq) : \
  ((Index) == UART1 ? PcdGet8 (PcdFchUart1Irq) : \
  ((Index) == UART2 ? PcdGet8 (PcdFchUart2Irq) : 0)))

#define GET_UART_ENCODING(Index) \
  ((Index == UART0) ? PcdGet8 (FchUart0LegacyEnable) : \
  ((Index == UART1) ? PcdGet8 (FchUart1LegacyEnable) : \
  ((Index == UART2) ? PcdGet8 (FchUart2LegacyEnable) : 0)))

///
/// Legacy UART base address length
///
#define LEGACY_UART_BASE_ADDRESS_LENGTH  16

///
/// MMIO UART base address length
///
#define MMIO_UART_BASE_ADDRESS_LENGTH  32

///
/// UART Assignment Values for Which_UART_Range fields
///
#define AL2AHB_UART_ASSIGNMENT_UART0  0x0
#define AL2AHB_UART_ASSIGNMENT_UART1  0x1
#define AL2AHB_UART_ASSIGNMENT_UART2  0x2
#define AL2AHB_UART_ASSIGNMENT_UART3  0x3

///
/// AL2AHB Legacy UART I/O Enable Register (AL2AHBxFEDC0020) for PPR
///
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] I/O range 0 (0x2E8-0x2EF) decode enable
    /// 0: I/O Range 0 decode disabled
    /// 1: I/O Range 0 decode enabled
    ///
    UINT32    IoEnableRange0  : 1;

    ///
    /// [Bit 1] I/O range 1 (0x2F8-0x2FF) decode enable
    /// 0: I/O Range 1 decode disabled
    /// 1: I/O Range 1 decode enabled
    ///
    UINT32    IoEnableRange1  : 1;

    ///
    /// [Bit 2] I/O range 2 (0x3E8-0x3EF) decode enable
    /// 0: I/O Range 2 decode disabled
    /// 1: I/O Range 2 decode enabled
    ///
    UINT32    IoEnableRange2  : 1;

    ///
    /// [Bit 3] I/O range 3 (0x3F8-0x3FF) decode enable
    /// 0: I/O Range 3 decode disabled
    /// 1: I/O Range 3 decode enabled
    ///
    UINT32    IoEnableRange3  : 1;

    ///
    /// [Bits 7:4] Reserved
    ///
    UINT32    Reserved_7_4    : 4;

    ///
    /// [Bits 9:8] Assign which UART will receive I/O addresses 0x2E8-0x2EF
    /// 00b: UART0, 01b: UART1, 10b: UART2, 11b: UART3
    ///
    UINT32    WhichUartRange0 : 2;

    ///
    /// [Bits 11:10] Assign which UART will receive I/O addresses 0x2F8-0x2FF
    /// 00b: UART0, 01b: UART1, 10b: UART2, 11b: UART3
    ///
    UINT32    WhichUartRange1 : 2;

    ///
    /// [Bits 13:12] Assign which UART will receive I/O addresses 0x3E8-0x3EF
    /// 00b: UART0, 01b: UART1, 10b: UART2, 11b: UART3
    ///
    UINT32    WhichUartRange2 : 2;

    ///
    /// [Bits 15:14] Assign which UART will receive I/O addresses 0x3F8-0x3FF
    /// 00b: UART0, 01b: UART1, 10b: UART2, 11b: UART3
    ///
    UINT32    WhichUartRange3 : 2;

    ///
    /// [Bits 31:16] Reserved
    ///
    UINT32    Reserved_31_16  : 16;
  } Field;

  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Value;
} AL2AHB_LEGACY_UART_IO_ENABLE_REGISTER;

///
/// UART access mode enumeration
///
typedef enum {
  UartModeDisabled = 0,
  UartModeMmio,
  UartModeLegacyIo
} UART_ACCESS_MODE;

///
/// UART configuration entry for table-driven approach
///
typedef struct {
  UINT8     UartIndex;        ///< UART index (0, 1, 2)
  UINT32    EnableBit;        ///< Bit in FchRTDeviceEnableMap
  UINT64    MmioBase;         ///< MMIO base address
  CHAR8     *Name;            ///< UART name for debug output
} UART_CONFIG_ENTRY;

///
/// Serial port configuration structure
///

typedef struct {
  UINT64    BaseAddress;
  UINT8     Interrupt;
  UINT64    BaudRate;
} SERIAL_PORT_CONFIG;

/**
  Build the ACPI SSDT Serial Port table.

  @param [in]  SerialPortInfo   Pointer to the serial port information structure.
  @param [in]  SerialPortCount  Number of serial ports.
  @param [out] Table            Pointer to the built ACPI table.

  @retval EFI_SUCCESS           The table was built successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Others                Failed to build the table.
**/
EFI_STATUS
EFIAPI
AmdBuildSsdtSerialPortTable (
  IN SERIAL_PORT_CONFIG                  *SerialPortInfo,
  IN UINT8                               SerialPortCount,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  );

/**
  Get SoC specific UART information.

  @param [out]  SerialPortInfo   Pointer to the serial port information structure.
  @param [out]  SerialPortCount  Number of serial port count
  @retval EFI_SUCCESS           The information was retrieved successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
GetSocUartInfo (
  OUT SERIAL_PORT_CONFIG  **SerialPortInfo,
  OUT UINT8               *SerialPortCount
  );
