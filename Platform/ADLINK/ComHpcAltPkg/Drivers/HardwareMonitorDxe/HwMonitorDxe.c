/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/MdeModuleHii.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MmcLib.h>

#include "HwMonitorHii.h"

//
// uni string and Vfr Binary data.
//
extern UINT8  HwMonitorVfrBin[];
extern UINT8  HwMonitorDxeStrings[];

EFI_HANDLE      mDriverHandle = NULL;
EFI_HII_HANDLE  mHiiHandle    = NULL;

#pragma pack(1)

//
// HII specific Vendor Device Path definition.
//
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

// HW_MONITOR_FORMSET_GUID
EFI_GUID  gHardwareMonitorFormsetGuid = HW_MONITOR_FORMSET_GUID;

HII_VENDOR_DEVICE_PATH  mHwMonitorHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    HW_MONITOR_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

#define MAX_STRING_SIZE  64

UINTN
parse_char_to_int (
  IN CHAR16  c
  )
{
  if (('0' <= c) && (c <= '9')) {
    return c - '0';
  }

  if (('a' <= c) && (c <= 'f')) {
    return 10 + c - 'a';
  }

  if (('A' <= c) && (c <= 'F')) {
    return 10 + c - 'A';
  } else {
    return 0;
  }
}

STATIC
EFI_STATUS
UpdatePlatformInfoScreen (
  IN EFI_HII_HANDLE  *HiiHandle
  )
{
  CHAR16  Str[MAX_STRING_SIZE];

  VOID                *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *EndLabel;
  EFI_STATUS          Status;
  UINT8               Buffer[2];
  UINTN               chartoint;
  struct sensor_read {
    INT16    quot;
    INT16    rem;
  } sensor_read;

  // IPMI_P3V3
  Status = IPMI_P3V3_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*337)/10000;
  sensor_read.rem  = (chartoint*337)%10000;
  sensor_read.rem += 32;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_IPMI_P3V3_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : IPMI_P3V3 retrieving error\n", __func__));
  }

  // IPMI_P12V
  Status = IPMI_P12V_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*70)/1000;
  sensor_read.rem  = (chartoint*70)%1000;
  sensor_read.rem += 20;
  if (sensor_read.rem >= 1000) {
    sensor_read.quot++;
    sensor_read.rem -= 1000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_IPMI_P12V_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : IPMI_P12V retrieving error\n", __func__));
  }

  // IPMI_P5V
  Status = IPMI_P5V_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*53)/1000;
  sensor_read.rem  = (chartoint*53)%1000;
  sensor_read.rem -= 37;
  if (sensor_read.rem < 0) {
    sensor_read.quot--;
    sensor_read.rem += 1000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_IPMI_P5V_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : IPMI_P5V retrieving error\n", __func__));
  }

  // P1V5_VDDH
  Status = P1V5_VDDH_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P1V5_VDDH_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P1V5_VDDH retrieving error\n", __func__));
  }

  // P0V75_PCP
  Status = P0V75_PCP_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P0V75_PCP_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P0V75_PCP retrieving error\n", __func__));
  }

  // P0V9_VDDC_RCA
  Status = P0V9_VDDC_RCA_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P0V9_VDDC_RCA_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P0V9_VDDC_RCA retrieving error\n", __func__));
  }

  // P0V75_VDDC_SOC
  Status = P0V75_VDDC_SOC_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P0V75_VDDC_SOC_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P0V75_VDDC_SOC retrieving error\n", __func__));
  }

  // P1V2_VDDQ_AB
  Status = P1V2_VDDQ_AB_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P1V2_VDDQ_AB_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P1V2_VDDQ_AB retrieving error\n", __func__));
  }

  // P1V2_VDDQ_CD
  Status = P1V2_VDDQ_CD_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P1V2_VDDQ_CD_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P1V2_VDDQ_CD retrieving error\n", __func__));
  }

  // P1V8_PCP
  Status = P1V8_PCP_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/10000;
  sensor_read.rem  = (chartoint*100)%10000;
  sensor_read.rem += 50;
  if (sensor_read.rem >= 10000) {
    sensor_read.quot++;
    sensor_read.rem -= 10000;
  }

  UnicodeSPrint (Str, sizeof (Str), L": %d.%03d V", sensor_read.quot, sensor_read.rem/10);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_P1V8_PCP_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : P1V8_PCP retrieving error\n", __func__));
  }

  // CPU Core Temperature
  Status = CPU_Temp_Sensor_Reading (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  chartoint        = parse_char_to_int (Str[0]) * 16 + parse_char_to_int (Str[1]);
  sensor_read.quot = (chartoint*100)/100;
  // sensor_read.rem = (chartoint*100)%10000;
  // sensor_read.rem += 50;
  UnicodeSPrint (Str, sizeof (Str), L": %d°C", sensor_read.quot);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_CPU_CORE_TEMP_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : CPU Core Temperature retrieving error\n", __func__));
  }

  /* Initialize the container for dynamic opcodes */
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  /* Create Hii Extend Label OpCode as the start opcode */
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  ASSERT (StartLabel != NULL);
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_UPDATE;

  /* Create Hii Extend Label OpCode as the end opcode */
  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  ASSERT (EndLabel != NULL);
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  HiiUpdateForm (
    mHiiHandle,                   // HII handle
    &gHardwareMonitorFormsetGuid, // Formset GUID
    HW_MONITOR_FORM_ID,           // Form ID
    StartOpCodeHandle,            // Label for where to insert opcodes
    EndOpCodeHandle               // Insert data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PlatformInfoUnload (
  VOID
  )
{
  if (mDriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mHwMonitorHiiVendorDevicePath,
           NULL
           );
    mDriverHandle = NULL;
  }

  if (mHiiHandle != NULL) {
    HiiRemovePackages (mHiiHandle);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PlatformInfoEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHwMonitorHiiVendorDevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  mHiiHandle = HiiAddPackages (
                 &gHardwareMonitorFormsetGuid,
                 mDriverHandle,
                 HwMonitorDxeStrings,
                 HwMonitorVfrBin,
                 NULL
                 );
  if (mHiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mHwMonitorHiiVendorDevicePath,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UpdatePlatformInfoScreen (mHiiHandle);
  if (EFI_ERROR (Status)) {
    PlatformInfoUnload ();
    DEBUG ((
      DEBUG_ERROR,
      "%a %d Fail to update the hardware monitor screen \n",
      __func__,
      __LINE__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}
