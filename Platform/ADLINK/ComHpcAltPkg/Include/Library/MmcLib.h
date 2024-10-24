/** @file

**/

#ifndef __MMC_LIB_H__
#define __MMC_LIB_H__

#include <Uefi.h>
#include <Library/UefiLib.h>

/**
  Sends a 32-bit value to a POST card.

  Sends the 32-bit value specified by Value to a POST card, and returns Value.
  Some implementations of this library function may perform I/O operations
  directly to a POST card device.  Other implementations may send Value to
  ReportStatusCode(), and the status code reporting mechanism will eventually
  display the 32-bit value on the status reporting device.

  MmcPostCode() must actively prevent recursion.  If MmcPostCode() is called while
  processing another Post Code Library function, then
  MmcPostCode() must return Value immediately.

  @param  Value  The 32-bit value to write to the POST card.

  @return The 32-bit value to write to the POST card.

**/
EFI_STATUS
MmcPostCode (
  IN UINT32  Value
  );

EFI_STATUS
MmcSetPowerOffType (
  IN UINT8  Value
  );

EFI_STATUS
MmcFirmwareVersion (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
IPMI_P3V3_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
IPMI_P12V_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
IPMI_P5V_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P1V5_VDDH_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P0V75_PCP_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P0V9_VDDC_RCA_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P0V75_VDDC_SOC_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P1V2_VDDQ_AB_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P1V2_VDDQ_CD_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
P1V8_PCP_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
CPU_Temp_Sensor_Reading (
  IN UINT8  *Buffer,
  IN UINTN  BufferSize
  );

EFI_STATUS
WolDisableCmd (
  );

EFI_STATUS
WolEnableCmd (
  );

#endif
