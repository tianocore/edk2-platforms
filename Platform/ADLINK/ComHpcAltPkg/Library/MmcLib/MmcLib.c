/** @file
  MMC Library.

  Copyright (c) 2022, ADLink. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmcLib.h>
#include <Library/NVParamLib.h>
#include <Library/PcdLib.h>
#include <Library/PL011UartLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <NVParamDef.h>


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

UINT8
GetFirmwareMajorVersion (
  VOID
  )
{
  UINT16      ACLRd = NV_PERM_ALL;
  EFI_STATUS  Status;
  UINT32      Val;

  Status = NVParamGet (NV_SI_RO_BOARD_I2C_VRD_CONFIG_INFO, ACLRd, &Val);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, " I2C brd config info    0x%X (%d)\n", Val, Val));
  }

  if (Val == 0x6A685860) {
    return 0xA2;
  } else if (Val == 0x6A687F60) {
    return 0xA1;
  } else {
    return 0xA0;
  }
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

