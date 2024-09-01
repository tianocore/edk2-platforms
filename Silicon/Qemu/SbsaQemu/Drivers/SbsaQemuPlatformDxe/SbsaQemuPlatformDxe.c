/** @file
*  FDT client protocol driver for qemu,mach-virt-ahci DT node
*
*  Copyright (c) 2019, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <IndustryStandard/SbsaQemuPlatformVersion.h>

EFI_STATUS
EFIAPI
InitializeSbsaQemuPlatformDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS       Status;
  UINTN            Size;
  VOID             *Base;
  GicInfo          GicInfo;
  PlatformVersion  PlatVer;

  DEBUG ((DEBUG_INFO, "%a: InitializeSbsaQemuPlatformDxe called\n", __func__));

  Base = (VOID *)(UINTN)PcdGet64 (PcdPlatformAhciBase);
  ASSERT (Base != NULL);
  Size = (UINTN)PcdGet32 (PcdPlatformAhciSize);
  ASSERT (Size != 0);

  DEBUG ((
    DEBUG_INFO,
    "%a: Got platform AHCI %llx %u\n",
    __func__,
    Base,
    Size
    ));

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeAhci,
             NonDiscoverableDeviceDmaTypeCoherent,
             NULL,
             NULL,
             1,
             Base,
             Size
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: NonDiscoverable: Cannot install AHCI device @%p (Staus == %r)\n",
      __func__,
      Base,
      Status
      ));
    return Status;
  }

  GetPlatformVersion (&PlatVer);

  PcdSet32S (PcdPlatformVersionMajor, PlatVer.Major);
  PcdSet32S (PcdPlatformVersionMinor, PlatVer.Minor);

  DEBUG ((DEBUG_INFO, "Platform version: %d.%d\n", PlatVer.Major, PlatVer.Minor));

  GetGicInformation (&GicInfo);

  PcdSet64S (PcdGicDistributorBase, GicInfo.DistributorBase);
  PcdSet64S (PcdGicRedistributorsBase, GicInfo.RedistributorBase);
  PcdSet64S (PcdGicItsBase, GicInfo.ItsBase);

  if (!PLATFORM_VERSION_LESS_THAN (0, 3)) {
    Base = (VOID *)(UINTN)PcdGet64 (PcdPlatformXhciBase);
    ASSERT (Base != NULL);
    Size = (UINTN)PcdGet32 (PcdPlatformXhciSize);
    ASSERT (Size != 0);

    DEBUG ((
      DEBUG_INFO,
      "%a: Got platform XHCI %llx %u\n",
      __func__,
      Base,
      Size
      ));

    Status = RegisterNonDiscoverableMmioDevice (
               NonDiscoverableDeviceTypeXhci,
               NonDiscoverableDeviceDmaTypeCoherent,
               NULL,
               NULL,
               1,
               Base,
               Size
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: NonDiscoverable: Cannot install XHCI device @%p (Status == %r)\n",
        __func__,
        Base,
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}
