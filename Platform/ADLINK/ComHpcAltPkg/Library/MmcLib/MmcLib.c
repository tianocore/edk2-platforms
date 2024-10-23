/** @file
  MMC Library.

  Copyright (c) 2022, ADLink. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmcLib.h>
#include <Library/NVLib.h>
#include <Library/PcdLib.h>
#include <Library/PL011UartLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

EFI_STATUS
MmcPostCode (
  IN UINT32  Value
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[C0 00 80 11]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);

  AsciiSPrint ((CHAR8 *)IpmiCmdBuf, sizeof (IpmiCmdBuf), "[C0 00 80 %2X]\r\n", (UINT8)Value);

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Write MMC POST code data\n", __func__));
    return EFI_NO_RESPONSE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MmcSetPowerOffType (
  IN UINT8  Value
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[C0 00 15 01]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);

  AsciiSPrint ((CHAR8 *)IpmiCmdBuf, sizeof (IpmiCmdBuf), "[C0 00 80 %02X]\r\n", Value);

  DEBUG ((DEBUG_INFO, "%a Write MMC Power off type %d\n", __func__, Value));
  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Write MMC Power off type\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MmcFirmwareVersion (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[18 00 01]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[19 * 3 + 5 + 12];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get MMC Version\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get MMC Version\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 66, BufferSize - 1);

  Buffer[2] = '.';

  return EFI_SUCCESS;
}

EFI_STATUS
IPMI_P3V3_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 04 2D 01]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[72];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "%a: IpmiCmdBuf=%a, Len=%d\n", __func__, IpmiCmdBuf, IpmiCmdBufSize));

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  DEBUG ((DEBUG_INFO, "%a: Send %d bytes\n", __func__, NumberOfBytes));

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get IPMI_P3V3_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  DEBUG ((DEBUG_INFO, "%a: Trying to read %d bytes\n", __func__, xBufferSize));

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  DEBUG ((DEBUG_INFO, "%a: Read %d bytes\n", __func__, NumberOfBytes));

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get IPMI_P3V3_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 63, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
IPMI_P12V_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 08 2D 02]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get IPMI_P12V_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get IPMI_P12V_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
IPMI_P5V_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 0C 2D 03]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get IPMI_P5V_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get IPMI_P5V_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P1V5_VDDH_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 10 2D 05]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V5_VDDH_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V5_VDDH_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P0V75_PCP_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 14 2D 06]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P0V75_PCP_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P0V75_PCP_Sensor_Reading\n", __func__));

    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P0V9_VDDC_RCA_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 18 2D 07]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P0V9_VDDC_RCA_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P0V9_VDDC_RCA_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P0V75_VDDC_SOC_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 1C 2D 08]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P0V75_VDDC_SOC_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P0V75_VDDC_SOC_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P1V2_VDDQ_AB_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 20 2D 09]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V2_VDDQ_AB_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V2_VDDQ_AB_Sensor_Reading\n", __func__));

    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P1V2_VDDQ_CD_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 24 2D 0A]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V2_VDDQ_CD_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V2_VDDQ_CD_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
P1V8_PCP_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 28 2D 0B]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V8_PCP_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get P1V8_PCP_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
CPU_Temp_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  )
{
  UINTN  NumberOfBytes;
  UINT8  IpmiCmdBuf[]   = { "[10 2C 2D 0C]\r\n" };
  UINTN  IpmiCmdBufSize = sizeof (IpmiCmdBuf);
  UINT8  xBuffer[42];
  UINTN  xBufferSize = sizeof (xBuffer);

  if (GetFirmwareMajorVersion () == 0xA1) {
    DEBUG ((DEBUG_INFO, "%a A1 is not supported\n", __func__));
    return EFI_UNSUPPORTED;
  }

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), IpmiCmdBuf, IpmiCmdBufSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get CPU_Temp_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  NumberOfBytes = PL011UartRead ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), xBuffer, xBufferSize);

  if (NumberOfBytes == 0) {
    DEBUG ((DEBUG_ERROR, "%a Failed to Get CPU_Temp_Sensor_Reading\n", __func__));
    return EFI_NO_RESPONSE;
  }

  CopyMem (Buffer, xBuffer + 33, BufferSize);

  return EFI_SUCCESS;
}

EFI_STATUS
WolDisableCmd (
  VOID
  )
{
  UINTN  NumberOfBytes;
  UINT8  wolCmdBuf_1[]   = { "[C0 00 3C 00]\r\n" }; // cmd to disable WOL
  UINTN  wolCmdBufSize_1 = sizeof (wolCmdBuf_1);

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), wolCmdBuf_1, wolCmdBufSize_1);

  if (NumberOfBytes == 0) {
    return EFI_NO_RESPONSE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
WolEnableCmd (
  VOID
  )
{
  UINTN  NumberOfBytes;
  UINT8  wolCmdBuf_1[]   = { "[C0 00 3C 01]\r\n" }; // cmd to enable WOL
  UINTN  wolCmdBufSize_1 = sizeof (wolCmdBuf_1);

  NumberOfBytes = PL011UartWrite ((UINTN)PcdGet64 (PcdSerialDbgRegisterBase), wolCmdBuf_1, wolCmdBufSize_1);

  if (NumberOfBytes == 0) {
    return EFI_NO_RESPONSE;
  }

  return EFI_SUCCESS;
}
