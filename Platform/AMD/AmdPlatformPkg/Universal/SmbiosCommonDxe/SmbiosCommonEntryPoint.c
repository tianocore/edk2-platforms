/** @file
  AMD Smbios Common DXE entry point.

  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "SmbiosCommon.h"

//
// Keep order by type number, from lower type to higher.
//
COMMON_SMBIOS_DATA  mSmbiosCommonDataFuncTable[] = {
  { &BiosVendorFunction            },     // Type 0
  { &SystemManufacturerFunction    },     // Type 1
  { &BaseBoardManufacturerFunction },     // Type 2
  { &ChassisManufacturerFunction   },     // Type 3
  { &PortConnectorInfoFunction     },     // Type 8
  { &SystemSlotInfoFunction        },     // Type 9
  { &OemStringsFunction            },     // Type 11
  { &SystemCfgOptionsFunction      },     // Type 12
  { &BiosLanguageInfoFunction      },     // Type 13
  { &BuildInPointingDeviceFunction },     // Type 21
  { &VoltageProbeFunction          },     // Type 26
  { &BootInfoStatusFunction        },     // Type 32
  { &IpmiDeviceInformation         },     // Type 38
  { &SystemPowerSupplyFunction     },     // Type 39
  { &HostInterface                 }      // Type 42
};

/**
  Add an SMBIOS record.

  @param[in]  Smbios                The EFI_SMBIOS_PROTOCOL instance.
  @param[out] SmbiosHandle          A unique handle will be assigned to the SMBIOS record.
  @param[in]  Record                The data for the fixed portion of the SMBIOS record. The format of the record is
                                    determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined
                                    by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or
                                    a set of null terminated strings and a null.

  @retval EFI_SUCCESS               Record was added.
  @retval EFI_OUT_OF_RESOURCES      Record was not added due to lack of system resources.

**/
EFI_STATUS
AddCommonSmbiosRecord (
  IN EFI_SMBIOS_PROTOCOL      *Smbios,
  OUT EFI_SMBIOS_HANDLE       *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER  *Record
  )
{
  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  return Smbios->Add (
                   Smbios,
                   NULL,
                   SmbiosHandle,
                   Record
                   );
}

/**
  PciEnumerationComplete Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
OnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS           EfiStatus;
  EFI_SMBIOS_PROTOCOL  *Smbios;

  EfiStatus = gBS->LocateProtocol (
                     &gEfiSmbiosProtocolGuid,
                     NULL,
                     (VOID **)&Smbios
                     );
  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
  }

  // Install Type 41 when PCI enumeration is complete
  EfiStatus = OnboardDevExtInfoFunction (Smbios);
  if (EFI_ERROR (EfiStatus)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "Skip installing SMBIOS Table 41, ReturnStatus=%r\n",
       EfiStatus
      )
      );
  }
}

/**
  EFI driver entry point. This driver parses mSmbiosCommonDataFuncTable
  structure and generates common platform smbios records.

  @param[in]  ImageHandle     Handle for the image of this driver
  @param[in]  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

**/
EFI_STATUS
EFIAPI
SmbiosCommonEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                Index;
  EFI_STATUS           EfiStatus;
  EFI_SMBIOS_PROTOCOL  *Smbios;
  EFI_EVENT            ProtocolNotifyEvent;
  VOID                 *Registration;

  DEBUG ((DEBUG_INFO, "%a: Entry.\n", __func__));

  EfiStatus = gBS->LocateProtocol (
                     &gEfiSmbiosProtocolGuid,
                     NULL,
                     (VOID **)&Smbios
                     );
  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
    return EfiStatus;
  }

  ProtocolNotifyEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiPciEnumerationCompleteProtocolGuid,
                          TPL_CALLBACK,
                          OnPciEnumerationComplete,
                          NULL,
                          &Registration
                          );
  if (ProtocolNotifyEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "Could not create PCI enumeration complete event\n"));
  }

  for (Index = 0; Index < sizeof (mSmbiosCommonDataFuncTable)/sizeof (mSmbiosCommonDataFuncTable[0]); ++Index) {
    EfiStatus = (*mSmbiosCommonDataFuncTable[Index].Function)(Smbios);
    if (EFI_ERROR (EfiStatus)) {
      // Continue installing remaining tables if one table fails.
      DEBUG (
        (
         DEBUG_ERROR,
         "Skip installing SMBIOS Table Index=%d, ReturnStatus=%r\n",
         Index,
         EfiStatus
        )
        );
      continue;
    }
  }

  return EFI_SUCCESS;
}
