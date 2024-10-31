/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/I2cLib.h>
#include <Library/MmcLib.h>

#include "PlatformInfoHii.h"

//
// uni string and Vfr Binary data.
//
extern UINT8  PlatformInfoVfrBin[];
extern UINT8  PlatformInfoDxeStrings[];

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

// PLATFORM_INFO_FORMSET_GUID
EFI_GUID  gPlatformInfoFormSetGuid = PLATFORM_INFO_FORMSET_GUID;

HII_VENDOR_DEVICE_PATH  mPlatformInfoHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    PLATFORM_INFO_FORMSET_GUID
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

#define MAX_STRING_SIZE   64
#define MHZ_SCALE_FACTOR  1000000

STATIC
CHAR8 *
GetCCIXLinkSpeed (
  IN UINTN  Speed
  )
{
  switch (Speed) {
    case 1:
      return "2.5 GT/s";

    case 2:
      return "5 GT/s";

    case 3:
      return "8 GT/s";

    case 4:
    case 6:
      return "16 GT/s";

    case 0xa:
      return "20 GT/s";

    case 0xf:
      return "25 GT/s";
  }

  return "Unknown";
}

EFI_STATUS
GetLanFwVer (
  IN OUT CHAR8  *Ver
  )
{
  EFI_STATUS  Status;
  UINT32      TmpLen;
  UINT8       I2cCommand[] = "V";

  Status = I2cProbe (PcdGet8 (PcdNicI2cBusAddress), PcdGet32 (PcdNicI2cBusSpeed), FALSE, FALSE);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Send the slave address command for read version number
  //
  TmpLen = 1;
  Status = I2cWrite (PcdGet8 (PcdNicI2cBusAddress), PcdGet8 (PcdNicI2cDeviceAddress), I2cCommand, &TmpLen);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Read back the version string
  //
  TmpLen = 4;
  Status = I2cRead (PcdGet8 (PcdNicI2cBusAddress), PcdGet8 (PcdNicI2cDeviceAddress), NULL, 0, (UINT8 *)Ver, &TmpLen);
  if (EFI_ERROR (Status)) {
    TmpLen = 1;
    Status = I2cWrite (PcdGet8 (PcdNicI2cBusAddress), 0x30, I2cCommand, &TmpLen);
    TmpLen = 4;
    Status = I2cRead (PcdGet8 (PcdNicI2cBusAddress), 0x30, NULL, 0, (UINT8 *)Ver, &TmpLen);
  }

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Ver[4] = 0;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdatePlatformInfoScreen (
  IN EFI_HII_HANDLE  *HiiHandle
  )
{
  VOID               *Hob;
  PLATFORM_INFO_HOB  *PlatformHob;
  CHAR16             Str[MAX_STRING_SIZE];

  VOID                *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *EndLabel;
  CHAR8               LAN_FW_VER[5];
  EFI_STATUS          Status;
  UINT8               Buffer[6];

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return EFI_DEVICE_ERROR;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  /* SCP Version */
  AsciiStrToUnicodeStrS ((const CHAR8 *)PlatformHob->SmPmProVer, Str, MAX_STRING_SIZE);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_SCPVER_VALUE),
    Str,
    NULL
    );

  /* SCP build */
  AsciiStrToUnicodeStrS ((const CHAR8 *)PlatformHob->SmPmProBuild, Str, MAX_STRING_SIZE);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_SCPBUILD_VALUE),
    Str,
    NULL
    );

  /* Broadcom MCU Version */
  if (!EFI_ERROR (GetLanFwVer (LAN_FW_VER))) {
    AsciiStrToUnicodeStrS ((const CHAR8 *)LAN_FW_VER, Str, MAX_STRING_SIZE);
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_PLATFORM_INFO_LANMCUVER_VALUE),
      Str,
      NULL
      );
  }

  /* Get MMC FW Version */
  Status = MmcFirmwareVersion (Buffer, sizeof (Buffer));
  AsciiStrToUnicodeStrS ((const CHAR8 *)Buffer, Str, MAX_STRING_SIZE);
  if (!EFI_ERROR (Status)) {
    HiiSetString (
      HiiHandle,
      STRING_TOKEN (STR_MMC_FIRMWARE_REV_VALUE),
      Str,
      NULL
      );
  } else {
    DEBUG ((DEBUG_ERROR, "%a : MMC firmware version retrieving error\n", __func__));
  }

  /* CPU Info */
  AsciiStrToUnicodeStrS ((const CHAR8 *)PlatformHob->CpuInfo, Str, MAX_STRING_SIZE);
  UnicodeSPrint (Str, sizeof (Str), L"%s", Str);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_CPUINFO_VALUE),
    Str,
    NULL
    );

  /* CPU clock */
  UnicodeSPrint (Str, sizeof (Str), L"%dMHz", PlatformHob->CpuClk / MHZ_SCALE_FACTOR);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_CPUCLK_VALUE),
    Str,
    NULL
    );

  /* PCP clock */
  UnicodeSPrint (Str, sizeof (Str), L"%dMHz", PlatformHob->PcpClk / MHZ_SCALE_FACTOR);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_PCPCLK_VALUE),
    Str,
    NULL
    );

  /* SOC clock */
  UnicodeSPrint (Str, sizeof (Str), L"%dMHz", PlatformHob->SocClk / MHZ_SCALE_FACTOR);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_SOCCLK_VALUE),
    Str,
    NULL
    );

  /* L1 Cache */
  UnicodeSPrint (Str, sizeof (Str), L"64KB");
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_L1ICACHE_VALUE),
    Str,
    NULL
    );
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_L1DCACHE_VALUE),
    Str,
    NULL
    );

  /* L2 Cache */
  UnicodeSPrint (Str, sizeof (Str), L"1MB");
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_L2CACHE_VALUE),
    Str,
    NULL
    );

  /* AHB clock */
  UnicodeSPrint (Str, sizeof (Str), L"%dMHz", PlatformHob->AhbClk / MHZ_SCALE_FACTOR);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_AHBCLK_VALUE),
    Str,
    NULL
    );

  /* SYS clock */
  UnicodeSPrint (Str, sizeof (Str), L"%dMHz", PlatformHob->SysClk / MHZ_SCALE_FACTOR);
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_PLATFORM_INFO_SYSCLK_VALUE),
    Str,
    NULL
    );

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

  if (IsSlaveSocketActive ()) {
    /* Create the inter socket link text string */
    UnicodeSPrint (
      Str,
      sizeof (Str),
      L"Width x%d / Speed %a",
      PlatformHob->Link2PWidth[0],
      GetCCIXLinkSpeed (PlatformHob->Link2PSpeed[0])
      );

    HiiSetString (
      mHiiHandle,
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK0_VALUE),
      Str,
      NULL
      );

    HiiCreateTextOpCode (
      StartOpCodeHandle,
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK0),
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK0),
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK0_VALUE)
      );

    UnicodeSPrint (
      Str,
      sizeof (Str),
      L"Width x%d / Speed %a",
      PlatformHob->Link2PWidth[1],
      GetCCIXLinkSpeed (PlatformHob->Link2PSpeed[1])
      );

    HiiSetString (
      mHiiHandle,
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK1_VALUE),
      Str,
      NULL
      );

    HiiCreateTextOpCode (
      StartOpCodeHandle,
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK1),
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK1),
      STRING_TOKEN (STR_CPU_FORM_INTER_SOCKET_LINK1_VALUE)
      );
  }

  HiiUpdateForm (
    mHiiHandle,                 // HII handle
    &gPlatformInfoFormSetGuid,  // Formset GUID
    PLATFORM_INFO_FORM_ID,      // Form ID
    StartOpCodeHandle,          // Label for where to insert opcodes
    EndOpCodeHandle             // Insert data
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
           &mPlatformInfoHiiVendorDevicePath,
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
                  &mPlatformInfoHiiVendorDevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  mHiiHandle = HiiAddPackages (
                 &gPlatformInfoFormSetGuid,
                 mDriverHandle,
                 PlatformInfoDxeStrings,
                 PlatformInfoVfrBin,
                 NULL
                 );
  if (mHiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mPlatformInfoHiiVendorDevicePath,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UpdatePlatformInfoScreen (mHiiHandle);
  if (EFI_ERROR (Status)) {
    PlatformInfoUnload ();
    DEBUG ((
      DEBUG_ERROR,
      "%a %d Fail to update the platform info screen \n",
      __func__,
      __LINE__
      ));
    return Status;
  }

  return EFI_SUCCESS;
}
