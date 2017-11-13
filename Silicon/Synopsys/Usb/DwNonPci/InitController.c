/** @file
  Initialize Designware USB controller via dummy PCI device

  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2016, Jeremy Linton

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Protocol/NonDiscoverableDevice.h>

EFI_STATUS
EFIAPI
DwNonPciUsbDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  DEBUG ((EFI_D_ERROR, "DwUsbExportController \n"));
  return RegisterNonDiscoverableMmioDevice (
           NonDiscoverableDeviceTypeEhci,
           NonDiscoverableDeviceDmaTypeNonCoherent,
           NULL,
           NULL,
           1,
           FixedPcdGet32 (PcdDwUsbBaseAddress), SIZE_256KB,
		   FixedPcdGet32 (PcdDwUsbSysCtrlBaseAddress), SIZE_8KB);
}
